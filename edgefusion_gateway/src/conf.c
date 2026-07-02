#include "conf.h"
#include "log.h"
/* 热重载目标模块（仅声明，避免循环包含） */
#include "alarm.h"
#include "cloud_ai.h"
#include "frame_grabber.h"
#include "stream_receiver.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 去除行首尾空白；返回处理后字符串指针（原地） */
static char *trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) { *end = 0; end--; }
    return s;
}

/* 去掉行内 # 注释（不在引号内时）。简单实现：首个 # 即注释 */
static void strip_comment(char *s)
{
    for (; *s; s++) {
        if (*s == '#') { *s = 0; break; }
    }
}

/* 把字符串值写入目标字段（带长度保护） */
static void set_str(char *dst, size_t n, const char *val)
{
    if (!val) { if (n) dst[0] = 0; return; }
    size_t l = strlen(val);
    if (l >= n) l = n - 1;
    memcpy(dst, val, l);
    dst[l] = 0;
}

/* 上次 conf_load 的配置文件路径，供 conf_save 写回 */
static char g_conf_path[256] = {0};
const char *conf_get_path(void) { return g_conf_path; }

int conf_load(gateway_conf_t *cfg, const char *path)
{
    memset(cfg, 0, sizeof(*cfg));

    /* 默认值 */
    cfg->rtsp_reconnect_s = 3;
    cfg->rtsp_timeout_us  = 5000000;
    strcpy(cfg->rtsp_transport, "tcp");
    cfg->rtsp_server_enable = 1;
    cfg->rtsp_server_port = 8554;
    cfg->segment_seconds  = 600;
    cfg->storage_min_free_mb = 500;
    strcpy(cfg->segment_prefix, "rec");
    cfg->ai_fps = 1;
    cfg->ai_enable = 1;   /* 默认启用云端 AI（仍需 token 非空才真正上云） */
    cfg->ai_frame_width = 640;
    cfg->ai_jpeg_quality = 70;
    cfg->ai_http_timeout_s = 30;
    cfg->ai_max_queue = 2;
    strcpy(cfg->anthropic_model, "deepseek-v4-pro[1m]");
    cfg->alarm_gpio_led = -1;
    cfg->alarm_gpio_buzzer = -1;
    strcpy(cfg->alarm_trigger_types, "person_detected,danger_alert");
    cfg->alarm_min_confidence = 0.6f;
    cfg->alarm_active_s = 3;
    strcpy(cfg->web_bind, "0.0.0.0");
    cfg->web_port = 8080;
    cfg->web_mjpeg_fps = 5;
    strcpy(cfg->web_root, "/mnt/extsd/gateway/web");
    cfg->mqtt_port = 1883;
    strcpy(cfg->mqtt_client_id, "edgefusion_gateway_imx6ull");
    strcpy(cfg->mqtt_topic, "edgefusion/events");
    strcpy(cfg->log_level, "info");

    FILE *fp = fopen(path, "r");
    if (!fp) {
        LOG_ERR("无法打开配置文件: %s", path);
        return -1;
    }
    /* 记录路径供 conf_save 写回 */
    strncpy(g_conf_path, path, sizeof(g_conf_path) - 1);
    g_conf_path[sizeof(g_conf_path) - 1] = 0;

    char line[1024];
    int lineno = 0;
    while (fgets(line, sizeof(line), fp)) {
        lineno++;
        strip_comment(line);
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char *key = trim(line);
        char *val = trim(eq + 1);
        if (!*key) continue;

        if      (!strcmp(key, "rtsp_url"))            set_str(cfg->rtsp_url, sizeof(cfg->rtsp_url), val);
        else if (!strcmp(key, "rtsp_sub_url"))        set_str(cfg->rtsp_sub_url, sizeof(cfg->rtsp_sub_url), val);
        else if (!strcmp(key, "rtsp_timeout_us"))     cfg->rtsp_timeout_us = strtol(val, NULL, 10);
        else if (!strcmp(key, "rtsp_transport"))      set_str(cfg->rtsp_transport, sizeof(cfg->rtsp_transport), val);
        else if (!strcmp(key, "rtsp_reconnect_s"))    cfg->rtsp_reconnect_s = atoi(val);
        else if (!strcmp(key, "rtsp_server_enable"))  cfg->rtsp_server_enable = atoi(val);
        else if (!strcmp(key, "rtsp_server_port"))    cfg->rtsp_server_port = atoi(val);
        else if (!strcmp(key, "storage_dir"))         set_str(cfg->storage_dir, sizeof(cfg->storage_dir), val);
        else if (!strcmp(key, "segment_seconds"))     cfg->segment_seconds = atoi(val);
        else if (!strcmp(key, "storage_min_free_mb")) cfg->storage_min_free_mb = strtol(val, NULL, 10);
        else if (!strcmp(key, "segment_prefix"))      set_str(cfg->segment_prefix, sizeof(cfg->segment_prefix), val);
        else if (!strcmp(key, "sqlite_db_path"))      set_str(cfg->sqlite_db_path, sizeof(cfg->sqlite_db_path), val);
        else if (!strcmp(key, "ai_enable"))            cfg->ai_enable = atoi(val);
        else if (!strcmp(key, "ai_fps"))              cfg->ai_fps = atoi(val);
        else if (!strcmp(key, "ai_frame_width"))      cfg->ai_frame_width = atoi(val);
        else if (!strcmp(key, "ai_jpeg_quality"))     cfg->ai_jpeg_quality = atoi(val);
        else if (!strcmp(key, "ai_http_timeout_s"))   cfg->ai_http_timeout_s = atoi(val);
        else if (!strcmp(key, "ai_max_queue"))        cfg->ai_max_queue = atoi(val);
        else if (!strcmp(key, "anthropic_base_url"))  set_str(cfg->anthropic_base_url, sizeof(cfg->anthropic_base_url), val);
        else if (!strcmp(key, "anthropic_auth_token"))set_str(cfg->anthropic_auth_token, sizeof(cfg->anthropic_auth_token), val);
        else if (!strcmp(key, "anthropic_model"))     set_str(cfg->anthropic_model, sizeof(cfg->anthropic_model), val);
        else if (!strcmp(key, "ai_prompt"))           set_str(cfg->ai_prompt, sizeof(cfg->ai_prompt), val);
        else if (!strcmp(key, "alarm_gpio_led"))      cfg->alarm_gpio_led = atoi(val);
        else if (!strcmp(key, "alarm_gpio_buzzer"))   cfg->alarm_gpio_buzzer = atoi(val);
        else if (!strcmp(key, "alarm_trigger_types")) set_str(cfg->alarm_trigger_types, sizeof(cfg->alarm_trigger_types), val);
        else if (!strcmp(key, "alarm_min_confidence"))cfg->alarm_min_confidence = strtof(val, NULL);
        else if (!strcmp(key, "alarm_active_s"))      cfg->alarm_active_s = atoi(val);
        else if (!strcmp(key, "web_bind"))            set_str(cfg->web_bind, sizeof(cfg->web_bind), val);
        else if (!strcmp(key, "web_port"))            cfg->web_port = atoi(val);
        else if (!strcmp(key, "web_mjpeg_fps"))       cfg->web_mjpeg_fps = atoi(val);
        else if (!strcmp(key, "web_root"))            set_str(cfg->web_root, sizeof(cfg->web_root), val);
        else if (!strcmp(key, "mqtt_enable"))         cfg->mqtt_enable = atoi(val);
        else if (!strcmp(key, "mqtt_host"))           set_str(cfg->mqtt_host, sizeof(cfg->mqtt_host), val);
        else if (!strcmp(key, "mqtt_port"))           cfg->mqtt_port = atoi(val);
        else if (!strcmp(key, "mqtt_client_id"))      set_str(cfg->mqtt_client_id, sizeof(cfg->mqtt_client_id), val);
        else if (!strcmp(key, "mqtt_topic"))          set_str(cfg->mqtt_topic, sizeof(cfg->mqtt_topic), val);
        else if (!strcmp(key, "mqtt_user"))           set_str(cfg->mqtt_user, sizeof(cfg->mqtt_user), val);
        else if (!strcmp(key, "mqtt_pass"))           set_str(cfg->mqtt_pass, sizeof(cfg->mqtt_pass), val);
        else if (!strcmp(key, "log_level"))           set_str(cfg->log_level, sizeof(cfg->log_level), val);
        else if (!strcmp(key, "log_file"))            set_str(cfg->log_file, sizeof(cfg->log_file), val);
        /* 未知 key 静默忽略 */
    }
    fclose(fp);
    return 0;
}

