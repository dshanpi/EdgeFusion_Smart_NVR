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
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include <plat_type.h>
#include <utils/plat_log.h>

#include "motion_detect_common.h"

int motion_detect_init(MotionDetectContext *pMotionDetectContext)
{
    int ret = -1;

    if (!pMotionDetectContext)
    {
        aloge("pMotionDetectContext is null, motion_detect_init fail");
        return -1;
    }

    pMotionDetectContext->mHandle = MotionDetectCreat(&pMotionDetectContext->mMdStaCfg);
    if (!pMotionDetectContext->mHandle)
    {
        aloge("MotionDetect mHandle creat failed!");
        return -2;
    }

    ret = MotionDetectInit(pMotionDetectContext->mHandle, &pMotionDetectContext->mMdDynCfg);
    if (ret < 0)
    {
        aloge("MotionDetect init failed!");
        return -3;
    }

    ret = MotionDetectSetParam(pMotionDetectContext->mHandle, &pMotionDetectContext->mMdDynCfg);
    if (ret < 0)
    {
        aloge("MotionDetect set param failed!");
        return -4;
    }

    return 0;
}

int motion_detect_run(MotionDetectContext *pMotionDetectContext, unsigned char *pVir)
{
    int ret = -1;

    if (!pVir)
    {
        aloge("pVir is null");
        return -1;
    }

    if (!pMotionDetectContext->mHandle)
    {
        aloge("pMotionDetectContext->mHandle is null");
        return -1;
    }

    ret = MotionDetectProcessOneFrame(pMotionDetectContext->mHandle, pVir);
    if (ret != 0)
    {
        aloge("MotionDetect ProcessOneFrame failed!");
    }

    return ret;
}


int motion_detect_get_result(MotionDetectContext *pMotionDetectContext, MotionDetectRectInfo *pLastData)
{
    MOTION_DETECT_DYNAMIC_CFG_S *pDynCfg = &pMotionDetectContext->mMdDynCfg;
    MOTION_DETECT_RESULT_S *pResult =      &pMotionDetectContext->mResult;
    MOTION_DETECT_REGION_S *pRegion =      NULL;
    int nLastHorRegNum =                   pResult->nHorRegionNum;
    int nLastVerRegNum =                   pResult->nVerRegionNum;
    int nLastTotalRegNum =                 nLastHorRegNum * nLastVerRegNum;
    int nCurrTotalRegNum =                 pDynCfg->nHorRegionNum * pDynCfg->nVerRegionNum;
    int x = 0, y = 0, idx = 0;

    if ( !pMotionDetectContext || !pLastData)
    {
        aloge("pMotionDetectContext or pLastData is null");
        return -1;
    }

    if (nLastTotalRegNum < nCurrTotalRegNum || !pResult->mRegion)
    {
        if (pResult->mRegion)
        {
            free(pResult->mRegion);
            pResult->mRegion = NULL;
        }
        pResult->mRegion = (MOTION_DETECT_REGION_S *)malloc(nCurrTotalRegNum * sizeof(MOTION_DETECT_REGION_S));
        if (!pResult->mRegion)
        {
            aloge("pResult->mRegion malloc failed!");
            return -1;
        }
    }

    unsigned int motion_result_cnt = 0;
    MotionDetectGetResult(pMotionDetectContext->mHandle, pResult);
    pRegion = pResult->mRegion;

    for (idx = 0, y = 0; y < pResult->nVerRegionNum; y++)
    {
        for (x = 0; x < pResult->nHorRegionNum; x++)
        {
            if(pRegion[idx].bIsMotion)
            {
                pLastData->rect[idx].left   = pRegion[idx].nPixXBgn;
                pLastData->rect[idx].top    = pRegion[idx].nPixYBgn;
                pLastData->rect[idx].right  = pRegion[idx].nPixXEnd;
                pLastData->rect[idx].bottom = pRegion[idx].nPixYEnd;
                pLastData->rect[idx].bIsMotion = 1;
                motion_result_cnt++;
            }
            else
            {
                pLastData->rect[idx].bIsMotion = 0;
            }
            idx++;
        }
    }

    pLastData->num = pResult->nMotionRegionNum;
    pMotionDetectContext->mMotionRatio = (float)pResult->nMotionRegionNum / (pResult->nVerRegionNum* pResult->nHorRegionNum);

    if (pResult->nMotionRegionNum > 0)
    {
        alogd("motion: %d, ratio: %f", pResult->nMotionRegionNum, \
            (float)pResult->nMotionRegionNum / (pResult->nVerRegionNum* pResult->nHorRegionNum));
    }

    return 0;
}

void motion_detect_uninit(MotionDetectContext *pMotionDetectContext)
{
    if (!pMotionDetectContext->mHandle)
    {
        aloge("pMotionDetectContext->mHandle is null");
        return;
    }

    MotionDetectDestroy(pMotionDetectContext->mHandle);
}
