#include "stream_receiver.h"
#include "log.h"
#include "frame_grabber.h"
#include "rtsp_server.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static pthread_t      g_tid;
static volatile int   g_running = 0;
static AVFormatContext *g_ic = NULL;
static recorder_t     *g_rec = NULL;
static int             g_fg_inited = 0;
static int             g_rtsp_inited = 0;

/* 打开 RTSP 输入。返回视频流索引，失败 -1 */
static int open_input(const gateway_conf_t *cfg, AVFormatContext **pic, int *vstream_idx)
{
    AVFormatContext *ic = avformat_alloc_context();
    if (!ic) { LOG_ERR("avformat_alloc_context 失败"); return -1; }

    /* RTSP 选项：贴近板子 ffmpeg 命令行成功配置（仅 rtsp_transport），
     * 额外加探测限制以减少 find_stream_info 耗时与解码噪声 */
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "rtsp_transport", cfg->rtsp_transport, 0);
    av_dict_set(&opts, "analyzeduration", "1000000", 0);  /* 1s */
    av_dict_set(&opts, "probesize",         "524288", 0); /* 512KB */
    av_dict_set(&opts, "fflags", "+discardcorrupt", 0);

    int ret = avformat_open_input(&ic, cfg->rtsp_url, NULL, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        char err[128] = {0};
        av_strerror(ret, err, sizeof(err));
        LOG_ERR("打开 RTSP 失败: %s (%s)", cfg->rtsp_url, err);
        avformat_free_context(ic);
        return -1;
    }

    ret = avformat_find_stream_info(ic, NULL);
    if (ret < 0) {
        LOG_ERR("find_stream_info 失败");
        avformat_close_input(&ic);
        return -1;
    }

    int idx = -1;
    for (unsigned i = 0; i < ic->nb_streams; i++) {
        if (ic->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        LOG_ERR("未找到视频流");
        avformat_close_input(&ic);
        return -1;
    }

    AVCodecParameters *cp = ic->streams[idx]->codecpar;
    LOG_INF("视频流: %s %dx%d %d/%d fps",
            avcodec_get_name(cp->codec_id),
            cp->width, cp->height,
            ic->streams[idx]->avg_frame_rate.num,
            ic->streams[idx]->avg_frame_rate.den);

    *pic = ic;
    *vstream_idx = idx;
    return 0;
}

