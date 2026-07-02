#include "rtsp_server.h"
#include "log.h"

#include "libavutil/mathematics.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MTU 1400
#define RTP_HDR_LEN 12
#define MAX_CLIENTS 8

/* ---------- Base64 ---------- */
static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void b64_encode(const uint8_t *src, int len, char *out)
{
    int o = 0;
    for (int i = 0; i < len; i += 3) {
        uint32_t v = (uint32_t)src[i] << 16;
        if (i + 1 < len) v |= (uint32_t)src[i + 1] << 8;
        if (i + 2 < len) v |= src[i + 2];
        out[o++] = B64[(v >> 18) & 0x3f];
        out[o++] = B64[(v >> 12) & 0x3f];
        out[o++] = (i + 1 < len) ? B64[(v >> 6) & 0x3f] : '=';
        out[o++] = (i + 2 < len) ? B64[v & 0x3f] : '=';
    }
    out[o] = 0;
}

/* ---------- NALU 解析：从 extradata/packet 切出 NALU ---------- */
/* Annex B: 00 00 00 01 或 00 00 01 起始码分隔。AVCC: 4字节大端长度前缀。
 * 调用 nal_visit(data,len, cb, user)，cb 收到每个 NALU(不含起始码/长度)。 */
typedef void (*nal_cb_t)(const uint8_t *nal, int len, void *user);

/* 找下一个起始码位置，返回起始码长度(3/4)或 0 */
static int find_startcode(const uint8_t *p, int len, int *sc_len)
{
    for (int i = 0; i < len - 3; i++) {
        if (p[i] == 0 && p[i + 1] == 0) {
            if (p[i + 2] == 1) { *sc_len = 3; return i; }
            if (p[i + 2] == 0 && i + 3 < len && p[i + 3] == 1) { *sc_len = 4; return i; }
        }
    }
    return -1;
}

static void parse_nalus(const uint8_t *data, int len, nal_cb_t cb, void *user)
{
    if (len <= 0) return;
    /* AVCC 检测：首字节为 1（configurationVersion）且不像起始码 */
    if (data[0] == 1 && data[1] != 0) {
        int idx = 5;
        int num_sps = data[4] & 0x1f;
        for (int i = 0; i < num_sps && idx + 2 <= len; i++) {
            int nlen = (data[idx] << 8) | data[idx + 1];
            idx += 2;
            if (idx + nlen > len) break;
            cb(data + idx, nlen, user);
            idx += nlen;
        }
        if (idx + 1 <= len) {
            int num_pps = data[idx];
            idx++;
            for (int i = 0; i < num_pps && idx + 2 <= len; i++) {
                int nlen = (data[idx] << 8) | data[idx + 1];
                idx += 2;
                if (idx + nlen > len) break;
                cb(data + idx, nlen, user);
                idx += nlen;
            }
        }
        return;
    }
    /* Annex B */
    int pos = 0;
    /* 跳到首个起始码 */
    int sc = 0;
    int p = find_startcode(data, len, &sc);
    if (p < 0) { cb(data, len, user); return; }
    pos = p + sc;
    while (pos < len) {
        int next_sc = 0;
        int q = find_startcode(data + pos, len - pos, &next_sc);
        if (q < 0) {
            cb(data + pos, len - pos, user);
            break;
        }
        cb(data + pos, q, user);
        pos += q + next_sc;
    }
}

/* ---------- SDP: 提取 SPS/PPS ---------- */
static char g_sps_b64[512];
static char g_pps_b64[256];
static int  g_have_sps = 0, g_have_pps = 0;

static void sdp_collect_cb(const uint8_t *nal, int len, void *user)
{
    (void)user;
    if (len < 1) return;
    int nal_type = nal[0] & 0x1f;
    if (nal_type == 7 && !g_have_sps) {       /* SPS */
        b64_encode(nal, len, g_sps_b64);
        g_have_sps = 1;
    } else if (nal_type == 8 && !g_have_pps) { /* PPS */
        b64_encode(nal, len, g_pps_b64);
        g_have_pps = 1;
    }
}

