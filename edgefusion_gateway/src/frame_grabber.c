#include "frame_grabber.h"
#include "log.h"

#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static AVCodecContext    *g_dec = NULL;
static AVCodecContext    *g_enc = NULL;        /* MJPEG 编码器 */
static struct SwsContext *g_sws = NULL;
static AVFrame           *g_dec_frame = NULL;  /* 解码输出帧 */
static AVFrame           *g_dst_frame = NULL;  /* 缩放后帧(YUVJ420P) */
static int                g_dst_w = 0, g_dst_h = 0;

static frame_grabber_cfg_t g_cfg;

/* 最新 JPEG 帧缓存 + 锁 */
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static uint8_t *g_jpeg = NULL;
static int       g_jpeg_size = 0;

/* 帧率节流 */
static int64_t g_last_emit_us = 0;

static int64_t now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int frame_grabber_init(const frame_grabber_cfg_t *cfg, AVCodecParameters *codecpar)
{
    g_cfg = *cfg;

    /* 解码器 */
    const AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
    if (!dec) { LOG_ERR("frame_grabber: 找不到解码器 %d", codecpar->codec_id); return -1; }
    g_dec = avcodec_alloc_context3(dec);
    if (!g_dec) return -1;
    if (avcodec_parameters_to_context(g_dec, codecpar) < 0) { LOG_ERR("参数到上下文失败"); goto fail; }
    g_dec->flags2 |= AV_CODEC_FLAG2_FAST;
    if (avcodec_open2(g_dec, dec, NULL) < 0) { LOG_ERR("解码器打开失败"); goto fail; }

    g_dec_frame = av_frame_alloc();
    g_dst_frame = av_frame_alloc();
    if (!g_dec_frame || !g_dst_frame) goto fail;

    /* 输出尺寸 */
    g_dst_w = cfg->out_width > 0 ? cfg->out_width : codecpar->width;
    /* 高度按比例(对齐到2) */
    if (cfg->out_width > 0 && codecpar->width > 0) {
        g_dst_h = (codecpar->height * cfg->out_width / codecpar->width) & ~1;
    } else {
        g_dst_h = codecpar->height;
    }
    if (g_dst_h <= 0) g_dst_h = 480;

    /* MJPEG 编码器 */
    const AVCodec *enc = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!enc) { LOG_ERR("找不到 MJPEG 编码器"); goto fail; }
    g_enc = avcodec_alloc_context3(enc);
    g_enc->width = g_dst_w;
    g_enc->height = g_dst_h;
    g_enc->pix_fmt = AV_PIX_FMT_YUVJ420P;
    g_enc->time_base = (AVRational){1, 25};
    /* JPEG 质量通过 qmin/qmax 控制（取值 1-31，越小质量越高；映射 quality->qscale） */
    int qscale = 31 - (cfg->jpeg_quality * 30 / 100);
    if (qscale < 1) qscale = 1;
    if (qscale > 31) qscale = 31;
    g_enc->qmin = qscale;
    g_enc->qmax = qscale;
    AVDictionary *opts = NULL;
    if (avcodec_open2(g_enc, enc, &opts) < 0) {
        char err[128] = {0}; av_strerror(errno, err, sizeof(err));
        LOG_ERR("MJPEG 编码器打开失败 (errno:%s)", err);
        av_dict_free(&opts); goto fail;
    }
    av_dict_free(&opts);

    /* 缩放器：解码帧 pix_fmt -> YUVJ420P 目标尺寸 */
    g_sws = sws_getContext(codecpar->width, codecpar->height, g_dec->pix_fmt,
                           g_dst_w, g_dst_h, AV_PIX_FMT_YUVJ420P,
                           SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (!g_sws) { LOG_ERR("sws_getContext 失败"); goto fail; }

    /* 给目标帧分配 buffer */
    if (av_image_alloc(g_dst_frame->data, g_dst_frame->linesize,
                       g_dst_w, g_dst_h, AV_PIX_FMT_YUVJ420P, 32) < 0) {
        LOG_ERR("av_image_alloc 失败"); goto fail;
    }
    g_dst_frame->width = g_dst_w;
    g_dst_frame->height = g_dst_h;
    g_dst_frame->format = AV_PIX_FMT_YUVJ420P;

    LOG_INF("frame_grabber 就绪: %dx%d -> %dx%d JPEG q=%d fps=%d",
            codecpar->width, codecpar->height, g_dst_w, g_dst_h, cfg->jpeg_quality, cfg->target_fps);
    return 0;
fail:
    frame_grabber_deinit();
    return -1;
}

