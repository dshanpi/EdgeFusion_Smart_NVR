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
#include <utils/plat_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include "sample_common_isp.h"

static int IspChnlTesting[HW_ISP_DEVICE_NUM];

// 人脸框位置权重
static unsigned short ae_face_weight_win[16] = {
    1,     1,     1,     1,
    1,     2,     2,     1,
    1,     2,     2,     1,
    1,     1,     1,     1
};

// 多人脸场景的人脸位置权重
static HW_S32 ae_face_pos_weight_win[64] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 3, 3, 3, 3, 3, 3, 1,
    1, 3, 6, 6, 6, 6, 3, 1,
    1, 3, 6, 8, 8, 6, 3, 1,
    1, 3, 6, 8, 8, 6, 3, 1,
    1, 3, 6, 6, 6, 6, 3, 1,
    1, 3, 3, 3, 3, 3, 3, 1,
    1, 1, 1, 1, 1, 1, 1, 1
};

void ispTestAeMode(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestAeMode, Current ae_mode = %d\n", pIspTestContext->mTestAeCfg.mAeMode);
    if (pIspTestContext->mTestAeCfg.mAeMode != EXP_MANUAL) {
        pIspTestContext->mTestAeCfg.mAeMode = EXP_MANUAL;
        /* set ae_mode to 1 for manual ae */
        AW_MPI_ISP_AE_SetMode(pIspTestContext->mTetstIspChannelId, EXP_MANUAL);
        // AW_MPI_ISP_AE_SetISOSensitiveMode(pIspTestContext->mTetstIspChannelId, ISO_MANUAL);
        /* set exp */
        AW_MPI_ISP_AE_SetExposure(pIspTestContext->mTetstIspChannelId, 8000);
        /* set gain */
        AW_MPI_ISP_AE_SetGain(pIspTestContext->mTetstIspChannelId, 16);
    } else {
        /* reset to auto AE */
        pIspTestContext->mTestAeCfg.mAeMode = EXP_AUTO;
        AW_MPI_ISP_AE_SetMode(pIspTestContext->mTetstIspChannelId, EXP_AUTO);
        // AW_MPI_ISP_AE_SetISOSensitiveMode(pIspTestContext->mTetstIspChannelId, ISO_AUTO);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestAeExpBias(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestAeExpBias, mExpBais = %d\n", pIspTestContext->mTestAeCfg.mExpBais);
    /* exp_bias_qmenu[] = { -4, -3, -2, -1, 0, 1, 2, 3, 4} */
    AW_MPI_ISP_AE_SetExposureBias(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mExpBais);
    if (++pIspTestContext->mTestAeCfg.mExpBais > 8) {
        pIspTestContext->mTestAeCfg.mExpBais = 0;
        /* reset to 0 */
        AW_MPI_ISP_AE_SetExposureBias(pIspTestContext->mTetstIspChannelId, 4);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestAeIsoSensitive(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestAeIsoSensitive, mIsoSensitive = %d\n", pIspTestContext->mTestAeCfg.mIsoSensitive);
    /* iso_qmenu[] = { 100, 200, 400, 800, 1600, 3200, 6400} */
    AW_MPI_ISP_AE_SetISOSensitiveMode(pIspTestContext->mTetstIspChannelId, ISO_MANUAL);
    AW_MPI_ISP_AE_SetISOSensitive(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mIsoSensitive);
    if (++pIspTestContext->mTestAeCfg.mIsoSensitive > 7) {
        pIspTestContext->mTestAeCfg.mIsoSensitive = 0;
        AW_MPI_ISP_AE_SetISOSensitiveMode(pIspTestContext->mTetstIspChannelId, ISO_AUTO);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestAeMetering(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestAeMetering, mAeMetering = %d\n", pIspTestContext->mTestAeCfg.mAeMetering);
    /*
        enum ae_metering_mode {
            AE_METERING_MODE_AVERAGE    = 0,
            AE_METERING_MODE_CENTER     = 1,
            AE_METERING_MODE_SPOT       = 2,
            AE_METERING_MODE_MATRIX     = 3, default
        };
    */
    AW_MPI_ISP_AE_SetMetering(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mAeMetering);
    if (--pIspTestContext->mTestAeCfg.mAeMetering < 0) {
        pIspTestContext->mTestAeCfg.mAeMetering = AE_METERING_MODE_MATRIX;
        AW_MPI_ISP_AE_SetMetering(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mAeMetering);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestAeEvIdx(IspApiTestCtrlConfig *pIspTestContext)
{
    AW_MPI_ISP_AE_GetMaxEvIdx(pIspTestContext->mTetstIspChannelId, &pIspTestContext->mTestAeCfg.mAeMaxEvIdx);
    pIspTestContext->mTestAeCfg.mAeEvIdx = pIspTestContext->mTestAeCfg.mAeMaxEvIdx - 2;
    alogd("ispTestAeEvIdx, mAeMaxEvIdx = %d, ready to set mAeEvIdx = mAeMaxEvIdx - 2\n", pIspTestContext->mTestAeCfg.mAeMaxEvIdx);
    AW_MPI_ISP_AE_SetEvIdx(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mAeEvIdx);
    pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
}

void ispTestAeLock(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestAeLock, mAeLock = %d\n", pIspTestContext->mTestAeCfg.mAeLock);
    if (!pIspTestContext->mTestAeCfg.mAeLock) {
        /* AE LOCK */
        pIspTestContext->mTestAeCfg.mAeLock = 1;
        AW_MPI_ISP_AE_SetLock(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mAeLock);
    } else {
        /* reset to AE UNLOCK */
        pIspTestContext->mTestAeCfg.mAeLock = 0;
        AW_MPI_ISP_AE_SetLock(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mAeLock);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestAeTable(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestAeTable\n");
    pIspTestContext->mTestAeCfg.mAeTable.length = 1;
    pIspTestContext->mTestAeCfg.mAeTable.ev_step = 30;
    pIspTestContext->mTestAeCfg.mAeTable.shutter_shift = 30;
    pIspTestContext->mTestAeCfg.mAeTable.ae_tbl[0].min_exp = 8000;
    pIspTestContext->mTestAeCfg.mAeTable.ae_tbl[0].max_exp = 20;
    pIspTestContext->mTestAeCfg.mAeTable.ae_tbl[0].min_gain = 256;
    pIspTestContext->mTestAeCfg.mAeTable.ae_tbl[0].max_gain = 256;
    pIspTestContext->mTestAeCfg.mAeTable.ae_tbl[0].min_iris = 266;
    pIspTestContext->mTestAeCfg.mAeTable.ae_tbl[0].max_iris = 266;
    AW_MPI_ISP_AE_SetTable(pIspTestContext->mTetstIspChannelId, &pIspTestContext->mTestAeCfg.mAeTable);
    pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
}

void ispTestAeScene(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestAeScene, mAeScene = %d\n", pIspTestContext->mTestAeCfg.mAeScene);
    AW_MPI_ISP_SetScene(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mAeScene);
    if (++pIspTestContext->mTestAeCfg.mAeScene > 2) {
        pIspTestContext->mTestAeCfg.mAeScene = 0;
        AW_MPI_ISP_SetScene(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mAeScene);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestAeFlicker(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestAeFlicker, mAeFlicker = %d\n", pIspTestContext->mTestAeCfg.mAeFlicker);
    AW_MPI_ISP_SetFlicker(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mAeFlicker);
    if (--pIspTestContext->mTestAeCfg.mAeFlicker < 0) {
        pIspTestContext->mTestAeCfg.mAeFlicker = FREQUENCY_AUTO;
        AW_MPI_ISP_SetFlicker(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mAeFlicker);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestFaceAe(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestFaceAe, mTiggerFaceAeForTest = %d\n", pIspTestContext->mTestAeCfg.mTiggerFaceAeForTest);
    if (++pIspTestContext->mTestAeCfg.mTiggerFaceAeForTest < 5) {
        /* config face roi rgn */
        pIspTestContext->mTestAeCfg.mFaceRoiRgn[0].X = 320;
        pIspTestContext->mTestAeCfg.mFaceRoiRgn[0].Y = 180;
        pIspTestContext->mTestAeCfg.mFaceRoiRgn[0].Width = 50;
        pIspTestContext->mTestAeCfg.mFaceRoiRgn[0].Height = 50;
        /* enable faceAE */
        pIspTestContext->mTestAeCfg.mFaceAeInfo.enable = true;
        pIspTestContext->mTestAeCfg.mFaceAeInfo.vaild_face_num = 1;
        memcpy(&pIspTestContext->mTestAeCfg.mFaceAeInfo.face_roi_rgn, &pIspTestContext->mTestAeCfg.mFaceRoiRgn, AE_FACE_MAX_NUM * sizeof(RECT_S));
        pIspTestContext->mTestAeCfg.mFaceAeInfo.face_ae_target = 55;	//配置当前场景的人脸期望亮度
        pIspTestContext->mTestAeCfg.mFaceAeInfo.face_ae_speed = 31;	// 配置收敛速度
        pIspTestContext->mTestAeCfg.mFaceAeInfo.face_ae_delay_cnt = 40;	// 配置延迟恢复帧数
        pIspTestContext->mTestAeCfg.mFaceAeInfo.face_down_percent = 55;	//配置人脸期望亮度向下偏移百分比，百分比越低效果越明显，越容易过曝
        pIspTestContext->mTestAeCfg.mFaceAeInfo.face_up_percent = 20;	//配置人脸期望亮度向上偏移百分比，百分比越低效果越明显
        pIspTestContext->mTestAeCfg.mFaceAeInfo.ae_face_block_weight = 70;//配置多人脸场景下，人脸块数数量权重大小
        pIspTestContext->mTestAeCfg.mFaceAeInfo.ae_over_face_max_exp_control = 20;//配置多人脸过亮抑制权重
        memcpy(&pIspTestContext->mTestAeCfg.mFaceAeInfo.ae_face_win_weight, ae_face_weight_win, 16 * sizeof(unsigned short));
        memcpy(&pIspTestContext->mTestAeCfg.mFaceAeInfo.ae_face_pos_weight, ae_face_pos_weight_win, 64 * sizeof(HW_S32));
        alogd("mRes: %dx%d\n", pIspTestContext->mTestAeCfg.mRes.Width, pIspTestContext->mTestAeCfg.mRes.Height);
        AW_MPI_ISP_AE_SetFaceAeCfg(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mFaceAeInfo, pIspTestContext->mTestAeCfg.mRes);
    } else {
        pIspTestContext->mTestAeCfg.mTiggerFaceAeForTest = 0;
        /* close faceAE */
        pIspTestContext->mTestAeCfg.mFaceAeInfo.vaild_face_num = 0;
        pIspTestContext->mTestAeCfg.mFaceAeInfo.enable = false;
        memset(&pIspTestContext->mTestAeCfg.mFaceRoiRgn, 0, AE_FACE_MAX_NUM * sizeof(RECT_S));
        AW_MPI_ISP_AE_SetFaceAeCfg(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestAeCfg.mFaceAeInfo, pIspTestContext->mTestAeCfg.mRes);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestAwbMode(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestAwbMode, Current awb_mode = %d\n", pIspTestContext->mTetstAwbCfg.mAwbMode);
    if (!pIspTestContext->mTetstAwbCfg.mAwbMode) {
        pIspTestContext->mTetstAwbCfg.mAwbMode = 1;
        /* set awb_mode to 1 for manual awb */
        AW_MPI_ISP_AWB_SetMode(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTetstAwbCfg.mAwbMode);
        pIspTestContext->mTetstAwbCfg.mAwbColorTemp = 6500;
        pIspTestContext->mTetstAwbCfg.mAwbGain.r_gain = 580;
        pIspTestContext->mTetstAwbCfg.mAwbGain.b_gain = 380;
        pIspTestContext->mTetstAwbCfg.mAwbGain.gr_gain = 256;
        pIspTestContext->mTetstAwbCfg.mAwbGain.gb_gain = 256;
        AW_MPI_ISP_AWB_SetColorTemp(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTetstAwbCfg.mAwbColorTemp);
        AW_MPI_ISP_AWB_SetRGain(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTetstAwbCfg.mAwbGain.r_gain);
        AW_MPI_ISP_AWB_SetBGain(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTetstAwbCfg.mAwbGain.b_gain);
        AW_MPI_ISP_AWB_SetGrGain(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTetstAwbCfg.mAwbGain.gr_gain);
        AW_MPI_ISP_AWB_SetGbGain(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTetstAwbCfg.mAwbGain.gb_gain);
    } else {
            /* reset to auto AWB */
        pIspTestContext->mTetstAwbCfg.mAwbMode = 0;
        AW_MPI_ISP_AWB_SetMode(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTetstAwbCfg.mAwbMode);
        pIspTestContext->mTetstAwbCfg.mAwbMode = WB_MANUAL;
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSpecBrightess(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSpecBrightess, mBrightess = %d\n", pIspTestContext->mTestSpecEffectCfg.mBrightess);
    AW_MPI_ISP_SetBrightness(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mBrightess);
    pIspTestContext->mTestSpecEffectCfg.mBrightess += 64;
    if (pIspTestContext->mTestSpecEffectCfg.mBrightess > 512) {
        /* reset to default value */
        pIspTestContext->mTestSpecEffectCfg.mBrightess = 256;
        AW_MPI_ISP_SetBrightness(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mBrightess);
        pIspTestContext->mTestSpecEffectCfg.mBrightess = 0;
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSpecContrast(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSpecContrast, mContrast = %d\n", pIspTestContext->mTestSpecEffectCfg.mContrast);
    AW_MPI_ISP_SetContrast(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mContrast);
    pIspTestContext->mTestSpecEffectCfg.mContrast += 64;
    if (pIspTestContext->mTestSpecEffectCfg.mContrast > 512) {
        /* reset to default value */
        pIspTestContext->mTestSpecEffectCfg.mContrast = 256;
        AW_MPI_ISP_SetContrast(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mContrast);
        pIspTestContext->mTestSpecEffectCfg.mContrast = 0;
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSpecSaturation(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSpecSaturation, mSaturation = %d\n", pIspTestContext->mTestSpecEffectCfg.mSaturation);
    AW_MPI_ISP_SetSaturation(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mSaturation);
    pIspTestContext->mTestSpecEffectCfg.mSaturation += 64;
    if (pIspTestContext->mTestSpecEffectCfg.mSaturation > 768) {
        /* reset to default value */
        pIspTestContext->mTestSpecEffectCfg.mSaturation = 256;
        AW_MPI_ISP_SetSaturation(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mSaturation);
        pIspTestContext->mTestSpecEffectCfg.mSaturation = 0;
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSpecSharpness(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSpecSharpness, mSharpness = %d\n", pIspTestContext->mTestSpecEffectCfg.mSharpness);
    if (pIspTestContext->mTestSpecEffectCfg.mSharpness == 0) {
        pIspTestContext->mTestSpecEffectCfg.mSharpness = 1;
    }
    AW_MPI_ISP_SetSharpness(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mSharpness);
    pIspTestContext->mTestSpecEffectCfg.mSharpness += 512;
    if (pIspTestContext->mTestSpecEffectCfg.mSharpness > 4095) {
        /* reset to default value */
        pIspTestContext->mTestSpecEffectCfg.mSharpness = 256;
        AW_MPI_ISP_SetSharpness(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mSharpness);
        pIspTestContext->mTestSpecEffectCfg.mSharpness = 1;
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSpecHue(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSpecHue, mHue = %d\n", pIspTestContext->mTestSpecEffectCfg.mHue);
    AW_MPI_ISP_SetHue(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mHue);
    pIspTestContext->mTestSpecEffectCfg.mHue += 51;
    if (pIspTestContext->mTestSpecEffectCfg.mHue > 255) {
        /* reset to default value */
        pIspTestContext->mTestSpecEffectCfg.mHue = 0;
        AW_MPI_ISP_SetHue(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mHue);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSpecPltmStren(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSpecPltmStren, mPltmStren = %d\n", pIspTestContext->mTestSpecEffectCfg.mPltmStren);
    if (pIspTestContext->mTestSpecEffectCfg.mPltmStren == 0) {
        pIspTestContext->mTestSpecEffectCfg.mPltmStren = -2500;
    }
    AW_MPI_ISP_SetPltmWDR(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mPltmStren);
    pIspTestContext->mTestSpecEffectCfg.mPltmStren += 1000;
    if (pIspTestContext->mTestSpecEffectCfg.mPltmStren > 2500) {
        /* reset to default value */
        pIspTestContext->mTestSpecEffectCfg.mPltmStren = 0;
        AW_MPI_ISP_SetPltmWDR(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mPltmStren);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSpec2dnrStren(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSpec2dnrStren, m2dnrStren = %d\n", pIspTestContext->mTestSpecEffectCfg.m2dnrStren);
    if (pIspTestContext->mTestSpecEffectCfg.m2dnrStren == 0) {
        pIspTestContext->mTestSpecEffectCfg.m2dnrStren = 1;
    }
    AW_MPI_ISP_SetNRAttr(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.m2dnrStren);
    pIspTestContext->mTestSpecEffectCfg.m2dnrStren += 512;
    if (pIspTestContext->mTestSpecEffectCfg.m2dnrStren > 4095) {
        /* reset to default value */
        pIspTestContext->mTestSpecEffectCfg.m2dnrStren = 256;
        AW_MPI_ISP_SetNRAttr(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.m2dnrStren);
        pIspTestContext->mTestSpecEffectCfg.m2dnrStren = 1;
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSpec3dnrStren(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSpec3dnrStren, m3dnrStren = %d\n", pIspTestContext->mTestSpecEffectCfg.m3dnrStren);
    if (pIspTestContext->mTestSpecEffectCfg.m3dnrStren == 0) {
        pIspTestContext->mTestSpecEffectCfg.m3dnrStren = 1;
    }
    AW_MPI_ISP_Set3NRAttr(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.m3dnrStren);
    pIspTestContext->mTestSpecEffectCfg.m3dnrStren += 512;
    if (pIspTestContext->mTestSpecEffectCfg.m3dnrStren > 4095) {
        /* reset to default value */
        pIspTestContext->mTestSpecEffectCfg.m3dnrStren = 256;
        AW_MPI_ISP_Set3NRAttr(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.m3dnrStren);
        pIspTestContext->mTestSpecEffectCfg.m3dnrStren = 1;
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSpecColorEffect(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSpecColorEffect, mColorStyle = %d\n", pIspTestContext->mTestSpecEffectCfg.mColorStyle);
    AW_MPI_ISP_SetColorEffect(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mColorStyle);
    if (++pIspTestContext->mTestSpecEffectCfg.mColorStyle > ISP_COLORFX_SUNSET) {
        /* reset to default value */
        pIspTestContext->mTestSpecEffectCfg.mColorStyle = ISP_COLORFX_NONE;
        AW_MPI_ISP_SetColorEffect(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mColorStyle);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSpecMirror(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSpecMirror, mMirror = %d\n", pIspTestContext->mTestSpecEffectCfg.mMirror);
    if (!pIspTestContext->mTestSpecEffectCfg.mMirror) {
        pIspTestContext->mTestSpecEffectCfg.mMirror = true;
        AW_MPI_ISP_SetMirror(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mMirror);
        // AW_MPI_VI_SetVippMirror(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mMirror);
    } else {
        /* reset to default value */
        pIspTestContext->mTestSpecEffectCfg.mMirror = false;
        AW_MPI_ISP_SetMirror(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mMirror);
        // AW_MPI_VI_SetVippMirror(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mMirror);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSpecFlip(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSpecFlip, mFlip = %d\n", pIspTestContext->mTestSpecEffectCfg.mFlip);
    if (!pIspTestContext->mTestSpecEffectCfg.mFlip) {
        pIspTestContext->mTestSpecEffectCfg.mFlip = true;
        AW_MPI_ISP_SetFlip(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mFlip);
    } else {
        /* reset to default value */
        pIspTestContext->mTestSpecEffectCfg.mFlip = false;
        AW_MPI_ISP_SetFlip(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mFlip);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSetIrStatus(IspApiTestCtrlConfig *pIspTestContext)
{
    alogd("ispTestSetIrStatus, mIrStatus = %d\n", pIspTestContext->mTestSpecEffectCfg.mIrStatus);
    if (pIspTestContext->mTestSpecEffectCfg.mIrStatus == NORMAL_CFG) {
        pIspTestContext->mTestSpecEffectCfg.mIrStatus = IR_NORMAL_CFG;
        AW_MPI_ISP_SwitchIspConfig(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mIrStatus);
    } else {
        pIspTestContext->mTestSpecEffectCfg.mIrStatus = NORMAL_CFG;
        AW_MPI_ISP_SwitchIspConfig(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestSpecEffectCfg.mIrStatus);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    }
}

void ispTestSettingsIspFreq(IspApiTestCtrlConfig *pIspTestContext)
{
    AW_MPI_ISP_GetIspAlgoFreq(pIspTestContext->mTetstIspChannelId, &pIspTestContext->mTetstSettingsCfg.mIspAlgoFreq);
    if (++pIspTestContext->mTetstSettingsCfg.mIspAlgoFreq > DIV_RATIO_MAX) {
        pIspTestContext->mTetstSettingsCfg.mIspAlgoFreq = DIV_RATIO_MIN;
        AW_MPI_ISP_SetIspAlgoFreq(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTetstSettingsCfg.mIspAlgoFreq);
        pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask] = 1;
    } else {
        AW_MPI_ISP_SetIspAlgoFreq(pIspTestContext->mTetstIspChannelId, pIspTestContext->mTetstSettingsCfg.mIspAlgoFreq);
    }
    alogd("ispTestSettingsIspFreq, mIspAlgoFreq = %d\n", pIspTestContext->mTetstSettingsCfg.mIspAlgoFreq);
}

static void (*taskFunctions[])(IspApiTestCtrlConfig *pIspTestContext) = {
    (void (*)(IspApiTestCtrlConfig *))&ispTestAeMode,
    (void (*)(IspApiTestCtrlConfig *))&ispTestAeExpBias,
    (void (*)(IspApiTestCtrlConfig *))&ispTestAeIsoSensitive,
    (void (*)(IspApiTestCtrlConfig *))&ispTestAeMetering,
    (void (*)(IspApiTestCtrlConfig *))&ispTestAeEvIdx,
    (void (*)(IspApiTestCtrlConfig *))&ispTestAeLock,
    (void (*)(IspApiTestCtrlConfig *))&ispTestAeTable,
    (void (*)(IspApiTestCtrlConfig *))&ispTestAeScene,
    (void (*)(IspApiTestCtrlConfig *))&ispTestAeFlicker,
    (void (*)(IspApiTestCtrlConfig *))&ispTestFaceAe,
    (void (*)(IspApiTestCtrlConfig *))&ispTestAwbMode,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSpecBrightess,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSpecContrast,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSpecSaturation,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSpecSharpness,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSpecHue,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSpecPltmStren,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSpec2dnrStren,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSpec3dnrStren,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSpecColorEffect,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSpecMirror,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSpecFlip,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSetIrStatus,
    (void (*)(IspApiTestCtrlConfig *))&ispTestSettingsIspFreq
};

static int initIspTasks(IspApiTestCtrlConfig *pIspTestContext)
{
    int i;

    pIspTestContext->mIspApiTask.mTasksNum = sizeof(taskFunctions) / sizeof(taskFunctions[0]);
    alogd("mTasksNum: %d, TEST_TOTAL = %d, TEST_ISP_ALL = 0x%x\n", pIspTestContext->mIspApiTask.mTasksNum, TEST_TOTAL, TEST_ISP_ALL);
    if (pIspTestContext->mIspApiTask.mTasksNum > TEST_TOTAL) {
        aloge("mTasksNum: %d is more than MAX_TASKS\n", pIspTestContext->mIspApiTask.mTasksNum);
        return -1;
    }
    memset(&pIspTestContext->mIspApiTask.mTaskList, 0, TEST_TOTAL * sizeof(unsigned int));
    for (i = 0, pIspTestContext->mIspApiTask.mActuralTaskNum = 0; i < pIspTestContext->mIspApiTask.mTasksNum; i++) {
        pIspTestContext->mIspApiTask.tasks[i] = taskFunctions[i];
        if (pIspTestContext->mIspApiTask.mtaskSchedule & (1 << i)) {
            pIspTestContext->mIspApiTask.mTaskList[pIspTestContext->mIspApiTask.mActuralTaskNum] = i;
            pIspTestContext->mIspApiTask.mActuralTaskNum++;
            alogd("Get task id: %d, mActuralTaskNum = %d\n", i, pIspTestContext->mIspApiTask.mActuralTaskNum);
        }
        pIspTestContext->mIspApiTask.completed[i] = 0;
    }
    pIspTestContext->mTestAeCfg.mAeFlicker = FREQUENCY_AUTO;

    return 0;
}

static int ispTestFunc(IspApiTestCtrlConfig *pIspTestContext)
{
    if (pIspTestContext->mIspApiTask.mCurrentTask >= 0 && pIspTestContext->mIspApiTask.mCurrentTask < pIspTestContext->mIspApiTask.mActuralTaskNum) {
        alogd("mActuralTaskNum = %d, mCurrentTask = %d, mtaskSchedule = 0x%x\n",
            pIspTestContext->mIspApiTask.mActuralTaskNum, pIspTestContext->mIspApiTask.mCurrentTask, pIspTestContext->mIspApiTask.mtaskSchedule);
        pIspTestContext->mIspApiTask.tasks[pIspTestContext->mIspApiTask.mTaskList[pIspTestContext->mIspApiTask.mCurrentTask]](pIspTestContext);
        if (pIspTestContext->mIspApiTask.completed[pIspTestContext->mIspApiTask.mCurrentTask]) {
            pIspTestContext->mIspApiTask.mCurrentTask++;
        }
    }
    if (pIspTestContext->mIspApiTask.mCurrentTask >= pIspTestContext->mIspApiTask.mActuralTaskNum) {
        alogd("It will reset mIspApiTask\n");
        pIspTestContext->mIspApiTask.mCurrentTask = 0;
        for (int i = 0; i < pIspTestContext->mIspApiTask.mTasksNum; i++) {
            pIspTestContext->mIspApiTask.completed[i] = 0;
        }
        pIspTestContext->mTestAeCfg.mAeFlicker = FREQUENCY_AUTO;
    }

    return 0;
}

static void* ispTestThread(void *arg)
{
    IspApiTestCtrlConfig *pIspTestContext = (IspApiTestCtrlConfig *)arg;

    while (1)
    {
        ispTestFunc(pIspTestContext);
        usleep(pIspTestContext->mTestIntervalMs * 1000);
    }

    return NULL;
}

static int ispTestThreadCreate(IspApiTestCtrlConfig *pIspTestContext)
{
    int ret;
    char strThreadName[32];

    ret = pthread_create(&pIspTestContext->ispTestThreadId, NULL, ispTestThread, (void *)pIspTestContext);
    if (ret < 0) {
        aloge("IspTestThreadCreate failed!\n");
        return -1;
    } else {
        snprintf(strThreadName, 32, "ISP%d_TestThread", pIspTestContext->mTetstIspChannelId);
        pthread_setname_np(pIspTestContext->ispTestThreadId, strThreadName);
        alogd("IspTestThreadCreate done\n");
    }

    return 0;
}

static int parserTestCtrlConfig(IspApiTestCtrlConfig *pIspTestContext)
{
    if (!pIspTestContext) {
        aloge("pIspTestContext is NULL, could not parser config!!!\n");
        return -1;
    }

    if (pIspTestContext->mTetstIspChannelId >= HW_ISP_DEVICE_NUM) {
        aloge("mTetstIspChannelId: %d is invaild!!!\n", pIspTestContext->mTetstIspChannelId);
        return -1;
    }

    if (pIspTestContext->mTestIntervalMs <= 500) {
        aloge("Please set mTestIntervalMs more than 500 ms\n");
        return -1;
    }
    alogd("####### mTetstIspChannelId: %d, mTetstIspChannelId: %d ms\n",
        pIspTestContext->mTetstIspChannelId, pIspTestContext->mTestIntervalMs);

    return 0;
}

int ispApiTestInit(IspApiTestCtrlConfig *pIspTestContext)
{
    if (!pIspTestContext) {
        aloge("pIspTestContext is NULL, please check it!!!\n");
        return -1;
    }

    if (parserTestCtrlConfig(pIspTestContext) < 0) {
        aloge("parserTestCtrlConfig failed!\n");
        return -1;
    }

    if (IspChnlTesting[pIspTestContext->mTetstIspChannelId]++ > 0) {
        alogd("ISP%d is testing, will not create new one\n", pIspTestContext->mTetstIspChannelId);
        return 0;
    }

    if (ispTestThreadCreate(pIspTestContext) < 0) {
        IspChnlTesting[pIspTestContext->mTetstIspChannelId]--;
        aloge("IspTestThreadCreate failed!\n");
        return -1;
    }

    if (initIspTasks(pIspTestContext) < 0) {
        aloge("initIspTasks failed!\n");
        ispApiTestExit(pIspTestContext);
        return -1;
    }
    alogd("ispApiTestInit done\n");

    return 0;
}

int ispApiTestExit(IspApiTestCtrlConfig *pIspTestContext)
{
    if (!pIspTestContext) {
        aloge("pIspTestContext is NULL, please check it!!!\n");
        return -1;
    }
    IspChnlTesting[pIspTestContext->mTetstIspChannelId]--;
    pthread_cancel(pIspTestContext->ispTestThreadId);
    alogd("ispApiTestExit done\n");

    return 0;
}

static int updateSunxiDumpDump(unsigned int regsBase, unsigned int regsEnd)
{
    int fd;
    char buffer[128];
    ssize_t num_written;

    fd = open(SUXI_DUMP_DUMP_PATH, O_WRONLY);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }

    sprintf(buffer, "0x%x, 0x%x", regsBase, regsEnd);
    num_written = write(fd, buffer, strlen(buffer));
    if (num_written == -1) {
        perror("Error writing to file");
        return -1;
    }
    close(fd);

    return 0;
}

static int updateSunxiDumpWrite(unsigned int regsAddr, unsigned int regsValue)
{
	int fd;
    char buffer[128];
    ssize_t num_written;

    fd = open(SUXI_DUMP_WRITE_PATH, O_WRONLY);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }

    sprintf(buffer, "%x %x", regsAddr, regsValue);
    num_written = write(fd, buffer, strlen(buffer));
    if (num_written == -1) {
        perror("Error writing to file");
        return -1;
    }
    close(fd);

    return 0;
}

static int catRegs(unsigned int regsBase, unsigned int regsEnd)
{
	int fd;
	char buffer[BUF_SIZE];
	ssize_t bytes_read;

    if (regsEnd < regsBase) {
        aloge("regsEnd:%x < regsBase:%x", regsEnd, regsBase);
        return -1;
    }

    updateSunxiDumpDump(regsBase, regsEnd);
    fd = open(SUXI_DUMP_DUMP_PATH, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }

    while ((bytes_read = read(fd, buffer, BUF_SIZE)) > 0) {
        write(STDOUT_FILENO, buffer, bytes_read);
    }

    if (bytes_read == -1) {
        perror("Error reading file");
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}

static int writeRegs(unsigned int regsAddr, unsigned int regsValue)
{
    if (regsAddr == 0) {
        aloge("regsAddr = %x is eroor", regsAddr);
        return -1;
    }
    if (updateSunxiDumpWrite(regsAddr, regsValue) < 0) {
        perror("writeSunxiDumpWrite failed");
        return -1;
    }

    return 0;
}

static int readRegs(unsigned int regsAddr, unsigned int *regsValue)
{
    int fd;
    char buffer[32];
    unsigned int value;
    unsigned int address;

    if (regsAddr == 0) {
        printf("regsAddr = %x is eroor", regsAddr);
        return -1;
    }

    updateSunxiDumpDump(regsAddr, regsAddr);
    fd = open(SUXI_DUMP_DUMP_PATH, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }

    if (read(fd, buffer, sizeof(buffer)) == -1) {
        perror("Error reading file");
        close(fd);
        return -1;
    }

    if (sscanf(buffer, "%x%*[^0123456789ABCDEFabcdef]%x", &value, &address) == 2) {
        *regsValue = value;
    } else {
        perror("Error parsing register values\n");
        return -1;
    }

    return 0;
}

static int checkMipiPayLoadStatus(MipiDeskConfig *pdetMipiCfg)
{
    if (pdetMipiCfg == NULL) {
        aloge("pdetMipiCfg is NULL!!!\n");
        return -1;
    }

    writeRegs(pdetMipiCfg->mPayLoadAddr, REGS_FLUSH);
    readRegs(pdetMipiCfg->mPayLoadAddr, &pdetMipiCfg->mPayLoadStatus);
    alogd("checkMipiPayLoadStatus 0x%08x: 0x%08x\n", pdetMipiCfg->mPayLoadAddr, pdetMipiCfg->mPayLoadStatus);

    if (pdetMipiCfg->mPayLoadStatus & MIPI_PAYLOAD_ERRPD_MASK) {
        return -1;
    } else {
        return 0;
    }
}

static int updateMipiPhyDeskew(MipiDeskConfig *pdetMipiCfg)
{
    unsigned int currRegsValue = 0;
    unsigned int targetRegsValue = 0;
    unsigned int targetDeskValue = 0;

    if (pdetMipiCfg == NULL) {
        aloge("pdetMipiCfg is NULL!!!\n");
        return -1;
    }

    if (pdetMipiCfg->mPhyDeskValue > pdetMipiCfg->mPhyDeskValueMax) {
        aloge("mPhyDeskValue: %x > mPhyDeskValueMax: %x\n", pdetMipiCfg->mPhyDeskValue, pdetMipiCfg->mPhyDeskValueMax);
        return -1;
    }

    readRegs(pdetMipiCfg->mPhyDeskRegsAddr, &currRegsValue);
    targetRegsValue = currRegsValue;
    targetDeskValue = (pdetMipiCfg->mPhyDeskValue & MIPI_PHY_DESKEW_CLK_DLY_MASK) << MIPI_PHY_DESKEW_CLK_DLY_SET;
    targetRegsValue &= ~(MIPI_PHY_DESKEW_CLK_DLY_MASK << MIPI_PHY_DESKEW_CLK_DLY_SET);
    targetRegsValue |= targetDeskValue;
    alogd("read deskew status: 0x%08x, ready to set 0x%x, update regs value to 0x%08x\n", currRegsValue, pdetMipiCfg->mPhyDeskValue, targetRegsValue);
    writeRegs(pdetMipiCfg->mPhyDeskRegsAddr, targetRegsValue);

    return 0;
}

static int detectMipiDeskewProcess(DetMipiDeskCtrlConfig *pDetMipiDeskContext)
{
    if (pDetMipiDeskContext == NULL) {
        aloge("pDetMipiDeskContext is NULL!!!\n");
        return -1;
    }

    if (pDetMipiDeskContext->mMipiDeskCfg.mPayLoadAddr == 0) {
        aloge("Invaild mPayLoadAddr!!!");
        return -1;
    }

    if (checkMipiPayLoadStatus(&pDetMipiDeskContext->mMipiDeskCfg) < 0) {
        aloge("deskew set 0x%x, mipi payload error!!!!\n", pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskValue);
        pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskEnd = pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskPass;
    } else {
        if (pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskStart == 0) {
            pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskStart = pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskValue;
        }
        pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskPass = pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskValue;
    }
    if (++pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskValue > pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskValueMax) {
        pDetMipiDeskContext->mdetMipiDeskDone = true;
        if (pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskEnd == 0) {
            pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskEnd = pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskValueMax;
        }
        alogd(">>>>>>>>>>>>>>>>>>>> detectMipiDeskew value: [0x%x, 0x%x]\n",
            pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskStart, pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskEnd);
        pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskBest = (pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskStart + pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskEnd) >> 1;
        pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskValue = pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskBest;
        alogd(">>>>>>>>>>>>>>>>>>>> BestMipiDeskew value: 0x%x, it will set it for capturing\n", pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskBest);
        updateMipiPhyDeskew(&pDetMipiDeskContext->mMipiDeskCfg);
        /* refsh mipi payload regs */
        writeRegs(pDetMipiDeskContext->mMipiDeskCfg.mPayLoadAddr, REGS_FLUSH);
    } else {
        updateMipiPhyDeskew(&pDetMipiDeskContext->mMipiDeskCfg);
    }

    return 0;
}

static void* detectMipiDeskewThread(void *arg)
{
    DetMipiDeskCtrlConfig *pDetMipiDeskContext = (DetMipiDeskCtrlConfig *)arg;

    while (1)
    {
        if (!pDetMipiDeskContext->mdetMipiDeskDone) {
            detectMipiDeskewProcess(pDetMipiDeskContext);
        } else {
            alogd("mdetMipiDeskDone, it will exit!\n");
            pthread_exit(NULL);
        }
        usleep(pDetMipiDeskContext->mDetectIntervalMs * 1000);
    }

    return NULL;
}

static int detectMipiDeskewThreadCreate(DetMipiDeskCtrlConfig *pDetMipiDeskContext)
{
    int ret;
    char strThreadName[32];

    pDetMipiDeskContext->mdetMipiDeskDone = false;
    ret = pthread_create(&pDetMipiDeskContext->mdetMipiDeskThreadId, NULL, detectMipiDeskewThread, (void *)pDetMipiDeskContext);
    if (ret < 0) {
        aloge("detectMipiDeskewThreadCreate failed!\n");
        return -1;
    } else {
        snprintf(strThreadName, 32, "detectMipiDeskewThread");
        pthread_setname_np(pDetMipiDeskContext->mdetMipiDeskThreadId, strThreadName);
    }

    return 0;
}

static int parserMipiDeskTestCtrlConfig(DetMipiDeskCtrlConfig *pDetMipiDeskContext)
{
    if (pDetMipiDeskContext->mDetectIntervalMs < 500) {
        alogw("Set mDetectIntervalMs default:1000ms\n");
        pDetMipiDeskContext->mDetectIntervalMs = 1000;
    }

    switch(pDetMipiDeskContext->mMipiDeskCfg.mPayLoadAddr)
    {
        case MIPIA_PAYLOAD_REGS:
        {
            alogd("It will detect MIPIA(0x%x) Deskew\n", MIPIA_PAYLOAD_REGS);
            pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskRegsAddr = MIPIA_PHY_DESKEW_REGS;
            break;
        }
        case MIPIB_PAYLOAD_REGS:
        {
            alogd("It will detect MIPIB(0x%x) Deskew\n", MIPIB_PAYLOAD_REGS);
            pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskRegsAddr = MIPIB_PHY_DESKEW_REGS;
            break;
        }
        default:
        {
            aloge("Invaild mPayLoadAddr, please check it!!!");
            return -1;
        }
    }

    if (pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskValueMax == 0) {
        alogw("Please check mPhyDeskValueMax = 0x%x, it will set default(0x1f)\n", pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskValueMax);
        pDetMipiDeskContext->mMipiDeskCfg.mPhyDeskValueMax = MIPI_PHY_DESKEW_CLK_DLY_MAX;
    }

    return 0;
}

int detectMipiDeskTestInit(DetMipiDeskCtrlConfig *pDetMipiDeskContext)
{
    if (!pDetMipiDeskContext) {
        aloge("pDetMipiDeskContext is NULL, please check it!!!\n");
        return -1;
    }

    if (!pDetMipiDeskContext->mdetMipiDeskEnable) {
        alogw("mdetMipiDeskEnable is flase, it will not to deect!\n");
        return -1;
    }

    if (parserMipiDeskTestCtrlConfig(pDetMipiDeskContext) < 0) {
        return -1;
    }

    if (detectMipiDeskewThreadCreate(pDetMipiDeskContext) < 0) {
        aloge("detectMipiDeskewThreadCreate failed!\n");
    } else {
        pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskStart = 0;
        pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskPass = 0;
        pDetMipiDeskContext->mMipiDeskCfg.mCalcDeskEnd = 0;
        alogd("detectMipiDeskewThreadCreate done\n");
    }

    return 0;
}

int detectMipiDeskTestExit(DetMipiDeskCtrlConfig *pDetMipiDeskContext)
{
    if (!pDetMipiDeskContext) {
        aloge("pDetMipiDeskContext is NULL, please check it!!!\n");
        return -1;
    }

    if (pDetMipiDeskContext->mdetMipiDeskEnable) {
        if (!pDetMipiDeskContext->mdetMipiDeskDone) {
            pthread_cancel(pDetMipiDeskContext->mdetMipiDeskThreadId);
        }
    }
    alogd("detectMipiDeskTestExit done\n");

    return 0;
}
