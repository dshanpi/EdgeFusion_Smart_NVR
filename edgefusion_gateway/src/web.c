#include "web.h"
#include "log.h"
#include "conf.h"
#include "frame_grabber.h"
#include "event_bus.h"
#include "stream_receiver.h"
#include "rtsp_server.h"
#include "cloud_ai.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static volatile int g_running = 0;
static pthread_t    g_tid;
static int          g_listen_fd = -1;
static char         g_bind[64];
static int          g_port;
static char         g_storage_dir[256];
static char         g_web_root[256];
static gateway_conf_t g_cfg;

/* ─── URL decode ─── */
static void url_decode(char *s)
{
    char *p = s;
    while (*s) {
        if (*s == '%' && isxdigit((unsigned char)s[1]) && isxdigit((unsigned char)s[2])) {
            int hi = (s[1] >= 'A') ? (s[1] & 0xDF) - 'A' + 10 : s[1] - '0';
            int lo = (s[2] >= 'A') ? (s[2] & 0xDF) - 'A' + 10 : s[2] - '0';
            *p++ = (char)((hi << 4) | lo);
            s += 3;
        } else if (*s == '+') {
            *p++ = ' '; s++;
        } else {
            *p++ = *s++;
        }
    }
    *p = 0;
}

/* ─── MIME type from file extension ─── */
static const char *mime_type(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (!strcmp(ext, ".html") || !strcmp(ext, ".htm")) return "text/html; charset=utf-8";
    if (!strcmp(ext, ".css"))  return "text/css; charset=utf-8";
    if (!strcmp(ext, ".js"))   return "application/javascript; charset=utf-8";
    if (!strcmp(ext, ".json")) return "application/json; charset=utf-8";
    if (!strcmp(ext, ".png"))  return "image/png";
    if (!strcmp(ext, ".jpg") || !strcmp(ext, ".jpeg")) return "image/jpeg";
    if (!strcmp(ext, ".svg"))  return "image/svg+xml";
    if (!strcmp(ext, ".ico"))  return "image/x-icon";
    if (!strcmp(ext, ".woff2"))return "font/woff2";
    if (!strcmp(ext, ".woff")) return "font/woff";
    if (!strcmp(ext, ".mp4"))  return "video/mp4";
    return "application/octet-stream";
}

/* ─── JSON string escape ─── */
static int json_escape(char *dst, size_t cap, const char *s)
{
    size_t o = 0;
    for (; *s && o + 2 < cap; s++) {
        switch (*s) {
            case '"':  if (o + 2 < cap) { dst[o++]='\\'; dst[o++]='"'; } break;
            case '\\': if (o + 2 < cap) { dst[o++]='\\'; dst[o++]='\\'; } break;
            case '\n': if (o + 2 < cap) { dst[o++]='\\'; dst[o++]='n'; } break;
            case '\r': if (o + 2 < cap) { dst[o++]='\\'; dst[o++]='r'; } break;
            case '\t': if (o + 2 < cap) { dst[o++]='\\'; dst[o++]='t'; } break;
            default:
                if ((unsigned char)*s < 0x20) {}
                else { dst[o++] = *s; }
        }
    }
    dst[o] = 0;
    return (int)o;
}

/* ─── Send minimal HTTP response ─── */
static void send_http(int cfd, int code, const char *ctype, const char *body, int body_len)
{
    const char *status = (code == 200) ? "200 OK" :
                         (code == 206) ? "206 Partial Content" :
                         (code == 400) ? "400 Bad Request" :
                         (code == 403) ? "403 Forbidden" :
                         (code == 404) ? "404 Not Found" :
                         (code == 405) ? "405 Method Not Allowed" : "500 Internal Server Error";
    char hdr[512];
    int hlen;
    if (ctype) {
        hlen = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
            status, ctype, body_len);
    } else {
        hlen = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
            status, body_len);
    }
    send(cfd, hdr, hlen, 0);
    if (body && body_len > 0) send(cfd, body, body_len, 0);
}

static void send_json(int cfd, int code, const char *body, int len)
{
    send_http(cfd, code, "application/json; charset=utf-8", body, len);
}