void frame_grabber_deinit(void)
{
    if (g_dec) avcodec_free_context(&g_dec);
    if (g_enc) avcodec_free_context(&g_enc);
    if (g_sws) sws_freeContext(g_sws), g_sws = NULL;
    if (g_dec_frame) av_frame_free(&g_dec_frame);
    if (g_dst_frame) {
        if (g_dst_frame->data[0]) av_freep(&g_dst_frame->data[0]);
        av_frame_free(&g_dst_frame);
    }
    pthread_mutex_lock(&g_lock);
    if (g_jpeg) free(g_jpeg), g_jpeg = NULL;
    g_jpeg_size = 0;
    pthread_mutex_unlock(&g_lock);
}

/* 处理一个已解码帧：缩放+编码JPEG+缓存（带帧率节流） */
static void emit_jpeg_from_frame(void)
{
    /* 帧率节流 */
    if (g_cfg.target_fps > 0) {
        int64_t interval = 1000000 / g_cfg.target_fps;
        int64_t now = now_us();
        if (now - g_last_emit_us < interval) {
            return;
        }
        g_last_emit_us = now;
    }

    /* 缩放 */
    sws_scale(g_sws, (const uint8_t *const *)g_dec_frame->data, g_dec_frame->linesize,
              0, g_dec_frame->height, g_dst_frame->data, g_dst_frame->linesize);
    g_dst_frame->pts = g_dec_frame->pts;

    /* 编码 JPEG */
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) return;
    if (avcodec_send_frame(g_enc, g_dst_frame) < 0) { av_packet_free(&pkt); return; }
    if (avcodec_receive_packet(g_enc, pkt) < 0) { av_packet_free(&pkt); return; }

    /* 缓存最新帧 */
    pthread_mutex_lock(&g_lock);
    uint8_t *nb = realloc(g_jpeg, pkt->size);
    if (nb) {
        g_jpeg = nb;
        memcpy(g_jpeg, pkt->data, pkt->size);
        g_jpeg_size = pkt->size;
    }
    pthread_mutex_unlock(&g_lock);

    av_packet_free(&pkt);
}

int frame_grabber_feed(AVPacket *pkt)
{
    if (!g_dec) return 0;
    int ret = avcodec_send_packet(g_dec, pkt);
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
        return 0; /* 偶发坏包，忽略 */
    }
    /* 取出所有可解码帧 */
    while (1) {
        ret = avcodec_receive_frame(g_dec, g_dec_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) break;
        emit_jpeg_from_frame();
    }
    return 0;
}

int frame_grabber_get_jpeg(uint8_t **out_data)
{
    pthread_mutex_lock(&g_lock);
    if (!g_jpeg || g_jpeg_size <= 0) {
        pthread_mutex_unlock(&g_lock);
        *out_data = NULL;
        return 0;
    }
    uint8_t *buf = malloc(g_jpeg_size);
    if (!buf) {
        pthread_mutex_unlock(&g_lock);
        return 0;
    }
    memcpy(buf, g_jpeg, g_jpeg_size);
    pthread_mutex_unlock(&g_lock);
    *out_data = buf;
    return g_jpeg_size;
}

void frame_grabber_set_target_fps(int fps)
{
    pthread_mutex_lock(&g_lock);
    g_cfg.target_fps = fps;
    pthread_mutex_unlock(&g_lock);
}
