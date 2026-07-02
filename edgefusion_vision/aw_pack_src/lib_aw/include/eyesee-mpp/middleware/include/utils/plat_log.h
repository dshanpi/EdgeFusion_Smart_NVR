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

#ifndef __IPCLINUX_LOG_H__
#define __IPCLINUX_LOG_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#define OPTION_LOG_STYLE_PRINTF     0
#define OPTION_LOG_STYLE_GLOG       1

#ifdef USE_STD_LOG
#define CONFIG_LOG_STYLE OPTION_LOG_STYLE_PRINTF
#else
#define CONFIG_LOG_STYLE OPTION_LOG_STYLE_GLOG
#endif

// option for debug level.
#define OPTION_LOG_LEVEL_CLOSE      0
#define OPTION_LOG_LEVEL_ERROR      1
#define OPTION_LOG_LEVEL_WARN       2
#define OPTION_LOG_LEVEL_DEBUG      3
#define OPTION_LOG_LEVEL_VERBOSE    4

#define COLOR_LOG                   1
#define FORMAT_LOG                  1

#ifndef CONFIG_LOG_LEVEL
  #ifdef LOG_NDEBUG
    #if(LOG_NDEBUG == 0)
      #define CONFIG_LOG_LEVEL    OPTION_LOG_LEVEL_VERBOSE
    #endif
  #endif
#endif

#ifndef CONFIG_LOG_LEVEL
#define CONFIG_LOG_LEVEL    OPTION_LOG_LEVEL_DEBUG
#endif

#ifndef LOG_TAG
#define LOG_TAG ""
#endif

#define StripFileName(s) (strrchr(s, '/')?(strrchr(s, '/')+1):s)

extern int MPP_GLOBAL_LOG_LEVEL; //OPTION_LOG_LEVEL_DEBUG

#if (CONFIG_LOG_STYLE == OPTION_LOG_STYLE_GLOG)
    #include <glogCWrapper/log.h>

    #define PLAT_LOG_LEVEL_INFO   _GLOG_INFO
    #define PLAT_LOG_LEVEL_WARN   _GLOG_WARN
    #define PLAT_LOG_LEVEL_ERROR  _GLOG_ERROR
    #define PLAT_LOG_LEVEL_FATAL  _GLOG_FATAL

#define PLATLOG(mppLogLevel, level, fmt, arg...)  \
        do { \
            if (mppLogLevel <= MPP_GLOBAL_LOG_LEVEL) \
            { \
                GLOG_PRINT(level, fmt, ##arg); \
            } \
        } while (0)

#if CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_CLOSE

#define aloge(fmt, arg...)
#define alogw(fmt, arg...)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_ERROR

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, fmt, ##arg)
#define alogw(fmt, arg...)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_WARN

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, fmt, ##arg)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_DEBUG

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, fmt, ##arg)
#define alogd(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_DEBUG, PLAT_LOG_LEVEL_INFO, fmt, ##arg)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_VERBOSE

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, fmt, ##arg)
#define alogd(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_DEBUG, PLAT_LOG_LEVEL_INFO, fmt, ##arg)
#define alogv(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_VERBOSE, PLAT_LOG_LEVEL_INFO, fmt, ##arg)

#else
    #error "invalid configuration of debug level."
#endif

#elif (CONFIG_LOG_STYLE == OPTION_LOG_STYLE_PRINTF)
    #include <stdio.h>
    #include <string.h>
    #include <log/std_log.h>

    #define PLAT_LOG_LEVEL_ERROR     FMT_COLOR_BG(BLACK, RED) " E " FMT_DEFAULT
    #define PLAT_LOG_LEVEL_WARN      FMT_COLOR_BG(BLACK, YELLOW) " W " FMT_DEFAULT
    #define PLAT_LOG_LEVEL_DEBUG     FMT_COLOR_BG(BLACK, BLUE) " D " FMT_DEFAULT
    #define PLAT_LOG_LEVEL_VERBOSE   FMT_COLOR_BG(BLACK, WRITE) " V " FMT_DEFAULT

    #define PLATLOG(mppLogLevel, level, fmt, arg...) \
        do { \
            if(mppLogLevel <= MPP_GLOBAL_LOG_LEVEL) \
            { \
                printf("\t%s %s " fmt "\n", StripFileName(__FILE__), XPOSTO(50) level, __FUNCTION__, __LINE__, ##arg); \
            } \
        } while(0)

#if CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_CLOSE

#define aloge(fmt, arg...)
#define alogw(fmt, arg...)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_ERROR

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, FMT_COLOR_FG(LIGHT, RED) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogw(fmt, arg...)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_WARN

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, FMT_COLOR_FG(LIGHT, RED) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, FMT_COLOR_FG(LIGHT, YELLOW) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_DEBUG

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, FMT_COLOR_FG(LIGHT, RED) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, FMT_COLOR_FG(LIGHT, YELLOW) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogd(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_DEBUG, PLAT_LOG_LEVEL_DEBUG, FMT_COLOR_FG(LIGHT, BLUE) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_VERBOSE

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, FMT_COLOR_FG(LIGHT, RED) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, FMT_COLOR_FG(LIGHT, YELLOW) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogd(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_DEBUG, PLAT_LOG_LEVEL_DEBUG, FMT_COLOR_FG(LIGHT, BLUE) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogv(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_VERBOSE, PLAT_LOG_LEVEL_VERBOSE, FMT_COLOR_FG(LIGHT, WRITE) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)

#else
    #error "invalid configuration of debug level."
#endif

#else
    #error "invalid configuration of os."
#endif

#ifdef __cplusplus
}
#endif

#endif

