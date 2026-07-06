/******************************************************************************
  Copyright (C), 2001-2017, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : sample_OnlineVenc.c
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2017/1/5
  Last Modified :
  Description   : mpp component implement
  Function List :
  History       :
******************************************************************************/

//#define LOG_NDEBUG 0
#define LOG_TAG "sample_rtsp"

#include <utils/plat_log.h>
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
#include <sys/prctl.h>
#include <sys/time.h>

#include "media/mm_comm_vi.h"
#include "media/mpi_vi.h"
#include "media/mpi_isp.h"
#include "media/mpi_venc.h"
#include "media/mpi_sys.h"
#include "mm_common.h"
#include "mm_comm_venc.h"
#include "mm_comm_rc.h"
#include <mpi_videoformat_conversion.h>
#include <SystemBase.h>

#include <confparser.h>
#include "sample_rtsp.h"
#include "sample_rtsp_config.h"
#include "../common/rtsp_server.h"
#include "../common/sample_common_venc.h"


#define SUPPORT_RTSP_TEST

static SampleRtspContext *gpSampleRtspContext = NULL;

static unsigned int getSysTickMs()
{
    unsigned int ms = 0;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    ms = tv.tv_sec*1000 + tv.tv_usec/1000;
    return ms;
}

static void GenerateARGB1555(void* pBuf, int nSize)
{
    unsigned short nA0Color = 0x7C00;
    unsigned short nA1Color = 0xFC00;
    unsigned short *pShort = (unsigned short*)pBuf;

    int i = 0;
    for(i=0; i< nSize/4; i++)
    {
        *pShort = nA0Color;
        pShort++;
    }
    for(i=0; i< nSize/4; i++)
    {
        *pShort = nA1Color;
        pShort++;
    }
}

static void handle_exit(int signo)
{
    alogd("user want to exit!");
    if(NULL != gpSampleRtspContext)
    {
        cdx_sem_up(&gpSampleRtspContext->mSemExit);
    }
}

static int ParseCmdLine(int argc, char **argv, SampleRtspCmdLineParam *pCmdLinePara)
{
    alogd("sample virvi2venc path:[%s], arg number is [%d]", argv[0], argc);
    int ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(SampleRtspCmdLineParam));
    while(i < argc)
    {
        if(!strcmp(argv[i], "-path"))
        {
            if(++i >= argc)
            {
                aloge("fatal error! use -h to learn how to set parameter!!!");
                ret = -1;
                break;
            }
            if(strlen(argv[i]) >= MAX_FILE_PATH_SIZE)
            {
                aloge("fatal error! file path[%s] too long: [%d]>=[%d]!", argv[i], strlen(argv[i]), MAX_FILE_PATH_SIZE);
            }
            strncpy(pCmdLinePara->mConfigFilePath, argv[i], MAX_FILE_PATH_SIZE-1);
            pCmdLinePara->mConfigFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
        }
        else if(!strcmp(argv[i], "-h"))
        {
            alogd("CmdLine param:\n"
                "\t-path /home/sample_OnlineVenc.conf\n");
            ret = 1;
            break;
        }
        else
        {
            alogd("ignore invalid CmdLine param:[%s], type -h to get how to set parameter!", argv[i]);
        }
        i++;
    }
    return ret;
}

