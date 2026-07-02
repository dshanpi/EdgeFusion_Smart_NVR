#ifndef EDGEFUSION_EVENT_BUS_H
#define EDGEFUSION_EVENT_BUS_H

#include <stdint.h>

/* event_bus: 本地事件中枢。SQLite 持久化 + 快照证据 + 订阅者广播。 */

typedef struct {
    long   id;                  /* SQLite 自增 id（publish 后回填，-1=未入库） */
    int64_t created_at;         /* Unix 秒 */
    char   event_type[32];      /* person_detected/danger_alert/none... */
    float  confidence;          /* 0-1 */
    char   description[256];
    char   suggested_action[32];/* send_alert/record/ignore */
    char   jpeg_path[256];      /* 触发帧快照本地路径，空串则无快照 */
} event_t;

/* 事件订阅回调。在 publish 线程内同步调用，回调内禁止长时间阻塞/调 event_bus_*。 */
typedef void (*event_cb_t)(const event_t *ev, void *user);

/* 初始化：打开/建库、建表、建快照目录。min_free_mb<=0 时不做磁盘清理。成功返回 0 */
int  event_bus_init(const char *db_path, const char *snapshot_dir, long min_free_mb);
void event_bus_deinit(void);

/* 注册订阅者。须在 cloud_ai 启动前调用 */
void event_bus_subscribe(event_cb_t cb, void *user);

/* 发布事件：INSERT + 存快照(若 jpeg 非空) + 通知订阅者。返回事件 id，失败 -1。
 * jpeg/jpeg_size 可为 NULL/0 表示不存快照。ev->id 被回填。 */
long event_bus_publish(event_t *ev, const uint8_t *jpeg, int jpeg_size);

/* 查询最近 max_count 条（按 created_at DESC）。返回实际写入 out 的条数 */
int  event_bus_query(event_t *out, int max_count);

/* 事件总数。type_filter 为 NULL 或空串时统计全部，否则按 event_type 精确匹配 */
long event_bus_count(const char *type_filter);

/* 分页筛选查询：按 event_type(可空) + offset 偏移。返回实际条数 */
int  event_bus_query_filtered(event_t *out, int max_count, const char *type_filter, int offset);

/* 取单条详情。成功返回 0，无此 id 返回 -1 */
int  event_bus_get(long id, event_t *out);

#endif
