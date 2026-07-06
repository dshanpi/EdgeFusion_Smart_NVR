# =============================================================================
# V821 (AW_V821, RISC-V) eyesee-mpp sample 外编 Makefile
# 参考 edgefusion_vision (V853) 外编工程改写，用于脱离 Tina SDK 单独编译
#   sample_smartIPC_demo。
# 工具链: riscv32-linux-gcc/g++ (glibc, nds32le-linux-glibc-v5d)
# MPP 库/头文件: aw_pack_src/lib_aw/{lib,include}  (从 SDK staging_dir 拷贝)
# 运行期动态库:   share_lib  (liblog.so, libasound.so.2，设备 rootfs 也存在)
# =============================================================================

#----------------------- 目录变量 (需先定义, 供后续 := 立即求值使用) -----
ROOT_DIR          := $(abspath .)
OUTPUT_DIR        := $(ROOT_DIR)/output
OUTPUT_DIR_OBJS   := $(OUTPUT_DIR)/objs

#----------------------- 工具链 -----------------------
TOOLCHAIN_DIR := $(ROOT_DIR)/toolchain
COMPILE_PREX  := $(TOOLCHAIN_DIR)/bin/riscv32-linux-musl-
CC            := $(COMPILE_PREX)gcc
CXX           := $(COMPILE_PREX)g++
AR            := $(COMPILE_PREX)ar
STRIP         := $(COMPILE_PREX)strip
NM            := $(COMPILE_PREX)nm

#----------------------- 芯片宏 -----------------------
# AW_V821 == 0x1882 (见 middleware/sun300iw1/config/mpp_config.mk)
AWCHIP_CFLAGS := -DAWCHIP=0x1882#AW_V821

# 与 SDK tina.mk 中 CEDARX_EXT_CFLAGS 对齐的关键编译宏
# (sample 源码主要受 AWCHIP 与 SOFTWARE_TDM_FOR_ISP 影响，此处 TDM 未使能)
CEDARX_EXT_CFLAGS := \
	-DTINA_PLATFORM \
	-DUSE_STD_LOG \
	-DCONFIG_LOG_LEVEL=3 \
	-DCDXCFG_FILE_SYSTEM=0 \
	-DCDXCFG_HW_DISPLAY=0 \
	-DMPPCFG_SUPPORT_FASTBOOT=0 \
	-DMPPCFG_COMPILE_STATIC_LIB \
	-DMPPCFG_VI=1 \
	-DMPPCFG_VENC=1 \
	-DMPPCFG_AIO=1 \
	-DMPPCFG_AENC=1 \
	-DMPPCFG_ADEC=1 \
	-DMPPCFG_MUXER=1 \
	-DMPPCFG_DEMUXER=1 \
	-DMPPCFG_SAMPLE_CONFIGFILEPARSER=1

#----------------------- 目录变量 -----------------------
USER_SRC_BASE_DIR := $(ROOT_DIR)/sample
MPP_LIB_DIR       := $(ROOT_DIR)/aw_pack_src/lib_aw/lib/eyesee-mpp
MPP_INC_BASE_DIR  := $(ROOT_DIR)/aw_pack_src/lib_aw/include
SHARED_LIB_DIR    := $(ROOT_DIR)/share_lib

#----------------------- 编译参数 -----------------------
# RISC-V 架构由工具链默认值给出 (rv32imafdcxandes / ilp32d / andes-25-series)，
# 无需显式 -march/-mabi。-mstrict-align 与 SDK 一致。
CCFLAGS := -g -Os -pipe -fPIC -mstrict-align -fno-caller-saves \
           -Wall -Wno-unused-but-set-variable -Wno-unused-variable \
           -Wno-unused-label -Wno-unused-function

DEFINES := -Wformat -Werror=format-security -fstack-protector \
           -Wl,-z,now -Wl,-z,relro -Wl,-z,stack-size=1048576

# 递归包含所有头文件目录 (与参考工程一致，最稳)
# 排除:
#  1) eyesee-mpp/external 下的 C++ 第三方库目录 (jsoncpp 的 json/features.h 会遮蔽 musl <features.h>)
#  2) uapi (内核用户态头) 整棵子树 —— 不能把 uapi/linux, uapi/asm-generic 等作为平铺 -I,
#     否则会遮蔽 musl 的 <signal.h>/<time.h> 导致 struct timeval/sigset_t 重定义。
#     uapi 仅按 SDK tina.mk 的方式加 3 个特定路径 (见 LINUX_UAPI_INCS)。
USER_INC_DIRS := $(shell find $(MPP_INC_BASE_DIR) -type d \
                    -not -path "*/eyesee-mpp/external/*" \
                    -not -path "*/uapi*") \
                 $(shell find $(USER_SRC_BASE_DIR) -type d)

