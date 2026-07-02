#ifndef EDGEFUSION_FRAME_GRABBER_H
#define EDGEFUSION_FRAME_GRABBER_H

#include <stddef.h>
#include <stdint.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

/* frame_grabber: 旁路解码视频流，按目标帧率产出 JPEG 帧，缓存“最新一帧”供消费者(Web/云)取。 */

typedef struct {
    int   target_fps;     /* 产出帧率(0=不限) */
    int   out_width;      /* 输出宽度(0=原宽) */
    int   jpeg_quality;   /* 1-100 */
} frame_grabber_cfg_t;

/* 初始化解码器（用源流 codecpar）。成功返回 0 */
int  frame_grabber_init(const frame_grabber_cfg_t *cfg, AVCodecParameters *codecpar);

/* 释放资源 */
void frame_grabber_deinit(void);

/* 输入一帧压缩包（由 stream_receiver 调用）。内部解码并按帧率节流产出 JPEG。
 * 返回 0 正常。pkt 不会被修改，调用者仍负责 unref。 */
int  frame_grabber_feed(AVPacket *pkt);

/* 取最新 JPEG 帧的拷贝。返回帧大小(字节)，0=暂无帧。
 * 调用者用完需 free(*out_data)。
 * 线程安全：内部加锁拷贝。 */
int  frame_grabber_get_jpeg(uint8_t **out_data);

/* 热重载产出帧率（节流用，立即生效） */
void frame_grabber_set_target_fps(int fps);

#endif
