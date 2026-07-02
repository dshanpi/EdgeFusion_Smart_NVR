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
#ifndef __TEXTENC_DEBUG_H__ 
#define __TEXTENC_DEBUG_H__

#ifndef LOG_TAG
#define LOG_TAG "TextEncApi"
#endif


#define __EYESEE_LINUX__

#ifdef __EYESEE_LINUX__
    #include <utils/plat_log.h>
    //#include <cutils/log.h>
    #define LOG_LEVEL_ERROR     _GLOG_ERROR
    #define LOG_LEVEL_WARNING   _GLOG_WARN
    #define LOG_LEVEL_INFO      _GLOG_INFO
    #define LOG_LEVEL_VERBOSE   _GLOG_INFO
    #define LOG_LEVEL_DEBUG     _GLOG_INFO

    #define AWLOG(level, fmt, arg...)  \
        GLOG_PRINT(level, fmt, ##arg)

#define CC_LOG_ASSERT(e, fmt, arg...)                               \
        LOG_ALWAYS_FATAL_IF(                                        \
                !(e),                                               \
                "<%s:%d>check (%s) failed:" fmt,                    \
                __FUNCTION__, __LINE__, #e, ##arg)                  \

#elif defined(__LINUX__)

#include <stdio.h>
#include <string.h>

#define LOG_LEVEL_ERROR     "error  "
#define LOG_LEVEL_WARNING   "warning"
#define LOG_LEVEL_INFO      "info   "
#define LOG_LEVEL_VERBOSE   "verbose"
#define LOG_LEVEL_DEBUG     "debug  "

#define AWLOG(level, fmt, arg...)  \
    printf("%s: %s <%s:%u>: "fmt"\n", level, LOG_TAG, __FUNCTION__, __LINE__, ##arg)

#define CC_LOG_ASSERT(e, fmt, arg...)                                       \
                do {                                                        \
                    if (!(e))                                               \
                    {                                                       \
                        loge("check (%s) failed:"fmt, #e, ##arg);           \
                        assert(0);                                          \
                    }                                                       \
                } while (0)

#endif




#define LOGD(fmt, arg...) AWLOG(LOG_LEVEL_DEBUG, "debug  " fmt, ##arg)


#if CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_CLOSE

#define LOGE(fmt, arg...)
#define LOGW(fmt, arg...)
#define LOGI(fmt, arg...)
#define LOGV(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_ERROR

#define LOGE(fmt, arg...) AWLOG(LOG_LEVEL_ERROR, "\033[40;31m" fmt "\033[0m", ##arg)
#define LOGW(fmt, arg...)
#define LOGI(fmt, arg...)
#define LOGV(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_WARN

#define LOGE(fmt, arg...) AWLOG(LOG_LEVEL_ERROR, "\033[40;31m" fmt "\033[0m", ##arg)
#define LOGW(fmt, arg...) AWLOG(LOG_LEVEL_WARNING, fmt, ##arg)
#define LOGI(fmt, arg...)
#define LOGV(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_DEBUG

#define LOGE(fmt, arg...) AWLOG(LOG_LEVEL_ERROR, "\033[40;31m" fmt "\033[0m", ##arg)
#define LOGW(fmt, arg...) AWLOG(LOG_LEVEL_WARNING, fmt, ##arg)
#define LOGI(fmt, arg...) AWLOG(LOG_LEVEL_INFO, fmt, ##arg)
#define LOGV(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_VERBOSE

#define LOGE(fmt, arg...) AWLOG(LOG_LEVEL_ERROR, "\033[40;31m" fmt "\033[0m", ##arg)
#define LOGW(fmt, arg...) AWLOG(LOG_LEVEL_WARNING, fmt, ##arg)
#define LOGI(fmt, arg...) AWLOG(LOG_LEVEL_INFO, fmt, ##arg)
#define LOGV(fmt, arg...) AWLOG(LOG_LEVEL_VERBOSE, fmt, ##arg)

#else

#error "invalid configuration of debug level."

#endif  // CONFIG_LOG_LEVEL



#endif
