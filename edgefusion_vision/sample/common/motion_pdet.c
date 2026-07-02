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
#include "mpi_sys.h"
#include "plat_math.h"
#include "mm_comm_rc.h"
#include "mm_common.h"
#include "mpi_region.h"
#include "mm_comm_region.h"

#include "motion_detect_common.h"
#include "motion_pdet.h"

#define SUPPORT_MOTION_PDET
#define SUPPORT_PDET_OSD_TEST

static int check_cross_rect(MotionRect *ra, MotionRect *rb)
{
    int left = ra->left > rb->left ? ra->left : rb->left;
    int right = ra->right < rb->right ? ra->right : rb->right;
    int top = ra->top > rb->top ? ra->top : rb->top;
    int bottom = ra->bottom < rb->bottom ? ra->bottom : rb->bottom;
    int width = right > left ? right - left : 0;
    int height = bottom > top ? bottom - top : 0;
    return width * height;
}

static int rect_area(MotionRect *rect)
{
    int width = rect->right - rect->left;
    int height = rect->bottom - rect->top;
    if (width <= 0 || height <= 0) {
        return 0; // Invalid rectangle
    }
    return width * height;
}

static int check_union_rect(MotionRect *ra, MotionRect *rb)
{
    int area_a = rect_area(ra);
    int area_b = rect_area(rb);
    int intersection_area = check_cross_rect(ra, rb);

    // Union area = Area of A + Area of B - Intersection Area
    int union_area = area_a + area_b - intersection_area;

    return union_area;
}

static float calculate_iou(MotionRect *ra, MotionRect *rb)
{
    int intersection_area = check_cross_rect(ra, rb);
    int union_area = check_union_rect(ra, rb);

    if (union_area == 0)
    {
        return 0.0f; // Avoid division by zero
    }

    return (float)intersection_area / union_area;
}

static float score_pdet_rect_iou(MotionDetectContext *pMotionDetectContext, MotionRect *pdet_rect, MotionDetectRectInfo *data)
{
    if (!pMotionDetectContext || !pdet_rect || !data)
    {
        aloge("fatal error! param is null");
        return -1;
    }

    float score = 0, total_score = 0;
    int x = 0, y = 0, idx = 0;
    MOTION_DETECT_RESULT_S *pResult = &pMotionDetectContext->mResult;

    for (idx= 0, y = 0; y < pResult->nVerRegionNum; y++)
    {
        for (x = 0; x < pResult->nHorRegionNum; x++)
        {
            if(data->rect[idx].bIsMotion == 0)
            {
                idx++;
                continue;
            }

            score = calculate_iou(pdet_rect, &data->rect[idx]);
            total_score += score;
            alogv("score[%d]: %f, pdet[%d, %d, %d, %d], motion[%d, %d, %d, %d] m[%d] \n", idx, score, \
                    pdet_rect->left, pdet_rect->top, pdet_rect->right, pdet_rect->bottom, \
                    data->rect[idx].left, data->rect[idx].top, data->rect[idx].right, data->rect[idx].bottom, data->rect[idx].bIsMotion);
            idx++;
        }
    }

    return total_score;
}

int motion_pdet_score(MotionPdetParam *pMotionPdetParam, MotionDetectRectInfo *pMotionLastData)
{
    if (!pMotionPdetParam || !pMotionPdetParam)
    {
        aloge("fatal error! param is null");
        return -1;
    }

    SampleDewbInfo *pDewbInfo = pMotionPdetParam->pDewbInfo;
    SamplePdetInfo *pPdetInfo = pMotionPdetParam->pPdetInfo;
    if (!pDewbInfo || !pPdetInfo)
    {
        aloge("fatal error! pDewbInfo or pPdetInfo is null");
        return -2;
    }

    float ratio_w = (float)pDewbInfo->mInputInfo.mWidth / pDewbInfo->mOutputInfo.mWidth;
    float ratio_h = (float)pDewbInfo->mInputInfo.mHeight / pDewbInfo->mOutputInfo.mHeight;

    if(pPdetInfo->pdet_outputs->num > 0)
    {
        int det_flag = 0;
        MotionRect rect;
        for (int i = 0; i < pPdetInfo->pdet_num; i++)
        {
            rect.left   = pPdetInfo->mCoordinate[i].x * ratio_w;
            rect.top    = pPdetInfo->mCoordinate[i].y * ratio_h;
            rect.right  = (pPdetInfo->mCoordinate[i].x + pPdetInfo->mCoordinate[i].width) * ratio_w;
            rect.bottom = (pPdetInfo->mCoordinate[i].y + pPdetInfo->mCoordinate[i].height) * ratio_h;

            pMotionPdetParam->total_score[i] = score_pdet_rect_iou(&pMotionPdetParam->mMotionDetectContext, &rect, pMotionLastData);
            alogd("score[%d] = %f , mMotionLastData.num: %d", i, pMotionPdetParam->total_score[i], pMotionLastData->num);
        }
    }

    return 0;
}