/* ─── Static file serving ─── */
static void serve_static(int cfd, const char *url_path)
{
    /* Default to index.html for "/" */
    if (!strcmp(url_path, "/")) url_path = "/index.html";

    /* Path traversal guard */
    if (strstr(url_path, "..")) {
        const char *e = "HTTP/1.1 403 Forbidden\r\nConnection: close\r\n\r\n";
        send(cfd, e, strlen(e), 0);
        return;
    }

    char fpath[512];
    snprintf(fpath, sizeof(fpath), "%s%s", g_web_root, url_path);

    /* If directory, try index.html */
    struct stat st;
    if (stat(fpath, &st) == 0 && S_ISDIR(st.st_mode)) {
        size_t l = strlen(fpath);
        snprintf(fpath + l, sizeof(fpath) - l, "/index.html");
    }

    int fd = open(fpath, O_RDONLY);
    if (fd < 0) {
        const char *e = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        send(cfd, e, strlen(e), 0);
        return;
    }

    fstat(fd, &st);
    long total = (long)st.st_size;

    const char *mime = mime_type(fpath);
    char hdr[512];
    int hlen = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n"
        "Cache-Control: max-age=3600\r\nConnection: close\r\n\r\n",
        mime, total);
    send(cfd, hdr, hlen, 0);

    char buf[32 * 1024];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (send(cfd, buf, n, 0) < 0) break;
    }
    close(fd);
}

/* ─── MJPEG stream ─── */
static void serve_mjpeg(int cfd)
{
    const char *header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=framebound\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: close\r\n\r\n";
    if (send(cfd, header, strlen(header), 0) < 0) return;

    int empty_count = 0;
    while (g_running) {
        uint8_t *jpeg = NULL;
        int sz = frame_grabber_get_jpeg(&jpeg);
        if (sz <= 0 || !jpeg) {
            if (jpeg) free(jpeg);
            usleep(100 * 1000);
            if (++empty_count > 100) break;
            continue;
        }
        empty_count = 0;

        char part_hdr[128];
        int hlen = snprintf(part_hdr, sizeof(part_hdr),
            "\r\n--framebound\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", sz);
        if (send(cfd, part_hdr, hlen, 0) < 0) { free(jpeg); break; }
        if (send(cfd, jpeg, sz, 0) < 0) { free(jpeg); break; }
        send(cfd, "\r\n", 2, 0);
        free(jpeg);
        usleep(200 * 1000);
    }
}

/* ─── Single JPEG snapshot ─── */
static void serve_snapshot_jpeg(int cfd)
{
    uint8_t *jpeg = NULL;
    int sz = frame_grabber_get_jpeg(&jpeg);
    if (sz <= 0 || !jpeg) {
        const char *e = "HTTP/1.1 503 Service Unavailable\r\n"
                        "Content-Type: text/plain\r\nConnection: close\r\n\r\n"
                        "No frame available";
        send(cfd, e, strlen(e), 0);
        return;
    }
    char hdr[256];
    int hlen = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n"
        "Cache-Control: no-cache\r\nConnection: close\r\n\r\n", sz);
    send(cfd, hdr, hlen, 0);
    send(cfd, jpeg, sz, 0);
    free(jpeg);
}

/* ─── /api/status ─── */
static void serve_status(int cfd)
{
    char json[4096];
    int off = 0;

    stream_status_t ss;
    int have_stream = (stream_receiver_get_status(&ss) == 0);

    /* stream_receiver owns the recorder; status includes recording/cur_segment/bitrate */

    int rtsp_clients = rtsp_server_client_count();

    cloud_ai_status_t ai;
    int have_ai = (cloud_ai_get_status(&ai) == 0);

    long total_events = event_bus_count(NULL);

    off += snprintf(json + off, sizeof(json) - off, "{");

    /* stream */
    off += snprintf(json + off, sizeof(json) - off,
        "\"stream\":{");
    if (have_stream) {
        off += snprintf(json + off, sizeof(json) - off,
            "\"connected\":%s,\"width\":%d,\"height\":%d,\"fps\":%d,"
            "\"recording\":%s,\"cur_segment\":\"%s\",\"bitrate_kbps\":%d",
            ss.connected ? "true" : "false",
            ss.width, ss.height, ss.fps,
            ss.recording ? "true" : "false",
            ss.cur_segment, ss.bitrate_kbps);
    } else {
        off += snprintf(json + off, sizeof(json) - off,
            "\"connected\":false");
    }
    off += snprintf(json + off, sizeof(json) - off, "}");

    /* rtsp_server */
    off += snprintf(json + off, sizeof(json) - off,
        ",\"rtsp_server\":{\"clients\":%d}", rtsp_clients);

    /* cloud_ai */
    off += snprintf(json + off, sizeof(json) - off,
        ",\"cloud_ai\":{");
    if (have_ai) {
        off += snprintf(json + off, sizeof(json) - off,
            "\"enabled\":%s,\"running\":%s",
            ai.enabled ? "true" : "false",
            ai.running ? "true" : "false");
    } else {
        off += snprintf(json + off, sizeof(json) - off,
            "\"enabled\":false,\"running\":false");
    }
    off += snprintf(json + off, sizeof(json) - off, "}");

    /* events total */
    off += snprintf(json + off, sizeof(json) - off,
        ",\"events_total\":%ld", total_events);

    off += snprintf(json + off, sizeof(json) - off, "}");
    send_json(cfd, 200, json, off);
}

