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
#ifndef STD_LOG_H
#define STD_LOG_H

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>

#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------
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

#define PRINTF_LOG_ERROR   FMT_COLOR_BG(BLACK, RED) " E " FMT_DEFAULT
#define PRINTF_LOG_WARN    FMT_COLOR_BG(BLACK, YELLOW) " W " FMT_DEFAULT
#define PRINTF_LOG_DEBUG   FMT_COLOR_BG(BLACK, BLUE) " D " FMT_DEFAULT
#define PRINTF_LOG_INFO    FMT_COLOR_BG(BLACK, GREEN) " I " FMT_DEFAULT
#define PRINTF_LOG_VERBOSE FMT_COLOR_BG(BLACK, WRITE) " V " FMT_DEFAULT

// ---------------------------------------------------------------------

/*
 * Normally we strip ALOGV (VERBOSE messages) from release builds.
 * You can modify this (for example with "#define LOG_NDEBUG 0"
 * at the top of your source file) to change that behavior.
 */
#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif

/*
 * This is the local tag used for the following simplified
 * logging macros.  You can change this preprocessor definition
 * before using the other macros to change the tag.
 */
#ifndef LOG_TAG
#define LOG_TAG "Unknown"
#endif


// ---------------------------------------------------------------------

/*
 * Simplified macro to send a verbose log message using the current LOG_TAG.
 */
#ifndef ALOGV
#if LOG_NDEBUG
#define ALOGV(...)   ((void)0)
#else
#define ALOGV(...) ALOG(LOG_VERBOSE, LOG_TAG, FMT_COLOR_FG(LIGHT, WRITE) "<%s:%d> " XPOSTO(60) FMT_DEFAULT, __VA_ARGS__)
#endif
#endif

/*
 * Simplified macro to send a debug log message using the current LOG_TAG.
 */
#ifndef ALOGD
#define ALOGD(...) ALOG(LOG_DEBUG, LOG_TAG, FMT_COLOR_FG(LIGHT, BLUE) "<%s:%d> " XPOSTO(60) FMT_DEFAULT, __VA_ARGS__)
#endif

/*
 * Simplified macro to send an info log message using the current LOG_TAG.
 */
#ifndef ALOGI
#define ALOGI(...) ALOG(LOG_INFO, LOG_TAG, FMT_COLOR_FG(LIGHT, GREEN) "<%s:%d> " XPOSTO(60) FMT_DEFAULT, __VA_ARGS__)
#endif

/*
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef ALOGW
#define ALOGW(...) ALOG(LOG_WARN, LOG_TAG, FMT_COLOR_FG(LIGHT, YELLOW) "<%s:%d> " XPOSTO(60) FMT_DEFAULT, __VA_ARGS__)
#endif
/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef ALOGE
#define ALOGE(...) ALOG(LOG_ERROR, LOG_TAG, FMT_COLOR_FG(LIGHT, RED) "<%s:%d> " XPOSTO(60) FMT_DEFAULT, __VA_ARGS__)
#endif

// ---------------------------------------------------------------------
/*
 * Basic log message macro.
 *
 * Example:
 *  ALOG(LOG_WARN, NULL, "Failed with error %d", errno);
 *
 * The second argument may be NULL or "" to indicate the "global" tag.
 */
#ifndef ALOG
#define ALOG(priority, tag, func, ...) \
do { \
    LOG_PRI(PRINTF_##priority, tag); \
    printf(func, __FUNCTION__, __LINE__); \
    printf(__VA_ARGS__); \
    printf("\n"); \
} while(0)
#endif

/*
 * Log macro that allows you to specify a number for the priority.
 */
#ifndef LOG_PRI
#define LOG_PRI(priority, tag) \
    printf("%s %s ", tag, \
            XPOSTO(30) priority)
#endif

#ifdef __cplusplus
}
#endif

#endif // STD_LOG_H
