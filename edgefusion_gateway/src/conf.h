#ifndef EDGEFUSION_CONF_H
#define EDGEFUSION_CONF_H

#include <stddef.h>

typedef struct {
    /* RTSP */
    char  rtsp_url[256];
    char  rtsp_sub_url[256];
    long  rtsp_timeout_us;
    char  rtsp_transport[8];      /* "tcp" / "udp" */
    int   rtsp_reconnect_s;

    /* RTSP server（本地预览转发) */
    int   rtsp_server_enable;
    int   rtsp_server_port;

    /* 录像 */
    char  storage_dir[256];
    int   segment_seconds;
    long  storage_min_free_mb;
    char  segment_prefix[32];

    /* SQLite */
    char  sqlite_db_path[256];

    /* AI */
    int   ai_enable;   /* 云端 AI 总开关：1=启用(仍需 token 非空)，0=禁用不上云 */
    int   ai_fps;
    int   ai_frame_width;
    int   ai_jpeg_quality;
    int   ai_http_timeout_s;
    int   ai_max_queue;
    char  anthropic_base_url[256];
    char  anthropic_auth_token[256];
    char  anthropic_model[64];
    char  ai_prompt[1024];

    /* 报警 */
    int   alarm_gpio_led;
    int   alarm_gpio_buzzer;
    char  alarm_trigger_types[128];
    float alarm_min_confidence;
    int   alarm_active_s;

    /* Web */
    char  web_bind[64];
    int   web_port;
    int   web_mjpeg_fps;
    char  web_root[256];        /* 静态前端文件目录 */

    /* MQTT */
    int   mqtt_enable;
    char  mqtt_host[128];
    int   mqtt_port;
    char  mqtt_client_id[64];
    char  mqtt_topic[128];
    char  mqtt_user[64];
    char  mqtt_pass[64];

    /* 日志 */
    char  log_level[16];
    char  log_file[256];
} gateway_conf_t;

/* 解析配置文件，失败返回 -1 */
int conf_load(gateway_conf_t *cfg, const char *path);

/* 根据 cfg 设置日志级别与文件 */
void conf_apply_log(const gateway_conf_t *cfg);

/* 打印配置（脱敏) */
void conf_dump(const gateway_conf_t *cfg);

/* 取上次 conf_load 的配置文件路径 */
const char *conf_get_path(void);

/* 把 cfg 写回配置文件（逐行替换值，保留注释与 key 顺序，原子 rename).成功返回 0 */
int conf_save(const gateway_conf_t *cfg);

/* Hot-reload: push live-updateable params to running modules
 * (log_level, ai_fps, ai_prompt, anthropic_model, alarm_*, web_mjpeg_fps, segment_seconds).
 * Params requiring restart are NOT handled here. */
void conf_apply_hot(const gateway_conf_t *cfg);

#endif