static void *receiver_thread(void *arg)
{
    const gateway_conf_t *cfg = (const gateway_conf_t *)arg;

    recorder_cfg_t rcfg;
    memset(&rcfg, 0, sizeof(rcfg));
    strncpy(rcfg.dir, cfg->storage_dir, sizeof(rcfg.dir) - 1);
    strncpy(rcfg.prefix, cfg->segment_prefix, sizeof(rcfg.prefix) - 1);
    rcfg.segment_seconds = cfg->segment_seconds;
    rcfg.min_free_mb = cfg->storage_min_free_mb;

    while (g_running) {
        AVFormatContext *ic = NULL;
        int vidx = -1;
        if (open_input(cfg, &ic, &vidx) != 0) {
            LOG_WRN("拉流失败，%d 秒后重连...", cfg->rtsp_reconnect_s);
            for (int i = 0; i < cfg->rtsp_reconnect_s && g_running; i++) sleep(1);
            continue;
        }
        g_ic = ic;

        /* (重)创建 recorder（用源流 codecpar） */
        if (!g_rec) {
            g_rec = recorder_create(&rcfg, ic->streams[vidx]->codecpar);
            if (!g_rec) {
                LOG_ERR("recorder_create 失败");
                avformat_close_input(&ic);
                g_ic = NULL;
                break;
            }
        }

        /* 初始化 frame_grabber（解码→缩放→JPEG，供 Web 预览/云） */
        if (!g_fg_inited) {
            frame_grabber_cfg_t fgc;
            memset(&fgc, 0, sizeof(fgc));
            fgc.target_fps  = cfg->web_mjpeg_fps;   /* 预览帧率 */
            fgc.out_width   = cfg->ai_frame_width;  /* 缩放宽度 640 */
            fgc.jpeg_quality= cfg->ai_jpeg_quality;
            if (frame_grabber_init(&fgc, ic->streams[vidx]->codecpar) == 0) {
                g_fg_inited = 1;
            } else {
                LOG_WRN("frame_grabber 初始化失败，预览/上云将不可用");
            }
        }

        /* 初始化 RTSP 本地预览转发（H.264 passthrough） */
        if (!g_rtsp_inited && cfg->rtsp_server_enable) {
            if (rtsp_server_init(cfg->rtsp_server_port, ic->streams[vidx]->codecpar) == 0) {
                g_rtsp_inited = 1;
            } else {
                LOG_WRN("rtsp_server 初始化失败，RTSP 预览将不可用");
            }
        }

        AVPacket pkt;
        av_init_packet(&pkt);
        int err_count = 0;
        while (g_running) {
            int ret = av_read_frame(ic, &pkt);
            if (ret < 0) {
                char err[128] = {0}; av_strerror(ret, err, sizeof(err));
                LOG_WRN("av_read_frame: %s，准备重连", err);
                break;
            }
            if (pkt.stream_index == vidx) {
                int is_key = (pkt.flags & AV_PKT_FLAG_KEY) != 0;
                /* 时间戳(秒)：RTSP 视频流时基通常 1/90000 */
                AVRational tb = ic->streams[vidx]->time_base;
                double ts = tb.den ? pkt.pts * (double)tb.num / tb.den : 0;
                if (recorder_write_packet(g_rec, &pkt, is_key, ts) != 0) {
                    if (++err_count > 20) { LOG_WRN("录像写错误过多，重连"); break; }
                } else {
                    err_count = 0;
                }
                /* 旁路送解码器产出 JPEG 预览帧 */
                if (g_fg_inited) {
                    frame_grabber_feed(&pkt);
                }
                /* 旁路送 RTSP server 转发给预览客户端（H.264 passthrough） */
                if (g_rtsp_inited) {
                    rtsp_server_on_packet(&pkt, &ic->streams[vidx]->time_base);
                }
            }
            av_packet_unref(&pkt);
        }
        av_packet_unref(&pkt);

        g_ic = NULL;
        avformat_close_input(&ic);

        if (g_running) {
            LOG_INF("休眠 %d 秒后重连...", cfg->rtsp_reconnect_s);
            for (int i = 0; i < cfg->rtsp_reconnect_s && g_running; i++) sleep(1);
        }
    }

    return NULL;
}

int stream_receiver_start(const gateway_conf_t *cfg)
{
    if (g_running) return 0;
    /* FFmpeg 网络层初始化（必须，否则 RTSP 连接异常） */
    avformat_network_init();
    g_running = 1;
    if (pthread_create(&g_tid, NULL, receiver_thread, (void *)cfg) != 0) {
        g_running = 0;
        LOG_ERR("创建拉流线程失败");
        return -1;
    }
    LOG_INF("拉流线程已启动");
    return 0;
}

void stream_receiver_stop(void)
{
    if (!g_running) return;
    g_running = 0;
    pthread_join(g_tid, NULL);
    if (g_ic) avformat_close_input(&g_ic);
    if (g_rec) { recorder_destroy(g_rec); g_rec = NULL; }
    if (g_fg_inited) { frame_grabber_deinit(); g_fg_inited = 0; }
    if (g_rtsp_inited) { rtsp_server_deinit(); g_rtsp_inited = 0; }
    LOG_INF("拉流线程已停止");
}

int stream_receiver_get_status(stream_status_t *st)
{
    if (!st) return -1;
    memset(st, 0, sizeof(*st));
    st->connected = (g_ic != NULL);
    if (g_ic) {
        /* 找视频流读分辨率/fps */
        for (unsigned i = 0; i < g_ic->nb_streams; i++) {
            if (g_ic->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                AVCodecParameters *cp = g_ic->streams[i]->codecpar;
                st->width = cp->width;
                st->height = cp->height;
                AVRational fr = g_ic->streams[i]->avg_frame_rate;
                if (fr.den) st->fps = (int)((fr.num + fr.den / 2) / fr.den);
                break;
            }
        }
    }
    if (g_rec) {
        recorder_status_t rs;
        if (recorder_get_status(g_rec, &rs) == 0) {
            st->recording = rs.recording;
            st->bitrate_kbps = rs.bitrate_kbps;
            snprintf(st->cur_segment, sizeof(st->cur_segment), "%s", rs.cur_segment);
        }
    }
    return 0;
}

void stream_receiver_set_segment_seconds(int seconds)
{
    if (g_rec) recorder_set_segment_seconds(g_rec, seconds);
}
