# EdgeFusion 边侧网关（IMX6ULL-Pro AI NVR 核心）实现任务书

**文档版本**：V1.0
**创建日期**：2026-06-30
**对应立项书**：`EdgeFusion/IMX6ULL_V821_Project.md` §3.3 / §5.2
**工程目录**：`~/EdgeFusion/edgefusion_gateway/`
**状态**：P0/P1 已验证运行（V821 推流 + IMX6ULL 拉流/录像/转发/Web 全链路打通，2026-07-01）

---

## 0. 背景与定位

V821 端（`edgefusion_vision`）已完成 RTSP 视频推流，推流地址 `rtsp://<V821-IP>:8554/ch0`（端口 8554，URL 路径 `ch<id>`，见 memory `v821-rtsp-stream`）。

本任务实现“边侧：IMX6ULL-Pro（AI NVR 核心）”：IMX6ULL 作为 **RTSP 拉流端** 接收 V821 视频流，完成本地分段录像、关键帧提取上云、云端结果解析、报警联动、事件入库与 Web 预览/回放。

> 说明：立项书 §3.3 写“IMX6ULL 启动 RTSP/RTMP 服务器接收”，但 V821 端实际运行的是 RTSP **server**。因此 IMX6ULL 侧采用 **FFmpeg 拉流（RTSP client）** 模式，与 V821 对接；同时 IMX6ULL 可对内再以 MJPEG/HTTP-FLV 方式供 Web 预览。

---

## 1. 开发环境与工具链（已核实）

| 项目 | 路径 / 值 |
|------|----------|
| 交叉工具链 | `~/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot` |
| 前缀 | `arm-buildroot-linux-gnueabihf-`（gcc/g++） |
| sysroot | `<toolchain>/arm-buildroot-linux-gnueabihf/sysroot` |
| 目标 ABI | armhf, glibc 2.30, ARMv7 (Cortex-A7) |
| 备用工具链 | `gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf`（不优先） |

**sysroot 中已预编译可直接链接的库（含头文件）**：
- FFmpeg：`libavcodec/libavformat/libavutil/libswscale/libswresample/libavfilter/libavdevice`（.so 全套）
- 网络/存储：`libcurl`、`libsqlite3`、`openssl(libssl/libcrypto)`、`libjson-c`
- 基础：glibc、pthread、zlib

**缺失库**：`libmosquitto`（MQTT）—— sysroot 无，buildroot 有 `package/mosquitto` 但 `output/` 未构建。处理方案见 §6。

---

## 2. 系统架构（边侧进程内模块）

单一 C 守护进程 `edgefusion_gateway`，多线程，模块化：

```
                ┌─────────────────────────────────────────────┐
                │            edgefusion_gateway (C)           │
 RTSP 拉流  ┌───┴─────────┐   1fps 解码帧    ┌──────────────┐
 rtsp://... │  stream_     │────────────────▶│  frame_      │── JPEG ──┐
  ─────────▶│  receiver    │   原始 H.264包   │  grabber     │          │
  (V821)    │  (FFmpeg     │────────────┐    └──────────────┘          │
            │   avformat)  │            │                              ▼
            └───┬─────────┘            │                       ┌──────────────┐
                │                      ▼                       │  cloud_ai    │
                │               ┌──────────┐                   │  (libcurl +  │
                │               │ recorder │ MP4 分段           │   json-c)    │
                │               │ (remux)  │────────▶ TF卡      │  HTTPS POST  │
                │               └──────────┘                   └──────┬───────┘
                │                                                     │ JSON 结果
                │                                              ┌──────▼───────┐
                │                                              │ event_bus    │
                │                                              │ (sqlite3 入库)│
                │                                              └──┬───┬───┬───┘
                │                                  ┌──────────────┘   │   │
                │                            ┌─────▼─────┐  ┌─────────▼┐ │
                │                            │  alarm    │  │   web    │ │
                │                            │ (GPIO     │  │ (HTTP    │ │
                │                            │  LED/蜂鸣)│  │  预览/回放│ │
                │                            └───────────┘  └──────────┘ │
                │                                                       ▼
                │                                              ┌──────────────┐
                └─────────────────────────────────────────────▶│   mqtt       │
                                                               │ (事件上报,可选)│
                                                               └──────────────┘
```

### 模块职责

