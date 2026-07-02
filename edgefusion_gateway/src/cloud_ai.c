#include "cloud_ai.h"
#include "log.h"
#include "frame_grabber.h"
#include "event_bus.h"

#include <curl/curl.h>
#include <json-c/json.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Base64 编码，返回 malloc 缓冲(无填充也加'='，NUL 结尾)。调用者 free */
static char *b64_encode(const uint8_t *src, int len)
{
    int out_len = 4 * ((len + 2) / 3) + 1;
    char *out = malloc(out_len);
    if (!out) return NULL;
    int o = 0;
    for (int i = 0; i < len; i += 3) {
        uint32_t v = (uint32_t)src[i] << 16;
        if (i + 1 < len) v |= (uint32_t)src[i + 1] << 8;
        if (i + 2 < len) v |= src[i + 2];
        out[o++] = B64[(v >> 18) & 0x3f];
        out[o++] = B64[(v >> 12) & 0x3f];
        out[o++] = (i + 1 < len) ? B64[(v >> 6) & 0x3f] : '=';
        out[o++] = (i + 2 < len) ? B64[v & 0x3f] : '=';
    }
    out[o] = 0;
    return out;
}

/* ---------- libcurl 响应缓冲 ---------- */
struct resp_buf {
    char  *data;
    size_t size;
};

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *user)
{
    struct resp_buf *r = (struct resp_buf *)user;
    size_t n = size * nmemb;
    char *nb = realloc(r->data, r->size + n + 1);
    if (!nb) return 0;
    r->data = nb;
    memcpy(r->data + r->size, ptr, n);
    r->size += n;
    r->data[r->size] = 0;
    return n;
}

/* ---------- 配置缓存 ---------- */
static gateway_conf_t g_cfg;
static pthread_mutex_t g_cfg_mtx = PTHREAD_MUTEX_INITIALIZER;
static volatile bool  g_running = false;
static pthread_t      g_tid;

/* JSON 转义不必要——用 json-c 构造请求体，自动转义 prompt */

/* 构造 OpenAI Chat Completions 请求体(JSON 字符串)，图像走 image_url data URL。
 * 适用于阿里百炼/DashScope 的 OpenAI 兼容端点（qwen-vl / qwen3.x-plus 等多模态模型）。
 * 返回 malloc，失败 NULL */
static char *build_request_body(const char *prompt, const char *model, const char *b64_img)
{
    struct json_object *root = json_object_new_object();
    json_object_object_add(root, "model", json_object_new_string(model));
    json_object_object_add(root, "max_tokens", json_object_new_int(300));

    struct json_object *content = json_object_new_array();
    struct json_object *txt_blk = json_object_new_object();
    json_object_object_add(txt_blk, "type", json_object_new_string("text"));
    json_object_object_add(txt_blk, "text", json_object_new_string(prompt));
    json_object_array_add(content, txt_blk);

    /* data URL: data:image/jpeg;base64,<...> */
    char *data_url = malloc(strlen(b64_img) + 32);
    if (!data_url) { json_object_put(root); return NULL; }
    sprintf(data_url, "data:image/jpeg;base64,%s", b64_img);

    struct json_object *img_blk = json_object_new_object();
    json_object_object_add(img_blk, "type", json_object_new_string("image_url"));
    struct json_object *iu = json_object_new_object();
    json_object_object_add(iu, "url", json_object_new_string(data_url));
    json_object_object_add(img_blk, "image_url", iu);
    json_object_array_add(content, img_blk);

    struct json_object *msg = json_object_new_object();
    json_object_object_add(msg, "role", json_object_new_string("user"));
    json_object_object_add(msg, "content", content);

    struct json_object *messages = json_object_new_array();
    json_object_array_add(messages, msg);
    json_object_object_add(root, "messages", messages);

    /* qwen3 系列默认开启思考(reasoning)，会拖慢响应且多耗 token；
     * 我们只要结构化 JSON，关闭思考以获得干净直接的 content 输出 */
    json_object_object_add(root, "enable_thinking", json_object_new_boolean(0));

    free(data_url);
    const char *s = json_object_to_json_string(root);
    char *out = strdup(s);
    json_object_put(root);
    return out;
}

/* 从模型文本输出中提取首个 {...} 子串(剥 ```json 围栏)。返回 malloc 或 NULL */
static char *extract_json_object(const char *text)
{
    if (!text) return NULL;
    const char *p = strchr(text, '{');
    if (!p) return NULL;
    int depth = 0;
    const char *start = p;
    for (; *p; p++) {
        if (*p == '{') depth++;
        else if (*p == '}') {
            depth--;
            if (depth == 0) {
                size_t len = (size_t)(p - start) + 1;
                char *out = malloc(len + 1);
                if (out) { memcpy(out, start, len); out[len] = 0; }
                return out;
            }
        }
    }
    return NULL;
}