static void build_sdp(char *out, int outcap)
{
    snprintf(out, outcap,
        "v=0\r\n"
        "o=- 0 0 IN IP4 0.0.0.0\r\n"
        "s=EdgeFusion NVR\r\n"
        "c=IN IP4 0.0.0.0\r\n"
        "t=0 0\r\n"
        "m=video 0 RTP/AVP 96\r\n"
        "a=control:track0\r\n"
        "a=rtpmap:96 H264/90000\r\n"
        "a=fmtp:96 packetization-mode=1; sprop-parameter-sets=%s,%s\r\n",
        g_have_sps ? g_sps_b64 : "Z0LAHtkA2D3oP5gQgABH",
        g_have_pps ? g_pps_b64 : "aM48gA==");
}

/* ---------- 客户端 ---------- */
typedef enum { TR_NONE, TR_TCP, TR_UDP } transport_t;

typedef struct client {
    int                fd;           /* RTSP 控制 TCP 连接 */
    transport_t        tr;
    /* TCP interleaved */
    int                rtp_channel;  /* 通常 0 */
    /* UDP */
    int                udp_rtp_fd;
    int                udp_rtcp_fd;
    struct sockaddr_in client_rtp_addr;
    struct sockaddr_in client_rtcp_addr;
    int                client_rtp_port;
    /* 会话 */
    uint32_t           session_id;
    int                playing;
    uint16_t           seq;
    uint32_t           ssrc;
    uint32_t           ts_base;
    int                fail_count;
    struct client     *next;
} client_t;

static client_t      *g_clients = NULL;
static pthread_mutex_t g_mtx = PTHREAD_MUTEX_INITIALIZER;
static int            g_listen_fd = -1;
static pthread_t      g_tid;
static volatile int   g_running = 0;
static char           g_sdp[1024];
static int            g_sdp_len;

/* ---------- 工具 ---------- */
static void set_nonblock(int fd)
{
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl >= 0) fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

/* 发送全部（阻塞小响应，控制信道）。返回写入字节数或 -1 */
static int send_all(int fd, const char *buf, int len)
{
    int total = 0;
    while (total < len) {
        ssize_t n = send(fd, buf + total, len - total, MSG_NOSIGNAL);
        if (n <= 0) { if (errno == EINTR) continue; return -1; }
        total += (int)n;
    }
    return total;
}