/* ─── /api/recordings ─── */
static void serve_recordings(int cfd)
{
    char json[8192];
    int off = 0;
    off += snprintf(json + off, sizeof(json) - off, "[");

    DIR *d = opendir(g_storage_dir);
    if (d) {
        struct dirent *e;
        int first = 1;
        while ((e = readdir(d)) != NULL) {
            const char *name = e->d_name;
            if (name[0] == '.') continue;
            size_t l = strlen(name);
            if (l < 5 || strcmp(name + l - 4, ".mp4")) continue;
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", g_storage_dir, name);
            struct stat st;
            if (stat(path, &st) != 0) continue;
            if (!first) off += snprintf(json + off, sizeof(json) - off, ",");
            first = 0;
            off += snprintf(json + off, sizeof(json) - off,
                "{\"name\":\"%s\",\"size\":%ld}", name, (long)st.st_size);
        }
        closedir(d);
    }
    off += snprintf(json + off, sizeof(json) - off, "]");
    send_json(cfd, 200, json, off);
}

/* ─── /rec/<name> mp4 file serve with Range support ─── */
static void serve_file(int cfd, const char *name, const char *range_hdr)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", g_storage_dir, name);
    if (strstr(name, "..")) {
        const char *e = "HTTP/1.1 403 Forbidden\r\nConnection: close\r\n\r\n";
        send(cfd, e, strlen(e), 0); return;
    }
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        const char *e = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        send(cfd, e, strlen(e), 0); return;
    }
    struct stat st;
    fstat(fd, &st);
    long total = (long)st.st_size;
    long start = 0, end = total - 1;

    int partial = 0;
    if (range_hdr && strstr(range_hdr, "bytes=")) {
        const char *p = strstr(range_hdr, "bytes=") + 6;
        start = strtol(p, NULL, 10);
        const char *dash = strchr(p, '-');
        if (dash && *(dash + 1) >= '0' && *(dash + 1) <= '9') {
            end = strtol(dash + 1, NULL, 10);
        }
        if (start < 0 || start >= total) start = 0;
        if (end < 0 || end >= total) end = total - 1;
        partial = 1;
    }

    long content_len = end - start + 1;
    char hdr[512];
    int hlen;
    if (partial) {
        hlen = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 206 Partial Content\r\n"
            "Content-Type: video/mp4\r\n"
            "Accept-Ranges: bytes\r\n"
            "Content-Range: bytes %ld-%ld/%ld\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n\r\n",
            start, end, total, content_len);
    } else {
        hlen = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: video/mp4\r\n"
            "Accept-Ranges: bytes\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n\r\n",
            content_len);
    }
    send(cfd, hdr, hlen, 0);

    lseek(fd, start, SEEK_SET);
    char buf[32 * 1024];
    long sent = 0;
    while (sent < content_len) {
        long want = content_len - sent;
        if (want > (long)sizeof(buf)) want = sizeof(buf);
        ssize_t n = read(fd, buf, want);
        if (n <= 0) break;
        if (send(cfd, buf, n, 0) < 0) break;
        sent += n;
    }
    close(fd);
}

