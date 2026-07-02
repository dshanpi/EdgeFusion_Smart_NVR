#ifndef EDGEFUSION_RECORDER_H
#define EDGEFUSION_RECORDER_H

#include <stdint.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

/* 录像分段的参数 */
typedef struct {
    char  dir[256];          /* 录像目录 */
    char  prefix[32];        /* 文件名前缀 */
    int   segment_seconds;   /* 单段时长(秒) */
    long  min_free_mb;       /* TF卡保留容量下限(MB)，低于则删最旧段 */
} recorder_cfg_t;

typedef struct recorder recorder_t;

/* 创建录像器。stream_codecpar 为源流(视频)的编解码参数，用于写 MP4 头 */
recorder_t *recorder_create(const recorder_cfg_t *cfg, AVCodecParameters *stream_codecpar);
void        recorder_destroy(recorder_t *r);

/* 写一帧 packet。ts 为该帧时间戳(秒,用于分段)。
 * 返回 0 成功；首帧非关键帧时内部缓存直到关键帧。
 * is_key: 该帧是否关键帧(IDR) */
int recorder_write_packet(recorder_t *r, AVPacket *pkt, int is_key, double ts);

/* 录像运行状态快照（供 Web /api/status 聚合） */
typedef struct {
    int  recording;          /* 当前是否在录（段已打开且写过头） */
    char cur_segment[128];   /* 当前段文件名（basename） */
    int  seg_index;          /* 段序号 */
    int  seg_elapsed_s;      /* 当前段已录秒数 */
    int  bitrate_kbps;       /* 当前段平均码率（估算） */
} recorder_status_t;
int  recorder_get_status(const recorder_t *r, recorder_status_t *st);

/* 热重载分段时长（下一段生效） */
void recorder_set_segment_seconds(recorder_t *r, int seconds);

#endif