| 模块 | 职责 | 关键库 |
|------|------|--------|
| `stream_receiver` | RTSP 拉流，读取 H.264 packet；分发给 recorder 与 frame_grabber | libavformat |
| `recorder` | 按 H.264 直接 remux 成 MP4 分段（不解码），按时间分段、循环覆盖 | libavformat |
| `frame_grabber` | 软解 H.264（1fps）→ swscale 缩放 → JPEG 编码，送云端 | libavcodec/libswscale |
| `cloud_ai` | JPEG Base64 + HTTPS POST 到阿里百炼 qwen3.6-plus（OpenAI 兼容端点）；解析返回 JSON；Web 开关热启停 | libcurl/json-c/openssl |
| `event_bus` | 事件入库 SQLite；向 alarm/web/mqtt 广播 | sqlite3 |
| `alarm` | 依据 event_type/confidence 驱动 GPIO（sysfs）控制 LED/蜂鸣器 | glibc/sysfs |
| `web` | 轻量 HTTP server：MJPEG 实时预览、录像列表/回放、事件列表 JSON | 自研 socket |
| `mqtt` | 事件 MQTT 上报（libmosquitto 或占位） | mosquitto（见 §6） |

---

## 3. 性能与可行性策略（针对 800MHz 单核无 VPU）

i.MX6ULL **无 H.264 硬解 VPU**，软件解码风险高（立项书 §8 已识别）。采取：

1. **录像路径不解码**：RTSP 的 H.264 NAL 直接 remux 为 MP4（`av_interleaved_write_frame`），CPU 极低。
2. **AI 路径低频解码**：仅按 **1fps** 解码（且可降到 0.5fps），解码后 swscale 缩到 **640×480** 再编码 JPEG 上云。720P@1fps 软解在 800MHz 上可行。
3. **建议**：与 V821 端协商把主码流降到 720P@15fps（或专设一子码流 640×480@10fps 供 AI），降低边侧压力。
4. 云端调用 **异步**：上云线程独立，不阻塞拉流/录像；云端响应慢时丢弃过期帧（只保留最新）。

---

## 4. 目录结构（待创建）

```
edgefusion_gateway/
├── README.md
├── Makefile                  # 交叉编译，基于 buildroot 工具链
├── toolchain.env             # 工具链/ sysroot 环境变量
├── config/
│   └── gateway.conf          # RTSP 源、云端 API、GPIO、存储路径等配置
├── src/
│   ├── main.c
│   ├── log.h / log.c
│   ├── conf.h / conf.c       # 配置解析
│   ├── stream_receiver.h/.c
│   ├── recorder.h/.c
│   ├── frame_grabber.h/.c
│   ├── cloud_ai.h/.c
│   ├── event_bus.h/.c        # sqlite + 内存事件广播
│   ├── alarm.h/.c            # GPIO sysfs
│   ├── web.h/.c              # 轻量 HTTP server
│   └── mqtt.h/.c             # MQTT（可选/占位）
├── web/
│   ├── index.html            # 预览/回放/事件列表页
│   └── assets/
├── scripts/
│   ├── deploy.sh             # 推送到板子（adb/scp）
│   └── test_rtsp.sh          # 仅用 ffmpeg 拉流验证
└── docs/
    └── TASK.md               # 本文件
```

---

## 5. 实施阶段与里程碑

| 阶段 | 内容 | 验收 |
|------|------|------|
| **P0 工程骨架** | 目录、Makefile、toolchain.env、配置解析、log、交叉编译空壳能跑 | `make` 产出 armhf 二进制，板子上能启动 |
| **P1 拉流+录像** | `stream_receiver` + `recorder`：拉 `rtsp://...:8554/ch0`，remux 成 MP4 分段，TF 卡循环覆盖 | 板上生成可播放 MP4，自动分段/覆盖 |
| **P2 关键帧上云** | `frame_grabber` 1fps 解码→JPEG；`cloud_ai` HTTPS POST qwen3.6-plus（OpenAI 兼容端点），解析 JSON | 云端返回结构化结果，日志可见，事件入库 ✅ |
| **P3 事件入库+报警** | `event_bus`（sqlite3）；`alarm` GPIO 驱动 LED/蜂鸣器 | 检测到事件→入库→GPIO 动作 |
| **P4 Web 预览/回放** | `web`：MJPEG 实时预览、录像列表回放、事件列表 JSON | 浏览器可看实时画面/回放/事件 |
| **P5 MQTT 上报** | `mqtt` 事件推送（依赖 §6 方案） | 手机/平台收到事件 |
| **P6 联调与稳定性** | 与 V821 端联调，长时运行 | 连续运行达标 |

---

## 6. 关键技术决策点（已确认定稿）

