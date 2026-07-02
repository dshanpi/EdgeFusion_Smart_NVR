# EdgeFusion — 云端协同智能 NVR

基于 **V821（端）+ IMX6ULL（边）+ 云端大模型（云）** 三层架构的智能网络视频录像机。V821 无线采集 H.264 推流，IMX6ULL 拉流录像并提取关键帧上云做视觉分析，低成本实现「会思考」的 NVR。

```
┌─────────────┐  Wi-Fi RTSP   ┌─────────────┐  HTTPS    ┌─────────────┐
│  V821 端侧  │ ────────────▶ │ IMX6ULL 边侧│ ────────▶ │  云侧 AI    │
│ 摄像头采集  │  H.264 1080P  │ 录像+上云+  │  关键帧    │ qwen3.6-plus│
│ H.264硬编码 │               │ Web+RTSP转发│  JSON事件  │ 视觉理解    │
└─────────────┘               └─────────────┘           └─────────────┘
   edgefusion_vision           edgefusion_gateway        阿里百炼
```
<img width="1905" height="928" alt="Snipaste_2026-07-02_15-27-31" src="https://github.com/user-attachments/assets/e2bc250e-7b21-4137-8bdd-02a9486d287f" />
<img width="1908" height="933" alt="Snipaste_2026-07-02_15-18-28" src="https://github.com/user-attachments/assets/8c357844-a62a-480e-a9b7-a3ff391e29e5" />
<img width="1888" height="937" alt="Snipaste_2026-07-02_15-07-36" src="https://github.com/user-attachments/assets/40976a5c-f92a-400b-b40c-09ff8eac16eb" />
<img width="1889" height="925" alt="Snipaste_2026-07-02_15-07-53" src="https://github.com/user-attachments/assets/60e104f3-a21d-484d-babe-3686270863cd" />

---

## 仓库结构

| 路径 | 说明 |
|------|------|
| [`edgefusion_vision/`](edgefusion_vision/README.md) | **端侧**：V821 sample 工程，MIPI CSI → H.264 硬编码 → RTSP 推流 `rtsp://<V821-IP>:8554/ch0` |
| [`edgefusion_gateway/`](edgefusion_gateway/README.md) | **边侧**：IMX6ULL 网关守护进程，拉流录像 + 关键帧上云 + 事件入库 + Web/RTSP |

> 日常开发用 `edgefusion_vision`（含调好的运行配置和 RTSP URL 打印）。

---

## 硬件平台

| 板子 | 芯片 | 角色 | OS |
|------|------|------|-----|
| 百问网 AvaotaF1 V821 | 全志 V821（RISC-V 1.2GHz，内置 Wi-Fi，H.264 硬编码） | 端侧摄像头 | Tina Linux 5.0（musl） |
| 百问网 IMX6ULL-Pro | NXP i.MX6ULL（ARM Cortex-A7 800MHz） | 边侧网关/AI NVR | Buildroot Linux（glibc） |
| 阿里百炼 | qwen3.6-plus 多模态视觉模型 | 云侧 AI | — |

完整硬件规格与外设清单见 [`IMX6ULL_V821_Project.md`](IMX6ULL_V821_Project.md) §4。

---

## 核心能力

- **无线采集推流**：V821 MIPI CSI → H.264 硬编码 → Wi-Fi RTSP 推流（1920×1080@15fps）
- **本地录像**：IMX6ULL 拉 RTSP，H.264 直接 remux 成 fragmented MP4 分段（不解码，断电安全，循环覆盖）
- **云端 AI 分析**：1fps 软解关键帧 → JPEG → HTTPS 上云 qwen3.6-plus → 结构化 JSON → 事件入库（已验证识别人脸/人员，置信度 0.95+）
- **报警联动**：GPIO 驱动 LED/蜂鸣器（引脚待定）
- **Web 管理**：MJPEG 实时预览 + OSD + 录像回放 + 事件告警列表 + 配置热加载 + 云端 AI 开关
- **RTSP 转发**：IMX6ULL 自研轻量 RTSP server，H.264 passthrough 供 VLC/NVR 拉流

---

## 快速开始

### 0. 前置条件（主机 Ubuntu）

```bash
sudo apt install android-tools-adb build-essential
sudo usermod -aG dialout $USER      # 访问串口，需重新登录生效
```

配置 udev 规则 `/etc/udev/rules.d/51-edgefusion-android.rules`：
```
SUBSYSTEM=="usb", ATTR{idVendor}=="1d6b", ATTR{idProduct}=="0104", MODE="0666"  # IMX6ULL
SUBSYSTEM=="usb", ATTR{idVendor}=="1f3a", ATTR{idProduct}=="0000", MODE="0666"  # V821
```
```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
adb devices   # 应同时看到 100ask_IMX6ULL 和 4c48410491c30a72512
```

两板出厂已烧好系统，无需重新烧录固件。

### 1. V821 端：编译并启动推流

```bash
cd ~/EdgeFusion/edgefusion_vision
./build.sh                                    # 产出 output/sample_smartIPC_demo_strip

adb -s 4c48410491c30a72512 push output/sample_smartIPC_demo_strip /mnt/extsd/sample_smartIPC_demo
adb -s 4c48410491c30a72512 push output/sample_smartIPC_demo.conf   /mnt/extsd/
adb -s 4c48410491c30a72512 shell 'chmod +x /mnt/extsd/sample_smartIPC_demo'
```