/* ---------- RTP 发送 ---------- */
/* 为一个 NALU 生成 RTP 包并写到客户端。返回 0 成功，-1 该客户端应断开。 */
static int send_nal_to_client(client_t *c, const uint8_t *nal, int nal_len,
                              uint32_t ts, int marker)
{
    uint8_t rtp[MTU + RTP_HDR_LEN + 2];
    int max_payload = MTU - RTP_HDR_LEN;

    if (nal_len <= max_payload) {
        /* 单 NAL 包 */
        rtp[0] = 0x80;                       /* V=2 */
        rtp[1] = (marker ? 0x80 : 0) | 96;   /* M, PT=96 */
        rtp[2] = (c->seq >> 8) & 0xff; rtp[3] = c->seq & 0xff; c->seq++;
        rtp[4] = (ts >> 24) & 0xff; rtp[5] = (ts >> 16) & 0xff;
        rtp[6] = (ts >> 8) & 0xff;  rtp[7] = ts & 0xff;
        rtp[8] = (c->ssrc >> 24) & 0xff; rtp[9] = (c->ssrc >> 16) & 0xff;
        rtp[10] = (c->ssrc >> 8) & 0xff; rtp[11] = c->ssrc & 0xff;
        memcpy(rtp + RTP_HDR_LEN, nal, nal_len);
        int pktlen = RTP_HDR_LEN + nal_len;

        if (c->tr == TR_TCP) {
            /* interleaved: '$' channel len16 */
            uint8_t hdr[4] = { '$', (uint8_t)c->rtp_channel,
                               (uint8_t)((pktlen >> 8) & 0xff), (uint8_t)(pktlen & 0xff) };
            if (send_all(c->fd, (char *)hdr, 4) < 0) return -1;
            if (send_all(c->fd, (char *)rtp, pktlen) < 0) return -1;
        } else { /* UDP */
            if (sendto(c->udp_rtp_fd, rtp, pktlen, 0,
                       (struct sockaddr *)&c->client_rtp_addr, sizeof(c->client_rtp_addr)) < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return 0; /* 丢帧 */
                return -1;
            }
        }
        return 0;
    }

    /* FU-A 分片 */
    uint8_t nal_hdr = nal[0];
    uint8_t indicator = (nal_hdr & 0xE0) | 28; /* FU-A type 28 */
    int offset = 1; /* 跳过原 NAL 头，FU header 承载 type */
    int first = 1;
    while (offset < nal_len) {
        int frag = nal_len - offset;
        if (frag > max_payload - 2) frag = max_payload - 2;
        int last = (offset + frag >= nal_len);

        rtp[0] = 0x80;
        rtp[1] = ((last && marker) ? 0x80 : 0) | 96;
        rtp[2] = (c->seq >> 8) & 0xff; rtp[3] = c->seq & 0xff; c->seq++;
        rtp[4] = (ts >> 24) & 0xff; rtp[5] = (ts >> 16) & 0xff;
        rtp[6] = (ts >> 8) & 0xff;  rtp[7] = ts & 0xff;
        rtp[8] = (c->ssrc >> 24) & 0xff; rtp[9] = (c->ssrc >> 16) & 0xff;
        rtp[10] = (c->ssrc >> 8) & 0xff; rtp[11] = c->ssrc & 0xff;
        rtp[RTP_HDR_LEN] = indicator;
        uint8_t fu = nal_hdr & 0x1f;
        if (first) fu |= 0x80;      /* start */
        if (last)  fu |= 0x40;      /* end */
        rtp[RTP_HDR_LEN + 1] = fu;
        memcpy(rtp + RTP_HDR_LEN + 2, nal + offset, frag);
        int pktlen = RTP_HDR_LEN + 2 + frag;

        if (c->tr == TR_TCP) {
            uint8_t hdr[4] = { '$', (uint8_t)c->rtp_channel,
                               (uint8_t)((pktlen >> 8) & 0xff), (uint8_t)(pktlen & 0xff) };
            if (send_all(c->fd, (char *)hdr, 4) < 0) return -1;
            if (send_all(c->fd, (char *)rtp, pktlen) < 0) return -1;
        } else {
            if (sendto(c->udp_rtp_fd, rtp, pktlen, 0,
                       (struct sockaddr *)&c->client_rtp_addr, sizeof(c->client_rtp_addr)) < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
                return -1;
            }
        }
        offset += frag;
        first = 0;
    }
    return 0;
}

/* 收集 NALU 到数组，便于知道 marker 该打在最后一包 */
#define MAX_NALS 32
typedef struct { const uint8_t *p; int len; } nal_t;
typedef struct { nal_t nals[MAX_NALS]; int count; } nal_list_t;
static void collect_cb(const uint8_t *nal, int len, void *user)
{
    nal_list_t *l = (nal_list_t *)user;
    if (l->count < MAX_NALS) { l->nals[l->count].p = nal; l->nals[l->count].len = len; l->count++; }
}

void rtsp_server_on_packet(const AVPacket *pkt, const AVRational *tb)
{
    if (!g_running || !pkt || pkt->size <= 0) return;

    /* 时间戳转 90kHz */
    uint32_t ts;
    if (pkt->pts != AV_NOPTS_VALUE && tb && tb->den) {
        int64_t v = av_rescale_q(pkt->pts, *tb, (AVRational){1, 90000});
        ts = (uint32_t)v;
    } else {
        ts = 0; /* 无 pts 时由各客户端 ssrc 基址 + 后续自增（简化：用 0） */
    }

    nal_list_t list; list.count = 0;
    parse_nalus(pkt->data, pkt->size, collect_cb, &list);
    if (list.count == 0) return;

    pthread_mutex_lock(&g_mtx);
    client_t *c = g_clients;
    while (c) {
        if (!c->playing) { c = c->next; continue; }
        int err = 0;
        for (int i = 0; i < list.count && !err; i++) {
            int marker = (i == list.count - 1); /* 最后一 NAL 最后一包置 marker */
            if (send_nal_to_client(c, list.nals[i].p, list.nals[i].len, ts, marker) < 0) {
                err = 1;
            }
        }
        if (err) {
            c->fail_count++;
            if (c->fail_count > 3) {
                LOG_INF("rtsp 客户端 session=%u 发送失败过多，标记断开", c->session_id);
                c->playing = 0;
            }
        } else {
            c->fail_count = 0;
        }
        c = c->next;
    }
    pthread_mutex_unlock(&g_mtx);
}

