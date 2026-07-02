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
#include "sample_common_venc.h"
#include "media/mpi_isp.h"
#include <mpi_vi.h>

int configVencRateCtrlParam(const VencRateCtrlConfig *pRcConfig, VENC_CHN_ATTR_S *pVencChnAttr, VENC_RC_PARAM_S *pVencRcParam)
{
    int ret = 0;
    if (NULL == pVencChnAttr || NULL == pVencRcParam || NULL == pRcConfig)
    {
        aloge("fatal error! invalid input params! %p, %p, %p", pVencChnAttr, pVencRcParam, pRcConfig);
        return -1;
    }
    if(PT_H264 == pRcConfig->mEncodeType)
    {
        pVencChnAttr->VeAttr.AttrH264e.Profile = 2;
        pVencChnAttr->VeAttr.AttrH264e.bByFrame = TRUE;
        pVencChnAttr->VeAttr.AttrH264e.PicWidth = pRcConfig->mEncodeWidth;
        pVencChnAttr->VeAttr.AttrH264e.PicHeight = pRcConfig->mEncodeHeight;
        pVencChnAttr->VeAttr.AttrH264e.mLevel = 0;
        pVencChnAttr->VeAttr.AttrH264e.mbPIntraEnable = TRUE;
        pVencChnAttr->VeAttr.AttrH264e.mThreshSize = pRcConfig->mVbvThreshSize;
        pVencChnAttr->VeAttr.AttrH264e.BufSize = pRcConfig->mVbvBufSize;
        pVencChnAttr->RcAttr.mRcMode = pRcConfig->mRcMode;
        switch (pVencChnAttr->RcAttr.mRcMode)
        {
        case VENC_RC_MODE_H264CBR:
            pVencChnAttr->RcAttr.mAttrH264Cbr.mBitRate = pRcConfig->mEncodeBitrate;
            pVencChnAttr->RcAttr.mAttrH264Cbr.mSrcFrmRate = pRcConfig->mSrcFrameRate;
            pVencChnAttr->RcAttr.mAttrH264Cbr.mDstFrmRate = pRcConfig->mDstFrameRate;
            pVencRcParam->ParamH264Cbr.mMaxQp = pRcConfig->mMaxIQp;
            pVencRcParam->ParamH264Cbr.mMinQp = pRcConfig->mMinIQp;
            pVencRcParam->ParamH264Cbr.mMaxPqp = pRcConfig->mMaxPQp;
            pVencRcParam->ParamH264Cbr.mMinPqp = pRcConfig->mMinPQp;
            pVencRcParam->ParamH264Cbr.mQpInit = pRcConfig->mInitQp;
            pVencRcParam->ParamH264Cbr.mbEnMbQpLimit = pRcConfig->mEnMbQpLimit;
            break;
        case VENC_RC_MODE_H264VBR:
            pVencChnAttr->RcAttr.mAttrH264Vbr.mMaxBitRate = pRcConfig->mEncodeBitrate;
            pVencChnAttr->RcAttr.mAttrH264Vbr.mSrcFrmRate = pRcConfig->mSrcFrameRate;
            pVencChnAttr->RcAttr.mAttrH264Vbr.mDstFrmRate = pRcConfig->mDstFrameRate;
            pVencRcParam->ParamH264Vbr.mMaxQp = pRcConfig->mMaxIQp;
            pVencRcParam->ParamH264Vbr.mMinQp = pRcConfig->mMinIQp;
            pVencRcParam->ParamH264Vbr.mMaxPqp = pRcConfig->mMaxPQp;
            pVencRcParam->ParamH264Vbr.mMinPqp = pRcConfig->mMinPQp;
            pVencRcParam->ParamH264Vbr.mQpInit = pRcConfig->mInitQp;
            pVencRcParam->ParamH264Vbr.mbEnMbQpLimit = pRcConfig->mEnMbQpLimit;
            pVencRcParam->ParamH264Vbr.mQuality = pRcConfig->mQuality;
            pVencRcParam->ParamH264Vbr.mMovingTh = pRcConfig->mMovingTh;
            pVencRcParam->ParamH264Vbr.mIFrmBitsCoef = pRcConfig->mIBitsCoef;
            pVencRcParam->ParamH264Vbr.mPFrmBitsCoef = pRcConfig->mPBitsCoef;
            break;
        case VENC_RC_MODE_H264FIXQP:
            pVencChnAttr->RcAttr.mAttrH264FixQp.mIQp = pRcConfig->mMinIQp;
            pVencChnAttr->RcAttr.mAttrH264FixQp.mPQp = pRcConfig->mMinPQp;
            pVencChnAttr->RcAttr.mAttrH264FixQp.mSrcFrmRate = pRcConfig->mSrcFrameRate;
            pVencChnAttr->RcAttr.mAttrH264FixQp.mDstFrmRate = pRcConfig->mDstFrameRate;
            break;
        default:
            aloge("fatal error! H264 rc mode %d is not support!", pRcConfig->mRcMode);
            ret = -1;
            break;
        }
    }
    else if(PT_H265 == pRcConfig->mEncodeType)
    {
        pVencChnAttr->VeAttr.AttrH265e.mProfile = 0;
        pVencChnAttr->VeAttr.AttrH265e.mbByFrame = TRUE;
        pVencChnAttr->VeAttr.AttrH265e.mPicWidth = pRcConfig->mEncodeWidth;
        pVencChnAttr->VeAttr.AttrH265e.mPicHeight = pRcConfig->mEncodeHeight;
        pVencChnAttr->VeAttr.AttrH265e.mLevel = 0;
        pVencChnAttr->VeAttr.AttrH265e.mbPIntraEnable = TRUE;
        pVencChnAttr->VeAttr.AttrH265e.mThreshSize = pRcConfig->mVbvThreshSize;
        pVencChnAttr->VeAttr.AttrH265e.mBufSize = pRcConfig->mVbvBufSize;
        pVencChnAttr->RcAttr.mRcMode = pRcConfig->mRcMode;
        switch (pVencChnAttr->RcAttr.mRcMode)
        {
        case VENC_RC_MODE_H265CBR:
            pVencChnAttr->RcAttr.mAttrH265Cbr.mBitRate = pRcConfig->mEncodeBitrate;
            pVencChnAttr->RcAttr.mAttrH265Cbr.mSrcFrmRate = pRcConfig->mSrcFrameRate;
            pVencChnAttr->RcAttr.mAttrH265Cbr.mDstFrmRate = pRcConfig->mDstFrameRate;
            pVencRcParam->ParamH265Cbr.mMaxQp = pRcConfig->mMaxIQp;
            pVencRcParam->ParamH265Cbr.mMinQp = pRcConfig->mMinIQp;
            pVencRcParam->ParamH265Cbr.mMaxPqp = pRcConfig->mMaxPQp;
            pVencRcParam->ParamH265Cbr.mMinPqp = pRcConfig->mMinPQp;
            pVencRcParam->ParamH265Cbr.mQpInit = pRcConfig->mInitQp;
            pVencRcParam->ParamH265Cbr.mbEnMbQpLimit = pRcConfig->mEnMbQpLimit;
            break;
        case VENC_RC_MODE_H265VBR:
            pVencChnAttr->RcAttr.mAttrH265Vbr.mMaxBitRate = pRcConfig->mEncodeBitrate;
            pVencChnAttr->RcAttr.mAttrH265Vbr.mSrcFrmRate = pRcConfig->mSrcFrameRate;
            pVencChnAttr->RcAttr.mAttrH265Vbr.mDstFrmRate = pRcConfig->mDstFrameRate;
            pVencRcParam->ParamH265Vbr.mMaxQp = pRcConfig->mMaxIQp;
            pVencRcParam->ParamH265Vbr.mMinQp = pRcConfig->mMinIQp;
            pVencRcParam->ParamH265Vbr.mMaxPqp = pRcConfig->mMaxPQp;
            pVencRcParam->ParamH265Vbr.mMinPqp = pRcConfig->mMinPQp;
            pVencRcParam->ParamH265Vbr.mQpInit = pRcConfig->mInitQp;
            pVencRcParam->ParamH265Vbr.mbEnMbQpLimit = pRcConfig->mEnMbQpLimit;
            pVencRcParam->ParamH265Vbr.mQuality = pRcConfig->mQuality;
            pVencRcParam->ParamH265Vbr.mMovingTh = pRcConfig->mMovingTh;
            pVencRcParam->ParamH265Vbr.mIFrmBitsCoef = pRcConfig->mIBitsCoef;
            pVencRcParam->ParamH265Vbr.mPFrmBitsCoef = pRcConfig->mPBitsCoef;
            break;
        case VENC_RC_MODE_H265FIXQP:
            pVencChnAttr->RcAttr.mAttrH265FixQp.mIQp = pRcConfig->mMinIQp;
            pVencChnAttr->RcAttr.mAttrH265FixQp.mPQp = pRcConfig->mMinPQp;
            pVencChnAttr->RcAttr.mAttrH265FixQp.mSrcFrmRate = pRcConfig->mSrcFrameRate;
            pVencChnAttr->RcAttr.mAttrH265FixQp.mDstFrmRate = pRcConfig->mDstFrameRate;
            break;
        default:
            aloge("fatal error! H265 rc mode %d is not support!", pRcConfig->mRcMode);
            ret = -1;
            break;
        }
    }
    else if(PT_MJPEG == pRcConfig->mEncodeType)
    {
        pVencChnAttr->VeAttr.AttrMjpeg.mbByFrame = TRUE;
        pVencChnAttr->VeAttr.AttrMjpeg.mPicWidth= pRcConfig->mEncodeWidth;
        pVencChnAttr->VeAttr.AttrMjpeg.mPicHeight = pRcConfig->mEncodeHeight;
        pVencChnAttr->VeAttr.AttrMjpeg.mThreshSize = pRcConfig->mVbvThreshSize;
        pVencChnAttr->VeAttr.AttrMjpeg.mBufSize = pRcConfig->mVbvBufSize;
        pVencChnAttr->RcAttr.mRcMode = pRcConfig->mRcMode;
        switch (pVencChnAttr->RcAttr.mRcMode)
        {
        case VENC_RC_MODE_MJPEGCBR:
            pVencChnAttr->RcAttr.mAttrMjpegeCbr.mBitRate = pRcConfig->mEncodeBitrate;
            pVencChnAttr->RcAttr.mAttrMjpegeCbr.mSrcFrmRate = pRcConfig->mSrcFrameRate;
            pVencChnAttr->RcAttr.mAttrMjpegeCbr.mDstFrmRate = pRcConfig->mDstFrameRate;
            break;
        case VENC_RC_MODE_MJPEGFIXQP:
            pVencChnAttr->RcAttr.mAttrMjpegeFixQp.mQfactor = 40;
            pVencChnAttr->RcAttr.mAttrMjpegeFixQp.mSrcFrmRate = pRcConfig->mSrcFrameRate;
            pVencChnAttr->RcAttr.mAttrMjpegeFixQp.mDstFrmRate = pRcConfig->mDstFrameRate;
            break;
        default:
            aloge("fatal error! MJPEG rc mode %d is not support!", pRcConfig->mRcMode);
            ret = -1;
            break;
        }
    }

    return ret;
}

