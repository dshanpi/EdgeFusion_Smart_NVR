# EdgeFusion Gateway（IMX6ULL 边侧网关）

边侧网关守护进程：从 V821 摄像头 **RTSP 拉流** → MP4 分段录像 → 1fps 软解关键帧 → **HTTPS 上云**（阿里百炼 qwen3.6-plus 多模态视觉模型）→ 结构化 JSON → SQLite 事件入库 + GPIO 报警 + Web 预览/回放 + 本地 RTSP 转发。

运行平台：百问网 **IMX6ULL-Pro**（ARM Cortex-A7 800MHz，无 H.264 硬解 VPU）。架构与完整设计见上级 [`../PROJECT_INTRO.md`](../PROJECT_INTRO.md) 与 [`docs/TASK.md`](docs/TASK.md)。

---

## 1. 目录结构

```
edgefusion_gateway/
├── src/                 # C 源码（多线程守护进程）
│   ├── main.c           # 入口、信号、模块启动顺序
│   ├── conf.c/.h        # key=value 配置解析 + 原子保存 + 热加载
│   ├── stream_receiver.*# RTSP 拉流 + 内联扇出（录像/抽帧/RTSP转发）+ 重连
│   ├── recorder.*       # H.264 remux → fragmented MP4 分段 + 循环覆盖
│   ├── frame_grabber.*  # 软解 H.264 → swscale → MJPEG，缓存最新帧
│   ├── cloud_ai.*       # JPEG 上云（OpenAI 兼容端点）+ JSON 解析 + 热启停
│   ├── event_bus.*      # SQLite 事件入库 + 快照 + 订阅广播
│   ├── alarm.*          # GPIO sysfs 报警（LED/蜂鸣器）
│   ├── rtsp_server.*    # 自研轻量 RTSP server（H.264 passthrough 转发）
│   ├── web.*            # 自研 HTTP server（MJPEG/API/MP4 Range 回放/配置）
│   └── log.c/.h         # 日志
├── web/                 # Web 前端（SPA：实时预览/录像回放/事件告警/系统设置）
├── config/gateway.conf  # 运行配置（含凭证，已 .gitignore）
├── scripts/deploy.sh    # adb 部署脚本
├── toolchain.env        # 交叉编译环境变量（source 它）
├── Makefile             # 交叉编译
└── docs/TASK.md         # 任务书
```

---

## 2. 依赖

### 2.1 主机侧（Ubuntu 开发机）

| 依赖 | 用途 | 安装 |
|------|------|------|
| `adb` | 推送二进制/配置/读日志到板子 | `sudo apt install android-tools-adb` |
| 串口工具 | 访问 IMX6ULL 调试串口 `/dev/ttyACM0` | 仅需 `stty`/`cat`/`printf`（系统自带） |
| udev 规则 | 修 adb no permissions | 见下文 §2.4 |
| buildroot 工具链 | 交叉编译 | 见 §2.2 |

### 2.2 交叉工具链（百问网 IMX6ULL SDK）

```
路径: ~/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot
前缀: arm-buildroot-linux-gnueabihf-      (gcc/g++/ar/strip)
sysroot: <toolchain>/arm-buildroot-linux-gnueabihf/sysroot
目标 ABI: armhf, glibc 2.30, ARMv7 (Cortex-A7)
```

**sysroot 已预编译可直接链接的库（含头文件），无需自行交叉编译 FFmpeg**：
- FFmpeg：`libavformat / libavcodec / libavutil / libswscale / libswresample / libavfilter / libavdevice`（.so 全套）
- `libcurl`、`libsqlite3`、`openssl(libssl/libcrypto)`、`libjson-c`
- 基础：glibc、pthread、zlib、dl

**缺失库**：`libmosquitto`（MQTT）。sysroot 无，buildroot 有 `package/mosquitto` 但 `output/` 未构建。故 MQTT 模块当前仅有配置项，无网络代码（见 TASK.md §6.2）。

### 2.3 板子侧（IMX6ULL-Pro）

板子 rootfs 已预装上述所有 .so 运行库 + `/usr/bin/ffmpeg` 命令，**无需额外装库**。仅需：

- **CA 证书库**：buildroot 板子默认无 CA bundle，`cloud_ai` 调 HTTPS 需要 `/etc/ssl/certs/ca-certificates.crt`。`scripts/deploy.sh` 会自动从主机推送，无需手动。
- **DNS**：`/etc/resolv.conf` 需有 `nameserver`（如 `nameserver 8.8.8.8` 或路由器 IP）。板子默认 `nameserver 192.168.1.1`，通常已就绪；若云调用失败先检查此文件。
- **TF 卡**：`/dev/mmcblk0p1`(30G vfat) → `/mnt/extsd`。每次上电需 mount（deploy/运行脚本会自动尝试）。

### 2.4 udev 规则（修 adb 权限）