static log_level_t parse_log_level(const char *s)
{
    if (!strcmp(s, "debug")) return LOG_DEBUG;
    if (!strcmp(s, "info"))  return LOG_INFO;
    if (!strcmp(s, "warn"))  return LOG_WARN;
    if (!strcmp(s, "error")) return LOG_ERROR;
    return LOG_INFO;
}

/* 供 main 调用：根据 cfg 配置日志系统 */
void conf_apply_log(const gateway_conf_t *cfg)
{
    log_set_level(parse_log_level(cfg->log_level));
    log_set_file(cfg->log_file);
}

void conf_dump(const gateway_conf_t *cfg)
{
    LOG_INF("==== 配置摘要 ====");
    LOG_INF("RTSP 主码流   : %s", cfg->rtsp_url);
    LOG_INF("RTSP 子码流   : %s", cfg->rtsp_sub_url[0] ? cfg->rtsp_sub_url : "(空,用主码流降采样)");
    LOG_INF("RTSP 预览转发 : %s (端口 %d)", cfg->rtsp_server_enable ? "开" : "关", cfg->rtsp_server_port);
    LOG_INF("录像目录      : %s", cfg->storage_dir);
    LOG_INF("单段时长      : %d 秒", cfg->segment_seconds);
    LOG_INF("SQLite        : %s", cfg->sqlite_db_path);
    LOG_INF("AI 帧率       : %d fps, 缩放宽 %dpx, 启用=%s", cfg->ai_fps, cfg->ai_frame_width, cfg->ai_enable ? "是" : "否");
    LOG_INF("云端端点      : %s", cfg->anthropic_base_url);
    LOG_INF("云端模型      : %s", cfg->anthropic_model);
    LOG_INF("云端Token     : %s", cfg->anthropic_auth_token[0] ? "(已设置)" : "(未设置)");
    LOG_INF("报警 GPIO LED : %d, 蜂鸣器: %d", cfg->alarm_gpio_led, cfg->alarm_gpio_buzzer);
    LOG_INF("Web           : %s:%d", cfg->web_bind, cfg->web_port);
    LOG_INF("MQTT          : %s (enable=%d)", cfg->mqtt_host, cfg->mqtt_enable);
    LOG_INF("日志          : level=%s file=%s", cfg->log_level, cfg->log_file);
}