void setVenc2Dnr(VENC_CHN mVEncChn, int enable)
{
    s2DfilterParam m2DnrPara;
    memset(&m2DnrPara, 0, sizeof(s2DfilterParam));
    if (enable)
    {
        m2DnrPara.enable_2d_filter = 1;
        m2DnrPara.filter_strength_y = 127;
        m2DnrPara.filter_strength_uv = 127;
        m2DnrPara.filter_th_y = 11;
        m2DnrPara.filter_th_uv = 7;
    }
    else
    {
        m2DnrPara.enable_2d_filter = 0;
    }
    AW_MPI_VENC_Set2DFilter(mVEncChn, &m2DnrPara);
    alogd("VencChn[%d] %s", mVEncChn, enable ? "enable and set 2DFilter param" : "disable 2DFilter");
}

void setVenc3Dnr(VENC_CHN mVEncChn, int enable)
{
    Venc3dFilterParam mE3DParam;
    memset(&mE3DParam, 0, sizeof(Venc3dFilterParam));
    if (enable)
    {
        mE3DParam.e3dEn = 1;
    }
    else
    {
        mE3DParam.e3dEn = 0;
    }
    AW_MPI_VENC_Set3DFilter(mVEncChn, &mE3DParam);
    alogd("VencChn[%d] %s", mVEncChn, enable ? "enable and set 3DFilter param" : "disable 3DFilter");
}