1. **云端 AI 对接**：✅ 阿里百炼 **qwen3.6-plus** 多模态视觉模型，走 **OpenAI 兼容端点**（`anthropic_base_url=https://llm-7zt4t2ucj8a9834x.cn-beijing.maas.aliyuncs.com/compatible-mode/v1`）。`cloud_ai` 按 **OpenAI Chat Completions API** 格式发请求：`POST /chat/completions`，header `Authorization: Bearer <key>`，图像作为 content block `{"type":"image_url","image_url":{"url":"data:image/jpeg;base64,<...>"}}`，并带 `enable_thinking:false` 关闭 qwen3 思考以提速。模型 `qwen3.6-plus`。凭证从 `config/gateway.conf` 读取（`anthropic_*` 字段为历史命名，实际是 OpenAI 协议），文件加 `.gitignore`。**已验证可识别人脸/人员，置信度 0.95+**。注：字段名仍叫 `anthropic_*` 是历史遗留，重命名会牵动 conf/web/多模块，保持不动。
2. **MQTT 实现**：✅ 内嵌精简 MQTT v3.1.1 publisher（基于 openssl/socket，自研约 300 行，免外部依赖）。
3. **Web 实时预览**：✅ 自研 HTTP server 输出 MJPEG，监听 `0.0.0.0:8080`。
4. **V821 码流**：✅ 通过改 V821 `sample_smartIPC_demo.conf` 增设子码流（`sub_rtsp_id=0`→新地址 `rtsp://<IP>:8554/ch1`）供 AI。P1 先按主码流拉流 + 边侧降采样推进，子码流后续接入。
5. **GPIO 引脚**：保留，待定。报警规则默认：`event_type` 含 `person_detected`/`*_alert` 且 `confidence≥0.6` → LED 闪 + 蜂鸣 3 秒。
6. **云端返回 JSON 字段**：✅ 固定解析 `event_type/confidence/description/timestamp/suggested_action`；prompt 强约束模型输出 JSON，`cloud_ai` 容错解析（剥 ```json 围栏、取首个 `{...}`）。
7. **TF 卡挂载点**：✅ 设备 `/dev/mmcblk0p1`（30G vfat），挂载 `/mnt/extsd`。录像 `/mnt/extsd/recordings`，库 `/mnt/extsd/gateway.db`。
8. **云端 AI 开关**：✅ 新增 `ai_enable` 配置字段（conf.h/cloud_ai/web 前端全链路）。启用条件 `ai_enable && token非空`。Web 系统设置页拨片开关**即时保存**（拨动即 POST 单字段，toast 提示），后端 `cloud_ai_reload` 检测开关翻转后**热启停**云端线程（关→stop，开→stop+start），无需重启网关。状态持久化到 conf。

## 6.1 板子访问与环境（已核实）

| 项目 | 值 |
|------|-----|
| IMX6ULL adb 序列号 | `100ask_IMX6ULL`（USB gadget `1d6b:0104`，NXP） |
| V821 adb 序列号 | `4c48410491c30a72512`（全志 `1f3a:0000` Tina ADB） |
| IMX6ULL 调试串口 | `/dev/ttyACM0`（115200 8N1，用户已在 dialout 组） |
| V821 调试串口 | `/dev/ttyUSB0` |
| IMX6ULL IP | `192.168.1.59`（eth0，与主机同网段互通） |
| 板子内核 | Linux 4.9.88 armv7l，Cortex-A7（CPU part 0xc07） |
| 板子已有库 | FFmpeg(.so 全套)、libcurl、libsqlite3、libssl、libjson-c 均已预装 |
| 板子已有命令 | `/usr/bin/ffmpeg`（可用于 P1 前置拉流自测） |
| 主机 udev 规则 | `/etc/udev/rules.d/51-edgefusion-android.rules`（已配置 1d6b:0104 与 1f3a） |
| TF 卡 | `/dev/mmcblk0p1` → `/mnt/extsd`（vfat, 30G, rw） |
| 部署方式 | adb push（`scripts/deploy.sh`），串口看日志 |

---

## 7. 风险与对策（承接立项书 §8）

| 风险 | 对策 |
|------|------|
| 1080P 软解性能不足 | 录像 remux 不解码；AI 仅 1fps、降分辨率；建议 V821 降码流 |
| 云端延迟高 | 异步上云，丢弃过期帧，频率可配（默认 1fps，可调到 0.2fps） |
| RTSP 断流 | 拉流线程带重连（指数退避），录像分段平滑续接 |
| TF 卡写满 | 录像按总容量阈值循环删除最旧段 |
| libmosquitto 缺失 | 采用 §6.2-(b) 内嵌精简 publisher |

---

## 8. 下一步

所有决策点（§6）与环境（§6.1）已确认就绪，进入 **P0：工程骨架 + Makefile + 配置 + toolchain.env**，并在板子上验证空壳可启动。

---

## 9. 运行方法（已验证，2026-07-01）

### 9.1 板子访问入口（已核实）

| 板子 | ADB 序列号 | 调试串口 | 当前 IP |
|------|-----------|----------|---------|
| **IMX6ULL**（边侧网关） | `100ask_IMX6ULL` | `/dev/ttyACM0`（115200 8N1，ubuntu 在 dialout 组，免 sudo） | `192.168.1.59`（eth0，固定） |
| **V821**（视频源） | `4c48410491c30a72512` | `/dev/ttyUSB0`（115200 8N1，root:dialout，需 `sudo`） | `192.168.1.28`（wlan0，**DHCP 动态**，曾为 .44） |

- `adb devices` 应同时看到两台 `device`。udev 规则 `/etc/udev/rules.d/51-edgefusion-android.rules` 已配。
- 两板均与主机同网段（192.168.1.0/24）互通。**V821 走 wlan0 DHCP，IP 会变，每次上电务必 `adb shell ip -4 addr show wlan0` 确认后改 `config/gateway.conf` 的 `rtsp_url`。**

### 9.2 V821 端：启动 RTSP 推流

```sh
# 串口方式（推荐，进程能存活）
sudo stty -F /dev/ttyUSB0 115200 cs8 -cstopb -parenb -ixon -ixoff raw -echo
sudo bash -c 'printf "killall sample_smartIPC_demo 2>/dev/null\r\n" > /dev/ttyUSB0'
sudo bash -c 'printf "cd /mnt/extsd && ./sample_smartIPC_demo -path /mnt/extsd/sample_smartIPC_demo.conf > /mnt/extsd/run.log 2>&1 &\r\n" > /dev/ttyUSB0'
```
- 推流地址：`rtsp://192.168.1.28:8554/ch0`（H.264 High 1920×1080@15fps，已在 IMX6ULL 上 `ffmpeg` 拉流验证可播）。
- conf 关键项：`main_rtsp_id = 0`（启用主码流 RTSP）、`rtsp_net_type = 3`（wlan0）。
- **坑**：V821 busybox 无 `setsid`/`nohup`，`adb shell '... &'` 后台进程在 adb 会话退出时被 SIGHUP 杀掉（即使 `( & )` 双 fork 也无效）。**必须从串口控制台启动**（串口会话持久，后台进程存活）。
- **坑**：反复启动会泄漏 ION 内存 → `vencInit fail`/`ion alloc fail` → 编码器起不来。出错后 `reboot` 清干净再单次启动。串口 `killall` 后不要立即重启同一程序。