/* ---------- RTSP 请求处理 ---------- */
/* 从请求头取某字段值到 out(bufsize)。成功返回 1 */
static int hdr_get(const char *req, const char *name, char *out, int outcap)
{
    char pat[64];
    snprintf(pat, sizeof(pat), "%s:", name);
    /* 大小写不敏感查找行首 */
    const char *p = req;
    while (*p) {
        const char *line_end = strstr(p, "\r\n");
        if (!line_end) break;
        int line_len = (int)(line_end - p);
        if (line_len > 0) {
            /* 比较行首（忽略大小写） */
            int i;
            for (i = 0; i < (int)strlen(pat) && i < line_len; i++) {
                if (tolower((unsigned char)p[i]) != tolower((unsigned char)pat[i])) break;
            }
            if (i == (int)strlen(pat)) {
                /* 取值：跳过空格 */
                const char *v = p + i;
                while (*v == ' ' || *v == '\t') v++;
                int vl = line_len - (int)(v - p);
                if (vl >= outcap) vl = outcap - 1;
                memcpy(out, v, vl);
                out[vl] = 0;
                return 1;
            }
        }
        p = line_end + 2;
    }
    return 0;
}

static void close_client(client_t *c)
{
    if (c->fd >= 0) close(c->fd);
    if (c->udp_rtp_fd >= 0) close(c->udp_rtp_fd);
    if (c->udp_rtcp_fd >= 0) close(c->udp_rtcp_fd);
    free(c);
}