void setVencSuperFrameCfg(VENC_CHN mVEncChn, int mBitrate, int mFramerate)
{
    VENC_SUPERFRAME_CFG_S mSuperFrmParam;
    memset(&mSuperFrmParam, 0, sizeof(VENC_SUPERFRAME_CFG_S));
#if VE_SUPER_FRM_MODE_REENCODE_ENABLE
    mSuperFrmParam.enSuperFrmMode = SUPERFRM_REENCODE;
#else
    mSuperFrmParam.enSuperFrmMode = SUPERFRM_NONE;
#endif
    mSuperFrmParam.MaxRencodeTimes = 1;
    mSuperFrmParam.MaxP2IFrameBitsRatio = 0.33;
    float cmp_bits = 1.5*1024*1024 / 20;
    float dst_bits = (float)mBitrate / mFramerate;
    float bits_ratio = dst_bits / cmp_bits;
    mSuperFrmParam.SuperIFrmBitsThr = (unsigned int)((8.0*250*1024) * bits_ratio);
    mSuperFrmParam.SuperPFrmBitsThr = mSuperFrmParam.SuperIFrmBitsThr / 3;
    alogd("SuperFrm Mode:%d, MaxRencodeTimes:%d, MaxP2IFrameBitsRatio:%.2f, IBitsThr:%d, PBitsThr:%d",
        mSuperFrmParam.enSuperFrmMode, mSuperFrmParam.MaxRencodeTimes, mSuperFrmParam.MaxP2IFrameBitsRatio,
        mSuperFrmParam.SuperIFrmBitsThr, mSuperFrmParam.SuperPFrmBitsThr);
    AW_MPI_VENC_SetSuperFrameCfg(mVEncChn, &mSuperFrmParam);
}