/* 解析模型返回 JSON → 填充 ev。成功返回 0 */
static int parse_event_json(const char *json_str, event_t *ev)
{
    struct json_object *root = json_tokener_parse(json_str);
    if (!root) return -1;
    struct json_object *o;
    if (json_object_object_get_ex(root, "event_type", &o) && json_object_is_type(o, json_type_string))
        strncpy(ev->event_type, json_object_get_string(o), sizeof(ev->event_type) - 1);
    if (json_object_object_get_ex(root, "confidence", &o))
        ev->confidence = (float)json_object_get_double(o);
    if (json_object_object_get_ex(root, "description", &o) && json_object_is_type(o, json_type_string))
        strncpy(ev->description, json_object_get_string(o), sizeof(ev->description) - 1);
    if (json_object_object_get_ex(root, "suggested_action", &o) && json_object_is_type(o, json_type_string))
        strncpy(ev->suggested_action, json_object_get_string(o), sizeof(ev->suggested_action) - 1);
    json_object_put(root);
    if (ev->event_type[0] == 0) strcpy(ev->event_type, "none");
    return 0;
}

/* 解析 OpenAI Chat Completions 响应，取 choices[0].message.content。
 * content 可能是字符串，也可能是 content block 数组(兼容部分实现)。返回 malloc 文本或 NULL */
static char *parse_openai_response(const char *body)
{
    struct json_object *root = json_tokener_parse(body);
    if (!root) return NULL;
    char *text_out = NULL;
    struct json_object *choices;
    if (json_object_object_get_ex(root, "choices", &choices) && json_object_is_type(choices, json_type_array)) {
        struct json_object *choice = json_object_array_get_idx(choices, 0);
        struct json_object *message;
        if (choice && json_object_object_get_ex(choice, "message", &message)) {
            struct json_object *content;
            if (json_object_object_get_ex(message, "content", &content)) {
                if (json_object_is_type(content, json_type_string)) {
                    text_out = strdup(json_object_get_string(content));
                } else if (json_object_is_type(content, json_type_array)) {
                    /* 数组形式：取首个 type==text 的 text */
                    int n = json_object_array_length(content);
                    for (int i = 0; i < n; i++) {
                        struct json_object *blk = json_object_array_get_idx(content, i);
                        struct json_object *t, *txt;
                        if (json_object_object_get_ex(blk, "type", &t) &&
                            json_object_is_type(t, json_type_string) &&
                            strcmp(json_object_get_string(t), "text") == 0 &&
                            json_object_object_get_ex(blk, "text", &txt) &&
                            json_object_is_type(txt, json_type_string)) {
                            text_out = strdup(json_object_get_string(txt));
                            break;
                        }
                    }
                }
            }
        }
    }
    json_object_put(root);
    return text_out;
}

/* 调云端(OpenAI 兼容端点)，成功返回响应体(malloc)，失败 NULL。*http_code 回填 */
static char *call_cloud(const char *body, long *http_code)
{
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    /* base_url 形如 https://.../compatible-mode/v1，拼 /chat/completions */
    char url[512];
    snprintf(url, sizeof(url), "%s/chat/completions", g_cfg.anthropic_base_url);

    struct curl_slist *hdrs = NULL;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
    char kh[256 + 64];
    snprintf(kh, sizeof(kh), "Authorization: Bearer %s", g_cfg.anthropic_auth_token);
    hdrs = curl_slist_append(hdrs, kh);

    struct resp_buf rbuf = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rbuf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)g_cfg.ai_http_timeout_s);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    /* 板子 buildroot 默认无 CA 库，显式指定 CA bundle（由部署脚本推送） */
    curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/ca-certificates.crt");

    CURLcode rc = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    *http_code = code;
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        LOG_WRN("cloud_ai curl 失败: %s", curl_easy_strerror(rc));
        free(rbuf.data);
        return NULL;
    }
    return rbuf.data;
}

static void process_one_frame(uint8_t *jpeg, int sz)
{
    char *b64 = b64_encode(jpeg, sz);
    if (!b64) return;
    char *body = build_request_body(g_cfg.ai_prompt, g_cfg.anthropic_model, b64);
    free(b64);
    if (!body) return;

    long http_code = 0;
    char *resp = call_cloud(body, &http_code);
    free(body);
    if (!resp) return;

    if (http_code != 200) {
        LOG_WRN("cloud_ai HTTP %ld: %.200s", http_code, resp);
        free(resp);
        return;
    }

    char *model_text = parse_openai_response(resp);
    free(resp);
    if (!model_text) { LOG_WRN("cloud_ai 响应无 choices[0].message.content"); return; }

    char *json_str = extract_json_object(model_text);
    free(model_text);
    if (!json_str) { LOG_WRN("cloud_ai 模型输出未含 JSON"); return; }

    event_t ev;
    memset(&ev, 0, sizeof(ev));
    if (parse_event_json(json_str, &ev) == 0) {
        /* 只对真实事件入库（none 不存，避免刷库）；快照用触发帧 */
        if (strcmp(ev.event_type, "none") != 0) {
            long id = event_bus_publish(&ev, jpeg, sz);
            LOG_INF("cloud_ai 检测事件 id=%ld type=%s conf=%.2f desc=%s", id, ev.event_type, ev.confidence, ev.description);
        } else {
            LOG_DBG("cloud_ai 无异常: %s", ev.description);
        }
    } else {
        LOG_WRN("cloud_ai JSON 解析失败: %.200s", json_str);
    }
    free(json_str);
}

