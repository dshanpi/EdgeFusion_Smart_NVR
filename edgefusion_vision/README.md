# EdgeFusion Vision（V821 端侧摄像头）

全志 **V821 (AvaotaF1)** 上的智能摄像头 sample：MIPI CSI 采集 → ISP → **H.264 硬件编码** → 内置 Wi-Fi **RTSP 推流**，供 IMX6ULL 边侧网关拉流。基于全志 eyesee-mpp 多媒体框架的 `sample_smartIPC_demo`，**脱离 Tina SDK 单独外编**。

本工程是 `v821_mpp_ex_compile`（干净基线）的工作副本，额外含：`rtsp_server.cpp` 推流 URL 打印、调好的运行配置。完整端-边-云架构见上级 [`../PROJECT_INTRO.md`](../PROJECT_INTRO.md)。

---

## 1. 平台与产物

| 项目 | 值 |
|------|-----|
| 芯片 | 全志 V821（双核 RISC-V，主核 1.2GHz + 小核 600MHz，22nm，内置 2.4G Wi-Fi） |
| 架构 | RISC-V 32 (`rv32imafdcxandes`, ABI ilp32d, Andes-25 系列) |
| OS | Tina Linux 5.0（OpenWrt 衍生，**musl libc**） |
| 内核 | Linux 5.4.220 |
| 摄像头 | GC2083 MIPI CSI（1920×1080@30fps） |
| 编译产物 | `output/sample_smartIPC_demo` + `output/sample_smartIPC_demo_strip` |
| 推流地址 | `rtsp://<板子IP>:8554/ch0`（H.264 1920×1080@15fps，2.5Mbps） |

产物校验（`readelf`）：
- 解释器：`/lib/ld-musl-riscv32.so.1`（musl）
- 文件类型：ELF 32-bit LSB, UCB RISC-V
- NEEDED 动态库：仅 `liblog.so / libasound.so.2 / libstdc++.so.6 / libgcc_s.so.1 / libc.so`（MPP/pdet/ISP/RTSP 全静态链接）
- RPATH：`/usr/lib`

---

## 2. 目录结构

```
edgefusion_vision/
├── toolchain -> .../prebuilt/rootfsbuilt/riscv/nds32le-linux-musl-v5d  # musl 工具链（符号链接）
├── aw_pack_src/lib_aw/
│   ├── lib/eyesee-mpp/      # 全部 MPP 静态库 .a（含 pdet 人形检测库）
│   └── include/             # 头文件（eyesee-mpp/libisp/libcedarc/pdet/uapi/...）
├── share_lib/               # 运行期动态库（liblog.so/libasound.so.2 + 传递依赖）
├── sample/
│   ├── sample_smartIPC_demo/   # 主 sample 源码 + 多种 .conf 变体
│   ├── common/                 # 共用源（rtsp_server.cpp/record.c/pdet/...）
│   └── configfileparser/       # confparser.h
├── output/                  # 编译产物 + 中间 .o + 运行配置
│   ├── sample_smartIPC_demo
│   ├── sample_smartIPC_demo_strip
│   └── sample_smartIPC_demo.conf   # 调好的运行配置（RTSP 已启用）
├── Makefile                 # 外编 Makefile（riscv32/musl, AWCHIP=0x1882）
├── build.sh                 # 一键编译脚本
└── README.md                # 本文件
```

---

## 3. 依赖

### 3.1 主机侧（Ubuntu 开发机）

| 依赖 | 用途 | 安装/来源 |
|------|------|----------|
| `adb` | 推送二进制/配置到板子 | `sudo apt install android-tools-adb` |
| 串口工具 | 访问 V821 调试串口 `/dev/ttyUSB0` | `stty`/`cat`/`printf`（系统自带，**无 picocom/screen**） |
| `make`、`gcc`（主机） | 跑 Makefile | `sudo apt install build-essential` |
| **Tina SDK**（提供工具链+MPP库+头文件） | 交叉编译 | 全志 Tina V821 SDK v1.3，见 §3.2 |
| udev 规则 | 修 adb no permissions | 见 §3.4 |

### 3.2 Tina SDK（工具链 + MPP 库来源）

本工程已把以下三样从一次构建好的 Tina SDK 中提取并打包进工程，**正常使用无需再装 SDK**：