void configBitsClipParam(VENC_RC_PARAM_S *pVEncRcParam)
{
   VencTargetBitsClipParam *pBitsClipParam = &pVEncRcParam->mBitsClipParam;
#if VE_IPC_PRODUCT_ROTATING
    pBitsClipParam->dis_default_para = 1;
    pBitsClipParam->mode = 0;
    pBitsClipParam->coef_th[0][0] = -0.5;
    pBitsClipParam->coef_th[0][1] = -0.2;
    pBitsClipParam->coef_th[1][0] = -0.3;
    pBitsClipParam->coef_th[1][1] = -0.1;
    pBitsClipParam->coef_th[2][0] = -0.3;
    pBitsClipParam->coef_th[2][1] =  0;
    pBitsClipParam->coef_th[3][0] = -0.5;
    pBitsClipParam->coef_th[3][1] =  0.1;
    pBitsClipParam->coef_th[4][0] =  0.4;
    pBitsClipParam->coef_th[4][1] =  0.7;
    pBitsClipParam->en_gop_clip = 0;
    pBitsClipParam->gop_bit_ratio_th[0] = 0;
    pBitsClipParam->gop_bit_ratio_th[1] = 1;
    pBitsClipParam->gop_bit_ratio_th[2] = 2;
#else
    pBitsClipParam->dis_default_para = 1;
    pBitsClipParam->mode = 1;
    pBitsClipParam->coef_th[0][0] = -0.5;
    pBitsClipParam->coef_th[0][1] =  0.2;
    pBitsClipParam->coef_th[1][0] = -0.3;
    pBitsClipParam->coef_th[1][1] =  0.3;
    pBitsClipParam->coef_th[2][0] = -0.3;
    pBitsClipParam->coef_th[2][1] =  0.3;
    pBitsClipParam->coef_th[3][0] = -0.5;
    pBitsClipParam->coef_th[3][1] =  0.5;
    pBitsClipParam->coef_th[4][0] =  0.4;
    pBitsClipParam->coef_th[4][1] =  0.7;
    pBitsClipParam->en_gop_clip = 0;
    pBitsClipParam->gop_bit_ratio_th[0] = 0;
    pBitsClipParam->gop_bit_ratio_th[1] = 1;
    pBitsClipParam->gop_bit_ratio_th[2] = 2;
#endif
    alogd("BitsClipParam: %d %d {%.2f,%.2f}, {%.2f,%.2f}, {%.2f,%.2f}, {%.2f,%.2f}, {%.2f,%.2f}, {%d,%.2f,%.2f,%.2f}",
        pBitsClipParam->dis_default_para, pBitsClipParam->mode,
        pBitsClipParam->coef_th[0][0], pBitsClipParam->coef_th[0][1],
        pBitsClipParam->coef_th[1][0], pBitsClipParam->coef_th[1][1],
        pBitsClipParam->coef_th[2][0], pBitsClipParam->coef_th[2][1],
        pBitsClipParam->coef_th[3][0], pBitsClipParam->coef_th[3][1],
        pBitsClipParam->coef_th[4][0], pBitsClipParam->coef_th[4][1],
        pBitsClipParam->en_gop_clip, pBitsClipParam->gop_bit_ratio_th[0], pBitsClipParam->gop_bit_ratio_th[1], pBitsClipParam->gop_bit_ratio_th[2]);
}