static void *ai_thread(void *arg)
{
    (void)arg;
    /* 启用条件：ai_enable 且 token 非空。否则纯本地模式，不调云 */
    pthread_mutex_lock(&g_cfg_mtx);
    int ok = g_cfg.ai_enable && (g_cfg.anthropic_auth_token[0] != 0);
    pthread_mutex_unlock(&g_cfg_mtx);
    if (!ok) {
        LOG_WRN("cloud_ai: 未启用（ai_enable=0 或 token 为空），纯本地模式，不调云");
        return NULL;
    }
    LOG_INF("cloud_ai 线程启动: %s model=%s fps=%d", g_cfg.anthropic_base_url, g_cfg.anthropic_model, g_cfg.ai_fps);

    int interval_us = (g_cfg.ai_fps > 0) ? (1000000 / g_cfg.ai_fps) : 1000000;
    while (g_running) {
        uint8_t *jpeg = NULL;
        int sz = frame_grabber_get_jpeg(&jpeg);
        if (sz <= 0 || !jpeg) {
            if (jpeg) free(jpeg);
            usleep(interval_us);
            continue;
        }
        process_one_frame(jpeg, sz);
        free(jpeg);
        /* 调用可能耗时数秒，这里只做上限节流 */
        if (g_running) usleep(interval_us);
    }
    return NULL;
}

int cloud_ai_start(const gateway_conf_t *cfg)
{
    if (g_running) return 0;
    pthread_mutex_lock(&g_cfg_mtx);
    g_cfg = *cfg;
    pthread_mutex_unlock(&g_cfg_mtx);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    g_running = true;
    if (pthread_create(&g_tid, NULL, ai_thread, NULL) != 0) {
        g_running = false;
        LOG_ERR("创建 cloud_ai 线程失败");
        return -1;
    }
    return 0;
}

int cloud_ai_get_status(cloud_ai_status_t *st)
{
    if (!st) return -1;
    pthread_mutex_lock(&g_cfg_mtx);
    int enabled_cfg = g_cfg.ai_enable && (g_cfg.anthropic_auth_token[0] != 0);
    pthread_mutex_unlock(&g_cfg_mtx);
    st->running = g_running;
    /* enabled = 配置允许上云 且 线程在跑 */
    st->enabled = enabled_cfg && st->running;
    return 0;
}

void cloud_ai_reload(const gateway_conf_t *cfg)
{
    pthread_mutex_lock(&g_cfg_mtx);
    int was_enabled = g_cfg.ai_enable && (g_cfg.anthropic_auth_token[0] != 0);
    /* 仅热更新可热生效字段，保留不可热改的（如 token 用原值） */
    g_cfg.ai_enable = cfg->ai_enable;
    g_cfg.ai_fps = cfg->ai_fps;
    g_cfg.ai_http_timeout_s = cfg->ai_http_timeout_s;
    strncpy(g_cfg.ai_prompt, cfg->ai_prompt, sizeof(g_cfg.ai_prompt) - 1);
    g_cfg.ai_prompt[sizeof(g_cfg.ai_prompt) - 1] = 0;
    strncpy(g_cfg.anthropic_model, cfg->anthropic_model, sizeof(g_cfg.anthropic_model) - 1);
    g_cfg.anthropic_model[sizeof(g_cfg.anthropic_model) - 1] = 0;
    strncpy(g_cfg.anthropic_base_url, cfg->anthropic_base_url, sizeof(g_cfg.anthropic_base_url) - 1);
    g_cfg.anthropic_base_url[sizeof(g_cfg.anthropic_base_url) - 1] = 0;
    int now_enabled = g_cfg.ai_enable && (g_cfg.anthropic_auth_token[0] != 0);
    pthread_mutex_unlock(&g_cfg_mtx);

    /* 开关状态翻转 → 热启停线程。
     * 注意：cloud_ai_stop 会 join 线程并 curl_global_cleanup；重新 start 会重新 init。
     * reload 在 web 线程上下文调用，join 上云线程是安全的（上云线程无锁依赖 web 线程）。 */
    if (!was_enabled && now_enabled) {
        LOG_INF("cloud_ai 开关已开启，启动线程");
        cloud_ai_stop();
        cloud_ai_start(cfg);
    } else if (was_enabled && !now_enabled) {
        LOG_INF("cloud_ai 开关已关闭，停止线程");
        cloud_ai_stop();
    } else {
        LOG_INF("cloud_ai 配置已热重载");
    }
}

void cloud_ai_stop(void)
{
    if (!g_running) return;
    g_running = false;
    pthread_join(g_tid, NULL);
    curl_global_cleanup();
    LOG_INF("cloud_ai 线程已停止");
}