/* ─── /api/events ─── */
static void serve_events(int cfd, const char *query)
{
    int limit = 50, offset = 0;
    char type_filter[32] = {0};
    if (query) {
        const char *p = strstr(query, "limit=");
        if (p) limit = atoi(p + 6);
        p = strstr(query, "offset=");
        if (p) offset = atoi(p + 7);
        p = strstr(query, "type=");
        if (p) {
            const char *v = p + 5;
            const char *amp = strchr(v, '&');
            int vl = amp ? (int)(amp - v) : (int)strlen(v);
            if (vl > 31) vl = 31;
            memcpy(type_filter, v, vl);
            type_filter[vl] = 0;
            url_decode(type_filter);
        }
        if (limit <= 0 || limit > 200) limit = 50;
        if (offset < 0) offset = 0;
    }

    event_t evs[64];
    int n = event_bus_query_filtered(evs, limit > 64 ? 64 : limit,
                                     type_filter[0] ? type_filter : NULL, offset);

    size_t cap = 4096;
    char *json = malloc(cap);
    int off = 0;
    off += snprintf(json + off, cap - off, "[");
    for (int i = 0; i < n; i++) {
        char esc_desc[600];
        json_escape(esc_desc, sizeof(esc_desc), evs[i].description);
        const char *snap = "";
        char snapname[64] = {0};
        if (evs[i].jpeg_path[0]) {
            const char *base = strrchr(evs[i].jpeg_path, '/');
            snprintf(snapname, sizeof(snapname), "%s", base ? base + 1 : evs[i].jpeg_path);
            snap = snapname;
        }
        int need = 512;
        while ((int)cap - off < need) {
            cap *= 2;
            char *nb = realloc(json, cap);
            if (!nb) { free(json); return; }
            json = nb;
        }
        off += snprintf(json + off, cap - off,
            "%s{\"id\":%ld,\"ts\":%lld,\"event_type\":\"%s\",\"confidence\":%.2f,"
            "\"description\":\"%s\",\"suggested_action\":\"%s\",\"snapshot\":\"%s\"}",
            i ? "," : "",
            evs[i].id, (long long)evs[i].created_at,
            evs[i].event_type, evs[i].confidence,
            esc_desc, evs[i].suggested_action, snap);
    }
    off += snprintf(json + off, cap - off, "]");
    send_json(cfd, 200, json, off);
    free(json);
}

/* ─── /api/event/<id> ─── */
static void serve_event_detail(int cfd, long id)
{
    event_t ev;
    if (event_bus_get(id, &ev) != 0) {
        const char *e = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        send(cfd, e, strlen(e), 0);
        return;
    }
    char esc_desc[600];
    json_escape(esc_desc, sizeof(esc_desc), ev.description);
    const char *base = strrchr(ev.jpeg_path, '/');
    char json[1024];
    int off = snprintf(json, sizeof(json),
        "{\"id\":%ld,\"ts\":%lld,\"event_type\":\"%s\",\"confidence\":%.2f,"
        "\"description\":\"%s\",\"suggested_action\":\"%s\",\"snapshot\":\"%s\"}",
        ev.id, (long long)ev.created_at, ev.event_type, ev.confidence,
        esc_desc, ev.suggested_action, ev.jpeg_path[0] ? (base ? base + 1 : ev.jpeg_path) : "");
    send_json(cfd, 200, json, off);
}

/* ─── /snap/evt_<id>.jpg ─── */
static void serve_snapshot(int cfd, const char *name)
{
    if (strstr(name, "..")) {
        const char *e = "HTTP/1.1 403 Forbidden\r\nConnection: close\r\n\r\n";
        send(cfd, e, strlen(e), 0); return;
    }
    long id = -1;
    if (sscanf(name, "evt_%ld.jpg", &id) != 1) {
        const char *e = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        send(cfd, e, strlen(e), 0); return;
    }
    event_t ev;
    if (event_bus_get(id, &ev) != 0 || ev.jpeg_path[0] == 0) {
        const char *e = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        send(cfd, e, strlen(e), 0); return;
    }
    int fd = open(ev.jpeg_path, O_RDONLY);
    if (fd < 0) {
        const char *e = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        send(cfd, e, strlen(e), 0); return;
    }
    struct stat st;
    fstat(fd, &st);
    char hdr[256];
    int hlen = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: %ld\r\n"
        "Cache-Control: no-cache\r\nConnection: close\r\n\r\n", (long)st.st_size);
    send(cfd, hdr, hlen, 0);
    char buf[16 * 1024];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (send(cfd, buf, n, 0) < 0) break;
    }
    close(fd);
}

