#include "alarm.h"
#include "log.h"
#include "event_bus.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int  g_led = -1;
static int  g_buzzer = -1;
static int  g_active_s = 0;
static char g_trigger_types[128];
static float g_min_conf = 0.6f;

static volatile bool g_running = false;
static pthread_t     g_tid;
static pthread_mutex_t g_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_cond = PTHREAD_COND_INITIALIZER;
/* 触发请求：>0 表示有待处理的报警(取最近一次 active_s) */
static int g_pending = 0;

/* 往 sysfs 写一行 */
static void sysfs_write(const char *path, const char *val)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0) return;
    write(fd, val, strlen(val));
    close(fd);
}

/* 导出并设置方向，再写值。失败只 LOG_WRN */
static void gpio_set(int pin, int val)
{
    if (pin < 0) return;
    char p[64];
    snprintf(p, sizeof(p), "/sys/class/gpio/gpio%d/value", pin);
    /* 若节点不存在则 export */
    if (access(p, F_OK) != 0) {
        char ep[64];
        snprintf(ep, sizeof(ep), "%d", pin);
        sysfs_write("/sys/class/gpio/export", ep);
        usleep(100 * 1000);
        char dp[64];
        snprintf(dp, sizeof(dp), "/sys/class/gpio/gpio%d/direction", pin);
        sysfs_write(dp, "out");
    }
    sysfs_write(p, val ? "1" : "0");
}

static bool type_matches(const char *event_type)
{
    if (g_trigger_types[0] == 0) return false;
    char buf[128];
    strncpy(buf, g_trigger_types, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    for (char *tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) {
        /* 去前导空格 */
        while (*tok == ' ') tok++;
        if (strcmp(tok, event_type) == 0) return true;
    }
    return false;
}

/* event_bus 订阅回调（在 publish 线程内执行，须快速返回） */
static void on_event(const event_t *ev, void *user)
{
    (void)user;
    if (type_matches(ev->event_type) && ev->confidence >= g_min_conf) {
        pthread_mutex_lock(&g_mtx);
        g_pending = g_active_s > 0 ? g_active_s : 3;
        pthread_cond_signal(&g_cond);
        pthread_mutex_unlock(&g_mtx);
        LOG_INF("alarm 触发: type=%s conf=%.2f", ev->event_type, ev->confidence);
    }
}

static void *alarm_thread(void *arg)
{
    (void)arg;
    while (g_running) {
        int wait_s = 0;
        pthread_mutex_lock(&g_mtx);
        while (g_running && g_pending == 0) {
            pthread_cond_wait(&g_cond, &g_mtx);
        }
        wait_s = g_pending;
        g_pending = 0;
        pthread_mutex_unlock(&g_mtx);
        if (!g_running || wait_s <= 0) continue;

        /* 点亮 LED + 蜂鸣 */
        gpio_set(g_led, 1);
        gpio_set(g_buzzer, 1);
        for (int i = 0; i < wait_s && g_running; i++) sleep(1);
        gpio_set(g_led, 0);
        gpio_set(g_buzzer, 0);
    }
    return NULL;
}

int alarm_init(const gateway_conf_t *cfg)
{
    g_led = cfg->alarm_gpio_led;
    g_buzzer = cfg->alarm_gpio_buzzer;
    g_active_s = cfg->alarm_active_s;
    strncpy(g_trigger_types, cfg->alarm_trigger_types, sizeof(g_trigger_types) - 1);
    g_min_conf = cfg->alarm_min_confidence;

    /* 无任何 GPIO 配置 → 不起线程，仅订阅（回调也不会触发任何动作）。
     * 仍订阅以保持统一接口；type 不匹配时回调早退。 */
    g_running = true;
    if (pthread_create(&g_tid, NULL, alarm_thread, NULL) != 0) {
        g_running = false;
        LOG_ERR("创建 alarm 线程失败");
        return -1;
    }
    event_bus_subscribe(on_event, NULL);
    if (g_led < 0 && g_buzzer < 0) {
        LOG_INF("alarm 就绪(无 GPIO 配置, 仅订阅, 不实际驱动)");
    } else {
        LOG_INF("alarm 就绪: led=%d buzzer=%d active=%ds 阈值=%.2f 触发类型=%s",
                g_led, g_buzzer, g_active_s, g_min_conf, g_trigger_types);
    }
    return 0;
}

void alarm_stop(void)
{
    if (!g_running) return;
    g_running = false;
    pthread_mutex_lock(&g_mtx);
    pthread_cond_signal(&g_cond);
    pthread_mutex_unlock(&g_mtx);
    pthread_join(g_tid, NULL);
    /* 确保退出时 GPIO 置低 */
    gpio_set(g_led, 0);
    gpio_set(g_buzzer, 0);
    LOG_INF("alarm 已停止");
}

void alarm_reload(const gateway_conf_t *cfg)
{
    pthread_mutex_lock(&g_mtx);
    g_led = cfg->alarm_gpio_led;
    g_buzzer = cfg->alarm_gpio_buzzer;
    g_active_s = cfg->alarm_active_s;
    strncpy(g_trigger_types, cfg->alarm_trigger_types, sizeof(g_trigger_types) - 1);
    g_trigger_types[sizeof(g_trigger_types) - 1] = 0;
    g_min_conf = cfg->alarm_min_confidence;
    pthread_mutex_unlock(&g_mtx);
    LOG_INF("alarm 参数已热重载: led=%d buzzer=%d 阈值=%.2f", g_led, g_buzzer, g_min_conf);
}
