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
#ifndef __MOTION_DETECT_COMMON_H__
#define __MOTION_DETECT_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "motion_detect.h"

#define MAX_VE_MOTION_NUM           (1200)

typedef struct {
	int left;
	int top;
	int right;
	int bottom;
    int bIsMotion;
} MotionRect;

typedef struct {
	int num;
	MotionRect rect[MAX_VE_MOTION_NUM];
} MotionDetectRectInfo;

typedef struct MotionDetectContext
{
    void *mHandle;
	MOTION_DETECT_RESULT_S mResult;
    MOTION_DETECT_STATIC_CFG_S mMdStaCfg;
    MOTION_DETECT_DYNAMIC_CFG_S mMdDynCfg;
    MotionDetectRectInfo mMotionLastData;

    unsigned int mMotionDetBufSize;
    unsigned int mMotionDetPhyAddr;
    void* mMotionDetVirtAddr;

    int mMotionPdetEnable;
    int mJudgeMotionNum;
    float mMotionRatio;
    float mMotionPdetThres;
    float mMotionDynamicThres;
} MotionDetectContext;

int motion_detect_init(MotionDetectContext *pMotionDetectContext);
int motion_detect_run(MotionDetectContext *pMotionDetectContext, unsigned char *pVir);
int motion_detect_get_result(MotionDetectContext *pMotionDetectContext, MotionDetectRectInfo *pLastData);
void motion_detect_uninit(MotionDetectContext *pMotionDetectContext);

#ifdef __cplusplus
}
#endif

#endif // __MOTION_DETECT_COMMON_H__