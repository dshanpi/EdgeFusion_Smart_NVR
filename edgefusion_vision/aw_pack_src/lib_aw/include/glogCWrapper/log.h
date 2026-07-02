/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <unistd.h>

#include "log_print.h"

// --------------------------------------------------------------------
#if 0
/* color */
#define LIGHT   "1"
#define DARK    "0"

#define FG      "3"
#define BG      "4"

#define BLACK   "0"
#define RED     "1"
#define GREEN   "2"
#define YELLOW  "3"
#define BLUE    "4"
#define PURPLE  "5"
#define CYAN    "6"
#define WRITE   "7"

#define FG_COLOR(color)    FG color
#define BG_COLOR(color)    BG color

#if COLOR_LOG
#define FMT_DEFAULT                 "\033[0m"
#define FMT_COLOR_FG(light, color)  "\033[" light ";" FG_COLOR(color) "m"
#define FMT_COLOR_BG(fg, color)     "\033[" BG_COLOR(color) ";" FG_COLOR(fg) "m"
#else
#define FMT_DEFAULT
#define FMT_COLOR_FG(light, color)
#define FMT_COLOR_BG(fg, color)
#endif

/* position */
#if FORMAT_LOG
#define XPOSTO(x)   "\033[" #x "D\033[" #x "C"
#else
#define XPOSTO(x)
#endif
#endif

#define GLOG_PRINT(level, fmt, arg...) \
do { \
    log_printf(__FILE__, __func__, __LINE__, level, fmt, ##arg); \
} while (0)

#define ALOGE(fmt, arg...) GLOG_PRINT(_GLOG_ERROR, fmt, ##arg)
#define ALOGD(fmt, arg...) GLOG_PRINT(_GLOG_INFO, fmt, ##arg)
#define ALOGV(fmt, arg...) GLOG_PRINT(_GLOG_INFO, fmt, ##arg)
#define ALOGI(fmt, arg...) GLOG_PRINT(_GLOG_INFO, fmt, ##arg)
#define ALOGW(fmt, arg...) GLOG_PRINT(_GLOG_WARN, fmt, ##arg)

#endif
