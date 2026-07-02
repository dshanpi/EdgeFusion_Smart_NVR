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
#ifndef __MOTION_PDET_H__
#define __MOTION_PDET_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <tsemaphore.h>

#include "de_hw_scale.h"
#include "aiservice_pdet.h"
#include "motion_detect_common.h"
#include "sample_common_venc.h"
#include "sample_common_osd.h"

#define MOTION_PDET_SENSITIVITY_TEST_MODE   0  // Ignore motion detect results and trigger pdet

#define MOTION_PDET_SENSITIVITY_LOW         1
#define MOTION_PDET_SENSITIVITY_MIDDLE      2
#define MOTION_PDET_SENSITIVITY_HIGH        3

typedef struct PdetOrlInfo {
    VI_DEV mVipp;
    VI_CHN mViChn;
    float mOrlWidthRatio;
    float mOrlHeightRatio;
} PdetOrlInfo;

typedef struct PdetOsdInfo {
    VENC_CHN mVEncChn;
    float mOsdWidthRatio;
    float mOsdHeightRatio;
    StreamOsdConfig mStreamOsdConfig;
} PdetOsdInfo;

typedef struct MotionPetParam {
    pthread_mutex_t mPdetLock;
    cdx_sem_t mSemPdetFrameReady;

    SampleDewbInfo *pDewbInfo;
    SamplePdetInfo *pPdetInfo;

    PdetOrlInfo mPdetOrlInfo;
    PdetOsdInfo mPdetOsdInfo;
    MotionDetectContext mMotionDetectContext;

    MotionDetectRectInfo mMotionLastData;

    float total_score[MAX_VE_MOTION_NUM];
} MotionPdetParam;

void motion_pdet_set_sensitivity(MotionPdetParam *pMotionPdetParam, int sensitivity);
int motion_pdet_init(MotionPdetParam *pMotionPdetParam);
int motion_pdet_prepare(MotionPdetParam *pMotionPdetParam, void* pVirAddr);
int motion_pdet_run(MotionPdetParam *pMotionPdetParam, MotionDetectRectInfo *pMotionLastData);
int motion_pdet_deinit(MotionPdetParam *pMotionPdetParam);

#ifdef __cplusplus
}
#endif

#endif // __MOTION_PDET_H__