void configIFrmMbRcMoveStatus(VENC_RC_PARAM_S *pVEncRcParam)
{
#if VE_IPC_PRODUCT_ROTATING
    pVEncRcParam->EnIFrmMbRcMoveStatusEnable = 1;
    pVEncRcParam->EnIFrmMbRcMoveStatus = 0;
#else
    pVEncRcParam->EnIFrmMbRcMoveStatusEnable = 1;
    pVEncRcParam->EnIFrmMbRcMoveStatus = 3;
#endif
}

int setVencLensMovingMaxQp(int mVEncChn, int nLensMovingMaxQp)
{
    alogd("VeChn[%d] set LensMovingMaxQp %d", mVEncChn, nLensMovingMaxQp);
    return AW_MPI_VENC_SetLensMovingMaxQp(mVEncChn, nLensMovingMaxQp);
}

void setRcVbrOptParam(int nVEncChn, VENC_RC_PRIORITY rcPriority, VENC_QUALITY_LEVEL rcQualityLevel)
{
    VencVbrOptParam stVbrOptParam;
    memset(&stVbrOptParam, 0, sizeof(VencVbrOptParam));
    AW_MPI_VENC_GetVbrOptParam(nVEncChn, &stVbrOptParam);
    if (stVbrOptParam.sRcPriority != rcPriority)
    {
        alogd("VencChn[%d] change RcPriority %d -> %d", nVEncChn, stVbrOptParam.sRcPriority, rcPriority);
        stVbrOptParam.sRcPriority = rcPriority;
    }
    if (stVbrOptParam.eQualityLevel != rcQualityLevel)
    {
        alogd("VencChn[%d] change RcQualityLevel %d -> %d", nVEncChn, stVbrOptParam.eQualityLevel, rcQualityLevel);
        stVbrOptParam.eQualityLevel = rcQualityLevel;
    }

    AW_MPI_VENC_SetVbrOptParam(nVEncChn, &stVbrOptParam);
}