**必须从串口启动**（V821 无 setsid，adb 后台进程会随会话退出被杀）：
```bash
sudo stty -F /dev/ttyUSB0 115200 cs8 -cstopb -parenb -ixon -ixoff raw -echo
sudo bash -c 'printf "cd /mnt/extsd && ./sample_smartIPC_demo -path /mnt/extsd/sample_smartIPC_demo.conf > /mnt/extsd/run.log 2>&1 &\r\n" > /dev/ttyUSB0'
```

确认推流地址（IP 随 wlan0 DHCP 变）：
```bash
adb -s 4c48410491c30a72512 shell 'grep "rtsp://" /mnt/extsd/run.log'        # rtsp://<V821-IP>:8554/ch0
adb -s 4c48410491c30a72512 shell 'ip -4 addr show wlan0 | grep inet'          # 记下 V821 IP
```

> 详细步骤与排错见 [`edgefusion_vision/README.md`](edgefusion_vision/README.md)。

### 2. IMX6ULL 端：编译、配置、部署、启动

```bash
cd ~/EdgeFusion/edgefusion_gateway
# 编辑 config/gateway.conf，把 rtsp_url 改成上一步的 V821 地址，填入云端 token
#   rtsp_url          = rtsp://<V821-IP>:8554/ch0
#   anthropic_auth_token = sk-...

source toolchain.env && make && make deploy    # 交叉编译 + adb 部署到 /mnt/extsd/gateway/

adb -s 100ask_IMX6ULL shell 'mountpoint -q /mnt/extsd || mount -t vfat /dev/mmcblk0p1 /mnt/extsd'
adb -s 100ask_IMX6ULL shell 'killall edgefusion_gateway 2>/dev/null; sleep 1; \
  cd /mnt/extsd/gateway && \
  setsid sh -c "./edgefusion_gateway -path /mnt/extsd/gateway/config/gateway.conf > /mnt/extsd/gateway.log 2>&1" \
  </dev/null >/dev/null 2>&1 &'
```

> 详细步骤与排错见 [`edgefusion_gateway/README.md`](edgefusion_gateway/README.md)。

### 3. 验证

浏览器打开 **`http://192.168.1.59:8080`**（IMX6ULL IP）：
- **实时预览**：看到摄像头画面 + OSD（通道/REC/分辨率/码率/时间戳）
- **事件告警**：云端 AI 识别到的人脸/人员事件（带快照、置信度、描述）
- **录像回放**：60 秒分段的 MP4 列表
- **系统设置**：可热切换云端 AI 开关、改配置

```bash
curl -s http://192.168.1.59:8080/api/status    # 状态：stream/recording/cloud_ai
# RTSP 转发：rtsp://192.168.1.59:8554/track0（VLC 可拉）
```

---

## 板子访问速查

| 板子 | ADB 序列号 | 调试串口 | 默认 IP |
|------|-----------|----------|---------|
| IMX6ULL | `100ask_IMX6ULL` | `/dev/ttyACM0`（115200，免 sudo） | `192.168.1.59`（eth0 固定） |
| V821 | `4c48410491c30a72512` | `/dev/ttyUSB0`（115200，需 sudo） | DHCP 动态（wlan0，曾 .28/.44） |

> V821 走 wlan0 DHCP，**IP 每次上电可能变**，需重新查并更新 `gateway.conf` 的 `rtsp_url`（Web 设置页可热改）。

---

## 当前状态（2026-07-02）

| 阶段 | 状态 |
|------|------|
| P0 工程骨架 + 交叉编译 | ✅ |
| P1 RTSP 拉流 + MP4 分段录像 | ✅ |
| P2 关键帧上云（qwen3.6-plus 视觉分析） | ✅ 已验证 |
| P3 事件入库 + GPIO 报警订阅 | ✅ |
| P4 Web 预览/回放/事件/配置 + 云端 AI 开关 | ✅ |
| 本地 RTSP passthrough 转发 | ✅ |
| P5 MQTT 上报 | ⏳ 仅配置项，未实现（缺 libmosquitto） |
| 成本优化（V821 pdet 预筛选） | ⏳ 待办 |
| OTA / 多路接入 / 72h 稳定性 | ⏳ 待办 |

---

## 文档导航

| 想了解 | 看这里 |
|--------|--------|
| 项目全貌、架构、模块职责 | [`PROJECT_INTRO.md`](PROJECT_INTRO.md) |
| 立项背景、硬件选型、实施计划 | [`IMX6ULL_V821_Project.md`](IMX6ULL_V821_Project.md) |
| V821 端编译/运行/排错 | [`edgefusion_vision/README.md`](edgefusion_vision/README.md) |
| IMX6ULL 端编译/运行/排错 | [`edgefusion_gateway/README.md`](edgefusion_gateway/README.md) |
| 网关任务书与决策点 | [`edgefusion_gateway/docs/TASK.md`](edgefusion_gateway/docs/TASK.md) |
