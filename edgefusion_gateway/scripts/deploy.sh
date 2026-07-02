#!/bin/bash
# 部署 gateway 二进制 + 配置到 IMX6ULL 板子
# 用法: make deploy  (内部调用)
#       BOARD_ADB_SERIAL=100ask_IMX6ULL bash scripts/deploy.sh build/edgefusion_gateway
set -e

BIN="${1:-build/edgefusion_gateway}"
SERIAL="${BOARD_ADB_SERIAL:-100ask_IMX6ULL}"
REMOTE_DIR="${BOARD_REMOTE_DIR:-/mnt/extsd/gateway}"

echo "==> 目标设备: $SERIAL"
echo "==> 远端目录: $REMOTE_DIR"

# 确保 TF 卡已挂载
adb -s "$SERIAL" shell 'mountpoint -q /mnt/extsd || mount -t vfat /dev/mmcblk0p1 /mnt/extsd 2>/dev/null || true' 2>&1
adb -s "$SERIAL" shell "mkdir -p $REMOTE_DIR/config $REMOTE_DIR/recordings" 2>&1

echo "==> 推送二进制 $BIN"
adb -s "$SERIAL" push "$BIN" "$REMOTE_DIR/edgefusion_gateway" 2>&1
adb -s "$SERIAL" shell "chmod +x $REMOTE_DIR/edgefusion_gateway" 2>&1

echo "==> 推送配置 config/gateway.conf"
adb -s "$SERIAL" push config/gateway.conf "$REMOTE_DIR/config/gateway.conf" 2>&1

echo "==> 推送 web/ 前端文件"
adb -s "$SERIAL" shell "mkdir -p $REMOTE_DIR/web" 2>&1
for f in web/index.html web/style.css web/app.js; do
    adb -s "$SERIAL" push "$f" "$REMOTE_DIR/web/" 2>&1
done

# 推送 CA 证书库（buildroot 板子默认无 CA bundle，cloud_ai 调 DeepSeek HTTPS 需要）
if [ -f /etc/ssl/certs/ca-certificates.crt ]; then
    adb -s "$SERIAL" shell 'mkdir -p /etc/ssl/certs' 2>&1
    echo "==> 推送 CA 证书库"
    adb -s "$SERIAL" push /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/ca-certificates.crt 2>&1
fi

echo "==> 部署完成"
echo "    运行: adb -s $SERIAL shell '$REMOTE_DIR/edgefusion_gateway -path $REMOTE_DIR/config/gateway.conf'"
