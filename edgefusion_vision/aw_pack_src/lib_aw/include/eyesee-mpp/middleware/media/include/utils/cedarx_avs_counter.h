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
#ifndef CEDARX_AVS_COUNTER_H
#define CEDARX_AVS_COUNTER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//#include <CDX_Types.h>
#include <pthread.h>

typedef enum
{
    AvsCounter_StateInvalid = 0,
    AvsCounter_StateIdle,
    AvsCounter_StateExecuting,
    AvsCounter_StatePause,
}CedarxAvscounterState;

typedef struct CedarxAvscounterContext {
	long long sample_time;  //unit:us, custom system time, when the current time period begins. when adjust_ratio change, sample_time will be set again.
	long long base_time;    //unit:us, cedarx clock time, when the current time period begins. when adjust_ratio change, base_time will be set again.
	int adjust_ratio;   //!vps: +40~-100, +:slow down, -:speed up.
	long long system_base_time; //unit:us, android system time, when init or reset().
	long long mSystemPauseBaseTime; //unit:us, android system time when pause happen.
	long long mSystemPauseDuration; //unit:us, android system time pause duration.

    CedarxAvscounterState mStatus;

	pthread_mutex_t mutex;

	void (*reset)(struct CedarxAvscounterContext *context);
	void (*get_time)(struct CedarxAvscounterContext *context, long long *curr); //get cedarx clock time.
	void (*get_time_diff)(struct CedarxAvscounterContext *context, long long *diff);    //get (cedarx clock time duration) - (custom system clock time duration).
	void (*adjust)(struct CedarxAvscounterContext *context, int val);
    
    int (*start)(struct CedarxAvscounterContext *context);
    int (*pause)(struct CedarxAvscounterContext *context);
    
}CedarxAvscounterContext;

CedarxAvscounterContext *cedarx_avs_counter_request();
int cedarx_avs_counter_release(CedarxAvscounterContext *context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