static void motion_pdet_draw(MotionPdetParam *pMotionPdetParam, BOOL orl_enable)
{
    SampleDewbInfo *pDewbInfo = pMotionPdetParam->pDewbInfo;
    SamplePdetInfo *pPdetInfo = pMotionPdetParam->pPdetInfo;
    PdetOrlInfo *pPdetOrlInfo = &pMotionPdetParam->mPdetOrlInfo;
    MotionDetectContext *pMotionDetectContext = &pMotionPdetParam->mMotionDetectContext;

    int ret;

    unsigned int orl_x = 0;
    unsigned int orl_y = 0;
    unsigned int orl_Width = 0;
    unsigned int orl_Height = 0;
    float prob = 0.0;

    MPP_CHN_S viChn = {MOD_ID_VIU, pMotionPdetParam->mPdetOrlInfo.mVipp, pMotionPdetParam->mPdetOrlInfo.mViChn};
    for (int i = 0; i < MAX_VIPP_ORL_NUM; i++)
    {
        AW_MPI_RGN_DetachFromChn(i, &viChn);
        AW_MPI_RGN_Destroy(i);
    }

    if (!orl_enable)
        return;

    RGN_ATTR_S stRgnAttr;
    RGN_CHN_ATTR_S stRgnChnAttr;
    memset(&stRgnAttr, 0, sizeof(RGN_ATTR_S));
    memset(&stRgnChnAttr, 0, sizeof(RGN_CHN_ATTR_S));

    for(int i = 0; i < pPdetInfo->pdet_num; i++)
    {
#ifdef SUPPORT_MOTION_PDET
        if ((pMotionPdetParam->total_score[i] < pMotionPdetParam->mMotionDetectContext.mMotionDynamicThres) && \
            (pPdetInfo->pdet_outputs->persons[i].prob < pMotionPdetParam->mMotionDetectContext.mMotionPdetThres) )
        continue;
#endif

        stRgnAttr.enType = ORL_RGN;
        ret = AW_MPI_RGN_Create(i, &stRgnAttr);
        if (ret != SUCCESS)
        {
            aloge("fatal error! why create ORL region fail?[0x%x]", ret);
            break;
        }

        if (pPdetInfo->mCoordinate[i].y + pPdetInfo->mCoordinate[i].height > pDewbInfo->mOutputInfo.mHeight)
        {
            pPdetInfo->mCoordinate[i].height = pDewbInfo->mOutputInfo.mHeight - pPdetInfo->mCoordinate[i].y;
        }

        orl_x = pPdetInfo->mCoordinate[i].x * pPdetOrlInfo->mOrlWidthRatio;;
        orl_y = pPdetInfo->mCoordinate[i].y * pPdetOrlInfo->mOrlHeightRatio;
        orl_Width = pPdetInfo->mCoordinate[i].width * pPdetOrlInfo->mOrlWidthRatio;;
        orl_Height = pPdetInfo->mCoordinate[i].height * pPdetOrlInfo->mOrlHeightRatio;

        alogv("mOrlWidthRatio:%f, mOrlHeightRatio:%f \n", pPdetOrlInfo->mOrlWidthRatio, pPdetOrlInfo->mOrlHeightRatio);

        stRgnChnAttr.bShow = TRUE;
        stRgnChnAttr.enType = ORL_RGN;
        stRgnChnAttr.unChnAttr.stOrlChn.enAreaType = AREA_RECT;
        stRgnChnAttr.unChnAttr.stOrlChn.stRect.X = orl_x;
        stRgnChnAttr.unChnAttr.stOrlChn.stRect.Y = orl_y;
        stRgnChnAttr.unChnAttr.stOrlChn.stRect.Width = orl_Width;
        stRgnChnAttr.unChnAttr.stOrlChn.stRect.Height = orl_Height;
        stRgnChnAttr.unChnAttr.stOrlChn.mColor = 0xFF0000;
        stRgnChnAttr.unChnAttr.stOrlChn.mThick = 4;
        stRgnChnAttr.unChnAttr.stOrlChn.mLayer = i;
        ret = AW_MPI_RGN_AttachToChn(i, &viChn, &stRgnChnAttr);
        if (ret != SUCCESS)
        {
            aloge("fatal error! why attach to vi channel[%d,%d] fail?", pMotionPdetParam->mPdetOrlInfo.mVipp, pMotionPdetParam->mPdetOrlInfo.mViChn);
        }
    }
}