1. **musl 工具链**：`toolchain` 符号链接 → `tina-v821-v1.3/prebuilt/rootfsbuilt/riscv/nds32le-linux-musl-v5d`
   - 前缀 `riscv32-linux-musl-gcc/g++`（musl libc，**不是 glibc**）
   - 注意：目录名 `nds32le` 是历史遗留，二进制实际是 RISC-V
2. **MPP 静态库 + 头文件**：`aw_pack_src/lib_aw/`（从 SDK `staging_dir/target/usr/lib/{,eyesee-mpp/,pdet/}` 和 `usr/include/` 拷贝组装）
3. **内核 UAPI 头**：`aw_pack_src/lib_aw/include/uapi`（从 `out/v821/kernel/build/user_headers/include`）

> 仅当 SDK 重新 `make && pack` 后需要更新 bundle 时，才用到完整 SDK（见 §8 更新方法）。

### 3.3 板子侧（AvaotaF1 V821）

板子 rootfs 已预装运行所需动态库，**无需额外装库**：
- `/usr/lib/liblog.so`、`/usr/lib/libasound.so.2`（直接依赖）
- `libstdc++.so.6`、`libgcc_s.so.1`、`libc.so`（musl，系统自带）

存储：TF 卡 `/dev/mmcblk0p1` → `/mnt/extsd`（vfat，可读写）。出厂已配自动挂载。

### 3.4 udev 规则（修 adb 权限）

`/etc/udev/rules.d/51-edgefusion-android.rules`：
```
# V821 (Allwinner 1f3a:0000)
SUBSYSTEM=="usb", ATTR{idVendor}=="1f3a", ATTR{idProduct}=="0000", MODE="0666"
# IMX6ULL (NXP USB gadget 1d6b:0104) — 与本工程共用同一规则文件
SUBSYSTEM=="usb", ATTR{idVendor}=="1d6b", ATTR{idProduct}=="0104", MODE="0666"
```
改完 `sudo udevadm control --reload-rules && sudo udevadm trigger`。

访问串口 `/dev/ttyUSB0` 需 `dialout` 组（V821 串口权限 `root:dialout`，**当前用户若不在组内需 `sudo`**）：`sudo usermod -aG dialout $USER`（重新登录生效）。

---

## 4. 编译

```bash
cd ~/EdgeFusion/edgefusion_vision
./build.sh
```

`build.sh` 内部执行 `make build_app`，成功后 `output/` 下生成：
- `sample_smartIPC_demo`（带 debug 符号）
- `sample_smartIPC_demo_strip`（strip 后，部署用这个）

**关键编译参数**（Makefile）：
- `AWCHIP=0x1882`（V821；V853 是 `0x1886`）
- `-DTINA_PLATFORM -DUSE_STD_LOG -DCONFIG_LOG_LEVEL=3`（printf 风格日志，不依赖 libglog）
- MPP 子系统宏：`MPPCFG_VI=1 MPPCFG_VENC=1 MPPCFG_AIO/AENC/ADEC=1 MPPCFG_MUXER/DEMUXER=1`
- 架构由工具链默认给出（`rv32imafdcxandes`/`ilp32d`），无需显式 `-march/-mabi`；`-mstrict-align` 与 SDK 一致

**链接方式**：
- MPP 库**静态链接**：`-Wl,-Bstatic -Wl,--start-group <所有 .a> --end-group`（**不用 `--whole-archive`**，否则 pdet 库 `libnn_p.a` 与 `libnn_qt_dq_p.a` 都含 `get_version_libnn` 会 multiple definition）
- 运行库**动态链接**：`-Wl,-Bdynamic -llog -lasound`
- `-rpath-link share_lib`（解析 liblog→libglog→libunwind→libz 传递依赖），`-rpath=/usr/lib`

### 编译注意
- 工具链符号链接 `toolchain` 必须指向有效路径（`ls -l toolchain` 确认）。
- 头文件包含路径有**两处必须排除**（已在 Makefile 处理，勿乱改 `-I`）：
  - `eyesee-mpp/external/*`：jsoncpp 的 `json/features.h` 会遮蔽 musl 系统 `<features.h>`
  - `uapi*` 子目录平铺：内核 `linux/`/`asm-generic/` 会遮蔽 musl `<signal.h>/<time.h>` 导致 `struct timeval/sigset_t` 重定义。UAPI 只加 3 个路径：`uapi`、`uapi/bsp`、`uapi/bsp/linux`

---

## 5. 配置