#if 0
int setIsp2VeLinkageParam(Isp2VeLinkageParam *pIsp2Ve)
{
    BOOL bIspAndVeLinkageEnable = pIsp2Ve->mIspAndVeLinkageEnable;
    BOOL bCameraAdaptiveMovingAndStaticEnable = pIsp2Ve->mCameraAdaptiveMovingAndStaticEnable;
    int nVEncChn = pIsp2Ve->mVEncChn;
    int nVipp = pIsp2Ve->mVipp;
    VencIsp2VeParam *pIsp2VeParam = pIsp2Ve->pIsp2VeParam;
    unsigned int nEncppSharpAttenCoefPer = pIsp2Ve->nEncppSharpAttenCoefPer;
    int ret = 0;

    if (pIsp2VeParam)
    {
        sEncppSharpParam *pSharpParam = &pIsp2VeParam->mSharpParam;
        ISP_DEV mIspDev = 0;
        ret = AW_MPI_VI_GetIspDev(nVipp, &mIspDev);
        if (ret)
        {
            aloge("fatal error, vipp[%d] GetIspDev failed! ret=%d", nVipp, ret);
            return -1;
        }

        struct enc_VencIsp2VeParam mIsp2VeParam;
        memset(&mIsp2VeParam, 0, sizeof(struct enc_VencIsp2VeParam));
        mIsp2VeParam.AeStatsInfo = (struct isp_ae_stats_s *)&pIsp2VeParam->mIspAeStatus;
        ret = AW_MPI_ISP_GetIsp2VeParam(mIspDev, &mIsp2VeParam);
        if (ret)
        {
            aloge("fatal error, isp[%d] GetIsp2VeParam failed! ret=%d", mIspDev, ret);
            return -1;
        }

        if (0 == pIsp2Ve->mEncppSharpDebugDisable)
        {
            if (mIsp2VeParam.encpp_en)
            {
                VENC_CHN_ATTR_S stVencAttr;
                memset(&stVencAttr, 0, sizeof(VENC_CHN_ATTR_S));
                AW_MPI_VENC_GetChnAttr(nVEncChn, &stVencAttr);
                if (FALSE == stVencAttr.EncppAttr.mbEncppEnable)
                {
                    if (stVencAttr.VeAttr.mOnlineEnable && stVencAttr.VeAttr.mOnlineShareBufNum == 1 && !checkIsAwLbcFormat(stVencAttr.VeAttr.PixelFormat))
                    {
                        alogv("VencChn[%d] online(%d)(%dbuf) no-lbc-pix(%d) forbid encpp-sharp!", nVEncChn, stVencAttr.VeAttr.mOnlineEnable,
                            stVencAttr.VeAttr.mOnlineShareBufNum, stVencAttr.VeAttr.PixelFormat);
                    }
                    else
                    {
                        stVencAttr.EncppAttr.mbEncppEnable = TRUE;
                        AW_MPI_VENC_SetChnAttr(nVEncChn, &stVencAttr);
                    }
                }
                if (100 != nEncppSharpAttenCoefPer)
                {
                    mIsp2VeParam.mDynamicSharpCfg.ss_blk_stren = mIsp2VeParam.mDynamicSharpCfg.ss_blk_stren * nEncppSharpAttenCoefPer / 100;
                    mIsp2VeParam.mDynamicSharpCfg.ss_wht_stren = mIsp2VeParam.mDynamicSharpCfg.ss_wht_stren * nEncppSharpAttenCoefPer / 100;
                    mIsp2VeParam.mDynamicSharpCfg.ls_blk_stren = mIsp2VeParam.mDynamicSharpCfg.ls_blk_stren * nEncppSharpAttenCoefPer / 100;
                    mIsp2VeParam.mDynamicSharpCfg.ls_wht_stren = mIsp2VeParam.mDynamicSharpCfg.ls_wht_stren * nEncppSharpAttenCoefPer / 100;
                }
                memcpy(&pSharpParam->mDynamicParam, &mIsp2VeParam.mDynamicSharpCfg,sizeof(sEncppSharpParamDynamic));
                memcpy(&pSharpParam->mStaticParam, &mIsp2VeParam.mStaticSharpCfg, sizeof(sEncppSharpParamStatic));
            }
            else
            {
                VENC_CHN_ATTR_S stVencAttr;
                memset(&stVencAttr, 0, sizeof(VENC_CHN_ATTR_S));
                AW_MPI_VENC_GetChnAttr(nVEncChn, &stVencAttr);
                if (TRUE == stVencAttr.EncppAttr.mbEncppEnable)
                {
                    stVencAttr.EncppAttr.mbEncppEnable = FALSE;
                    AW_MPI_VENC_SetChnAttr(nVEncChn, &stVencAttr);
                }
            }
        }

        if (bIspAndVeLinkageEnable)
        {
            pIsp2VeParam->mEnvLv = AW_MPI_ISP_GetEnvLV(mIspDev);
            pIsp2VeParam->mAeWeightLum = AW_MPI_ISP_GetAeWeightLum(mIspDev);
            if (bCameraAdaptiveMovingAndStaticEnable)
            {
                pIsp2VeParam->mEnCameraMove = CAMERA_ADAPTIVE_MOVING_AND_STATIC;
            }
            else
            {
                /**
                Example code:
                static int lens_start_moving_flag = 1; // obtain motor motion status from motor driver.
                if (lens_start_moving_flag)
                {
                    pIsp2VeParam->mEnCameraMove = CAMERA_FORCE_MOVING;
                    lens_start_moving_flag = 0;
                }
                else
                {
                    pIsp2VeParam->mEnCameraMove = CAMERA_ADAPTIVE_STATIC;
                }
                */
                pIsp2VeParam->mEnCameraMove = CAMERA_ADAPTIVE_STATIC;
            }
        }
    }

    return ret;
}