static void motion_pdet_osd(MotionPdetParam *pMotionPdetParam, BOOL osd_enable)
{
    SampleDewbInfo *pDewbInfo = pMotionPdetParam->pDewbInfo;
    SamplePdetInfo *pPdetInfo = pMotionPdetParam->pPdetInfo;
    PdetOsdInfo *pPdetOsdInfo = &pMotionPdetParam->mPdetOsdInfo;
    StreamOsdConfig *pStreamOsdConfig = &pPdetOsdInfo->mStreamOsdConfig;

    int ret;
    int ratio_w_0 = 0;
    int ratio_h_0 = 0;
    int ratio_w_1 = 0;
    int ratio_h_1 = 0;
    int sub_x = 0;
    int sub_y = 0;
    unsigned int sub_Width = 0;
    unsigned int sub_Height = 0;
    char pdet_text[128] = {0};

    MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pMotionPdetParam->mPdetOsdInfo.mVEncChn};
    for(int i = 0; i <= pPdetInfo->last_pdet_num; i++)
    {
        DestroyStreamOSD(pStreamOsdConfig, 0, i);
    }

    if (!osd_enable)
        return;

    for(int i = 0; i < pPdetInfo->pdet_num; i++)
    {
        alogv("pdet[%d], total_score:%d\n", i, pMotionPdetParam->total_score[i]);

#ifdef SUPPORT_MOTION_PDET
        if ((pMotionPdetParam->total_score[i] < pMotionPdetParam->mMotionDetectContext.mMotionDynamicThres) && \
            (pPdetInfo->pdet_outputs->persons[i].prob < pMotionPdetParam->mMotionDetectContext.mMotionPdetThres))
            continue;
#endif

        int pdet_osd_x = pPdetInfo->mCoordinate[i].x * pPdetOsdInfo->mOsdWidthRatio;
        int pdet_osd_y = pPdetInfo->mCoordinate[i].y * pPdetOsdInfo->mOsdHeightRatio;
        float prob = (float)(pPdetInfo->pdet_outputs->persons[i].prob);
        int label = pPdetInfo->pdet_outputs->persons[i].label;

        sprintf(pdet_text, "P[%.3f],L[%d],S[%.3f]", prob, pPdetInfo->pdet_outputs->persons[i].label, pMotionPdetParam->total_score[i]);
        CreateStreamOSD(pStreamOsdConfig, 0, pdet_text, pdet_osd_x, pdet_osd_y - 64, i);
        pPdetInfo->last_pdet_num = pPdetInfo->pdet_num;
    }
}

