#include "conf.h"
#include "log.h"
#include "stream_receiver.h"
#include "event_bus.h"
#include "cloud_ai.h"
#include "alarm.h"
#include "web.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t g_running = 1;

static void on_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "EdgeFusion Gateway (IMX6ULL AI NVR 核心)\n"
        "用法: %s -path <config>\n"
        "  -path <config>  配置文件路径 (默认 /mnt/extsd/gateway/config/gateway.conf)\n"
        "  -h              显示帮助\n",
        prog);
}

int main(int argc, char *argv[])
{
    const char *conf_path = "/mnt/extsd/gateway/config/gateway.conf";
    /* 手动解析参数：支持 -path <config> 与 -h（不使用 getopt，避免 -path 被拆成单字母） */
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]);
            return 0;
        } else if (!strcmp(argv[i], "-path") && i + 1 < argc) {
            conf_path = argv[++i];
        } else {
            fprintf(stderr, "未知参数: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    /* 先用 stdout 打 banner，加载配置后再切日志文件 */
    fprintf(stdout, "========================================\n");
    fprintf(stdout, " EdgeFusion Gateway 启动中...\n");
    fprintf(stdout, " 配置文件: %s\n", conf_path);
    fprintf(stdout, "========================================\n");

    static gateway_conf_t cfg;
    if (conf_load(&cfg, conf_path) != 0) {
        /* 配置加载失败，stdout 打印后退出 */
        return 1;
    }
    conf_apply_log(&cfg);
    conf_dump(&cfg);

    /* 信号：优雅退出 */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    /* 忽略 SIGPIPE（curl/socket 写断链） */
    signal(SIGPIPE, SIG_IGN);

    LOG_INF("EdgeFusion Gateway 已启动 (本地数据管理云 + DeepSeek 检测)");

    /* 快照目录派生自 storage_dir 同级（避免改 conf） */
    char snapshot_dir[512];
    snprintf(snapshot_dir, sizeof(snapshot_dir), "%s/../snapshots", cfg.storage_dir);

    /* 事件中枢必须最先初始化（cloud_ai/alarm 依赖它） */
    if (event_bus_init(cfg.sqlite_db_path, snapshot_dir, cfg.storage_min_free_mb) != 0) {
        LOG_WRN("event_bus 初始化失败（事件入库不可用）");
    }

    /* 报警订阅者：必须在 cloud_ai 启动前注册 */
    if (alarm_init(&cfg) != 0) {
        LOG_WRN("alarm 初始化失败（报警不可用）");
    }

    /* 启动拉流+录像线程 */
    if (stream_receiver_start(&cfg) != 0) {
        LOG_ERR("拉流模块启动失败，退出");
        return 1;
    }

    /* 启动 DeepSeek 检测线程（消费 frame_grabber，token 空则纯本地模式） */
    if (cloud_ai_start(&cfg) != 0) {
        LOG_WRN("cloud_ai 启动失败（检测不可用，不影响录像/预览）");
    }

    /* 启动 Web 管理界面 */
    if (web_start(&cfg) != 0) {
        LOG_WRN("Web 模块启动失败（不影响录像）");
    }

    LOG_INF("进入主循环，等待信号退出...");
    while (g_running) {
        sleep(1);
    }

    LOG_INF("收到退出信号，正在停止...");
    web_stop();
    cloud_ai_stop();
    stream_receiver_stop();
    alarm_stop();
    event_bus_deinit();
    LOG_INF("已退出，再见");
    return 0;
}