运行配置是 iniparser 风格的 `sample_smartIPC_demo.conf`。本工程 `output/sample_smartIPC_demo.conf` 是调好的（RTSP 已启用）。关键项：

```ini
rtsp_net_type = 3            # 0:lo 1:eth0 2:br0 3:wlan0（推流用哪个网卡的 IP）
main_rtsp_id   = 0           # -1 禁用 RTSP；0 启用主码流 → rtsp://IP:8554/ch0
main_encode_type  = "H.264"
main_src_width  = 1280       # 采集分辨率
main_src_height = 720
main_encode_width  = 1920    # 编码输出分辨率
main_encode_height = 1080
main_encode_frame_rate = 15  # 帧率
main_encode_bitrate = 2621440  # 2.5 Mbps
rc_mode = 1                  # 1=VBR
motion_pdet_sensitivity = 1  # 人形检测灵敏度（0 关闭/测试，1-3）
main_2nd_pdet_enable = 0     # 端侧人形检测默认关闭（由云端分析替代）
```

变体配置（在 `sample/sample_smartIPC_demo/`）：`-3M-rtsp`、`-4M-rtsp`、`-dual-rtsp`、`-three-rtsp`、`-dual-2M-stitch-rtsp` 等，支持多路/拼接。RTSP 最多 4 路（`ch0`~`ch3` = main/sub/three/four）。

> **RTSP 默认禁用**：`sample/sample_smartIPC_demo/sample_smartIPC_demo.conf`（仓库内）里 `main_rtsp_id = -1`。要推流必须用 `output/` 下那份（`=0`）或自己改。

---

## 6. 部署到板子

```bash
adb -s 4c48410491c30a72512 push output/sample_smartIPC_demo_strip /mnt/extsd/sample_smartIPC_demo
adb -s 4c48410491c30a72512 push output/sample_smartIPC_demo.conf   /mnt/extsd/
adb -s 4c48410491c30a72512 shell 'chmod +x /mnt/extsd/sample_smartIPC_demo'
```

> V821 adb 序列号 `4c48410491c30a72512`。`adb devices` 应显示 `device`。

---

## 7. 启动与运行

### 7.1 启动（**必须从串口启动**）

V821 busybox 极精简（**无 `setsid`/`nohup`/`tail`/`head`/`sed`**），`adb shell '... &'` 后台进程在 adb 会话退出时被 SIGHUP 杀掉（双 fork 也无效）。**长时间运行的程序必须从串口控制台启动**（串口会话持久）。

```bash
# 1. 配置串口
sudo stty -F /dev/ttyUSB0 115200 cs8 -cstopb -parenb -ixon -ixoff raw -echo

# 2. 杀掉旧进程
sudo bash -c 'printf "killall sample_smartIPC_demo 2>/dev/null\r\n" > /dev/ttyUSB0'

# 3. 从串口启动（后台 + 重定向日志）
sudo bash -c 'printf "cd /mnt/extsd && ./sample_smartIPC_demo -path /mnt/extsd/sample_smartIPC_demo.conf > /mnt/extsd/run.log 2>&1 &\r\n" > /dev/ttyUSB0'
```

### 7.2 验证

```bash
# 进程 + 端口（adb 查）
adb -s 4c48410491c30a72512 shell 'ps | grep sample_smartIPC; netstat -tln | grep 8554'

# 日志里的推流地址
adb -s 4c48410491c30a72512 shell 'grep "rtsp://" /mnt/extsd/run.log'
# 应输出: rtsp://192.168.1.<DHCP分配>:8554/ch0

# 当前 wlan0 IP（DHCP 动态，每次上电可能变）
adb -s 4c48410491c30a72512 shell 'ip -4 addr show wlan0 | grep inet'
```

从主机或 IMX6ULL 验证拉流：
```bash
ffmpeg -rtsp_transport tcp -i rtsp://<V821-IP>:8554/ch0 -t 3 -f null -   # 应解码出帧
```

### 7.3 停止

```bash
sudo bash -c 'printf "killall sample_smartIPC_demo\r\n" > /dev/ttyUSB0'
# 或 adb（若进程还在）
adb -s 4c48410491c30a72512 shell 'killall sample_smartIPC_demo'
```

---

## 8. 常见问题