/* ---------- conf_save：逐行替换值，保留注释与 key 顺序 ---------- */

/* 把指定 key 的当前值格式化为字符串写到 out。可写的 key 返回 1，否则 0。
 * 注意：敏感字段(anthropic_auth_token/mqtt_pass/mqtt_user)不在此处理(写回=原值,安全,但不在白名单) */
static int format_value(const gateway_conf_t *cfg, const char *key, char *out, size_t cap)
{
    if      (!strcmp(key, "rtsp_url"))            { snprintf(out, cap, "%s", cfg->rtsp_url); return 1; }
    else if (!strcmp(key, "rtsp_sub_url"))        { snprintf(out, cap, "%s", cfg->rtsp_sub_url[0] ? cfg->rtsp_sub_url : ""); return 1; }
    else if (!strcmp(key, "rtsp_timeout_us"))     { snprintf(out, cap, "%ld", cfg->rtsp_timeout_us); return 1; }
    else if (!strcmp(key, "rtsp_transport"))      { snprintf(out, cap, "%s", cfg->rtsp_transport); return 1; }
    else if (!strcmp(key, "rtsp_reconnect_s"))    { snprintf(out, cap, "%d", cfg->rtsp_reconnect_s); return 1; }
    else if (!strcmp(key, "rtsp_server_enable"))  { snprintf(out, cap, "%d", cfg->rtsp_server_enable); return 1; }
    else if (!strcmp(key, "rtsp_server_port"))    { snprintf(out, cap, "%d", cfg->rtsp_server_port); return 1; }
    else if (!strcmp(key, "storage_dir"))         { snprintf(out, cap, "%s", cfg->storage_dir); return 1; }
    else if (!strcmp(key, "segment_seconds"))     { snprintf(out, cap, "%d", cfg->segment_seconds); return 1; }
    else if (!strcmp(key, "storage_min_free_mb")) { snprintf(out, cap, "%ld", cfg->storage_min_free_mb); return 1; }
    else if (!strcmp(key, "segment_prefix"))      { snprintf(out, cap, "%s", cfg->segment_prefix); return 1; }
    else if (!strcmp(key, "sqlite_db_path"))      { snprintf(out, cap, "%s", cfg->sqlite_db_path); return 1; }
    else if (!strcmp(key, "ai_enable"))           { snprintf(out, cap, "%d", cfg->ai_enable); return 1; }
    else if (!strcmp(key, "ai_fps"))              { snprintf(out, cap, "%d", cfg->ai_fps); return 1; }
    else if (!strcmp(key, "ai_frame_width"))      { snprintf(out, cap, "%d", cfg->ai_frame_width); return 1; }
    else if (!strcmp(key, "ai_jpeg_quality"))     { snprintf(out, cap, "%d", cfg->ai_jpeg_quality); return 1; }
    else if (!strcmp(key, "ai_http_timeout_s"))   { snprintf(out, cap, "%d", cfg->ai_http_timeout_s); return 1; }
    else if (!strcmp(key, "ai_max_queue"))        { snprintf(out, cap, "%d", cfg->ai_max_queue); return 1; }
    else if (!strcmp(key, "anthropic_base_url"))  { snprintf(out, cap, "%s", cfg->anthropic_base_url); return 1; }
    else if (!strcmp(key, "anthropic_auth_token")){ snprintf(out, cap, "%s", cfg->anthropic_auth_token); return 1; }
    else if (!strcmp(key, "anthropic_model"))     { snprintf(out, cap, "%s", cfg->anthropic_model); return 1; }
    else if (!strcmp(key, "ai_prompt"))           { snprintf(out, cap, "%s", cfg->ai_prompt); return 1; }
    else if (!strcmp(key, "alarm_gpio_led"))      { snprintf(out, cap, "%d", cfg->alarm_gpio_led); return 1; }
    else if (!strcmp(key, "alarm_gpio_buzzer"))   { snprintf(out, cap, "%d", cfg->alarm_gpio_buzzer); return 1; }
    else if (!strcmp(key, "alarm_trigger_types")) { snprintf(out, cap, "%s", cfg->alarm_trigger_types); return 1; }
    else if (!strcmp(key, "alarm_min_confidence")){ snprintf(out, cap, "%.6g", cfg->alarm_min_confidence); return 1; }
    else if (!strcmp(key, "alarm_active_s"))      { snprintf(out, cap, "%d", cfg->alarm_active_s); return 1; }
    else if (!strcmp(key, "web_bind"))            { snprintf(out, cap, "%s", cfg->web_bind); return 1; }
    else if (!strcmp(key, "web_port"))            { snprintf(out, cap, "%d", cfg->web_port); return 1; }
    else if (!strcmp(key, "web_mjpeg_fps"))       { snprintf(out, cap, "%d", cfg->web_mjpeg_fps); return 1; }
    else if (!strcmp(key, "web_root"))            { snprintf(out, cap, "%s", cfg->web_root); return 1; }
    else if (!strcmp(key, "mqtt_enable"))         { snprintf(out, cap, "%d", cfg->mqtt_enable); return 1; }
    else if (!strcmp(key, "mqtt_host"))           { snprintf(out, cap, "%s", cfg->mqtt_host); return 1; }
    else if (!strcmp(key, "mqtt_port"))           { snprintf(out, cap, "%d", cfg->mqtt_port); return 1; }
    else if (!strcmp(key, "mqtt_client_id"))      { snprintf(out, cap, "%s", cfg->mqtt_client_id); return 1; }
    else if (!strcmp(key, "mqtt_topic"))          { snprintf(out, cap, "%s", cfg->mqtt_topic); return 1; }
    else if (!strcmp(key, "mqtt_user"))           { snprintf(out, cap, "%s", cfg->mqtt_user); return 1; }
    else if (!strcmp(key, "mqtt_pass"))           { snprintf(out, cap, "%s", cfg->mqtt_pass); return 1; }
    else if (!strcmp(key, "log_level"))           { snprintf(out, cap, "%s", cfg->log_level); return 1; }
    else if (!strcmp(key, "log_file"))            { snprintf(out, cap, "%s", cfg->log_file); return 1; }
    return 0;
}

