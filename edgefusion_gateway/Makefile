# EdgeFusion Gateway Makefile
# 先 `source toolchain.env` 再 `make`。

TARGET   := edgefusion_gateway
BUILD    := build
SRC_DIR  := src

# ---------- 工具链 ----------
CC       ?= arm-buildroot-linux-gnueabihf-gcc
CXX      ?= arm-buildroot-linux-gnueabihf-g++
STRIP    ?= arm-buildroot-linux-gnueabihf-strip
SYSROOT  ?= $(shell arm-buildroot-linux-gnueabihf-gcc -print-sysroot)

# ---------- 编译选项 ----------
CFLAGS  := -O2 -g -Wall -Wextra -std=gnu11 \
           -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
           --sysroot=$(SYSROOT) \
           -I$(SYSROOT)/usr/include \
           -I$(SRC_DIR)

# FFmpeg 在 4.x 后期默认不导出 libavutil 的某些宏，需此定义
CFLAGS  += -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS

LDFLAGS := --sysroot=$(SYSROOT) \
           -L$(SYSROOT)/usr/lib \
           -Wl,-rpath-link,$(SYSROOT)/usr/lib \
           -Wl,-rpath,/usr/lib

# ---------- 依赖库 ----------
# 顺序：被依赖者放后面
LIBS := -lavformat -lavcodec -lavutil -lswscale -lswresample \
        -lcurl -lssl -lcrypto \
        -lsqlite3 -ljson-c \
        -lpthread -lm -lz -ldl

# ---------- 源文件 ----------
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean strip deploy run-host

all: $(BUILD)/$(TARGET)

$(BUILD)/%.o: $(SRC_DIR)/%.c $(wildcard $(SRC_DIR)/*.h) | $(BUILD)
	@echo "  CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET): $(OBJS)
	@echo "  LD $@"
	@$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)

$(BUILD):
	@mkdir -p $@

strip: all
	@$(STRIP) $(BUILD)/$(TARGET)

# 推送到板子
deploy: strip
	BOARD_ADB_SERIAL=$(BOARD_ADB_SERIAL) bash scripts/deploy.sh $(BUILD)/$(TARGET)

# 主机侧语法/链接健全性（不运行，仅核对工具链是否就绪）
check-env:
	@echo "CC      = $(CC)"
	@echo "SYSROOT = $(SYSROOT)"
	@$(CC) --version | head -1
	@test -f $(SYSROOT)/usr/include/libavformat/avformat.h && echo "FFmpeg headers: OK" || (echo "FFmpeg headers: MISSING"; exit 1)
	@test -f $(SYSROOT)/usr/lib/libavformat.so && echo "FFmpeg libs: OK" || (echo "FFmpeg libs: MISSING"; exit 1)

clean:
	@rm -rf $(BUILD)