# 内核 UAPI 头 (对应 SDK tina.mk 的 LINUX_USER_HEADERS 三条路径)
LINUX_UAPI_INCS := \
	-I$(MPP_INC_BASE_DIR)/uapi \
	-I$(MPP_INC_BASE_DIR)/uapi/bsp \
	-I$(MPP_INC_BASE_DIR)/uapi/bsp/linux

USER_INCS := $(addprefix -I ,$(USER_INC_DIRS)) $(LINUX_UAPI_INCS)

CFLAGS  := $(CCFLAGS) $(DEFINES) $(USER_INCS) $(AWCHIP_CFLAGS) $(CEDARX_EXT_CFLAGS)
CXXFLAGS:= $(CCFLAGS) $(DEFINES) $(USER_INCS) $(AWCHIP_CFLAGS) $(CEDARX_EXT_CFLAGS)

#----------------------- sample_rtsp 源文件 -----------------------
# 与 SDK tina.mk 中 sample_rtsp 的 SRCCS 一致：仅 3 个文件，纯 RTSP 推流，无录像/检测。
APP_SRCS := \
	sample_rtsp/sample_rtsp.c \
	common/rtsp_server.cpp \
	common/sample_common_venc.c

USER_SRCS := $(addprefix $(USER_SRC_BASE_DIR)/,$(APP_SRCS))

# obj 输出路径 (源路径映射到 output/objs 下)
USER_OBJS_OUT := $(patsubst $(ROOT_DIR)/%,$(OUTPUT_DIR_OBJS)/%,$(USER_SRCS:%=%.o))

#----------------------- 链接库 -----------------------
# 自动遍历 mpp 库目录所有 .a，whole-archive + start-group 解决依赖顺序
lib_files := $(wildcard $(MPP_LIB_DIR)/*.a)
lib_name  := $(patsubst lib%,%,$(patsubst %.a,%,$(notdir $(lib_files))))
ld_mpp_libs := $(addprefix -l,$(lib_name))

LINKFLAGS := \
	-L$(MPP_LIB_DIR) -L$(SHARED_LIB_DIR) \
	-Wl,-rpath-link,$(SHARED_LIB_DIR):$(MPP_LIB_DIR) \
	-Wl,-rpath,/usr/lib \
	-lpthread -lrt -ldl -lm -Wl,--gc-sections

# 静态链接 MPP 库；动态链接 liblog / libasound (与设备 rootfs 一致)
# 注意: 不使用 --whole-archive (与 SDK tina.mk LOCAL_BIN_LDFLAGS 一致, 仅 --start-group),
#       否则 pdet 库 libnn_p / libnn_qt_dq_p 都含 get_version_libnn 会 multiple definition,
#       且会把未引用对象全部塞进导致体积膨胀。
mpp_lib := \
	-Wl,-Bstatic -Wl,--start-group \
	$(ld_mpp_libs) \
	-Wl,--end-group \
	-Wl,-Bdynamic \
	-llog -lasound

#----------------------- 编译规则 -----------------------
APP_BIN_NAME := sample_rtsp

build_app: $(USER_OBJS_OUT)
	@mkdir -p $(OUTPUT_DIR)
	$(CXX) $(USER_OBJS_OUT) $(CFLAGS) $(LINKFLAGS) $(mpp_lib) -o $(OUTPUT_DIR)/$(APP_BIN_NAME)
	@echo "==== Build APP Finish: $(OUTPUT_DIR)/$(APP_BIN_NAME) ===="
	@cp $(OUTPUT_DIR)/$(APP_BIN_NAME) $(OUTPUT_DIR)/$(APP_BIN_NAME)_strip
	$(STRIP) $(OUTPUT_DIR)/$(APP_BIN_NAME)_strip

# 模式规则：把源文件编译到 output/objs 对应路径
$(OUTPUT_DIR_OBJS)/%.c.o: $(ROOT_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MD -MP -c -o $@ $<

$(OUTPUT_DIR_OBJS)/%.cpp.o: $(ROOT_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MD -MP -c -o $@ $<

-include $(USER_OBJS_OUT:.o=.d)

.PHONY: clean
clean:
	rm -rf $(OUTPUT_DIR)