int conf_save(const gateway_conf_t *cfg)
{
    if (g_conf_path[0] == 0) { LOG_ERR("conf_save: 配置路径未知"); return -1; }
    FILE *fp = fopen(g_conf_path, "r");
    if (!fp) { LOG_ERR("conf_save: 打开失败 %s", g_conf_path); return -1; }

    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", g_conf_path);
    FILE *tmp = fopen(tmp_path, "w");
    if (!tmp) { LOG_ERR("conf_save: 创建临时文件失败 %s", tmp_path); fclose(fp); return -1; }

    char line[2048];
    while (fgets(line, sizeof(line), fp)) {
        /* 提取 key：副本上 strip_comment + 找 = + trim */
        char copy[2048];
        strncpy(copy, line, sizeof(copy) - 1);
        copy[sizeof(copy) - 1] = 0;
        strip_comment(copy);
        char *eq = strchr(copy, '=');
        if (eq) {
            *eq = 0;
            char *key = trim(copy);
            char valbuf[1024];
            if (key[0] && format_value(cfg, key, valbuf, sizeof(valbuf))) {
                /* 保留原行尾部注释（首个 # 起的部分） */
                char *comment = strchr(line, '#');
                if (comment) {
                    fprintf(tmp, "%s = %s %s", key, valbuf, comment);
                    /* 原 line 含换行，comment 含到行尾(含\n)，已含 */
                } else {
                    fprintf(tmp, "%s = %s\n", key, valbuf);
                }
                continue;
            }
        }
        /* 注释/空行/未知 key：原样写出 */
        fputs(line, tmp);
    }
    fclose(fp);
    fflush(tmp);
    fsync(fileno(tmp));
    fclose(tmp);

    if (rename(tmp_path, g_conf_path) != 0) {
        LOG_ERR("conf_save: rename 失败 %s -> %s", tmp_path, g_conf_path);
        unlink(tmp_path);
        return -1;
    }
    LOG_INF("配置已保存: %s", g_conf_path);
    return 0;
}

void conf_apply_hot(const gateway_conf_t *cfg)
{
    log_set_level(parse_log_level(cfg->log_level));
    alarm_reload(cfg);
    cloud_ai_reload(cfg);
    frame_grabber_set_target_fps(cfg->web_mjpeg_fps);
    stream_receiver_set_segment_seconds(cfg->segment_seconds);
    LOG_INF("热重载已应用(log_level/alarm/ai/mjpeg_fps/segment_seconds)");
}