static void handle_client(int cfd, struct sockaddr_in *caddr)
{
    set_nonblock(cfd); /* 控制信道也非阻塞，但响应用 send_all 阻塞写 */
    /* 实际控制信令用阻塞更简单；恢复阻塞 */
    int fl = fcntl(cfd, F_GETFL, 0);
    if (fl >= 0) fcntl(cfd, F_SETFL, fl & ~O_NONBLOCK);

    client_t *c = calloc(1, sizeof(client_t));
    if (!c) { close(cfd); return; }
    c->fd = cfd;
    c->udp_rtp_fd = -1;
    c->udp_rtcp_fd = -1;
    c->tr = TR_NONE;
    c->ssrc = (uint32_t)((caddr->sin_addr.s_addr & 0xffff) ^ (time(NULL) & 0xffff)) | 0x80000000;

    char buf[4096];
    while (g_running) {
        /* 读一行请求（简化：一次读整段，按 \r\n\r\n 分） */
        ssize_t n = recv(cfd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;
        buf[n] = 0;

        /* 解析方法与 URL */
        char method[16] = {0}, url[256] = {0};
        sscanf(buf, "%15s %255s", method, url);

        char resp[2048];
        int rlen = 0;
        char transport[256] = {0};
        char session_hdr[64];

        if (!strcmp(method, "OPTIONS")) {
            rlen = snprintf(resp, sizeof(resp),
                "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
                "Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN, GET_PARAMETER\r\n\r\n",
                hdr_get(buf, "CSeq", transport, sizeof(transport)) ? transport : "1");
        } else if (!strcmp(method, "DESCRIBE")) {
            rlen = snprintf(resp, sizeof(resp),
                "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
                "Content-Type: application/sdp\r\n"
                "Content-Length: %d\r\n\r\n%s",
                hdr_get(buf, "CSeq", transport, sizeof(transport)) ? transport : "1",
                g_sdp_len, g_sdp);
        } else if (!strcmp(method, "SETUP")) {
            hdr_get(buf, "Transport", transport, sizeof(transport));
            char cseq[16];
            hdr_get(buf, "CSeq", cseq, sizeof(cseq));
            /* 生成 session id */
            c->session_id = (uint32_t)(time(NULL) ^ (caddr->sin_port << 16)) | 0x10000;
            snprintf(session_hdr, sizeof(session_hdr), "Session: %u;timeout=60", c->session_id);

            if (strstr(transport, "TCP") || strstr(transport, "interleaved")) {
                /* TCP interleaved */
                c->tr = TR_TCP;
                c->rtp_channel = 0;
                int ch = 0;
                const char *p = strstr(transport, "interleaved=");
                if (p) ch = atoi(p + 12);
                c->rtp_channel = ch;
                rlen = snprintf(resp, sizeof(resp),
                    "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%s\r\n"
                    "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n\r\n",
                    cseq, session_hdr, ch, ch + 1);
                LOG_INF("rtsp SETUP TCP interleaved=%d session=%u from %s:%d",
                        ch, c->session_id, inet_ntoa(caddr->sin_addr), ntohs(caddr->sin_port));
            } else {
                /* UDP unicast */
                c->tr = TR_UDP;
                int cport = 0;
                const char *p = strstr(transport, "client_port=");
                if (p) cport = atoi(p + 12);
                c->client_rtp_port = cport;
                /* 分配服务端 UDP 端口对 */
                c->udp_rtp_fd = socket(AF_INET, SOCK_DGRAM, 0);
                c->udp_rtcp_fd = socket(AF_INET, SOCK_DGRAM, 0);
                struct sockaddr_in sa; memset(&sa, 0, sizeof(sa));
                sa.sin_family = AF_INET; sa.sin_addr.s_addr = INADDR_ANY;
                sa.sin_port = 0;
                bind(c->udp_rtp_fd, (struct sockaddr *)&sa, sizeof(sa));
                bind(c->udp_rtcp_fd, (struct sockaddr *)&sa, sizeof(sa));
                socklen_t sl = sizeof(sa);
                int sport_rtp = 0, sport_rtcp = 0;
                getsockname(c->udp_rtp_fd, (struct sockaddr *)&sa, &sl); sport_rtp = ntohs(sa.sin_port);
                sl = sizeof(sa);
                getsockname(c->udp_rtcp_fd, (struct sockaddr *)&sa, &sl); sport_rtcp = ntohs(sa.sin_port);
                set_nonblock(c->udp_rtp_fd);
                /* 客户端地址 */
                memset(&c->client_rtp_addr, 0, sizeof(c->client_rtp_addr));
                c->client_rtp_addr.sin_family = AF_INET;
                c->client_rtp_addr.sin_addr = caddr->sin_addr;
                c->client_rtp_addr.sin_port = htons(cport);
                memset(&c->client_rtcp_addr, 0, sizeof(c->client_rtcp_addr));
                c->client_rtcp_addr.sin_family = AF_INET;
                c->client_rtcp_addr.sin_addr = caddr->sin_addr;
                c->client_rtcp_addr.sin_port = htons(cport + 1);
                rlen = snprintf(resp, sizeof(resp),
                    "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%s\r\n"
                    "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n\r\n",
                    cseq, session_hdr, cport, cport + 1, sport_rtp, sport_rtcp);
                LOG_INF("rtsp SETUP UDP client_port=%d server_port=%d-%d session=%u from %s:%d",
                        cport, sport_rtp, sport_rtcp, c->session_id,
                        inet_ntoa(caddr->sin_addr), ntohs(caddr->sin_port));
            }
        } else if (!strcmp(method, "PLAY")) {
            char cseq[16]; hdr_get(buf, "CSeq", cseq, sizeof(cseq));
            c->playing = 1;
            c->seq = 0;
            /* 加入客户端链表 */
            pthread_mutex_lock(&g_mtx);
            c->next = g_clients;
            g_clients = c;
            pthread_mutex_unlock(&g_mtx);
            rlen = snprintf(resp, sizeof(resp),
                "RTSP/1.0 200 OK\r\nCSeq: %s\r\nSession: %u\r\n"
                "Range: npt=0.000-\r\n"
                "RTP-Info: url=%s;seq=0;rtptime=0\r\n\r\n",
                cseq, c->session_id, url);
            LOG_INF("rtsp PLAY session=%u 开始推流", c->session_id);
        } else if (!strcmp(method, "GET_PARAMETER")) {
            char cseq[16]; hdr_get(buf, "CSeq", cseq, sizeof(cseq));
            rlen = snprintf(resp, sizeof(resp), "RTSP/1.0 200 OK\r\nCSeq: %s\r\nSession: %u\r\n\r\n",
                            cseq, c->session_id ? c->session_id : 0);
        } else if (!strcmp(method, "TEARDOWN")) {
            char cseq[16]; hdr_get(buf, "CSeq", cseq, sizeof(cseq));
            rlen = snprintf(resp, sizeof(resp), "RTSP/1.0 200 OK\r\nCSeq: %s\r\nSession: %u\r\n\r\n",
                            cseq, c->session_id);
            /* 从链表移除 */
            pthread_mutex_lock(&g_mtx);
            client_t **pp = &g_clients;
            while (*pp && *pp != c) pp = &(*pp)->next;
            if (*pp) *pp = c->next;
            pthread_mutex_unlock(&g_mtx);
            LOG_INF("rtsp TEARDOWN session=%u", c->session_id);
            send_all(cfd, resp, rlen);
            break;
        } else {
            rlen = snprintf(resp, sizeof(resp), "RTSP/1.0 501 Not Implemented\r\n\r\n");
        }

        if (rlen > 0) {
            if (send_all(cfd, resp, rlen) < 0) break;
        }
    }

    /* 客户端断开：若已入链表则移除 */
    pthread_mutex_lock(&g_mtx);
    client_t **pp = &g_clients;
    while (*pp && *pp != c) pp = &(*pp)->next;
    if (*pp) *pp = c->next;
    pthread_mutex_unlock(&g_mtx);
    close_client(c);
    LOG_INF("rtsp 控制连接断开");
}

static void *listen_thread(void *arg)
{
    (void)arg;
    while (g_running) {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        int cfd = accept(g_listen_fd, (struct sockaddr *)&caddr, &clen);
        if (cfd < 0) { if (errno == EINTR) continue; continue; }
        handle_client(cfd, &caddr);
    }
    return NULL;
}

int rtsp_server_init(int port, const AVCodecParameters *codecpar)
{
    /* 提取 SPS/PPS */
    g_have_sps = 0; g_have_pps = 0;
    if (codecpar && codecpar->extradata && codecpar->extradata_size > 0) {
        parse_nalus(codecpar->extradata, codecpar->extradata_size, sdp_collect_cb, NULL);
    }
    build_sdp(g_sdp, sizeof(g_sdp));
    g_sdp_len = (int)strlen(g_sdp);

    g_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listen_fd < 0) { LOG_ERR("rtsp socket 失败"); return -1; }
    int opt = 1;
    setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(g_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERR("rtsp bind :%d 失败: %s", port, strerror(errno));
        close(g_listen_fd); g_listen_fd = -1; return -1;
    }
    if (listen(g_listen_fd, 8) < 0) {
        LOG_ERR("rtsp listen 失败"); close(g_listen_fd); g_listen_fd = -1; return -1;
    }
    g_running = 1;
    if (pthread_create(&g_tid, NULL, listen_thread, NULL) != 0) {
        g_running = 0; close(g_listen_fd); g_listen_fd = -1; return -1;
    }
    LOG_INF("rtsp_server 就绪: 端口 %d, SPS=%s PPS=%s", port,
            g_have_sps ? "(已获取)" : "(缺省)", g_have_pps ? "(已获取)" : "(缺省)");
    return 0;
}

int rtsp_server_client_count(void)
{
    int n = 0;
    pthread_mutex_lock(&g_mtx);
    for (client_t *c = g_clients; c; c = c->next) {
        if (c->playing) n++;
    }
    pthread_mutex_unlock(&g_mtx);
    return n;
}

void rtsp_server_deinit(void)
{
    if (!g_running) return;
    if (!g_running) return;
    g_running = 0;
    if (g_listen_fd >= 0) { shutdown(g_listen_fd, SHUT_RDWR); close(g_listen_fd); g_listen_fd = -1; }
    pthread_join(g_tid, NULL);
    /* 关闭所有客户端 */
    pthread_mutex_lock(&g_mtx);
    client_t *c = g_clients;
    while (c) { client_t *n = c->next; close_client(c); c = n; }
    g_clients = NULL;
    pthread_mutex_unlock(&g_mtx);
    LOG_INF("rtsp_server 已停止");
}
