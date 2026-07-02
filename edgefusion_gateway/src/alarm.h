#ifndef EDGEFUSION_ALARM_H
#define EDGEFUSION_ALARM_H

#include "conf.h"

/* alarm: 订阅 event_bus 事件，命中规则(event_type 匹配 + confidence 达标)时
 * 通过 GPIO sysfs 驱动 LED/蜂鸣器响 alarm_active_s 秒。
 * GPIO 号为 -1 时该路 no-op（默认安全态）。 */

int  alarm_init(const gateway_conf_t *cfg);
void alarm_stop(void);

/* 热重载报警参数（GPIO/触发类型/阈值/时长） */
void alarm_reload(const gateway_conf_t *cfg);

#endif
