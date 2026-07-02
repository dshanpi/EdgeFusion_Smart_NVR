#ifndef EDGEFUSION_CLOUD_AI_H
#define EDGEFUSION_CLOUD_AI_H

#include "conf.h"

/* cloud_ai: 消费 frame_grabber 最新 JPEG，调云端(OpenAI 兼容端点) 做异常检测，
 * 解析返回的结构化 JSON，把事件 publish 到 event_bus（含快照）。
 * 启用条件：ai_enable=1 且 token 非空。任一不满足则进入纯本地模式（不调云）。
 * 支持热启停：reload 时若开关状态翻转，会自动启动/停止线程。 */

int  cloud_ai_start(const gateway_conf_t *cfg);
void cloud_ai_stop(void);

/* 运行状态快照 */
typedef struct {
    int  enabled;   /* token 已配置且线程在跑 */
    int  running;   /* 线程存活 */
} cloud_ai_status_t;
int  cloud_ai_get_status(cloud_ai_status_t *st);

/* 热重载配置（ai_enable/ai_fps/ai_prompt/anthropic_model/anthropic_base_url）。
 * ai_enable 翻转时会热启停线程。 */
void cloud_ai_reload(const gateway_conf_t *cfg);

#endif