/* ─── /api/config GET (mask sensitive) ─── */
static void serve_config_get(int cfd)
{
    char json[4096];
    int off = 0;
    gateway_conf_t *c = &g_cfg;

    off += snprintf(json + off, sizeof(json) - off, "{");

    /* RTSP */
    off += snprintf(json + off, sizeof(json) - off,
        "\"rtsp_url\":\"%s\",\"rtsp_sub_url\":\"%s\",\"rtsp_timeout_us\":%ld,"
        "\"rtsp_transport\":\"%s\",\"rtsp_reconnect_s\":%d,",
        c->rtsp_url, c->rtsp_sub_url, c->rtsp_timeout_us,
        c->rtsp_transport, c->rtsp_reconnect_s);

    /* RTSP server */
    off += snprintf(json + off, sizeof(json) - off,
        "\"rtsp_server_enable\":%s,\"rtsp_server_port\":%d,",
        c->rtsp_server_enable ? "true" : "false", c->rtsp_server_port);

    /* recorder */
    off += snprintf(json + off, sizeof(json) - off,
        "\"storage_dir\":\"%s\",\"segment_seconds\":%d,\"storage_min_free_mb\":%ld,"
        "\"segment_prefix\":\"%s\",",
        c->storage_dir, c->segment_seconds, c->storage_min_free_mb, c->segment_prefix);

    /* AI (mask token) */
    off += snprintf(json + off, sizeof(json) - off,
        "\"ai_enable\":%s,\"ai_fps\":%d,\"ai_frame_width\":%d,\"ai_jpeg_quality\":%d,"
        "\"ai_http_timeout_s\":%d,\"ai_max_queue\":%d,"
        "\"anthropic_base_url\":\"%s\",",
        c->ai_enable ? "true" : "false",
        c->ai_fps, c->ai_frame_width, c->ai_jpeg_quality,
        c->ai_http_timeout_s, c->ai_max_queue, c->anthropic_base_url);
    /* Token: show first 8 chars mask rest */
    if (c->anthropic_auth_token[0]) {
        char masked[64];
        int tl = (int)strlen(c->anthropic_auth_token);
        if (tl <= 8) {
            snprintf(masked, sizeof(masked), "****");
        } else {
            snprintf(masked, sizeof(masked), "%.8s****", c->anthropic_auth_token);
        }
        off += snprintf(json + off, sizeof(json) - off,
            "\"anthropic_auth_token\":\"%s\",", masked);
    } else {
        off += snprintf(json + off, sizeof(json) - off,
            "\"anthropic_auth_token\":\"\",");
    }
    off += snprintf(json + off, sizeof(json) - off,
        "\"anthropic_model\":\"%s\",\"ai_prompt\":\"%s\",",
        c->anthropic_model, c->ai_prompt);

    /* alarm */
    off += snprintf(json + off, sizeof(json) - off,
        "\"alarm_gpio_led\":%d,\"alarm_gpio_buzzer\":%d,"
        "\"alarm_trigger_types\":\"%s\",\"alarm_min_confidence\":%.2f,"
        "\"alarm_active_s\":%d,",
        c->alarm_gpio_led, c->alarm_gpio_buzzer,
        c->alarm_trigger_types, c->alarm_min_confidence, c->alarm_active_s);

    /* web */
    off += snprintf(json + off, sizeof(json) - off,
        "\"web_bind\":\"%s\",\"web_port\":%d,\"web_mjpeg_fps\":%d,\"web_root\":\"%s\",",
        c->web_bind, c->web_port, c->web_mjpeg_fps, c->web_root);

    /* MQTT (mask pass) */
    off += snprintf(json + off, sizeof(json) - off,
        "\"mqtt_enable\":%s,\"mqtt_host\":\"%s\",\"mqtt_port\":%d,"
        "\"mqtt_client_id\":\"%s\",\"mqtt_topic\":\"%s\","
        "\"mqtt_user\":\"%s\",\"mqtt_pass\":\"%s\",",
        c->mqtt_enable ? "true" : "false",
        c->mqtt_host, c->mqtt_port, c->mqtt_client_id, c->mqtt_topic,
        c->mqtt_user, c->mqtt_pass[0] ? "****" : "");

    /* log */
    off += snprintf(json + off, sizeof(json) - off,
        "\"log_level\":\"%s\",\"log_file\":\"%s\"",
        c->log_level, c->log_file);

    off += snprintf(json + off, sizeof(json) - off, "}");
    send_json(cfd, 200, json, off);
}

/* ─── Simple JSON string value extractor (no library) ─── */
static int json_get_str(const char *json, const char *key, char *out, int outcap)
{
    char pat[128];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(json, pat);
    if (!p) return -1;
    p += strlen(pat);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    if (*p != '"') return -1;
    p++;
    int o = 0;
    while (*p && *p != '"' && o < outcap - 1) {
        if (*p == '\\' && *(p + 1)) {
            p++;
            switch (*p) {
                case '"':  out[o++] = '"'; break;
                case '\\': out[o++] = '\\'; break;
                case 'n':  out[o++] = '\n'; break;
                case 'r':  out[o++] = '\r'; break;
                case 't':  out[o++] = '\t'; break;
                default:   out[o++] = *p; break;
            }
        } else {
            out[o++] = *p;
        }
        p++;
    }
    out[o] = 0;
    return 0;
}