`/etc/udev/rules.d/51-edgefusion-android.rules`：
```
# IMX6ULL (NXP USB gadget 1d6b:0104)
SUBSYSTEM=="usb", ATTR{idVendor}=="1d6b", ATTR{idProduct}=="0104", MODE="0666"
# V821 (Allwinner 1f3a:0000) — 与本工程共用同一规则文件
SUBSYSTEM=="usb", ATTR{idVendor}=="1f3a", ATTR{idProduct}=="0000", MODE="0666"
```
改完 `sudo udevadm control --reload-rules && sudo udevadm trigger`。用户加入 `dialout` 组以访问串口：`sudo usermod -aG dialout $USER`（重新登录生效）。

---

## 3. 编译

```bash
cd ~/EdgeFusion/edgefusion_gateway
source toolchain.env      # 导出 CC/CXX/SYSROOT/ADB 序列号等
make                      # 产出 build/edgefusion_gateway (ARM 二进制)
```

常用目标：
```bash
make check-env   # 校验工具链 + FFmpeg 头/库是否就绪（不改文件）
make strip       # 编译并 strip 符号
make clean       # 清 build/
```

**链接库**（顺序敏感，被依赖者在后）：
```
-lavformat -lavcodec -lavutil -lswscale -lswresample
-lcurl -lssl -lcrypto -lsqlite3 -ljson-c -lpthread -lm -lz -ldl
```

**验证产物架构**：
```bash
arm-buildroot-linux-gnueabihf-readelf -h build/edgefusion_gateway | grep Machine
# 应输出: Machine: ARM
```

### 编译注意
- 改任何 `src/*.h` 头文件后必须 `make clean && make`（Makefile 已让 `.o` 依赖所有 `.h`，避免结构体布局错位 bug）。
- 源文件用 `$(wildcard src/*.c)` 自动收集，新增 `.c` 无需改 Makefile。

---

## 4. 配置

编辑 [`config/gateway.conf`](config/gateway.conf)（已 `.gitignore`，含凭证）。关键项：

```ini
# ---- RTSP 视频源（V821）----
rtsp_url          = rtsp://192.168.1.28:8554/ch0   # V821 推流地址（IP 随 wlan0 DHCP 变，需改）
rtsp_transport    = tcp
rtsp_reconnect_s  = 3

# ---- 录像存储 ----
storage_dir       = /mnt/extsd/recordings
segment_seconds   = 60            # 每段时长（秒）
storage_min_free_mb = 500         # 容量不足时循环删最旧段

# ---- 云端 AI（阿里百炼 qwen3.6-plus，OpenAI 兼容端点）----
ai_enable         = 1             # 总开关（1启用 0禁用，Web 可热切换）
ai_fps            = 1             # 上云抽帧率（实际受云端响应限速，qwen 约 3-4s/次）
ai_frame_width    = 640           # 上云缩放宽度
anthropic_base_url = https://llm-7zt4t2ucj8a9834x.cn-beijing.maas.aliyuncs.com/compatible-mode/v1
anthropic_auth_token = sk-...     # 留空=纯本地模式；填入=启用云端视觉分析
anthropic_model    = qwen3.6-plus # 多模态视觉模型

# ---- Web ----
web_bind = 0.0.0.0
web_port = 8080
web_mjpeg_fps = 5

# ---- 报警（GPIO，引脚待硬件确定）----
alarm_gpio_led     = -1           # -1 禁用
alarm_gpio_buzzer  = -1

# ---- MQTT（当前未实现，仅配置项）----
mqtt_enable = 0
```

> `anthropic_*` 字段名为历史遗留，实际走 OpenAI Chat Completions 协议。

---

## 5. 部署到板子

```bash
cd ~/EdgeFusion/edgefusion_gateway
source toolchain.env
make deploy        # = strip + scripts/deploy.sh
```

`deploy.sh` 做的事：
1. 确保板子 TF 卡已挂载（`/dev/mmcblk0p1 → /mnt/extsd`）
2. `adb push` 二进制 → `/mnt/extsd/gateway/edgefusion_gateway`
3. `adb push` `config/gateway.conf` → `/mnt/extsd/gateway/config/`
4. `adb push` `web/` 前端 → `/mnt/extsd/gateway/web/`
5. `adb push` 主机 `/etc/ssl/certs/ca-certificates.crt` → 板子 `/etc/ssl/certs/`（HTTPS 需要）

仅更新前端时（改 web/ 后无需重新编译 C）：
```bash
adb -s 100ask_IMX6ULL push web/index.html /mnt/extsd/gateway/web/
adb -s 100ask_IMX6ULL push web/style.css  /mnt/extsd/gateway/web/
adb -s 100ask_IMX6ULL push web/app.js     /mnt/extsd/gateway/web/
```

---

## 6. 启动与运行

