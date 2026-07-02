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
#ifndef __SAMPLE_COMMON_VENC_H__
#define __SAMPLE_COMMON_VENC_H__

#include "media/mpi_venc.h"
#include "mm_common.h"
#include "mm_comm_venc.h"
#include "mm_comm_rc.h"
#include "mm_comm_region.h"

#ifdef __cplusplus
       extern "C" {
#endif

#define VE_VBV_CACHE_TIME                   (4)  // unit:seconds, exp:1,2,3,4...
#define VE_SUPER_FRM_MODE_REENCODE_ENABLE   (0)  // 0:disable reencode, 1:enable reencode
#define VE_IPC_PRODUCT_ROTATING             (0)  // 0:No rotation(qiang ji ...), 1:Rotating(qiu ji, yun tai ...)

typedef struct VencRateCtrlConfig {
    PAYLOAD_TYPE_E mEncodeType;
    VENC_RC_MODE_E mRcMode;
    int mInitQp;
    int mMinIQp;
    int mMaxIQp;
    int mMinPQp;
    int mMaxPQp;
    int mEnMbQpLimit;
    int mMovingTh;
    int mQuality;
    int mPBitsCoef;
    int mIBitsCoef;
    int mEncodeWidth;
    int mEncodeHeight;
    int mSrcFrameRate;
    int mDstFrameRate;
    int mEncodeBitrate;
    unsigned int mVbvThreshSize;
    unsigned int mVbvBufSize;
}VencRateCtrlConfig;

/*
typedef struct Isp2VeLinkageParam {
    BOOL mIspAndVeLinkageEnable;
    BOOL mCameraAdaptiveMovingAndStaticEnable;
    int mVEncChn;
    int mVipp;
    VencIsp2VeParam *pIsp2VeParam;
    unsigned int nEncppSharpAttenCoefPer;
    int mEncppSharpDebugDisable;
} Isp2VeLinkageParam;

typedef struct Ve2IspLinkageParam {
    BOOL mIspAndVeLinkageEnable;
    int mVEncChn;
    int mVipp;
    VencVe2IspParam *p2Ve2IspParam;
} Ve2IspLinkageParam;
*/
int configVencRateCtrlParam(const VencRateCtrlConfig *pRcConfig, VENC_CHN_ATTR_S *pVencChnAttr, VENC_RC_PARAM_S *pVencRcParam);
void setVenc2Dnr(VENC_CHN mVEncChn, int enable);
void setVenc3Dnr(VENC_CHN mVEncChn, int enable);
void setVencSuperFrameCfg(VENC_CHN mVEncChn, int mBitrate, int mFramerate);
void configBitsClipParam(VENC_RC_PARAM_S *pVEncRcParam);
void configIFrmMbRcMoveStatus(VENC_RC_PARAM_S *pVEncRcParam);
int setVencLensMovingMaxQp(int mVEncChn, int nLensMovingMaxQp);
void setRcVbrOptParam(int nVEncChn, VENC_RC_PRIORITY rcPriority, VENC_QUALITY_LEVEL rcQualityLevel);
//int setIsp2VeLinkageParam(Isp2VeLinkageParam *pIsp2Ve);
//int setVe2IspLinkageParam(Ve2IspLinkageParam *pVe2Isp);

#ifdef __cplusplus
       }
#endif

#endif
