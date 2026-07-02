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
#ifndef ALIB_LOG_H
#define ALIB_LOG_H

/*
* Register Ur Platform
* __OS_LINUX
* __OS_ANDROID
* __OS_DS5
* __OS_UCOS
* __OS_WIN
* Coming here...
*/

#define VERSION "ver : 2017-8-4-18-00"
#ifndef LOG_TAG
#define LOG_TAG "AllwinnerAlibs"
#endif

#define ALIB_DEBUG 0

#ifdef __OS_ANDROID//Use android sdk
    #include <utils/Log.h>
    #ifdef  LOG_NDEBUG
    #undef  LOG_NDEBUG
    #define LOG_NDEBUG 0
    #endif
    #define alib_loge(fmt, arg...) ALOGE("%s "fmt"" ,VERSION, ##arg)
    #define alib_logd(fmt, arg...) if(ALIB_DEBUG > 0) ALOGD("%s "fmt"" ,VERSION, ##arg)
    #define alib_logw(fmt, arg...) if(ALIB_DEBUG > 1) ALOGD("%s "fmt"" ,VERSION, ##arg)
    #define alib_logv(fmt, arg...) if(ALIB_DEBUG > 2) ALOGD("%s "fmt"" ,VERSION, ##arg)
#elif (defined __OS_LINUX) || (defined __OS_DS5) || (defined __OS_WIN)

    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>

    #if !defined(__ANDROID__) || (__ANDROID__ == 0)
        #define alib_loge(fmt, arg...) printf("(%s),line(%d) : "fmt"\n", LOG_TAG, __LINE__, ##arg)
        #define alib_logd(fmt, arg...) if(ALIB_DEBUG > 0) printf("(%s),line(%d) : "fmt"\n", LOG_TAG, __LINE__, ##arg)
        #define alib_logw(fmt, arg...) if(ALIB_DEBUG > 1) printf("(%s),line(%d) : "fmt"\n", LOG_TAG, __LINE__, ##arg)
        #define alib_logv(fmt, arg...) if(ALIB_DEBUG > 2) printf("(%s),line(%d) : "fmt"\n", LOG_TAG, __LINE__, ##arg)

    #else// Use android ndk...
        #include <android/log.h>
        #define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
        #define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

        #define alib_loge(fmt, arg...) ALOGE("%s "fmt"" ,VERSION, ##arg)
        #define alib_logd(fmt, arg...) if(ALIB_DEBUG > 0) ALOGD("%s "fmt"" ,VERSION, ##arg)
        #define alib_logw(fmt, arg...) if(ALIB_DEBUG > 1) ALOGD("%s "fmt"" ,VERSION, ##arg)
        #define alib_logv(fmt, arg...) if(ALIB_DEBUG > 2) ALOGD("%s "fmt"" ,VERSION, ##arg)
    #endif

#elif (defined __OS_UCOS)
    #define alib_loge(fmt, arg...) eLIBs_printf("(%s),line(%d) : "fmt"\n", LOG_TAG, __LINE__, ##arg)
    #define alib_logd(fmt, arg...) if(ALIB_DEBUG > 0) eLIBs_printf("(%s),line(%d) : "fmt"\n", LOG_TAG, __LINE__, ##arg)
    #define alib_logw(fmt, arg...) if(ALIB_DEBUG > 1) eLIBs_printf("(%s),line(%d) : "fmt"\n", LOG_TAG, __LINE__, ##arg)
    #define alib_logv(fmt, arg...) if(ALIB_DEBUG > 2) eLIBs_printf("(%s),line(%d) : "fmt"\n", LOG_TAG, __LINE__, ##arg)
#else
    #error "No specific platform..."
#endif

#endif