### 9.3 IMX6ULL 端：启动网关

```sh
# 主机侧编译+部署
cd ~/EdgeFusion/edgefusion_gateway
source toolchain.env && make && make deploy     # adb push 到 /mnt/extsd/gateway/

# 板上运行（IMX6ULL buildroot 有 setsid，adb 后台可存活）
adb -s 100ask_IMX6ULL shell 'mountpoint -q /mnt/extsd || mount -t vfat /dev/mmcblk0p1 /mnt/extsd'
adb -s 100ask_IMX6ULL shell 'pkill -f edgefusion_gateway; cd /mnt/extsd/gateway && \
  setsid sh -c "./edgefusion_gateway -path /mnt/extsd/gateway/config/gateway.conf > /mnt/extsd/gateway.log 2>&1" </dev/null >/dev/null 2>&1 &'
```
- 验证：`adb -s 100ask_IMX6ULL shell 'ps | grep edgefusion; netstat -tln | grep -E ":8080|:8554"'`
- 日志：`adb -s 100ask_IMX6ULL shell 'cat /mnt/extsd/gateway.log'`
- Web 预览/回放：`http://192.168.1.59:8080`；RTSP 转发：`rtsp://192.168.1.59:8554/track0`
- 录像：`/mnt/extsd/recordings/rec_YYYYMMDD_HHMMSS_NN.mp4`（60s 分段，已验证自动轮转）
- cloud_ai：`ai_enable=1` 且 `anthropic_auth_token` 非空时启用 qwen3.6-plus 视觉分析；任一不满足则纯本地模式（不调云）。Web 系统设置页有开关可热启停。

### 9.4 已验证全链路状态（2026-07-01 02:13+）

| 模块 | 状态 |
|------|------|
| stream_receiver | ✅ 拉 h264 1920×1080@15fps |
| recorder | ✅ MP4 分段录像，自动轮转（rec_..._00→01） |
| frame_grabber | ✅ 1fps 软解 → 640×360 JPEG |
| rtsp_server | ✅ H.264 passthrough 转发，SPS/PPS 已获取，:8554 |
| web | ✅ 0.0.0.0:8080 |
| event_bus | ✅ sqlite3 就绪 |
| alarm | ✅ 订阅就绪（无 GPIO 配置，仅订阅不驱动） |
| cloud_ai | ✅ qwen3.6-plus 视觉分析已验证（识别人脸/人员，置信度 0.95+），Web 开关热启停 |
| mqtt | ⏸ 默认禁用（mqtt_enable=0） |
