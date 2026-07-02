#!/bin/sh
# =============================================================================
# V821 eyesee-mpp sample 外编脚本
# 参考 edgefusion_vision/build.sh (V853) 改写。
# 用法: ./build.sh
#   成功后在 output/ 下生成 sample_smartIPC_demo 与 strip 后的版本。
# =============================================================================
set -e

cd "$(dirname "$0")"
sdk_dir="$(pwd)"

APP_BIN_NAME=sample_smartIPC_demo

# 工具链前缀 (riscv32-linux-musl-gcc/g++, musl libc)
toolchain_dir="${sdk_dir}/toolchain/bin"
toolchain_name=riscv32-linux-musl-

echo "=== V821 MPP sample 外编开始 ==="
make -C "${sdk_dir}" \
	APP_BIN_NAME="${APP_BIN_NAME}" \
	COMPILE_PREX="${toolchain_dir}/${toolchain_name}" \
	build_app

echo ""
echo "=== 编译成功 ==="
echo "产物:"
ls -l "${sdk_dir}/output/${APP_BIN_NAME}" "${sdk_dir}/output/${APP_BIN_NAME}_strip" 2>/dev/null
echo ""
echo "动态库依赖 (应仅 liblog/libasound/libstdc++/libgcc/libc):"
${toolchain_dir}/${toolchain_name}readelf -d "${sdk_dir}/output/${APP_BIN_NAME}_strip" 2>/dev/null | grep NEEDED || true
