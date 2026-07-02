#ifndef EDGEFUSION_STREAM_RECEIVER_H
#define EDGEFUSION_STREAM_RECEIVER_H

#include "conf.h"
#include "recorder.h"

/* 启动拉流线程（成功返回 0）。
 * 内部创建 recorder 并按 cfg 分段录像。
 * 线程持续运行直到 stream_receiver_stop 被调用或出错退出。 */
int  stream_receiver_start(const gateway_conf_t *cfg);

/* 停止拉流线程并释放资源 */
void stream_receiver_stop(void);

/* 拉流运行状态快照（供 Web /api/status 聚合） */
typedef struct {
    int  connected;          /* 源流是否连上 */
    int  width;              /* 源分辨率宽 */
    int  height;             /* 源分辨率高 */
    int  fps;                /* 源帧率（四舍五入） */
    int  recording;          /* 是否在录 */
    char cur_segment[128];   /* 当前段文件名 */
    int  bitrate_kbps;       /* 当前段码率 */
} stream_status_t;
int  stream_receiver_get_status(stream_status_t *st);

/* 热重载分段时长（转发给 recorder，下一段生效） */
void stream_receiver_set_segment_seconds(int seconds);

#endif