int motion_pdet_init_buffer(MotionPdetParam *pMotionPdetParam)
{
    if (!pMotionPdetParam)
    {
        aloge("fatal error! pMotionPdetParam is null");
        return -1;
    }

    ERRORTYPE ret = SUCCESS;
    SampleDewbInfo *pDewbInfo = pMotionPdetParam->pDewbInfo;
    SamplePdetInfo *pPdetInfo = pMotionPdetParam->pPdetInfo;
    MotionDetectContext *pMotionDetectContext = &pMotionPdetParam->mMotionDetectContext;

    /**
     * data path: vipp(yuv420, w*h) -> de resize(rgb, 640*480) -> pdet(r/g/b 3ch, 320*180)
     * De input : yuv420 format, buf size w*h*3/2
     * De output: rgb format, buf size w*h*c
     */
    pDewbInfo->mOutputInfo.nBufSize = pDewbInfo->mOutputInfo.mWidth * pDewbInfo->mOutputInfo.mHeight * pPdetInfo->pdet_input_c;
    ret = AW_MPI_SYS_MmzAlloc_Cached(&pDewbInfo->mOutputInfo.nPhyAddr, &pDewbInfo->mOutputInfo.pVirtAddr, pDewbInfo->mOutputInfo.nBufSize);
    if (ret != SUCCESS)
    {
        aloge("fatal error! malloc fail!\n");
        goto err1;
    }
    pDewbInfo->mOutputInfo.mIonFd = AW_MPI_SYS_MmzGetBufferFd(pDewbInfo->mOutputInfo.pVirtAddr);

    pPdetInfo->nBufSize = pPdetInfo->pdet_input_w * pPdetInfo->pdet_input_h * 3;
    ret = AW_MPI_SYS_MmzAlloc_Cached(&pPdetInfo->nPhyAddr, &pPdetInfo->pVirtAddr, pPdetInfo->nBufSize);
    if (ret != SUCCESS)
    {
        aloge("fatal error! malloc fail!\n");
        goto err1;
    }
    memset(pPdetInfo->pVirtAddr, 0, pPdetInfo->nBufSize);

#ifdef SUPPORT_MOTION_PDET
    // motion detect input buffer, must be 16 aligned
    int nPicWidth = AWALIGN(pMotionDetectContext->mMdStaCfg.nPicWidth, 16);
    int nPicHeight = AWALIGN(pMotionDetectContext->mMdStaCfg.nPicHeight, 16);
    pMotionDetectContext->mMotionDetBufSize = nPicWidth * nPicHeight;
    ret = AW_MPI_SYS_MmzAlloc_Cached(&pMotionDetectContext->mMotionDetPhyAddr,
                                    &pMotionDetectContext->mMotionDetVirtAddr,
                                    pMotionDetectContext->mMotionDetBufSize);
    if (ret != SUCCESS)
    {
        aloge("fatal error! malloc fail!\n");
        goto err2;
    }


    memset(pMotionDetectContext->mMotionDetVirtAddr, 0,
            pMotionDetectContext->mMotionDetBufSize);
#endif

    return ret;

err2:
    if (pPdetInfo->pVirtAddr)
    {
        AW_MPI_SYS_MmzFree(pPdetInfo->nPhyAddr, pPdetInfo->pVirtAddr);
        pPdetInfo->pVirtAddr = NULL;
        pPdetInfo->pVirtAddr = 0;
    }

err1:
    return ret;
}

int motion_pdet_uninit_buffer(MotionPdetParam *pMotionPdetParam)
{
    ERRORTYPE ret = SUCCESS;
    SampleDewbInfo *pDewbInfo = pMotionPdetParam->pDewbInfo;
    SamplePdetInfo *pPdetInfo = pMotionPdetParam->pPdetInfo;
    MotionDetectContext *pMotionDetectContext = &pMotionPdetParam->mMotionDetectContext;

    if (pDewbInfo->mOutputInfo.pVirtAddr)
    {
        AW_MPI_SYS_MmzFree(pDewbInfo->mInputInfo.nPhyAddr, pDewbInfo->mOutputInfo.pVirtAddr);
        pDewbInfo->mOutputInfo.pVirtAddr = NULL;
        pDewbInfo->mInputInfo.nPhyAddr = 0;
    }

    if (pPdetInfo->pVirtAddr)
    {
        AW_MPI_SYS_MmzFree(pPdetInfo->nPhyAddr, pPdetInfo->pVirtAddr);
        pPdetInfo->pVirtAddr = NULL;
        pPdetInfo->pVirtAddr = 0;
    }

#ifdef SUPPORT_MOTION_PDET
    if (pMotionDetectContext->mMotionDetVirtAddr)
    {
        AW_MPI_SYS_MmzFree(pMotionDetectContext->mMotionDetPhyAddr, pMotionDetectContext->mMotionDetVirtAddr);
        pMotionDetectContext->mMotionDetVirtAddr = NULL;
        pMotionDetectContext->mMotionDetPhyAddr = 0;
    }
#endif

    return ret;
}

int motion_pdet_prepare(MotionPdetParam *pMotionPdetParam, void* pVirAddr)
{
    if (!pMotionPdetParam)
    {
        aloge("fatal error! pMotionPdetParam is null");
        return -1;
    }

    int ret = -1;
    SampleDewbInfo *pDewbInfo = pMotionPdetParam->pDewbInfo;
    SamplePdetInfo *pPdetInfo = pMotionPdetParam->pPdetInfo;
    MotionDetectContext *pMotionDetectContext = &pMotionPdetParam->mMotionDetectContext;

    /* DE scale */
    ret = De_Wb_Scale(pDewbInfo);
    if (ret < 0)
    {
        aloge("fatal error! De_Wb_Scale fail!\n");
        /* when De_Wb_Scale fail, reset DE. */
        ret = De_reset(pDewbInfo);
        if (ret != 0)
        {
            aloge("fatal error! De_reset fail! ret = [%d] \n", ret);
            return ret;
        }
    }

    if (pPdetInfo->pVirtAddr && pDewbInfo->mOutputInfo.pVirtAddr)
    {
        memcpy((unsigned char*)pPdetInfo->pVirtAddr,(unsigned char*)pDewbInfo->mOutputInfo.pVirtAddr,
                  pDewbInfo->mOutputInfo.nBufSize);
    }

#ifdef SUPPORT_MOTION_PDET
    /* cpoy Y data for motiondetect */
    if (pMotionDetectContext->mMotionDetVirtAddr && pVirAddr)
    {
        memcpy((unsigned char*)pMotionDetectContext->mMotionDetVirtAddr,(unsigned char*)pVirAddr,
                pMotionDetectContext->mMotionDetBufSize);
    }
    else
    {
        aloge("fatal error! mMotionDetVirtAddr or pVirAddr is null\n");
        return -1;
    }
#endif

    return 0;
}

