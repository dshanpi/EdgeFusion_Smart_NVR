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
#ifndef LOG_WRAPPER_H_
#define LOG_WRAPPER_H_

#define COLOR_LOG                   1
#define FORMAT_LOG                  1

#ifdef USE_STD_LOG
#include "std_log.h"
#else
#include "glogCWrapper/log.h"
#endif

#ifndef NDEBUG
#define DB_ERROR
#define DB_WARN
#define DB_INFO
#define DB_DEBUG
#define DB_MSG
#else
#define DB_ERROR
#define DB_WARN
#endif

#ifdef DB_ERROR
#define LOGE(fmt, arg...) \
    do { \
        ALOGE(fmt, \
                ##arg); \
    } while(0)
#else
#define LOGE(fmt, arg...)
#endif

#ifdef DB_WARN
#define LOGW(fmt, arg...) \
    do { \
        ALOGW(fmt, \
                ##arg); \
    } while(0)
#else
#define LOGW(fmt, arg...)
#endif

#ifdef DB_INFO
#define LOGI(fmt, arg...) \
    do { \
        ALOGI(fmt, \
                ##arg); \
    } while(0)
#else
#define LOGI(fmt, arg...)
#endif

#ifdef DB_DEBUG
#define LOGD(fmt, arg...) \
    do { \
        ALOGD(fmt, \
                ##arg); \
    } while(0)
#else
#define LOGD(fmt, arg...)
#endif

#ifdef DB_MSG
#define LOGV(fmt, arg...) \
    do { \
        ALOGV(fmt, \
                ##arg); \
    } while(0)
#else
#define LOGV(fmt, arg...)
#endif

#endif