### 6.1 启动网关

IMX6ULL buildroot 有 `setsid`，adb 后台进程可存活：
```bash
adb -s 100ask_IMX6ULL shell 'mountpoint -q /mnt/extsd || mount -t vfat /dev/mmcblk0p1 /mnt/extsd'
adb -s 100ask_IMX6ULL shell 'killall edgefusion_gateway 2>/dev/null; sleep 1; \
  cd /mnt/extsd/gateway && \
  setsid sh -c "./edgefusion_gateway -path /mnt/extsd/gateway/config/gateway.conf > /mnt/extsd/gateway.log 2>&1" \
  </dev/null >/dev/null 2>&1 &'
```

### 6.2 验证

```bash
# 进程 + 端口
adb -s 100ask_IMX6ULL shell 'ps | grep edgefusion; netstat -tln | grep -E ":8080|:8554"'
# 日志
adb -s 100ask_IMX6ULL shell 'cat /mnt/extsd/gateway.log'
# 状态 API
curl -s http://192.168.1.59:8080/api/status
```

### 6.3 访问

| 入口 | 地址 |
|------|------|
| Web 管理界面 | `http://192.168.1.59:8080` |
| 实时预览（MJPEG） | 浏览器进 Web → 实时预览 |
| RTSP 转发（VLC/ffmpeg/NVR） | `rtsp://192.168.1.59:8554/track0` |
| 录像文件 | `/mnt/extsd/recordings/rec_YYYYMMDD_HHMMSS_NN.mp4` |
| 事件库 | `/mnt/extsd/gateway.db`（SQLite） |
| 日志 | `/mnt/extsd/gateway.log` |

### 6.4 Web 页面

| 页面 | 功能 |
|------|------|
| 实时预览 | MJPEG 画面 + OSD（通道/REC/分辨率/码率/时间戳）+ 状态磁贴 + 抓拍 |
| 录像回放 | MP4 列表（按时间倒序）+ 内嵌播放器 + 下载 |
| 事件告警 | AI 检测事件列表（缩略图/置信度条/类型徽章/描述）+ 分页 |
| 系统设置 | 分组配置表单 + 开关（云端AI/RTSP转发/MQTT 即时保存）+ 粘性保存栏 |

### 6.5 停止

```bash
adb -s 100ask_IMX6ULL shell 'killall edgefusion_gateway'
```

---

## 7. 常见问题

| 现象 | 原因 | 解决 |
|------|------|------|
| 拉流报 `Cannot assign requested address` | FFmpeg 未初始化网络（代码已 `avformat_network_init()`） | 若仍失败，检查 V821 是否在推流、IP 是否正确 |
| 拉流 `stimeout`/`timeout` 触发 EPERM | TCP RTSP 下这两个选项不兼容 | 代码已只用 `rtsp_transport=tcp` + `analyzeduration`/`probesize`，不要加 timeout |
| 录像 probe 报 `Invalid data found` | fragmented MP4 写入中 moov 未落盘 | 停进程 finalize 后再 probe；本项目用 `frag_keyframe+empty_moov` 已断电安全 |
| Web「视频源显示离线」但有画面 | 前端渲染异常被 catch 误判（已修复） | 强制刷新浏览器（Ctrl+Shift+R 或带 `?v=N`） |
| 云端 AI 永不产生事件 | `ai_enable=0` / token 空 / 模型不支持视觉 | 确认 `ai_enable=1` 且 token 非空；模型须为多模态（qwen3.6-plus） |
| 云端 HTTP 403 AccessDenied | 阿里百炼该模型未开通 | 百炼控制台开通 `qwen3.6-plus`，启动初期权限过渡也会 403，稍后自动恢复 |
| 云端调用失败（curl 错误） | 板子无 CA bundle 或 DNS | 确认 `/etc/ssl/certs/ca-certificates.crt` 存在（deploy 推送）；`/etc/resolv.conf` 有 nameserver |
| 改 `conf.h` 后部分模块读到空值 | Makefile 头文件依赖缺失（已修） | `make clean && make` |
| V821 IP 变了导致拉不到流 | V821 wlan0 DHCP 动态 | `adb shell ip -4 addr show wlan0` 确认后改 `gateway.conf` 的 `rtsp_url`（Web 设置页可热改） |

---

## 8. 与 V821 端的关系

本网关从 V821 拉流，V821 端工程见 [`../edgefusion_vision/README.md`](../edgefusion_vision/README.md)。完整端-边-云架构见 [`../PROJECT_INTRO.md`](../PROJECT_INTRO.md)。

**典型联调流程**：先在 V821 串口启动 `sample_smartIPC_demo` 推流（`rtsp://<V821-IP>:8554/ch0`）→ 确认 V821 IP → 填入本工程 `gateway.conf` 的 `rtsp_url` → `make deploy` → 启动网关 → Web 验证。