int motion_pdet_init(MotionPdetParam *pMotionPdetParam)
{
    int ret = 0;
    SamplePdetInfo *pPdetInfo = pMotionPdetParam->pPdetInfo;
    SampleDewbInfo *pDewbInfo = pMotionPdetParam->pDewbInfo;
    MotionDetectContext *pMotionDetectContext = &pMotionPdetParam->mMotionDetectContext;

    /* malloc buffer */
    ret = motion_pdet_init_buffer(pMotionPdetParam);
    if (ret != 0)
    {
        aloge("fatal error! motion_pdet_init_buffer");
        goto _deinit_motion_init_buffer;
    }

    /* de init */
    ret = De_Init(pDewbInfo);
    if (ret != 0)
    {
        aloge("fatal error! de init fail.\n");
        ret = -1;
        goto _de_stop;
    }

    /* pdet init */
    ret = pdet_init(pPdetInfo);
    if (ret < 0)
    {
        aloge("fatal error! pdet alog init fail.\n");
        ret = -1;
        goto _deinit_pdet;
    }

#ifdef SUPPORT_MOTION_PDET
    /* motion detect init */
    ret = motion_detect_init(&pMotionPdetParam->mMotionDetectContext);
    if (ret != 0)
    {
        aloge("fatal error! motion_detect_init fail[%d]", ret);
        goto _deinit_motion_detect;
    }
#endif

    return ret;

_deinit_motion_init_buffer:
    motion_pdet_uninit_buffer(pMotionPdetParam);
_deinit_motion_detect:
    pdet_deinit(pPdetInfo);
_deinit_pdet:
    De_Deinit(pDewbInfo);
_de_stop:
    return ret;
}

int motion_pdet_run(MotionPdetParam *pMotionPdetParam, MotionDetectRectInfo *pMotionLastData)
{
    if (!pMotionPdetParam)
    {
        aloge("fatal error! pMotionPdetParam is null");
        return -1;
    }

    if (!pMotionLastData)
    {
        aloge("fatal error! pMotionLastData is null");
        return -1;
    }

    int ret = -1;
    SamplePdetInfo *pPdetInfo = pMotionPdetParam->pPdetInfo;
    SampleDewbInfo *pDewbInfo = pMotionPdetParam->pDewbInfo;
    MotionDetectContext *pMotionDetectContext = &pMotionPdetParam->mMotionDetectContext;

    if (!pPdetInfo || !pDewbInfo)
    {
        aloge("fatal error! pPdetInfo or pDewbInfo is null");
    }

#ifdef SUPPORT_MOTION_PDET
    /* combined strategy of motion detect and person detect. */
    /* run motion detect */
    if (pMotionDetectContext->mMotionDetVirtAddr)
    {
        motion_detect_run(pMotionDetectContext, (unsigned char*)pMotionDetectContext->mMotionDetVirtAddr);
    }
    else
    {
        aloge("fatal error! mMotionDetVirtAddr is null!");
        return -1;
    }

    /* get motion detect result */
    ret = motion_detect_get_result(pMotionDetectContext, pMotionLastData);
    if (ret < 0)
        aloge("fatal error! getMotionDetecionOneFrameResult fail\n");

    /* when motion detect region num > mJudgeMotionNum, and then run person detect. */
    if ( pMotionLastData->num >= pMotionPdetParam->mMotionDetectContext.mJudgeMotionNum)
    {
#endif
        /* pdet_run */
        if (!pPdetInfo->pVirtAddr)
        {
            aloge("fatal error! pPdetInfo->pVirtAddr is null");
            return -1;
        }
        AW_Img input_img = aw_make_rgb_image(pPdetInfo->pVirtAddr, pPdetInfo->pdet_input_w, pPdetInfo->pdet_input_h, pPdetInfo->pdet_input_c);

        ret = pdet_run(pPdetInfo, &input_img);
        if (ret != 0)
        {
            aloge("fatal error! pdet_run fail");
            aw_free_image(&input_img);
            return -1;
        }
        aw_free_image(&input_img);
#ifdef SUPPORT_MOTION_PDET
        /* score */
        ret = motion_pdet_score(pMotionPdetParam, pMotionLastData);
        if (ret < 0)
        {
            aloge("fatal error! motion_pdet_score fail");
            return -1;
        }
    }
#endif

    /* draw */
    motion_pdet_draw(pMotionPdetParam, 1);

#ifdef SUPPORT_PDET_OSD_TEST
    /* osd */
    motion_pdet_osd(pMotionPdetParam, 1);
#endif

    return 0;
}