static int json_get_int(const char *json, const char *key, int *out)
{
    char pat[128];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(json, pat);
    if (!p) return -1;
    p += strlen(pat);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    *out = atoi(p);
    return 0;
}

static int json_get_float(const char *json, const char *key, float *out)
{
    char pat[128];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(json, pat);
    if (!p) return -1;
    p += strlen(pat);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    *out = (float)atof(p);
    return 0;
}

static int json_get_bool(const char *json, const char *key, int *out)
{
    char pat[128];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(json, pat);
    if (!p) return -1;
    p += strlen(pat);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    if (!strncmp(p, "true", 4)) { *out = 1; return 0; }
    *out = 0;
    return 0;
}

/* ─── /api/config POST ─── */
static void serve_config_post(int cfd, const char *body, int body_len)
{
    if (!body || body_len <= 0) {
        const char *e = "{\"ok\":false,\"error\":\"empty body\"}";
        send_json(cfd, 400, e, (int)strlen(e));
        return;
    }

    /* Copy body to null-terminated buffer */
    char *js = malloc(body_len + 1);
    memcpy(js, body, body_len);
    js[body_len] = 0;

    gateway_conf_t new_cfg = g_cfg; /* start from current */

    /* Parse and overlay each field if present in JSON */
    char buf[1024];
    int iv; float fv;

    if (json_get_str(js, "rtsp_url", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.rtsp_url, buf, sizeof(new_cfg.rtsp_url) - 1);
    if (json_get_str(js, "rtsp_sub_url", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.rtsp_sub_url, buf, sizeof(new_cfg.rtsp_sub_url) - 1);
    if (json_get_int(js, "rtsp_timeout_us", &iv) == 0) new_cfg.rtsp_timeout_us = iv;
    if (json_get_str(js, "rtsp_transport", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.rtsp_transport, buf, sizeof(new_cfg.rtsp_transport) - 1);
    if (json_get_int(js, "rtsp_reconnect_s", &iv) == 0) new_cfg.rtsp_reconnect_s = iv;
    if (json_get_bool(js, "rtsp_server_enable", &iv) == 0) new_cfg.rtsp_server_enable = iv;
    if (json_get_int(js, "rtsp_server_port", &iv) == 0) new_cfg.rtsp_server_port = iv;
    if (json_get_str(js, "storage_dir", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.storage_dir, buf, sizeof(new_cfg.storage_dir) - 1);
    if (json_get_int(js, "segment_seconds", &iv) == 0) new_cfg.segment_seconds = iv;
    if (json_get_int(js, "storage_min_free_mb", &iv) == 0) new_cfg.storage_min_free_mb = iv;
    if (json_get_bool(js, "ai_enable", &iv) == 0) new_cfg.ai_enable = iv;
    if (json_get_int(js, "ai_fps", &iv) == 0) new_cfg.ai_fps = iv;
    if (json_get_int(js, "ai_frame_width", &iv) == 0) new_cfg.ai_frame_width = iv;
    if (json_get_int(js, "ai_jpeg_quality", &iv) == 0) new_cfg.ai_jpeg_quality = iv;
    if (json_get_int(js, "ai_http_timeout_s", &iv) == 0) new_cfg.ai_http_timeout_s = iv;
    if (json_get_int(js, "ai_max_queue", &iv) == 0) new_cfg.ai_max_queue = iv;
    if (json_get_str(js, "anthropic_base_url", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.anthropic_base_url, buf, sizeof(new_cfg.anthropic_base_url) - 1);
    if (json_get_str(js, "anthropic_auth_token", buf, sizeof(buf)) == 0) {
        /* Only overwrite if it's not "****" (masked placeholder) */
        if (strcmp(buf, "****"))
            strncpy(new_cfg.anthropic_auth_token, buf, sizeof(new_cfg.anthropic_auth_token) - 1);
    }
    if (json_get_str(js, "anthropic_model", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.anthropic_model, buf, sizeof(new_cfg.anthropic_model) - 1);
    if (json_get_str(js, "ai_prompt", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.ai_prompt, buf, sizeof(new_cfg.ai_prompt) - 1);
    if (json_get_int(js, "alarm_gpio_led", &iv) == 0) new_cfg.alarm_gpio_led = iv;
    if (json_get_int(js, "alarm_gpio_buzzer", &iv) == 0) new_cfg.alarm_gpio_buzzer = iv;
    if (json_get_str(js, "alarm_trigger_types", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.alarm_trigger_types, buf, sizeof(new_cfg.alarm_trigger_types) - 1);
    if (json_get_float(js, "alarm_min_confidence", &fv) == 0) new_cfg.alarm_min_confidence = fv;
    if (json_get_int(js, "alarm_active_s", &iv) == 0) new_cfg.alarm_active_s = iv;
    if (json_get_str(js, "web_bind", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.web_bind, buf, sizeof(new_cfg.web_bind) - 1);
    if (json_get_int(js, "web_port", &iv) == 0) new_cfg.web_port = iv;
    if (json_get_int(js, "web_mjpeg_fps", &iv) == 0) new_cfg.web_mjpeg_fps = iv;
    if (json_get_str(js, "web_root", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.web_root, buf, sizeof(new_cfg.web_root) - 1);
    if (json_get_bool(js, "mqtt_enable", &iv) == 0) new_cfg.mqtt_enable = iv;
    if (json_get_str(js, "mqtt_host", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.mqtt_host, buf, sizeof(new_cfg.mqtt_host) - 1);
    if (json_get_int(js, "mqtt_port", &iv) == 0) new_cfg.mqtt_port = iv;
    if (json_get_str(js, "mqtt_client_id", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.mqtt_client_id, buf, sizeof(new_cfg.mqtt_client_id) - 1);
    if (json_get_str(js, "mqtt_topic", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.mqtt_topic, buf, sizeof(new_cfg.mqtt_topic) - 1);
    if (json_get_str(js, "mqtt_user", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.mqtt_user, buf, sizeof(new_cfg.mqtt_user) - 1);
    if (json_get_str(js, "mqtt_pass", buf, sizeof(buf)) == 0) {
        if (strcmp(buf, "****"))
            strncpy(new_cfg.mqtt_pass, buf, sizeof(new_cfg.mqtt_pass) - 1);
    }
    if (json_get_str(js, "log_level", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.log_level, buf, sizeof(new_cfg.log_level) - 1);
    if (json_get_str(js, "log_file", buf, sizeof(buf)) == 0)
        strncpy(new_cfg.log_file, buf, sizeof(new_cfg.log_file) - 1);

    free(js);

    /* Save to file */
    if (conf_save(&new_cfg) != 0) {
        const char *e = "{\"ok\":false,\"error\":\"save failed\"}";
        send_json(cfd, 500, e, (int)strlen(e));
        return;
    }

    /* Apply hot-reloadable params to running modules */
    conf_apply_hot(&new_cfg);

    /* Update local copy (except web_bind/web_port which require restart) */
    g_cfg = new_cfg;

    const char *ok = "{\"ok\":true}";
    send_json(cfd, 200, ok, (int)strlen(ok));
}

/* ─── Request header getter ─── */
static int req_hdr_get(const char *req, const char *name, char *out, int outcap)
{
    char pat[64];
    snprintf(pat, sizeof(pat), "%s:", name);
    const char *p = req;
    while (*p) {
        const char *le = strstr(p, "\r\n");
        if (!le) break;
        int ll = (int)(le - p);
        if (ll > 0) {
            int i;
            for (i = 0; i < (int)strlen(pat) && i < ll; i++)
                if (tolower((unsigned char)p[i]) != tolower((unsigned char)pat[i])) break;
            if (i == (int)strlen(pat)) {
                const char *v = p + i;
                while (*v == ' ' || *v == '\t') v++;
                int vl = ll - (int)(v - p);
                if (vl >= outcap) vl = outcap - 1;
                memcpy(out, v, vl); out[vl] = 0;
                return 1;
            }
        }
        p = le + 2;
    }
    return 0;
}

static void handle_client_inner(int cfd)
{
    char req[4096] = {0};
    ssize_t n = recv(cfd, req, sizeof(req) - 1, 0);
    if (n <= 0) return;

    /* Parse request line */
    char method[16], path[1024];
    path[0] = 0;
    sscanf(req, "%15s %1023s", method, path);
    url_decode(path);

    /* Separate query string */
    char *query = strchr(path, '?');
    if (query) { *query = 0; query++; }

    /* Find Content-Length for POST */
    char cl_str[32] = {0};
    int content_length = 0;
    if (req_hdr_get(req, "Content-Length", cl_str, sizeof(cl_str))) {
        content_length = atoi(cl_str);
    }
    /* Get request body (may be after headers in the same recv or need another) */
    const char *body_start = strstr(req, "\r\n\r\n");
    char *post_body = NULL;
    int post_len = 0;
    if (body_start) {
        body_start += 4;
        int already = (int)((req + n) - body_start);
        if (content_length > 0) {
            post_body = malloc(content_length + 1);
            /* Copy what we already have */
            int copy = already;
            if (copy > content_length) copy = content_length;
            if (copy > 0) memcpy(post_body, body_start, copy);
            /* Read the rest if needed */
            int remain = content_length - copy;
            int total = copy;
            while (remain > 0) {
                ssize_t rn = recv(cfd, post_body + total, remain, 0);
                if (rn <= 0) break;
                total += rn;
                remain -= rn;
            }
            post_body[total] = 0;
            post_len = total;
        }
    }

    /* Route */
    if (!strcmp(method, "GET")) {
        char range[128] = {0};
        req_hdr_get(req, "Range", range, sizeof(range));

        if (!strcmp(path, "/api/status")) {
            serve_status(cfd);
        } else if (!strcmp(path, "/api/snapshot")) {
            serve_snapshot_jpeg(cfd);
        } else if (!strcmp(path, "/api/recordings")) {
            serve_recordings(cfd);
        } else if (!strcmp(path, "/api/events")) {
            serve_events(cfd, query);
        } else if (!strncmp(path, "/api/event/", 11)) {
            serve_event_detail(cfd, atol(path + 11));
        } else if (!strcmp(path, "/api/config")) {
            serve_config_get(cfd);
        } else if (!strncmp(path, "/stream", 7)) {
            serve_mjpeg(cfd);
        } else if (!strncmp(path, "/rec/", 5)) {
            serve_file(cfd, path + 5, range);
        } else if (!strncmp(path, "/snap/", 6)) {
            serve_snapshot(cfd, path + 6);
        } else {
            /* Try static file from web_root */
            serve_static(cfd, path);
        }
    } else if (!strcmp(method, "POST")) {
        if (!strcmp(path, "/api/config")) {
            serve_config_post(cfd, post_body, post_len);
        } else {
            const char *e = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
            send(cfd, e, strlen(e), 0);
        }
    } else {
        const char *e = "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n";
        send(cfd, e, strlen(e), 0);
    }

    free(post_body);
}

/* ─── Thread per client ─── */
static void *client_thread(void *arg)
{
    int cfd = (int)(intptr_t)arg;
    handle_client_inner(cfd);
    close(cfd);
    return NULL;
}

static void *web_thread(void *arg)
{
    (void)arg;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(g_port);
    inet_pton(AF_INET, g_bind, &addr.sin_addr);

    g_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(g_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERR("Web bind %s:%d failed: %s", g_bind, g_port, strerror(errno));
        return NULL;
    }
    if (listen(g_listen_fd, 8) < 0) {
        LOG_ERR("Web listen failed");
        return NULL;
    }
    LOG_INF("Web server listening on %s:%d, root=%s", g_bind, g_port, g_web_root);

    while (g_running) {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        int cfd = accept(g_listen_fd, (struct sockaddr *)&caddr, &clen);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            continue;
        }
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, (void *)(intptr_t)cfd) != 0) {
            close(cfd); continue;
        }
        pthread_detach(tid);
    }
    return NULL;
}

int web_start(const gateway_conf_t *cfg)
{
    if (g_running) return 0;
    strcpy(g_bind, cfg->web_bind);
    g_port = cfg->web_port;
    strcpy(g_storage_dir, cfg->storage_dir);
    strcpy(g_web_root, cfg->web_root);
    g_cfg = *cfg;
    g_running = 1;
    if (pthread_create(&g_tid, NULL, web_thread, NULL) != 0) {
        g_running = 0;
        LOG_ERR("Failed to create web thread");
        return -1;
    }
    return 0;
}

void web_stop(void)
{
    if (!g_running) return;
    g_running = 0;
    if (g_listen_fd >= 0) { shutdown(g_listen_fd, SHUT_RDWR); close(g_listen_fd); g_listen_fd = -1; }
    pthread_join(g_tid, NULL);
    LOG_INF("Web server stopped");
}