int setVe2IspLinkageParam(Ve2IspLinkageParam *pVe2Isp)
{
    BOOL bIspAndVeLinkageEnable = pVe2Isp->mIspAndVeLinkageEnable;
    int nVEncChn = pVe2Isp->mVEncChn;
    int nVipp = pVe2Isp->mVipp;
    VencVe2IspParam *pVe2IspParam = (VencVe2IspParam *)(pVe2Isp->p2Ve2IspParam);
    int ret = 0;

    if (pVe2IspParam && bIspAndVeLinkageEnable)
    {
        ISP_DEV mIspDev = 0;
        ret = AW_MPI_VI_GetIspDev(nVipp, &mIspDev);
        if (ret)
        {
            aloge("fatal error, vipp[%d] GetIspDev failed! ret=%d", nVipp, ret);
            return -1;
        }
        alogv("update Ve2IspParam, route Isp[%d]-Vipp[%d]-VencChn[%d]", mIspDev, nVipp, nVEncChn);

        alogv("isp[%d] d2d_level=%d, d3d_level=%d, is_overflow=%d", mIspDev, pVe2IspParam->d2d_level, pVe2IspParam->d3d_level, pVe2IspParam->mMovingLevelInfo.is_overflow);
        struct enc_VencVe2IspParam mIspParam;
        memset(&mIspParam, 0, sizeof(struct enc_VencVe2IspParam));
        mIspParam.d2d_level = pVe2IspParam->d2d_level;
        mIspParam.d3d_level = pVe2IspParam->d3d_level;
        memcpy(&mIspParam.mMovingLevelInfo, &pVe2IspParam->mMovingLevelInfo, sizeof(MovingLevelInfo));
        ret = AW_MPI_ISP_SetVe2IspParam(mIspDev, &mIspParam);
        if (ret)
        {
            aloge("fatal error, isp[%d] SetVe2IspParam failed! ret=%d", mIspDev, ret);
            return -1;
        }
    }

    return ret;
}
#endif