int motion_pdet_deinit(MotionPdetParam *pMotionPdetParam)
{
    /* pdet uninit */
    int ret = 0;
    SamplePdetInfo *pPdetInfo = pMotionPdetParam->pPdetInfo;
    SampleDewbInfo *pDewbInfo = pMotionPdetParam->pDewbInfo;
    MotionDetectContext *pMotionDetectContext = &pMotionPdetParam->mMotionDetectContext;

    pdet_deinit(pPdetInfo);

    /* de uninit */
    De_Deinit(pDewbInfo);

#ifdef SUPPORT_MOTION_PDET
    /* motion_detect uninit */
    motion_detect_uninit(pMotionDetectContext);
#endif

    motion_pdet_draw(pMotionPdetParam, 0);

#ifdef SUPPORT_PDET_OSD_TEST
    motion_pdet_osd(pMotionPdetParam, 0);
#endif

    /* free buffer */
    motion_pdet_uninit_buffer(pMotionPdetParam);

    return 0;
}

void motion_pdet_set_sensitivity(MotionPdetParam *pMotionPdetParam, int sensitivity)
{
    SamplePdetInfo *pPdetInfo = pMotionPdetParam->pPdetInfo;
    MotionDetectContext *pMotionDetectContext = &pMotionPdetParam->mMotionDetectContext;

	if (!pMotionDetectContext)
	{
		aloge("fatal error!pMotionDetectContext is null, please check it.");
		return;
	}

	switch (sensitivity)
    {
        case MOTION_PDET_SENSITIVITY_HIGH:
        {
            pMotionDetectContext->mJudgeMotionNum = 50;
            pPdetInfo->pdet_conf_thres = 0.25;
            pMotionDetectContext->mMotionPdetThres = 0.4;
            pMotionDetectContext->mMotionDynamicThres = 0.43;
            break;
        }
        case MOTION_PDET_SENSITIVITY_MIDDLE:
        {
            pMotionDetectContext->mJudgeMotionNum = 50;
            pPdetInfo->pdet_conf_thres = 0.3;
            pMotionDetectContext->mMotionPdetThres = 0.4;
            pMotionDetectContext->mMotionDynamicThres = 0.43;
            break;
        }
        case MOTION_PDET_SENSITIVITY_LOW:
        {
            pMotionDetectContext->mJudgeMotionNum = 50;
            pPdetInfo->pdet_conf_thres = 0.5;
            pMotionDetectContext->mMotionPdetThres = 0.55;
            pMotionDetectContext->mMotionDynamicThres = 0.5;
            break;
        }
        case MOTION_PDET_SENSITIVITY_TEST_MODE:
        {
            pMotionDetectContext->mJudgeMotionNum = 0;
            pPdetInfo->pdet_conf_thres = 0.5;
            pMotionDetectContext->mMotionPdetThres = 0.55;
            pMotionDetectContext->mMotionDynamicThres = 0.5;
            break;
        }
        default:
        {
            pMotionDetectContext->mJudgeMotionNum = 50;
            pPdetInfo->pdet_conf_thres = 0.3;
            pMotionDetectContext->mMotionPdetThres = 0.4;
            pMotionDetectContext->mMotionDynamicThres = 0.43;
            break;
        }
    }

    alogd("mJudgeMotionNum:%d, pdet_conf_thres:%f, mMotionPdetThres:%f, mMotionDynamicThres:%f",
			pMotionDetectContext->mJudgeMotionNum, pPdetInfo->pdet_conf_thres,
			pMotionDetectContext->mMotionPdetThres, pMotionDetectContext->mMotionDynamicThres);
}