static PIXEL_FORMAT_E getPicFormatFromConfig(CONFPARSER_S *pConfParser, const char *key)
{
    PIXEL_FORMAT_E PicFormat = MM_PIXEL_FORMAT_BUTT;
    char *pStrPixelFormat = (char*)GetConfParaString(pConfParser, key, NULL);

    if (!strcmp(pStrPixelFormat, "nv21"))
    {
        PicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }
    else if (!strcmp(pStrPixelFormat, "yv12"))
    {
        PicFormat = MM_PIXEL_FORMAT_YVU_PLANAR_420;
    }
    else if (!strcmp(pStrPixelFormat, "nv12"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    }
    else if (!strcmp(pStrPixelFormat, "yu12"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_PLANAR_420;
    }
    else if (!strcmp(pStrPixelFormat, "aw_lbc_2_0x"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_AW_LBC_2_0X;
    }
    else if (!strcmp(pStrPixelFormat, "aw_lbc_2_5x"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_AW_LBC_2_5X;
    }
    else if (!strcmp(pStrPixelFormat, "aw_lbc_1_5x"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_AW_LBC_1_5X;
    }
    else if (!strcmp(pStrPixelFormat, "aw_lbc_1_0x"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_AW_LBC_1_0X;
    }
    else
    {
        aloge("fatal error! conf file pic_format is [%s]?", pStrPixelFormat);
        PicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }

    return PicFormat;
}

static PAYLOAD_TYPE_E getEncoderTypeFromConfig(CONFPARSER_S *pConfParser, const char *key)
{
    PAYLOAD_TYPE_E EncType = PT_BUTT;
    char *ptr = (char *)GetConfParaString(pConfParser, key, NULL);
    if(!strcmp(ptr, "H.264"))
    {
        EncType = PT_H264;
    }
    else if(!strcmp(ptr, "H.265"))
    {
        EncType = PT_H265;
    }
    else
    {
        aloge("fatal error! conf file encoder[%s] is unsupported", ptr);
        EncType = PT_H264;
    }

    return EncType;
}

static ERRORTYPE loadSampleConfig(SampleRtspConfig *pConfig, const char *conf_path)
{
    if (NULL == pConfig)
    {
        aloge("fatal error, pConfig is NULL!");
        return FAILURE;
    }

    if (NULL != conf_path)
    {
        char *ptr = NULL;
        CONFPARSER_S stConfParser;
        int ret = createConfParser(conf_path, &stConfParser);
        if(ret < 0)
        {
            aloge("fatal error, load conf failed!");
            return FAILURE;
        }

        // main stream
        pConfig->mMainIsp = GetConfParaInt(&stConfParser, CFG_MainIsp, 0);
        pConfig->mMainVipp = GetConfParaInt(&stConfParser, CFG_MainVipp, 0);
        pConfig->mMainViChn = GetConfParaInt(&stConfParser, CFG_MainViChn, 0);
        pConfig->mMainSrcWidth = GetConfParaInt(&stConfParser, CFG_MainSrcWidth, 0);
        pConfig->mMainSrcHeight = GetConfParaInt(&stConfParser, CFG_MainSrcHeight, 0);
        pConfig->mMainPixelFormat = getPicFormatFromConfig(&stConfParser, CFG_MainPixelFormat);
        pConfig->mMainWdrEnable = GetConfParaInt(&stConfParser, CFG_MainWdrEnable, 0);
        pConfig->mMainViBufNum = GetConfParaInt(&stConfParser, CFG_MainViBufNum, 0);
        pConfig->mMainSrcFrameRate = GetConfParaInt(&stConfParser, CFG_MainSrcFrameRate, 0);
        pConfig->mMainVEncChn = GetConfParaInt(&stConfParser, CFG_MainVEncChn, 0);
        pConfig->mMainEncodeType = getEncoderTypeFromConfig(&stConfParser, CFG_MainEncodeType);
        pConfig->mMainEncodeWidth = GetConfParaInt(&stConfParser, CFG_MainEncodeWidth, 0);
        pConfig->mMainEncodeHeight = GetConfParaInt(&stConfParser, CFG_MainEncodeHeight, 0);
        pConfig->mMainEncodeFrameRate = GetConfParaInt(&stConfParser, CFG_MainEncodeFrameRate, 0);
        pConfig->mMainEncodeBitrate = GetConfParaInt(&stConfParser, CFG_MainEncodeBitrate, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_MainFilePathh, NULL);
        strncpy(pConfig->mMainFilePath, ptr, MAX_FILE_PATH_SIZE);
        pConfig->mMainOnlineEnable = GetConfParaInt(&stConfParser, CFG_MainOnlineEnable, 0);
        pConfig->mMainOnlineShareBufNum = GetConfParaInt(&stConfParser, CFG_MainOnlineShareBufNum, 0);
        pConfig->mMainEncppEnable = GetConfParaInt(&stConfParser, CFG_MainEncppEnable, 0);

        // sub stream
        pConfig->mSubIsp = GetConfParaInt(&stConfParser, CFG_SubIsp, 0);
        pConfig->mSubVipp = GetConfParaInt(&stConfParser, CFG_SubVipp, 0);
        pConfig->mSubSrcWidth = GetConfParaInt(&stConfParser, CFG_SubSrcWidth, 0);
        pConfig->mSubSrcHeight = GetConfParaInt(&stConfParser, CFG_SubSrcHeight, 0);
        pConfig->mSubPixelFormat = getPicFormatFromConfig(&stConfParser, CFG_SubPixelFormat);
        pConfig->mSubWdrEnable = GetConfParaInt(&stConfParser, CFG_SubWdrEnable, 0);
        pConfig->mSubViBufNum = GetConfParaInt(&stConfParser, CFG_SubViBufNum, 0);
        pConfig->mSubSrcFrameRate = GetConfParaInt(&stConfParser, CFG_SubSrcFrameRate, 0);

        pConfig->mSubVippCropEnable = GetConfParaInt(&stConfParser, CFG_SubVippCropEnable, 0);
        pConfig->mSubVippCropRectX = GetConfParaInt(&stConfParser, CFG_SubVippCropRectX, 0);
        pConfig->mSubVippCropRectY = GetConfParaInt(&stConfParser, CFG_SubVippCropRectY, 0);
        pConfig->mSubVippCropRectWidth = GetConfParaInt(&stConfParser, CFG_SubVippCropRectWidth, 0);
        pConfig->mSubVippCropRectHeight = GetConfParaInt(&stConfParser, CFG_SubVippCropRectHeight, 0);

        pConfig->mSubViChn = GetConfParaInt(&stConfParser, CFG_SubViChn, 0);
        pConfig->mSubVEncChn = GetConfParaInt(&stConfParser, CFG_SubVEncChn, 0);
        pConfig->mSubEncodeType = getEncoderTypeFromConfig(&stConfParser, CFG_SubEncodeType);
        pConfig->mSubEncodeWidth = GetConfParaInt(&stConfParser, CFG_SubEncodeWidth, 0);
        pConfig->mSubEncodeHeight = GetConfParaInt(&stConfParser, CFG_SubEncodeHeight, 0);
        pConfig->mSubEncodeFrameRate = GetConfParaInt(&stConfParser, CFG_SubEncodeFrameRate, 0);
        pConfig->mSubEncodeBitrate = GetConfParaInt(&stConfParser, CFG_SubEncodeBitrate, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_SubFilePath, NULL);
        strncpy(pConfig->mSubFilePath, ptr, MAX_FILE_PATH_SIZE);
        pConfig->mSubEncppSharpAttenCoefPer = 100 * pConfig->mSubSrcWidth / pConfig->mMainSrcWidth;
        pConfig->mSubOnlineEnable = GetConfParaInt(&stConfParser, CFG_SubOnlineEnable, 0);
        pConfig->mSubOnlineShareBufNum = GetConfParaInt(&stConfParser, CFG_SubOnlineShareBufNum, 0);
        pConfig->mSubEncppEnable = GetConfParaInt(&stConfParser, CFG_SubEncppEnable, 0);

        // sub lapse stream
        pConfig->mSubLapseVipp = pConfig->mSubVipp;
        pConfig->mSubLapseSrcWidth = pConfig->mSubSrcWidth;
        pConfig->mSubLapseSrcHeight = pConfig->mSubSrcHeight;
        pConfig->mSubLapsePixelFormat = pConfig->mSubPixelFormat;
        pConfig->mSubLapseWdrEnable = pConfig->mSubWdrEnable;
        pConfig->mSubLapseViBufNum = pConfig->mSubViBufNum;
        pConfig->mSubLapseSrcFrameRate = pConfig->mSubSrcFrameRate;
        pConfig->mSubLapseViChn = GetConfParaInt(&stConfParser, CFG_SubLapseViChn, 0);
        pConfig->mSubLapseVEncChn = GetConfParaInt(&stConfParser, CFG_SubLapseVEncChn, 0);
        pConfig->mSubLapseEncodeType = getEncoderTypeFromConfig(&stConfParser, CFG_SubLapseEncodeType);
        pConfig->mSubLapseEncodeWidth = GetConfParaInt(&stConfParser, CFG_SubLapseEncodeWidth, 0);
        pConfig->mSubLapseEncodeHeight = GetConfParaInt(&stConfParser, CFG_SubLapseEncodeHeight, 0);
        pConfig->mSubLapseEncodeFrameRate = GetConfParaInt(&stConfParser, CFG_SubLapseEncodeFrameRate, 0);
        pConfig->mSubLapseEncodeBitrate = GetConfParaInt(&stConfParser, CFG_SubLapseEncodeBitrate, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_SubLapseFilePath, NULL);
        strncpy(pConfig->mSubLapseFilePath, ptr, MAX_FILE_PATH_SIZE);
        pConfig->mSubLapseEncppSharpAttenCoefPer = 100 * pConfig->mSubLapseSrcWidth / pConfig->mMainSrcWidth;
        pConfig->mSubLapseTime = GetConfParaInt(&stConfParser, CFG_SubLapseTime, 0);
        pConfig->mSubLapseEncppEnable = GetConfParaInt(&stConfParser, CFG_SubLapseEncppEnable, 0);

        // isp and ve linkage
        pConfig->mIspAndVeLinkageEnable = GetConfParaInt(&stConfParser, CFG_IspAndVeLinkageEnable, 0);
        pConfig->mCameraAdaptiveMovingAndStaticEnable = GetConfParaInt(&stConfParser, CFG_CameraAdaptiveMovingAndStaticEnable, 0);
        pConfig->mVencLensMovingMaxQp = GetConfParaInt(&stConfParser, CFG_VencLensMovingMaxQp, 0);
        alogd("IspAndVeLinkageEn:%d, AdaptEn:%d, LensMoveMaxQp:%d", pConfig->mIspAndVeLinkageEnable,
            pConfig->mCameraAdaptiveMovingAndStaticEnable, pConfig->mVencLensMovingMaxQp);

        // wb yuv
        pConfig->mWbYuvEnable = GetConfParaInt(&stConfParser, CFG_WbYuvEnable, 0);
        pConfig->mWbYuvBufNum = GetConfParaInt(&stConfParser, CFG_WbYuvBufNum, 0);
        pConfig->mWbYuvScalerRatio = GetConfParaInt(&stConfParser, CFG_WbYuvScalerRatio, 0);
        pConfig->mWbYuvStartIndex = GetConfParaInt(&stConfParser, CFG_WbYuvStartIndex, 0);
        pConfig->mWbYuvTotalCnt = GetConfParaInt(&stConfParser, CFG_WbYuvTotalCnt, 0);
        pConfig->mWbYuvStreamChn = GetConfParaInt(&stConfParser, CFG_WbYuvStreamChannel, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_WbYuvFilePath, NULL);
        strncpy(pConfig->mWbYuvFilePath, ptr, MAX_FILE_PATH_SIZE);

        // rtsp
        pConfig->mRtspNetType = GetConfParaInt(&stConfParser, CFG_RtspNetType, 0);
        pConfig->mStreamBufSize = GetConfParaInt(&stConfParser, CFG_StreamBufSize, 0);

        // others
        pConfig->mTestDuration = GetConfParaInt(&stConfParser, CFG_TestDuration, 0);

        destroyConfParser(&stConfParser);
    }
    else
    {
        alogw("user not set config file, use default configs.");
    }

    return SUCCESS;
}

static ERRORTYPE MPPCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    SampleRtspContext *pContext = (SampleRtspContext *)cookie;
    ERRORTYPE ret = 0;

    if (MOD_ID_VENC == pChn->mModId)
    {
        VENC_CHN mVEncChn = pChn->mChnId;
        VencStreamContext *pStreamContext = NULL;
        switch(mVEncChn)
        {
            case VENC_CHN_MAIN_STREAM:
            {
                pStreamContext = &pContext->mMainStream;
                break;
            }
            case VENC_CHN_SUB_STREAM:
            {
                pStreamContext = &pContext->mSubStream;
                break;
            }
            case VENC_CHN_SUB_LAPSE_STREAM:
            {
                pStreamContext = &pContext->mSubLapseStream;
                break;
            }
            default:
            {
                aloge("fatal error! invalid venc chn %d", mVEncChn);
                return FAILURE;
            }
        }

        switch(event)
        {
            /*case MPP_EVENT_LINKAGE_ISP2VE_PARAM:
            {
                Isp2VeLinkageParam stIsp2Ve;
                memset(&stIsp2Ve, 0, sizeof(Isp2VeLinkageParam));
                stIsp2Ve.mIspAndVeLinkageEnable = pContext->mConfigPara.mIspAndVeLinkageEnable;
                stIsp2Ve.mCameraAdaptiveMovingAndStaticEnable = pContext->mConfigPara.mCameraAdaptiveMovingAndStaticEnable;
                stIsp2Ve.mVEncChn = mVEncChn;
                stIsp2Ve.mVipp = pStreamContext->mVipp;
                stIsp2Ve.pIsp2VeParam = (VencIsp2VeParam *)pEventData;
                stIsp2Ve.nEncppSharpAttenCoefPer = pStreamContext->mEncppSharpAttenCoefPer;
                int ret = setIsp2VeLinkageParam(&stIsp2Ve);
                if (ret)
                {
                    aloge("fatal error, VEncChn[%d] set Isp2VeLinkageParam failed! ret=%d", mVEncChn, ret);
                    return -1;
                }
                break;
            }
            case MPP_EVENT_LINKAGE_VE2ISP_PARAM:
            {
                Ve2IspLinkageParam stVe2Isp;
                memset(&stVe2Isp, 0, sizeof(Ve2IspLinkageParam));
                stVe2Isp.mIspAndVeLinkageEnable = pContext->mConfigPara.mIspAndVeLinkageEnable;
                stVe2Isp.mVEncChn = mVEncChn;
                stVe2Isp.mVipp = pStreamContext->mVipp;
                stVe2Isp.p2Ve2IspParam = (VencVe2IspParam *)pEventData;
                int ret = setVe2IspLinkageParam(&stVe2Isp);
                if (ret)
                {
                    aloge("fatal error, VEncChn[%d] set Ve2IspLinkageParam failed! ret=%d", mVEncChn, ret);
                    return -1;
                }
                break;
            }*/
            case MPP_EVENT_LINKAGE_ISP2VE_PARAM_EXTRA:
            {
                VENC_Isp2VeExtraParam *pExtraParam = (VENC_Isp2VeExtraParam *)pEventData;
                if (pContext->mConfigPara.mCameraAdaptiveMovingAndStaticEnable)
                {
                    pExtraParam->eEnCameraMove = CAMERA_ADAPTIVE_MOVING_AND_STATIC;
                }
                else
                {
                    pExtraParam->eEnCameraMove = CAMERA_ADAPTIVE_STATIC;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
    return SUCCESS;
}

static void configMainStream(SampleRtspContext *pContext)
{
    pContext->mMainStream.priv = (void*)pContext;
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;

    pContext->mMainStream.mIsp = pContext->mConfigPara.mMainIsp;
    pContext->mMainStream.mVipp = pContext->mConfigPara.mMainVipp;
    pContext->mMainStream.mViChn = pContext->mConfigPara.mMainViChn;
    pContext->mMainStream.mVEncChn = pContext->mConfigPara.mMainVEncChn;

    if (pContext->mConfigPara.mMainOnlineEnable)
    {
        pContext->mMainStream.mViAttr.mOnlineEnable = 1;
        pContext->mMainStream.mViAttr.mOnlineShareBufNum = pContext->mConfigPara.mMainOnlineShareBufNum;
        pContext->mMainStream.mVEncChnAttr.VeAttr.mOnlineEnable = 1;
        pContext->mMainStream.mVEncChnAttr.VeAttr.mOnlineShareBufNum = pContext->mConfigPara.mMainOnlineShareBufNum;
    }
    alogd("main vipp%d ve_online_en:%d, dma_buf_num:%d, venc ch%d OnlineEnable:%d, OnlineShareBufNum:%d",
        pContext->mMainStream.mVipp, pContext->mMainStream.mViAttr.mOnlineEnable, pContext->mMainStream.mViAttr.mOnlineShareBufNum,
        pContext->mMainStream.mVEncChn, pContext->mMainStream.mVEncChnAttr.VeAttr.mOnlineEnable, pContext->mMainStream.mVEncChnAttr.VeAttr.mOnlineShareBufNum);

    pContext->mMainStream.mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pContext->mMainStream.mViAttr.memtype = V4L2_MEMORY_MMAP;
    pContext->mMainStream.mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pContext->mConfigPara.mMainPixelFormat);
    pContext->mMainStream.mViAttr.format.field = V4L2_FIELD_NONE;
    pContext->mMainStream.mViAttr.format.colorspace = V4L2_COLORSPACE_REC709_PART_RANGE;
    pContext->mMainStream.mViAttr.format.width = pContext->mConfigPara.mMainSrcWidth;
    pContext->mMainStream.mViAttr.format.height = pContext->mConfigPara.mMainSrcHeight;
    pContext->mMainStream.mViAttr.fps = pContext->mConfigPara.mMainSrcFrameRate;
    pContext->mMainStream.mViAttr.use_current_win = 0;
    pContext->mMainStream.mViAttr.nbufs = pContext->mConfigPara.mMainViBufNum;
    pContext->mMainStream.mViAttr.nplanes = 2;
    pContext->mMainStream.mViAttr.wdr_mode = pContext->mConfigPara.mMainWdrEnable;
    pContext->mMainStream.mViAttr.drop_frame_num = 10;

    pContext->mMainStream.mVEncChnAttr.VeAttr.mVippID = pContext->mConfigPara.mMainVipp;
    pContext->mMainStream.mVEncChnAttr.VeAttr.Type = pContext->mConfigPara.mMainEncodeType;
    pContext->mMainStream.mVEncChnAttr.VeAttr.MaxKeyInterval = 100;//40;
    pContext->mMainStream.mVEncChnAttr.VeAttr.SrcPicWidth = pContext->mConfigPara.mMainSrcWidth;
    pContext->mMainStream.mVEncChnAttr.VeAttr.SrcPicHeight = pContext->mConfigPara.mMainSrcHeight;
    pContext->mMainStream.mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pContext->mMainStream.mVEncChnAttr.VeAttr.PixelFormat = pContext->mConfigPara.mMainPixelFormat;
    pContext->mMainStream.mVEncChnAttr.VeAttr.mColorSpace = pContext->mMainStream.mViAttr.format.colorspace;
    pContext->mMainStream.mVEncChnAttr.RcAttr.mProductMode = PRODUCT_STATIC_IPC;
    //pContext->mMainStream.mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

    pContext->mMainStream.mEncppSharpAttenCoefPer = 100;
    alogd("main EncppSharpAttenCoefPer: %d%%", pContext->mMainStream.mEncppSharpAttenCoefPer);

    pContext->mMainStream.mVEncChnAttr.EncppAttr.eEncppSharpSetting = pContext->mConfigPara.mMainEncppEnable?VencEncppSharp_FollowISPConfig:VencEncppSharp_Disable;

    if (pContext->mConfigPara.mMainSrcFrameRate)
    {
        vbvThreshSize = pContext->mConfigPara.mMainEncodeBitrate/8/pContext->mConfigPara.mMainSrcFrameRate*15;
    }
    vbvBufSize = pContext->mConfigPara.mMainEncodeBitrate/8*4 + vbvThreshSize;
    alogd("main vbvThreshSize: %d, vbvBufSize: %d", vbvThreshSize, vbvBufSize);

    if(PT_H264 == pContext->mMainStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.Profile = 1;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.PicWidth = pContext->mConfigPara.mMainEncodeWidth;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.PicHeight = pContext->mConfigPara.mMainEncodeHeight;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.mLevel = 0;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.mbPIntraEnable = TRUE;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.mThreshSize = vbvThreshSize;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.BufSize = vbvBufSize;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264VBR;
        if (pContext->mMainStream.mVEncChnAttr.RcAttr.mRcMode == VENC_RC_MODE_H264VBR)
        {
            pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mMaxBitRate = pContext->mConfigPara.mMainEncodeBitrate;
            pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mSrcFrmRate = pContext->mConfigPara.mMainSrcFrameRate;
            pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mDstFrmRate = pContext->mConfigPara.mMainEncodeFrameRate;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mMaxQp = 50;//45;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mMinQp = 10;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mMaxPqp = 50;//45;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mMinPqp = 10;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mQpInit = 37;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mbEnMbQpLimit = 0;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mMovingTh = 20;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mQuality = 10;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mIFrmBitsCoef = 10;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mPFrmBitsCoef = 10;
        }
        else if (pContext->mMainStream.mVEncChnAttr.RcAttr.mRcMode == VENC_RC_MODE_H264CBR)
        {
            pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH264Cbr.mBitRate = pContext->mConfigPara.mMainEncodeBitrate;
            pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH264Cbr.mSrcFrmRate = pContext->mConfigPara.mMainSrcFrameRate;
            pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH264Cbr.mDstFrmRate = pContext->mConfigPara.mMainEncodeFrameRate;
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mMaxQp = 50;
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mMinQp = 10;
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mMaxPqp = 50;
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mMinPqp = 10;
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mQpInit = 37;
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mbEnMbQpLimit = 0;
        }
    }
    else if(PT_H265 == pContext->mMainStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mProfile = 0;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mbByFrame = TRUE;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mPicWidth = pContext->mConfigPara.mMainEncodeWidth;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mPicHeight = pContext->mConfigPara.mMainEncodeHeight;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mLevel = 0;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mbPIntraEnable = TRUE;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mThreshSize = vbvThreshSize;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mBufSize = vbvBufSize;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265VBR;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH265Vbr.mMaxBitRate = pContext->mConfigPara.mMainEncodeBitrate;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH265Vbr.mSrcFrmRate = pContext->mConfigPara.mMainSrcFrameRate;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH265Vbr.mDstFrmRate = pContext->mConfigPara.mMainEncodeFrameRate;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mMaxQp = 50;//45;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mMinQp = 10;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mMaxPqp = 50;//45;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mMinPqp = 10;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mQpInit = 37;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mbEnMbQpLimit = 0;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mMovingTh = 20;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mQuality = 10;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mIFrmBitsCoef = 10;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mPFrmBitsCoef = 10;
    }
    else if(PT_MJPEG == pContext->mMainStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrMjpeg.mbByFrame = TRUE;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrMjpeg.mPicWidth= pContext->mConfigPara.mMainEncodeWidth;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrMjpeg.mPicHeight = pContext->mConfigPara.mMainEncodeHeight;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrMjpeg.mThreshSize = vbvThreshSize;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrMjpeg.mBufSize = vbvBufSize;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = pContext->mConfigPara.mMainEncodeBitrate;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mSrcFrmRate = pContext->mConfigPara.mMainSrcFrameRate;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mDstFrmRate = pContext->mConfigPara.mMainEncodeFrameRate;
    }
    pContext->mMainStream.mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pContext->mMainStream.mVEncChnAttr.GopAttr.mGopSize = 2;
    pContext->mMainStream.mVEncFrameRateConfig.SrcFrmRate = pContext->mConfigPara.mMainSrcFrameRate;
    pContext->mMainStream.mVEncFrameRateConfig.DstFrmRate = pContext->mConfigPara.mMainEncodeFrameRate;
}

static void configSubStream(SampleRtspContext *pContext)
{
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;

    pContext->mSubStream.priv = (void*)pContext;
    pContext->mSubStream.mIsp = pContext->mConfigPara.mSubIsp;
    pContext->mSubStream.mVipp = pContext->mConfigPara.mSubVipp;
    pContext->mSubStream.mViChn = pContext->mConfigPara.mSubViChn;
    pContext->mSubStream.mVEncChn = pContext->mConfigPara.mSubVEncChn;

    if (pContext->mConfigPara.mSubOnlineEnable)
    {
        pContext->mSubStream.mViAttr.mOnlineEnable = 1;
        pContext->mSubStream.mViAttr.mOnlineShareBufNum = pContext->mConfigPara.mSubOnlineShareBufNum;
        pContext->mSubStream.mVEncChnAttr.VeAttr.mOnlineEnable = 1;
        pContext->mSubStream.mVEncChnAttr.VeAttr.mOnlineShareBufNum = pContext->mConfigPara.mSubOnlineShareBufNum;
    }
    alogd("sub vipp%d ve_online_en:%d, dma_buf_num:%d, venc ch%d OnlineEnable:%d, OnlineShareBufNum:%d",
        pContext->mSubStream.mVipp, pContext->mSubStream.mViAttr.mOnlineEnable, pContext->mSubStream.mViAttr.mOnlineShareBufNum,
        pContext->mSubStream.mVEncChn, pContext->mSubStream.mVEncChnAttr.VeAttr.mOnlineEnable, pContext->mSubStream.mVEncChnAttr.VeAttr.mOnlineShareBufNum);

    pContext->mSubStream.mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pContext->mSubStream.mViAttr.memtype = V4L2_MEMORY_MMAP;
    pContext->mSubStream.mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pContext->mConfigPara.mSubPixelFormat);
    pContext->mSubStream.mViAttr.format.field = V4L2_FIELD_NONE;
    pContext->mSubStream.mViAttr.format.colorspace = V4L2_COLORSPACE_REC709_PART_RANGE;
    pContext->mSubStream.mViAttr.format.width = pContext->mConfigPara.mSubSrcWidth;
    pContext->mSubStream.mViAttr.format.height = pContext->mConfigPara.mSubSrcHeight;
    pContext->mSubStream.mViAttr.fps = pContext->mConfigPara.mSubSrcFrameRate;
    pContext->mSubStream.mViAttr.use_current_win = 1;
    pContext->mSubStream.mViAttr.nbufs = pContext->mConfigPara.mSubViBufNum;
    pContext->mSubStream.mViAttr.nplanes = 2;
    pContext->mSubStream.mViAttr.wdr_mode = pContext->mConfigPara.mSubWdrEnable;
    pContext->mSubStream.mViAttr.drop_frame_num = 10;

    /* config vipp crop */
    pContext->mSubStream.mViAttr.mCropCfg.bEnable = pContext->mConfigPara.mSubVippCropEnable;
    pContext->mSubStream.mViAttr.mCropCfg.Rect.X = pContext->mConfigPara.mSubVippCropRectX;
    pContext->mSubStream.mViAttr.mCropCfg.Rect.Y = pContext->mConfigPara.mSubVippCropRectY;
    pContext->mSubStream.mViAttr.mCropCfg.Rect.Width = pContext->mConfigPara.mSubVippCropRectWidth;
    pContext->mSubStream.mViAttr.mCropCfg.Rect.Height = pContext->mConfigPara.mSubVippCropRectHeight;
    alogd("vipp[%d] crop en:%d X:%d Y:%d W:%d H:%d", pContext->mSubStream.mVipp,
        pContext->mConfigPara.mSubVippCropEnable,
        pContext->mConfigPara.mSubVippCropRectX,
        pContext->mConfigPara.mSubVippCropRectY,
        pContext->mConfigPara.mSubVippCropRectWidth,
        pContext->mConfigPara.mSubVippCropRectHeight);

    pContext->mSubStream.mVEncChnAttr.VeAttr.mVippID = pContext->mConfigPara.mSubVipp;
    pContext->mSubStream.mVEncChnAttr.VeAttr.Type = pContext->mConfigPara.mSubEncodeType;
    pContext->mSubStream.mVEncChnAttr.VeAttr.MaxKeyInterval = 100;//40;
    pContext->mSubStream.mVEncChnAttr.VeAttr.SrcPicWidth = pContext->mConfigPara.mSubSrcWidth;
    pContext->mSubStream.mVEncChnAttr.VeAttr.SrcPicHeight = pContext->mConfigPara.mSubSrcHeight;
    pContext->mSubStream.mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pContext->mSubStream.mVEncChnAttr.VeAttr.PixelFormat = pContext->mConfigPara.mSubPixelFormat;
    pContext->mSubStream.mVEncChnAttr.VeAttr.mColorSpace = pContext->mSubStream.mViAttr.format.colorspace;
    pContext->mSubStream.mVEncChnAttr.RcAttr.mProductMode = PRODUCT_STATIC_IPC;
    //pContext->mSubStream.mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

    pContext->mSubStream.mEncppSharpAttenCoefPer = pContext->mConfigPara.mSubEncppSharpAttenCoefPer;
    alogd("sub EncppSharpAttenCoefPer: %d%%", pContext->mSubStream.mEncppSharpAttenCoefPer);

    pContext->mSubStream.mVEncChnAttr.EncppAttr.eEncppSharpSetting = pContext->mConfigPara.mSubEncppEnable?VencEncppSharp_FollowISPConfig:VencEncppSharp_Disable;

    if (pContext->mConfigPara.mSubSrcFrameRate)
    {
        vbvThreshSize = pContext->mConfigPara.mSubEncodeBitrate/8/pContext->mConfigPara.mSubSrcFrameRate*15;
    }
    vbvBufSize = pContext->mConfigPara.mSubEncodeBitrate/8*4 + vbvThreshSize;
    alogd("sub vbvThreshSize: %d, vbvBufSize: %d", vbvThreshSize, vbvBufSize);

    if(PT_H264 == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.Profile = 1;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.PicWidth = pContext->mConfigPara.mSubEncodeWidth;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.PicHeight = pContext->mConfigPara.mSubEncodeHeight;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.mLevel = 0;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.mbPIntraEnable = TRUE;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.mThreshSize = vbvThreshSize;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.BufSize = vbvBufSize;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264VBR;
        if (pContext->mSubStream.mVEncChnAttr.RcAttr.mRcMode == VENC_RC_MODE_H264VBR)
        {
            pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mMaxBitRate = pContext->mConfigPara.mSubEncodeBitrate;
            pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mSrcFrmRate = pContext->mConfigPara.mSubSrcFrameRate;
            pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mDstFrmRate = pContext->mConfigPara.mSubEncodeFrameRate;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mMaxQp = 50;//45;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mMinQp = 10;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mMaxPqp = 50;//45;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mMinPqp = 10;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mQpInit = 37;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mbEnMbQpLimit = 0;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mMovingTh = 20;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mQuality = 10;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mIFrmBitsCoef = 10;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mPFrmBitsCoef = 10;
        }
        else if (pContext->mSubStream.mVEncChnAttr.RcAttr.mRcMode == VENC_RC_MODE_H264CBR)
        {
            pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH264Cbr.mBitRate = pContext->mConfigPara.mSubEncodeBitrate;
            pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH264Cbr.mSrcFrmRate = pContext->mConfigPara.mSubSrcFrameRate;
            pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH264Cbr.mDstFrmRate = pContext->mConfigPara.mSubEncodeFrameRate;
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mMaxQp = 50;
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mMinQp = 10;
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mMaxPqp = 50;
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mMinPqp = 10;
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mQpInit = 37;
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mbEnMbQpLimit = 0;
        }
    }
    else if(PT_H265 == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mProfile = 0;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mbByFrame = TRUE;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mPicWidth = pContext->mConfigPara.mSubEncodeWidth;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mPicHeight = pContext->mConfigPara.mSubEncodeHeight;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mLevel = 0;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mbPIntraEnable = TRUE;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mThreshSize = vbvThreshSize;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mBufSize = vbvBufSize;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265VBR;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH265Vbr.mMaxBitRate = pContext->mConfigPara.mSubEncodeBitrate;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH265Vbr.mSrcFrmRate = pContext->mConfigPara.mSubSrcFrameRate;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH265Vbr.mDstFrmRate = pContext->mConfigPara.mSubEncodeFrameRate;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mMaxQp = 50;//45;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mMinQp = 10;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mMaxPqp = 50;//45;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mMinPqp = 10;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mQpInit = 37;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mbEnMbQpLimit = 0;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mMovingTh = 20;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mQuality = 10;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mIFrmBitsCoef = 10;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mPFrmBitsCoef = 10;
    }
    else if(PT_MJPEG == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrMjpeg.mbByFrame = TRUE;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrMjpeg.mPicWidth= pContext->mConfigPara.mSubEncodeWidth;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrMjpeg.mPicHeight = pContext->mConfigPara.mSubEncodeHeight;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrMjpeg.mThreshSize = vbvThreshSize;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrMjpeg.mBufSize = vbvBufSize;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = pContext->mConfigPara.mSubEncodeBitrate;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mSrcFrmRate = pContext->mConfigPara.mSubSrcFrameRate;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mDstFrmRate = pContext->mConfigPara.mSubEncodeFrameRate;
    }
    pContext->mSubStream.mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pContext->mSubStream.mVEncChnAttr.GopAttr.mGopSize = 2;
    pContext->mSubStream.mVEncFrameRateConfig.SrcFrmRate = pContext->mConfigPara.mSubSrcFrameRate;
    pContext->mSubStream.mVEncFrameRateConfig.DstFrmRate = pContext->mConfigPara.mSubEncodeFrameRate;
}

static void configSubLapseStream(SampleRtspContext *pContext)
{
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;

    pContext->mSubLapseStream.priv = (void*)pContext;    
    pContext->mSubLapseStream.mIsp = pContext->mConfigPara.mSubIsp;
    pContext->mSubLapseStream.mVipp = pContext->mConfigPara.mSubLapseVipp;
    pContext->mSubLapseStream.mViChn = pContext->mConfigPara.mSubLapseViChn;
    pContext->mSubLapseStream.mVEncChn = pContext->mConfigPara.mSubLapseVEncChn;
    pContext->mSubLapseStream.mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pContext->mSubLapseStream.mViAttr.memtype = V4L2_MEMORY_MMAP;
    pContext->mSubLapseStream.mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pContext->mConfigPara.mSubLapsePixelFormat);
    pContext->mSubLapseStream.mViAttr.format.field = V4L2_FIELD_NONE;
    pContext->mSubLapseStream.mViAttr.format.colorspace = V4L2_COLORSPACE_REC709_PART_RANGE;
    pContext->mSubLapseStream.mViAttr.format.width = pContext->mConfigPara.mSubLapseSrcWidth;
    pContext->mSubLapseStream.mViAttr.format.height = pContext->mConfigPara.mSubLapseSrcHeight;
    pContext->mSubLapseStream.mViAttr.fps = pContext->mConfigPara.mSubLapseSrcFrameRate;
    pContext->mSubLapseStream.mViAttr.use_current_win = 1;
    pContext->mSubLapseStream.mViAttr.nbufs = pContext->mConfigPara.mSubLapseViBufNum;
    pContext->mSubLapseStream.mViAttr.nplanes = 2;
    pContext->mSubLapseStream.mViAttr.wdr_mode = pContext->mConfigPara.mSubLapseWdrEnable;
    pContext->mSubLapseStream.mViAttr.drop_frame_num = 10;

    pContext->mSubLapseStream.mVEncChnAttr.VeAttr.mVippID = pContext->mConfigPara.mSubLapseVipp;
    pContext->mSubLapseStream.mVEncChnAttr.VeAttr.Type = pContext->mConfigPara.mSubLapseEncodeType;
    pContext->mSubLapseStream.mVEncChnAttr.VeAttr.MaxKeyInterval = 100;//40;
    pContext->mSubLapseStream.mVEncChnAttr.VeAttr.SrcPicWidth = pContext->mConfigPara.mSubLapseSrcWidth;
    pContext->mSubLapseStream.mVEncChnAttr.VeAttr.SrcPicHeight = pContext->mConfigPara.mSubLapseSrcHeight;
    pContext->mSubLapseStream.mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pContext->mSubLapseStream.mVEncChnAttr.VeAttr.PixelFormat = pContext->mConfigPara.mSubLapsePixelFormat;
    pContext->mSubLapseStream.mVEncChnAttr.VeAttr.mColorSpace = pContext->mSubLapseStream.mViAttr.format.colorspace;
    pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mProductMode = PRODUCT_STATIC_IPC;
    //pContext->mSubLapseStream.mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

    pContext->mSubLapseStream.mEncppSharpAttenCoefPer = pContext->mConfigPara.mSubLapseEncppSharpAttenCoefPer;
    alogd("subLapse EncppSharpAttenCoefPer: %d%%", pContext->mSubLapseStream.mEncppSharpAttenCoefPer);

    pContext->mSubLapseStream.mVEncChnAttr.EncppAttr.eEncppSharpSetting = pContext->mConfigPara.mSubLapseEncppEnable?VencEncppSharp_FollowISPConfig:VencEncppSharp_Disable;

    if (pContext->mConfigPara.mSubLapseSrcFrameRate)
    {
        vbvThreshSize = pContext->mConfigPara.mSubLapseEncodeBitrate/8/pContext->mConfigPara.mSubLapseSrcFrameRate*15;
    }
    vbvBufSize = pContext->mConfigPara.mSubLapseEncodeBitrate/8*4 + vbvThreshSize;
    alogd("SubLapse vbvThreshSize: %d, vbvBufSize: %d", vbvThreshSize, vbvBufSize);

    if(PT_H264 == pContext->mSubLapseStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH264e.Profile = 1;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH264e.PicWidth = pContext->mConfigPara.mSubLapseEncodeWidth;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH264e.PicHeight = pContext->mConfigPara.mSubLapseEncodeHeight;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH264e.mLevel = 0;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH264e.mbPIntraEnable = TRUE;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH264e.mThreshSize = vbvThreshSize;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH264e.BufSize = vbvBufSize;
        pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264VBR;
        if (pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mRcMode == VENC_RC_MODE_H264VBR)
        {
            pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mMaxBitRate = pContext->mConfigPara.mSubLapseEncodeBitrate;
            pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mSrcFrmRate = pContext->mConfigPara.mSubLapseSrcFrameRate;
            pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mDstFrmRate = pContext->mConfigPara.mSubLapseEncodeFrameRate;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Vbr.mMaxQp = 50;//45;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Vbr.mMinQp = 10;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Vbr.mMaxPqp = 50;//45;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Vbr.mMinPqp = 10;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Vbr.mQpInit = 37;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Vbr.mbEnMbQpLimit = 0;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Vbr.mMovingTh = 20;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Vbr.mQuality = 10;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Vbr.mIFrmBitsCoef = 10;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Vbr.mPFrmBitsCoef = 10;
        }
        else if (pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mRcMode == VENC_RC_MODE_H264CBR)
        {
            pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrH264Cbr.mBitRate = pContext->mConfigPara.mSubLapseEncodeBitrate;
            pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrH264Cbr.mSrcFrmRate = pContext->mConfigPara.mSubLapseSrcFrameRate;
            pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrH264Cbr.mDstFrmRate = pContext->mConfigPara.mSubLapseEncodeFrameRate;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Cbr.mMaxQp = 50;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Cbr.mMinQp = 10;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Cbr.mMaxPqp = 50;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Cbr.mMinPqp = 10;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Cbr.mQpInit = 37;
            pContext->mSubLapseStream.mVEncRcParam.ParamH264Cbr.mbEnMbQpLimit = 0;
        }
    }
    else if(PT_H265 == pContext->mSubLapseStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH265e.mProfile = 0;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH265e.mbByFrame = TRUE;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH265e.mPicWidth = pContext->mConfigPara.mSubLapseEncodeWidth;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH265e.mPicHeight = pContext->mConfigPara.mSubLapseEncodeHeight;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH265e.mLevel = 0;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH265e.mbPIntraEnable = TRUE;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH265e.mThreshSize = vbvThreshSize;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrH265e.mBufSize = vbvBufSize;
        pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265VBR;
        pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrH265Vbr.mMaxBitRate = pContext->mConfigPara.mSubLapseEncodeBitrate;
        pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrH265Vbr.mSrcFrmRate = pContext->mConfigPara.mSubLapseSrcFrameRate;
        pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrH265Vbr.mDstFrmRate = pContext->mConfigPara.mSubLapseEncodeFrameRate;
        pContext->mSubLapseStream.mVEncRcParam.ParamH265Vbr.mMaxQp = 50;//45;
        pContext->mSubLapseStream.mVEncRcParam.ParamH265Vbr.mMinQp = 10;
        pContext->mSubLapseStream.mVEncRcParam.ParamH265Vbr.mMaxPqp = 50;//45;
        pContext->mSubLapseStream.mVEncRcParam.ParamH265Vbr.mMinPqp = 10;
        pContext->mSubLapseStream.mVEncRcParam.ParamH265Vbr.mQpInit = 37;
        pContext->mSubLapseStream.mVEncRcParam.ParamH265Vbr.mbEnMbQpLimit = 0;
        pContext->mSubLapseStream.mVEncRcParam.ParamH265Vbr.mMovingTh = 20;
        pContext->mSubLapseStream.mVEncRcParam.ParamH265Vbr.mQuality = 10;
        pContext->mSubLapseStream.mVEncRcParam.ParamH265Vbr.mIFrmBitsCoef = 10;
        pContext->mSubLapseStream.mVEncRcParam.ParamH265Vbr.mPFrmBitsCoef = 10;
    }
    else if(PT_MJPEG == pContext->mSubLapseStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrMjpeg.mbByFrame = TRUE;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrMjpeg.mPicWidth= pContext->mConfigPara.mSubLapseEncodeWidth;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrMjpeg.mPicHeight = pContext->mConfigPara.mSubLapseEncodeHeight;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrMjpeg.mThreshSize = vbvThreshSize;
        pContext->mSubLapseStream.mVEncChnAttr.VeAttr.AttrMjpeg.mBufSize = vbvBufSize;
        pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
        pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = pContext->mConfigPara.mSubLapseEncodeBitrate;
        pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mSrcFrmRate = pContext->mConfigPara.mSubLapseSrcFrameRate;
        pContext->mSubLapseStream.mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mDstFrmRate = pContext->mConfigPara.mSubLapseEncodeFrameRate;
    }
    pContext->mSubLapseStream.mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pContext->mSubLapseStream.mVEncChnAttr.GopAttr.mGopSize = 2;
    pContext->mSubLapseStream.mVEncFrameRateConfig.SrcFrmRate = pContext->mConfigPara.mSubLapseSrcFrameRate;
    pContext->mSubLapseStream.mVEncFrameRateConfig.DstFrmRate = pContext->mConfigPara.mSubLapseEncodeFrameRate;
}

static void* getWbYuvThread(void *pThreadData)
{
    int result = 0;
    SampleRtspContext *pContext = (SampleRtspContext*)pThreadData;
    VencStreamContext *pStreamContext = NULL;

    if (VENC_CHN_MAIN_STREAM == pContext->mConfigPara.mWbYuvStreamChn)
    {
        pStreamContext = &pContext->mMainStream;
    }
    else if (VENC_CHN_SUB_STREAM == pContext->mConfigPara.mWbYuvStreamChn)
    {
        pStreamContext = &pContext->mSubStream;
    }
    else if (VENC_CHN_SUB_LAPSE_STREAM == pContext->mConfigPara.mWbYuvStreamChn)
    {
        pStreamContext = &pContext->mSubLapseStream;
    }
    else
    {
        aloge("fatal error, WbYuvStreamChn:%d is not support for wb yuv!", pContext->mConfigPara.mWbYuvStreamChn);
        return NULL;
    }

    FILE* fp_wb_yuv = fopen(pContext->mConfigPara.mWbYuvFilePath, "wb");
    if (NULL == fp_wb_yuv)
    {
        aloge("fatal error! why open file[%s] fail?", pContext->mConfigPara.mWbYuvFilePath);
        return NULL;
    }

    unsigned int wbYuvCnt = 0;
    unsigned int preCnt = 0;
    unsigned mHadEnableWbYuv = 0;
    unsigned int wb_wdith  = 0;
    unsigned int wb_height = 0;

    if (PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        wb_wdith  = pStreamContext->mVEncChnAttr.VeAttr.AttrH264e.PicWidth;
        wb_height = pStreamContext->mVEncChnAttr.VeAttr.AttrH264e.PicHeight;
    }
    else if (PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        wb_wdith  = pStreamContext->mVEncChnAttr.VeAttr.AttrH265e.mPicWidth;
        wb_height = pStreamContext->mVEncChnAttr.VeAttr.AttrH265e.mPicHeight;
    }
    else
    {
        aloge("fatal error, Type:%d is not support for wb yuv!", pStreamContext->mVEncChnAttr.VeAttr.Type);
        if (fp_wb_yuv)
        {
            fclose(fp_wb_yuv);
            fp_wb_yuv = NULL;
        }
        return NULL;
    }
    wb_wdith  = AWALIGN(wb_wdith, 16);
    wb_height = AWALIGN(wb_height, 16);

    while (!pContext->mbExitFlag && wbYuvCnt < pContext->mConfigPara.mWbYuvTotalCnt)
    {
        if (pStreamContext->mStreamDataCnt < pContext->mConfigPara.mWbYuvStartIndex)
        {
            usleep(10*1000);
            continue;
        }
        else
        {
            if (mHadEnableWbYuv == 0)
            {
                mHadEnableWbYuv = 1;
                sWbYuvParam mWbYuvParam;
                memset(&mWbYuvParam, 0, sizeof(sWbYuvParam));
                mWbYuvParam.bEnableWbYuv = 1;
                mWbYuvParam.nWbBufferNum = pContext->mConfigPara.mWbYuvBufNum;
                mWbYuvParam.scalerRatio  = pContext->mConfigPara.mWbYuvScalerRatio;
                alogd("enable WbYuv, WbBufferNum:%d, scalerRatio:%d", mWbYuvParam.nWbBufferNum, mWbYuvParam.scalerRatio);
                result = AW_MPI_VENC_SetWbYuv(pStreamContext->mVEncChn, &mWbYuvParam);
                if (result)
                {
                    aloge("fatal error, VencChn[%d] SetWbYuv failed! ret:%d", pStreamContext->mVEncChn, result);
                }
            }
        }

        if (fp_wb_yuv)
        {
            VencThumbInfo mThumbInfo;
            memset(&mThumbInfo, 0, sizeof(VencThumbInfo));
            mThumbInfo.bWriteToFile = 1;
            mThumbInfo.fp = fp_wb_yuv;
            int startTime = getSysTickMs();
            result = AW_MPI_VENC_GetWbYuv(pStreamContext->mVEncChn, &mThumbInfo);
            int endTime = getSysTickMs();
            if (result == 0)
            {
                wbYuvCnt++;
                alogd("get wb yuv[%dx%d], curWbCnt = %d, saveTotalCnt = %d, time = %d ms, encodeCnt = %d, diffCnt = %d",
                    wb_wdith, wb_height, wbYuvCnt, pContext->mConfigPara.mWbYuvTotalCnt,
                    endTime - startTime, mThumbInfo.nEncodeCnt, (mThumbInfo.nEncodeCnt - preCnt));

                preCnt = mThumbInfo.nEncodeCnt;
            }
            else
            {
                usleep(10*1000);
            }
        }
        else
        {
            alogw("exit thread: fp_wb_yuv = %p", fp_wb_yuv);
        }
    }

    if (fp_wb_yuv)
    {
        fclose(fp_wb_yuv);
        fp_wb_yuv = NULL;
    }

    alogd("exit");

    return (void*)result;
}

static void* getVencStreamThread(void *pThreadData)
{
    VencStreamContext *pStreamContext = (VencStreamContext*)pThreadData;
    SampleRtspContext *pContext = (SampleRtspContext*)pStreamContext->priv;
    char strThreadName[32];
    sprintf(strThreadName, "venc%d-stream", pStreamContext->mVEncChn);
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    ERRORTYPE ret = SUCCESS;
    int result = 0;
    unsigned int nStreamLen = 0;

    VENC_STREAM_S stVencStream;
    VENC_PACK_S stVencPack;
    memset(&stVencStream, 0, sizeof(stVencStream));
    memset(&stVencPack, 0, sizeof(stVencPack));
    stVencStream.mPackCount = 1;
    stVencStream.mpPack = &stVencPack;

    //get spspps from venc.
    VencHeaderData stSpsPpsInfo;
    memset(&stSpsPpsInfo, 0, sizeof(stSpsPpsInfo));
    if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        ret = AW_MPI_VENC_GetH264SpsPpsInfo(pStreamContext->mVEncChn, &stSpsPpsInfo);
        if(ret != SUCCESS)
        {
            aloge("fatal error! get spspps fail[0x%x]!", ret);
        }
    }
    else if(PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        ret = AW_MPI_VENC_GetH265SpsPpsInfo(pStreamContext->mVEncChn, &stSpsPpsInfo);
        if(ret != SUCCESS)
        {
            aloge("fatal error! get spspps fail[0x%x]!", ret);
        }
    }
    else
    {
        aloge("fatal error! vencType:0x%x is wrong!", pStreamContext->mVEncChnAttr.VeAttr.Type);
    }
    if (pStreamContext->mFile)
    {
        if (stSpsPpsInfo.nLength > 0)
        {
            fwrite(stSpsPpsInfo.pBuffer, 1, stSpsPpsInfo.nLength, pStreamContext->mFile);
        }
    }

#ifdef SUPPORT_RTSP_TEST
    unsigned int buf_size = 0;
    int nEncodeWidth = 0;
    int nEncodeHeight = 0;
    if (pContext->mConfigPara.mMainVEncChn == pStreamContext->mVEncChn)
    {
        nEncodeWidth = pContext->mConfigPara.mMainEncodeWidth;
        nEncodeHeight = pContext->mConfigPara.mMainEncodeHeight;
    }
    else if (pContext->mConfigPara.mSubVEncChn == pStreamContext->mVEncChn)
    {
        nEncodeWidth = pContext->mConfigPara.mSubEncodeWidth;
        nEncodeHeight = pContext->mConfigPara.mSubEncodeHeight;
    }
    else
    {
        aloge("fatal error! invalid venc chn %d", pStreamContext->mVEncChn);
        return NULL;
    }
    if (0 == pContext->mConfigPara.mStreamBufSize)
    {
        buf_size = nEncodeWidth * nEncodeHeight * 3 / 2 / 10;
    }
    else
    {
        buf_size = pContext->mConfigPara.mStreamBufSize;
    }
    if (0 == buf_size)
    {
        aloge("fatal error! VencChn[%d] buf_size is 0.", pStreamContext->mVEncChn);
        return NULL;
    }
    unsigned char *stream_buf = (unsigned char *)malloc(buf_size);
    if (NULL == stream_buf)
    {
        aloge("malloc stream_buf failed, size=%d", buf_size);
        return NULL;
    }
    memset(stream_buf, 0, buf_size);
    alogd("VencChn[%d] stream_buf:%p, size=%d", pStreamContext->mVEncChn, stream_buf, buf_size);

    if (VENC_CHN_MAIN_STREAM == pStreamContext->mVEncChn)
    {
        rtsp_start(0);
    }
    else if (VENC_CHN_SUB_STREAM == pStreamContext->mVEncChn)
    {
        rtsp_start(1);
    }
#endif

    int mActualFrameRate = 0;
    int mTempFrameCnt = 0;
    int mTempCurrentTime = 0;
    int mTempLastTime = 0;
    pStreamContext->mStreamDataCnt = 0;

    int nMilliSec = 1000;
    // This operation will cause the test to wait too long when exiting.
    if (TRUE == pStreamContext->mbSubLapseEnable)
    {
        int64_t mTimeLapse = 0;
        AW_MPI_VENC_GetTimeLapse(pStreamContext->mVEncChn, &mTimeLapse);
        nMilliSec = 2 * mTimeLapse / 1000;
        alogd("Lapse VencChn[%d] update nMilliSec to %d ms", pStreamContext->mVEncChn, nMilliSec);
    }

    while (!pContext->mbExitFlag)
    {
        memset(stVencStream.mpPack, 0, sizeof(VENC_PACK_S));
        ret = AW_MPI_VENC_GetStream(pStreamContext->mVEncChn, &stVencStream, nMilliSec);
        if(SUCCESS == ret)
        {
            pStreamContext->mStreamDataCnt++;
            nStreamLen = stVencStream.mpPack[0].mLen0 + stVencStream.mpPack[0].mLen1 + stVencStream.mpPack[0].mLen2;
            if (nStreamLen <= 0)
            {
                aloge("fatal error! VencStream length error,[%d,%d,%d]!", stVencStream.mpPack[0].mLen0, stVencStream.mpPack[0].mLen1, stVencStream.mpPack[0].mLen2);
            }
            if (pStreamContext->mFile)
            {
                if (stVencStream.mpPack[0].mLen0 > 0)
                {
                    fwrite(stVencStream.mpPack[0].mpAddr0, 1, stVencStream.mpPack[0].mLen0, pStreamContext->mFile);
                }
                if (stVencStream.mpPack[0].mLen1 > 0)
                {
                    fwrite(stVencStream.mpPack[0].mpAddr1, 1, stVencStream.mpPack[0].mLen1, pStreamContext->mFile);
                }
                if (stVencStream.mpPack[0].mLen2 > 0)
                {
                    fwrite(stVencStream.mpPack[0].mpAddr2, 1, stVencStream.mpPack[0].mLen2, pStreamContext->mFile);
                }
            }

            if (FALSE == pStreamContext->mbSubLapseEnable)
            {
                // check actual framerate.
                mTempFrameCnt++;
                if (0 == mTempLastTime)
                {
                    mTempLastTime = getSysTickMs();
                }
                else
                {
                    mTempCurrentTime = getSysTickMs();
                    if (mTempCurrentTime >= mTempLastTime + 1000)
                    {
                        mActualFrameRate = mTempFrameCnt;
                        if (mActualFrameRate < pStreamContext->mVEncFrameRateConfig.DstFrmRate)
                        {
                            alogw("VencChn[%d], actualFrameRate %d < dstFrmRate %d", pStreamContext->mVEncChn, mActualFrameRate, pStreamContext->mVEncFrameRateConfig.DstFrmRate);
                        }
                        mTempLastTime = mTempCurrentTime;
                        mTempFrameCnt = 0;
                    }
                }
            }

#ifdef SUPPORT_RTSP_TEST
            if (stVencStream.mpPack != NULL && stVencStream.mpPack->mLen0 > 0)
            {
                uint64_t pts = stVencStream.mpPack->mPTS;
                int len = 0;
                RTSP_FRAME_DATA_TYPE frame_type = RTSP_FRAME_DATA_TYPE_LAST;

                if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                {
                    if (H264E_NALU_ISLICE == stVencStream.mpPack->mDataType.enH264EType)
                    {
                        if (NULL == stSpsPpsInfo.pBuffer)
                        {
                            alogd("SpsPpsInfo.pBuffer = NULL!!\n");
                        }
                        /* Get sps/pps first */
                        memcpy(stream_buf, stSpsPpsInfo.pBuffer, stSpsPpsInfo.nLength);
                        len += stSpsPpsInfo.nLength;
                        frame_type = RTSP_FRAME_DATA_TYPE_I;
                    }
                    else
                    {
                        frame_type = RTSP_FRAME_DATA_TYPE_P;
                    }
                }
                else if(PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                {
                    if (H265E_NALU_ISLICE == stVencStream.mpPack->mDataType.enH265EType)
                    {
                        if (NULL == stSpsPpsInfo.pBuffer)
                        {
                            alogd("SpsPpsInfo.pBuffer = NULL!!\n");
                        }
                        /* Get sps/pps first */
                        memcpy(stream_buf, stSpsPpsInfo.pBuffer, stSpsPpsInfo.nLength);
                        len += stSpsPpsInfo.nLength;
                        frame_type = RTSP_FRAME_DATA_TYPE_I;
                    }
                    else
                    {
                        frame_type = RTSP_FRAME_DATA_TYPE_P;
                    }
                }
                else
                {
                    aloge("fatal error! vencType:0x%x is wrong!", pStreamContext->mVEncChnAttr.VeAttr.Type);
                }

                if (buf_size >= len + stVencStream.mpPack->mLen0)
                {
                    memcpy(stream_buf + len, stVencStream.mpPack->mpAddr0, stVencStream.mpPack->mLen0);
                    len += stVencStream.mpPack->mLen0;
                }

                if (stVencStream.mpPack->mLen1 > 0)
                {
                    if (buf_size >= (len + stVencStream.mpPack->mLen1))
                    {
                        memcpy(stream_buf + len, stVencStream.mpPack->mpAddr1, stVencStream.mpPack->mLen1);
                        len += stVencStream.mpPack->mLen1;
                    }
                }
                if (stVencStream.mpPack->mLen2 > 0)
                {
                    if (buf_size >= (len + stVencStream.mpPack->mLen2))
                    {
                        memcpy(stream_buf + len, stVencStream.mpPack->mpAddr2, stVencStream.mpPack->mLen2);
                        len += stVencStream.mpPack->mLen2;
                    }
                }

                /* get current fps and set to rtsp to avoid rtsp checkDurationTime warn */
                int sensor_fps = 0;
                AW_MPI_ISP_GetSensorFps(pStreamContext->mIsp, &sensor_fps);

                RtspSendDataParam stRtspParam;
                memset(&stRtspParam, 0, sizeof(RtspSendDataParam));
                stRtspParam.buf = stream_buf;
                stRtspParam.size = len;
                stRtspParam.frame_type = frame_type;
                stRtspParam.pts = pts;
                stRtspParam.frame_rate = sensor_fps;

                if (VENC_CHN_MAIN_STREAM == pStreamContext->mVEncChn)
                {
                    rtsp_sendData(0, &stRtspParam);
                }
                else if (VENC_CHN_SUB_STREAM == pStreamContext->mVEncChn)
                {
                    rtsp_sendData(1, &stRtspParam);
                }
            }
#endif

            ret = AW_MPI_VENC_ReleaseStream(pStreamContext->mVEncChn, &stVencStream);
            if(ret != SUCCESS)
            {
                aloge("fatal error! venc_chn[%d] releaseStream fail", pStreamContext->mVEncChn);
            }                
        }
        else
        {
            alogw("fatal error! vencChn[%d] get frame failed! check code!", pStreamContext->mVEncChn);
            continue;
        }
    }

#ifdef SUPPORT_RTSP_TEST
    if (stream_buf)
    {
        free(stream_buf);
        stream_buf = NULL;
    }
#endif

    alogd("exit");

    return (void*)result;
}

int main(int argc, char *argv[])
{
    int result = 0;
    ERRORTYPE ret;

    SampleRtspContext *pContext = (SampleRtspContext*)malloc(sizeof(SampleRtspContext));
    gpSampleRtspContext = pContext;
    memset(pContext, 0, sizeof(SampleRtspContext));
    cdx_sem_init(&pContext->mSemExit, 0);

    /* register process function for SIGINT, to exit program. */
    if (signal(SIGINT, handle_exit) == SIG_ERR)
    {
        aloge("can't catch SIGSEGV");
    }

    if(ParseCmdLine(argc, argv, &pContext->mCmdLinePara) != 0)
    {
        aloge("fatal error! command line param is wrong, exit!");
        result = -1;
        goto _exit;
    }
    char *pConfigFilePath;
    if(strlen(pContext->mCmdLinePara.mConfigFilePath) > 0)
    {
        pConfigFilePath = pContext->mCmdLinePara.mConfigFilePath;
    }
    else
    {
        pConfigFilePath = NULL;
    }
    /* parse config file. */
    if(loadSampleConfig(&pContext->mConfigPara, pConfigFilePath) != SUCCESS)
    {
        aloge("fatal error! no config file or parse conf file fail");
        result = -1;
        goto _exit;
    }
    system("cat /proc/meminfo | grep Committed_AS");

    // prepare
    MPP_SYS_CONF_S stSysConf;
    memset(&stSysConf, 0, sizeof(MPP_SYS_CONF_S));
    stSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stSysConf);
    ret = AW_MPI_SYS_Init();
    if (ret < 0)
    {
        aloge("sys Init failed!");
        return -1;
    }

    pContext->mMainIspRunFlag = 0;  
    pContext->mSubIspRunFlag = 0;
    if (pContext->mConfigPara.mMainVEncChn >= 0 && pContext->mConfigPara.mMainViChn >= 0)
    {
        configMainStream(pContext);
        AW_MPI_VI_CreateVipp(pContext->mMainStream.mVipp);
        AW_MPI_VI_SetVippAttr(pContext->mMainStream.mVipp, &pContext->mMainStream.mViAttr);
        if (0 == pContext->mMainIspRunFlag)
        {
            AW_MPI_ISP_Run(pContext->mMainStream.mIsp);
            pContext->mMainIspRunFlag = 1;
        }
        AW_MPI_VI_EnableVipp(pContext->mMainStream.mVipp);

        if (strlen(pContext->mConfigPara.mMainFilePath) > 0)
        {
            pContext->mMainStream.mFile = fopen(pContext->mConfigPara.mMainFilePath, "wb");
            if(NULL == pContext->mMainStream.mFile)
            {
                aloge("fatal error! why open file[%s] fail?", pContext->mConfigPara.mMainFilePath);
            }
        }
        else
        {
            pContext->mMainStream.mFile = NULL;
        }
        AW_MPI_VI_CreateVirChn(pContext->mMainStream.mVipp, pContext->mMainStream.mViChn, NULL);
        AW_MPI_VENC_CreateChn(pContext->mMainStream.mVEncChn, &pContext->mMainStream.mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pContext->mMainStream.mVEncChn, &pContext->mMainStream.mVEncRcParam);

        /* set framerate in AW_MPI_VENC_CreateChn */
        //AW_MPI_VENC_SetFrameRate(pContext->mMainStream.mVEncChn, &pContext->mMainStream.mVEncFrameRateConfig);

        //setVenc2Dnr(pContext->mMainStream.mVEncChn, 1);
        //setVenc3Dnr(pContext->mMainStream.mVEncChn, 1);
        //setVencSuperFrameCfg(pContext->mMainStream.mVEncChn, pContext->mConfigPara.mMainEncodeBitrate, pContext->mConfigPara.mMainEncodeFrameRate);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pContext->mMainStream.mVEncChn, &cbInfo);

        VENC_IspVeLinkAttr stIspVeLinkAttr;
        memset(&stIspVeLinkAttr, 0, sizeof(VENC_IspVeLinkAttr));
        stIspVeLinkAttr.bEnableIsp2Ve = TRUE;
        stIspVeLinkAttr.bEnableVe2Isp = TRUE;
        stIspVeLinkAttr.nVipp = pContext->mMainStream.mVipp;
        AW_MPI_VENC_EnableIspVeLink(pContext->mMainStream.mVEncChn, &stIspVeLinkAttr);
        alogd("VencChn[%d] ispVeLink:%d-%d-%d", pContext->mMainStream.mVEncChn, stIspVeLinkAttr.bEnableIsp2Ve, stIspVeLinkAttr.bEnableVe2Isp,
            stIspVeLinkAttr.nVipp);

        if (pContext->mConfigPara.mCameraAdaptiveMovingAndStaticEnable)
        {
            setVencLensMovingMaxQp(pContext->mMainStream.mVEncChn, pContext->mConfigPara.mVencLensMovingMaxQp);
        }

        MPP_CHN_S ViChn = {MOD_ID_VIU, pContext->mMainStream.mVipp, pContext->mMainStream.mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pContext->mMainStream.mVEncChn};
        AW_MPI_SYS_Bind(&ViChn,&VeChn);
    }
    if (pContext->mConfigPara.mSubVEncChn >= 0 && pContext->mConfigPara.mSubViChn >= 0)
    {
        configSubStream(pContext);
        AW_MPI_VI_CreateVipp(pContext->mSubStream.mVipp);
        AW_MPI_VI_SetVippAttr(pContext->mSubStream.mVipp, &pContext->mSubStream.mViAttr);
        if (0 == pContext->mSubIspRunFlag && (pContext->mMainStream.mIsp != pContext->mSubStream.mIsp || 0 == pContext->mMainIspRunFlag))
        {
            AW_MPI_ISP_Run(pContext->mSubStream.mIsp);
            pContext->mSubIspRunFlag = 1;
        }

        if (pContext->mConfigPara.mSubVippCropEnable)
        {
            VI_CROP_CFG_S stCropCfg;
            memset(&stCropCfg, 0, sizeof(VI_CROP_CFG_S));
            stCropCfg.bEnable = pContext->mConfigPara.mSubVippCropEnable;
            stCropCfg.Rect.X = pContext->mConfigPara.mSubVippCropRectX;
            stCropCfg.Rect.Y = pContext->mConfigPara.mSubVippCropRectY;
            stCropCfg.Rect.Width = pContext->mConfigPara.mSubVippCropRectWidth;
            stCropCfg.Rect.Height = pContext->mConfigPara.mSubVippCropRectHeight;
            AW_MPI_VI_SetCrop(pContext->mSubStream.mVipp, &stCropCfg);
        }

        AW_MPI_VI_EnableVipp(pContext->mSubStream.mVipp);

        if (strlen(pContext->mConfigPara.mSubFilePath) > 0)
        {
            pContext->mSubStream.mFile = fopen(pContext->mConfigPara.mSubFilePath, "wb");
            if(NULL == pContext->mSubStream.mFile)
            {
                aloge("fatal error! why open file[%s] fail?", pContext->mConfigPara.mSubFilePath);
            }
        }
        else
        {
            pContext->mSubStream.mFile = NULL;
        }
        AW_MPI_VI_CreateVirChn(pContext->mSubStream.mVipp, pContext->mSubStream.mViChn, NULL);
        AW_MPI_VENC_CreateChn(pContext->mSubStream.mVEncChn, &pContext->mSubStream.mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pContext->mSubStream.mVEncChn, &pContext->mSubStream.mVEncRcParam);

        /* set framerate in AW_MPI_VENC_CreateChn */
        //AW_MPI_VENC_SetFrameRate(pContext->mSubStream.mVEncChn, &pContext->mSubStream.mVEncFrameRateConfig);

        //setVenc2Dnr(pContext->mSubStream.mVEncChn, 1);
        //setVenc3Dnr(pContext->mMainStream.mVEncChn, 1);
        //setVencSuperFrameCfg(pContext->mSubStream.mVEncChn, pContext->mConfigPara.mSubEncodeBitrate, pContext->mConfigPara.mSubEncodeFrameRate);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pContext->mSubStream.mVEncChn, &cbInfo);

        VENC_IspVeLinkAttr stIspVeLinkAttr;
        memset(&stIspVeLinkAttr, 0, sizeof(VENC_IspVeLinkAttr));
        stIspVeLinkAttr.bEnableIsp2Ve = TRUE;
        stIspVeLinkAttr.bEnableVe2Isp = FALSE;
        stIspVeLinkAttr.nVipp = pContext->mSubStream.mVipp;
        AW_MPI_VENC_EnableIspVeLink(pContext->mSubStream.mVEncChn, &stIspVeLinkAttr);
        alogd("VencChn[%d] ispVeLink:%d-%d-%d", pContext->mSubStream.mVEncChn, stIspVeLinkAttr.bEnableIsp2Ve, stIspVeLinkAttr.bEnableVe2Isp,
            stIspVeLinkAttr.nVipp);

        if (pContext->mConfigPara.mCameraAdaptiveMovingAndStaticEnable)
        {
            setVencLensMovingMaxQp(pContext->mSubStream.mVEncChn, pContext->mConfigPara.mVencLensMovingMaxQp);
        }

        MPP_CHN_S ViChn = {MOD_ID_VIU, pContext->mSubStream.mVipp, pContext->mSubStream.mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pContext->mSubStream.mVEncChn};
        AW_MPI_SYS_Bind(&ViChn, &VeChn);
    }
    if (pContext->mConfigPara.mSubLapseVEncChn >= 0 && pContext->mConfigPara.mSubLapseViChn >= 0)
    {
        configSubLapseStream(pContext);

        if (pContext->mSubStream.mVipp != pContext->mSubLapseStream.mVipp)
        {
            AW_MPI_VI_CreateVipp(pContext->mSubLapseStream.mVipp);
            AW_MPI_VI_SetVippAttr(pContext->mSubLapseStream.mVipp, &pContext->mSubLapseStream.mViAttr);
            if (0 == pContext->mSubIspRunFlag)
            {
                AW_MPI_ISP_Run(pContext->mSubLapseStream.mIsp);
                pContext->mSubIspRunFlag = 1;
            }
            AW_MPI_VI_EnableVipp(pContext->mSubLapseStream.mVipp);
        }

        if (strlen(pContext->mConfigPara.mSubLapseFilePath) > 0)
        {
            pContext->mSubLapseStream.mFile = fopen(pContext->mConfigPara.mSubLapseFilePath, "wb");
            if (NULL == pContext->mSubLapseStream.mFile)
            {
                aloge("fatal error! why open file[%s] fail?", pContext->mConfigPara.mSubLapseFilePath);
            }
        }
        else
        {
            pContext->mSubLapseStream.mFile = NULL;
        }
        AW_MPI_VI_CreateVirChn(pContext->mSubLapseStream.mVipp, pContext->mSubLapseStream.mViChn, NULL);
        AW_MPI_VENC_CreateChn(pContext->mSubLapseStream.mVEncChn, &pContext->mSubLapseStream.mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pContext->mSubLapseStream.mVEncChn, &pContext->mSubLapseStream.mVEncRcParam);

        /* set framerate in AW_MPI_VENC_CreateChn */
        //AW_MPI_VENC_SetFrameRate(pContext->mSubLapseStream.mVEncChn, &pContext->mSubLapseStream.mVEncFrameRateConfig);

        if (pContext->mConfigPara.mSubLapseTime)
        {
            pContext->mSubLapseStream.mbSubLapseEnable = TRUE;
            alogd("Lapse set TimeLapse %d us", pContext->mConfigPara.mSubLapseTime);
            AW_MPI_VENC_SetTimeLapse(pContext->mSubLapseStream.mVEncChn, pContext->mConfigPara.mSubLapseTime);
        }

        //setVenc2Dnr(pContext->mSubLapseStream.mVEncChn, 1);
        //setVenc3Dnr(pContext->mMainStream.mVEncChn, 1);
        //setVencSuperFrameCfg(pContext->mSubLapseStream.mVEncChn, pContext->mConfigPara.mSubLapseEncodeBitrate, pContext->mConfigPara.mSubLapseEncodeFrameRate);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pContext->mSubLapseStream.mVEncChn, &cbInfo);

        VENC_IspVeLinkAttr stIspVeLinkAttr;
        memset(&stIspVeLinkAttr, 0, sizeof(VENC_IspVeLinkAttr));
        stIspVeLinkAttr.bEnableIsp2Ve = TRUE;
        stIspVeLinkAttr.bEnableVe2Isp = FALSE;
        stIspVeLinkAttr.nVipp = pContext->mSubLapseStream.mVipp;
        AW_MPI_VENC_EnableIspVeLink(pContext->mSubLapseStream.mVEncChn, &stIspVeLinkAttr);
        alogd("VencChn[%d] ispVeLink:%d-%d-%d", pContext->mSubLapseStream.mVEncChn, stIspVeLinkAttr.bEnableIsp2Ve, stIspVeLinkAttr.bEnableVe2Isp,
            stIspVeLinkAttr.nVipp);

        if (pContext->mConfigPara.mCameraAdaptiveMovingAndStaticEnable)
        {
            setVencLensMovingMaxQp(pContext->mSubLapseStream.mVEncChn, pContext->mConfigPara.mVencLensMovingMaxQp);
        }

        MPP_CHN_S ViChn = {MOD_ID_VIU, pContext->mSubLapseStream.mVipp, pContext->mSubLapseStream.mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pContext->mSubLapseStream.mVEncChn};
        AW_MPI_SYS_Bind(&ViChn, &VeChn);
    }

    // start
    if (pContext->mConfigPara.mMainVEncChn >= 0 && pContext->mConfigPara.mMainViChn >= 0)
    {
        AW_MPI_VI_EnableVirChn(pContext->mMainStream.mVipp, pContext->mMainStream.mViChn);
        AW_MPI_VENC_StartRecvPic(pContext->mMainStream.mVEncChn);
#ifdef SUPPORT_RTSP_TEST
        RtspServerAttr rtsp_attr;
        memset(&rtsp_attr, 0, sizeof(RtspServerAttr));
        rtsp_attr.net_type = pContext->mConfigPara.mRtspNetType;

        if (PT_H264 == pContext->mMainStream.mVEncChnAttr.VeAttr.Type)
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_H264;
        else if (PT_H265 == pContext->mMainStream.mVEncChnAttr.VeAttr.Type)
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_H265;
        else
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_LAST;

        rtsp_attr.frame_rate = pContext->mConfigPara.mMainEncodeFrameRate;

        result = rtsp_open(0, &rtsp_attr);
        if (result)
        {
            aloge("Do rtsp_open fail! ret:%d", result);
            return -1;
        }
#endif
        result = pthread_create(&pContext->mMainStream.mStreamThreadId, NULL, getVencStreamThread, (void*)&pContext->mMainStream);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
    }
    if (pContext->mConfigPara.mSubVEncChn >= 0 && pContext->mConfigPara.mSubViChn >= 0)
    {
        AW_MPI_VI_EnableVirChn(pContext->mSubStream.mVipp, pContext->mSubStream.mViChn);
        AW_MPI_VENC_StartRecvPic(pContext->mSubStream.mVEncChn);
#ifdef SUPPORT_RTSP_TEST
        RtspServerAttr rtsp_attr;
        memset(&rtsp_attr, 0, sizeof(RtspServerAttr));
        rtsp_attr.net_type = pContext->mConfigPara.mRtspNetType;

        if (PT_H264 == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_H264;
        else if (PT_H265 == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_H265;
        else
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_LAST;

        rtsp_attr.frame_rate = pContext->mConfigPara.mSubEncodeFrameRate;

        result = rtsp_open(1, &rtsp_attr);
        if (result)
        {
            aloge("Do rtsp_open fail! ret:%d", result);
            return -1;
        }
#endif
        result = pthread_create(&pContext->mSubStream.mStreamThreadId, NULL, getVencStreamThread, (void*)&pContext->mSubStream);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
    }
    if (pContext->mConfigPara.mSubLapseVEncChn >= 0 && pContext->mConfigPara.mSubLapseViChn >= 0)
    {
        AW_MPI_VI_EnableVirChn(pContext->mSubLapseStream.mVipp, pContext->mSubLapseStream.mViChn);
        AW_MPI_VENC_StartRecvPic(pContext->mSubLapseStream.mVEncChn);
        result = pthread_create(&pContext->mSubLapseStream.mStreamThreadId, NULL, getVencStreamThread, (void*)&pContext->mSubLapseStream);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
    }

    // wb yuv
    if (pContext->mConfigPara.mWbYuvEnable)
    {
        result = pthread_create(&pContext->mWbYuvThreadId, NULL, getWbYuvThread, (void*)pContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
    }

    if (pContext->mConfigPara.mTestDuration > 0)
    {
        cdx_sem_down_timedwait(&pContext->mSemExit, pContext->mConfigPara.mTestDuration*1000);
    }
    else
    {
        cdx_sem_down(&pContext->mSemExit);
    }
    pContext->mbExitFlag = 1;

    void *pRetVal = NULL;

    if (pContext->mConfigPara.mWbYuvEnable)
    {
        pthread_join(pContext->mWbYuvThreadId, &pRetVal);
        alogd("WbYuv pRetVal=%p", pRetVal);
    }

    //stop
    if (pContext->mConfigPara.mMainVEncChn >= 0 && pContext->mConfigPara.mMainViChn >= 0)
    {
        pthread_join(pContext->mMainStream.mStreamThreadId, &pRetVal);
        alogd("mainStream pRetVal=%p", pRetVal);
        AW_MPI_VI_DisableVirChn(pContext->mMainStream.mVipp, pContext->mMainStream.mViChn);
        AW_MPI_VENC_StopRecvPic(pContext->mMainStream.mVEncChn);
        AW_MPI_VENC_ResetChn(pContext->mMainStream.mVEncChn);
        AW_MPI_VENC_DestroyChn(pContext->mMainStream.mVEncChn);
        AW_MPI_VI_DestroyVirChn(pContext->mMainStream.mVipp, pContext->mMainStream.mViChn);
        if(pContext->mMainStream.mFile)
        {
            fclose(pContext->mMainStream.mFile);
            pContext->mMainStream.mFile = NULL;
        }
    }
    if (pContext->mConfigPara.mSubVEncChn >= 0 && pContext->mConfigPara.mSubViChn >= 0)
    {
        pthread_join(pContext->mSubStream.mStreamThreadId, &pRetVal);
        alogd("subStream pRetVal=%p", pRetVal);
        AW_MPI_VI_DisableVirChn(pContext->mSubStream.mVipp, pContext->mSubStream.mViChn);
        AW_MPI_VENC_StopRecvPic(pContext->mSubStream.mVEncChn);
        AW_MPI_VENC_ResetChn(pContext->mSubStream.mVEncChn);
        AW_MPI_VENC_DestroyChn(pContext->mSubStream.mVEncChn);
        AW_MPI_VI_DestroyVirChn(pContext->mSubStream.mVipp, pContext->mSubStream.mViChn);
        if(pContext->mSubStream.mFile)
        {
            fclose(pContext->mSubStream.mFile);
            pContext->mSubStream.mFile = NULL;
        }
    }
    if (pContext->mConfigPara.mSubLapseVEncChn >= 0 && pContext->mConfigPara.mSubLapseViChn >= 0)
    {
        pthread_join(pContext->mSubLapseStream.mStreamThreadId, &pRetVal);
        alogd("LapseStream pRetVal=%p", pRetVal);
        AW_MPI_VI_DisableVirChn(pContext->mSubLapseStream.mVipp, pContext->mSubLapseStream.mViChn);
        AW_MPI_VENC_StopRecvPic(pContext->mSubLapseStream.mVEncChn);
        AW_MPI_VENC_ResetChn(pContext->mSubLapseStream.mVEncChn);
        AW_MPI_VENC_DestroyChn(pContext->mSubLapseStream.mVEncChn);
        AW_MPI_VI_DestroyVirChn(pContext->mSubLapseStream.mVipp, pContext->mSubLapseStream.mViChn);
        if(pContext->mSubLapseStream.mFile)
        {
            fclose(pContext->mSubLapseStream.mFile);
            pContext->mSubLapseStream.mFile = NULL;
        }
    }

    // deinit
    if (pContext->mConfigPara.mMainVEncChn >= 0 && pContext->mConfigPara.mMainViChn >= 0)
    {
        AW_MPI_VI_DisableVipp(pContext->mMainStream.mVipp);
        if (pContext->mMainIspRunFlag)
        {
            AW_MPI_ISP_Stop(pContext->mMainStream.mIsp);
            pContext->mMainIspRunFlag = 0;
        }
        AW_MPI_VI_DestroyVipp(pContext->mMainStream.mVipp);
    }
    if (pContext->mConfigPara.mSubVEncChn >= 0 && pContext->mConfigPara.mSubViChn >= 0)
    {
        AW_MPI_VI_DisableVipp(pContext->mSubStream.mVipp);
        if (pContext->mSubIspRunFlag)
        {
            AW_MPI_ISP_Stop(pContext->mSubStream.mIsp);
            pContext->mSubIspRunFlag = 0;
        }
        AW_MPI_VI_DestroyVipp(pContext->mSubStream.mVipp);
    }
    if (pContext->mConfigPara.mSubLapseVEncChn >= 0 && pContext->mConfigPara.mSubLapseViChn >= 0)
    {
        AW_MPI_VI_DisableVipp(pContext->mSubLapseStream.mVipp);
        if (pContext->mSubIspRunFlag)
        {
            AW_MPI_ISP_Stop(pContext->mSubLapseStream.mIsp);
            pContext->mSubIspRunFlag = 0;
        }
        AW_MPI_VI_DestroyVipp(pContext->mSubLapseStream.mVipp);
    }

#ifdef SUPPORT_RTSP_TEST
    if (pContext->mConfigPara.mMainVEncChn >= 0 && pContext->mConfigPara.mMainViChn >= 0)
    {
        rtsp_stop(0);
        rtsp_close(0);
    }
    if (pContext->mConfigPara.mSubVEncChn >= 0 && pContext->mConfigPara.mSubViChn >= 0)
    {
        rtsp_stop(1);
        rtsp_close(1);
    }
#endif

    ret = AW_MPI_SYS_Exit();
    if (ret != SUCCESS)
    {
        aloge("fatal error! sys exit failed!");
    }
    cdx_sem_deinit(&pContext->mSemExit);
_exit:
    if(pContext!=NULL)
    {
        free(pContext);
        pContext = NULL;
    }
    gpSampleRtspContext = NULL;
    alogd("%s test result: %s", argv[0], ((0 == result) ? "success" : "fail"));
    log_quit();
    return result;
}