| 现象 | 原因 | 解决 |
|------|------|------|
| `adb shell '... &'` 启动后进程很快消失 | busybox 无 setsid/nohup，adb 退出 SIGHUP 杀进程 | **从串口 `/dev/ttyUSB0` 启动**（见 §7.1） |
| 日志报 `vencInit fail`/`ion alloc fail` | 反复启动泄漏 ION 内存，编码器起不来 | `reboot` 清干净后单次启动；`killall` 后不要立即重启同一程序 |
| RTSP server 起来但拉不到流 | conf 里 `main_rtsp_id = -1`（默认禁用） | 改成 `0` 启用主码流 RTSP |
| 日志报 `rtsp_net_type` 网卡找不到 | wlan0 未连上 | 确认 Wi-Fi 已连，`ip addr show wlan0` 有 IP |
| IMX6ULL 拉不到流 | V821 IP 变了（wlan0 DHCP 动态） | 重新查 V821 IP，更新 IMX6ULL `gateway.conf` 的 `rtsp_url` |
| `cannot find -lxxx` | 某 `.a` 不存在（功能组件未在 menuconfig 选上） | Makefile 的 `mpp_lib` 里去掉对应 `-lxxx`；或去 SDK staging `grep -rn "符号"` 找 |
| `undefined reference xxx` | 某符号所在 `.a` 未拷入 | 去 staging `grep -rn "xxx"` 找到 `.a`，拷入 `aw_pack_src/lib_aw/lib/eyesee-mpp` |
| 头文件 `No such file` | staging 顶层平铺头或内核 UAPI 头未拷全 | 按 §9 更新方法补全 |
| 串口无回显 | 串口空闲无输出 | 发 `\r\n` 触发 shell 回显；提示符 `root@(none):/mnt/extsd#` |

---

## 9. 用新一次 SDK 编译产物更新本工程

SDK 重新 `make && pack` 后 `staging_dir` 更新，按以下步骤刷新 bundle：

1. **库**：用 `out/v821/avaota_f1/openwrt/staging_dir/target/usr/lib/*.a`、`.../usr/lib/eyesee-mpp/*.a`、`.../usr/lib/pdet/*.a` 覆盖 `aw_pack_src/lib_aw/lib/eyesee-mpp/`
2. **头**：用 staging 的 `usr/include/{eyesee-mpp,libisp,libcedarc,pdet,*.h}` 及 `out/v821/kernel/build/user_headers/include` 覆盖 `aw_pack_src/lib_aw/include/` 对应目录
3. **动态库**：用 staging `usr/lib/{liblog.so,libasound.so.2*,libglog.so*,libunwind.so*,libz.so*}` 覆盖 `share_lib/`
4. `./build.sh` 重新编译

---

## 10. 关键技术说明（外编方法）

本工程的核心是**脱离 Tina SDK 单独编译** sample，关键技术点：

- **工具链选 musl 而非 glibc**：V821 用户态是 musl（interpreter `ld-musl-riscv32.so.1`）。`out/toolchain/nds32le-linux-glibc-v5d` 仅用于内核；用户态 sample 用 `prebuilt/rootfsbuilt/riscv/nds32le-linux-musl-v5d`。staging 的 `.a` 也是 musl 编译，必须配 musl 工具链。
- **静态库不用 `--whole-archive`**：SDK `tina.mk` 的 `LOCAL_BIN_LDFLAGS` 只用 `--start-group`。whole-archive 会让 pdet 的 `libnn_p.a`/`libnn_qt_dq_p.a` 触发 `multiple definition`。
- **头文件路径排除**：递归 `-I include/` 但排除 `eyesee-mpp/external/*`（jsoncpp 遮蔽 musl `<features.h>`）和 `uapi*`（内核头遮蔽 musl `<signal.h>/<time.h>`）。UAPI 只按 SDK tina.mk 加 3 路径。
- **liblog 传递依赖**：`liblog.so` → `libglog.so` → `libunwind.so` → `libz.so`，全放 `share_lib` 供链接期 `-rpath-link` 解析；设备 rootfs 都已存在。

---

## 11. 与 IMX6ULL 端的关系

本工程推流供 IMX6ULL 边侧网关拉流，网关工程见 [`../edgefusion_gateway/README.md`](../edgefusion_gateway/README.md)。完整端-边-云架构见 [`../PROJECT_INTRO.md`](../PROJECT_INTRO.md)。

**典型联调流程**：串口启动本 sample 推流（`rtsp://<V821-IP>:8554/ch0`）→ 确认 V821 IP → 在 IMX6ULL `gateway.conf` 填入该地址 → 启动网关 → Web 验证预览/录像/AI 分析。
