/******************************************************************************
  Copyright (C), 2001-2022, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : sample_smartIPC_demo.c
  Version       : Initial Draft
  Author        : Allwinner
  Created       : 2022/5/12
  Last Modified :
  Description   : Demonstrate Smart IPC scenarios
  Function List :
  History       :
******************************************************************************/

//#define LOG_NDEBUG 0
#define LOG_TAG "sample_smartIPC_demo"

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
#include <utils/VIDEO_FRAME_INFO_S.h>
#include <mpi_ai.h>
#include <mpi_ao.h>
#include <media_common.h>
#include <media_common_aio.h>
#include <media_common_vcodec.h>

#include "sample_smartIPC_demo.h"
#include "sample_smartIPC_demo_config.h"
#include "../common/rtsp_server.h"
#include "../common/record.h"
#include "../common/sample_common_venc.h"
#include "../common/sample_common_osd.h"
#include "../common/tdm_raw_process.h"

#include "motion_pdet.h"

//#define ENABLE_VENC_ADVANCED_PARAM
// #define ENABLE_VENC_GET_DIRMVINFO
#define SUPPORT_RTSP_TEST
//#define SUPPORT_STREAM_OSD_TEST
#define SUPPORT_SAVE_STREAM
#define SUPPORT_AUIDO_TEST
#define SUPPORT_TAKE_PICTURE
#define SUPPORT_MOTION_SEARCH
//#define MOTION_SEARCH_DYNAMIC_ADJUST_UPDATE_INTV
#define SUPPORT_IR_CUT_SWITCH_TEST
#define SUPPORT_SENSOR_FLIP_MIRROR
// #define SUPPORT_OFFLINE_RAW_SIMULATE
#define SUPPORT_PERSON_DETECT

#ifdef SUPPORT_TAKE_PICTURE
#define SUPPORT_RECREATE_JPEG
#endif

#ifdef SUPPORT_IR_CUT_SWITCH_TEST
#define IRCUT_DIR "/sys/devices/platform/soc@2002000/42000358.ircut/sunxi_ircut/ircut"
#define DAY2IR_EVLV_TH 150
#define IR2DAY_EVLV_TH 400
#define IR2DAY_IRWEIGHT_TH 30
#define CHANGE_COUNT_TH 30
#endif

static SampleSmartIPCDemoContext *gpSampleSmartIPCDemoContext = NULL;
static void getVencSpsPpsInfo(VencStreamContext *pStreamContext);

static unsigned int getSysTickMs()
{
    unsigned int ms = 0;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    ms = tv.tv_sec*1000 + tv.tv_usec/1000;
    return ms;
}

static void handle_exit(int signo)
{
    alogd("user want to exit!");
    if(NULL != gpSampleSmartIPCDemoContext)
    {
        cdx_sem_up(&gpSampleSmartIPCDemoContext->mSemExit);
    }
}

static int ParseCmdLine(int argc, char **argv, SampleSmartIPCDemoCmdLineParam *pCmdLinePara)
{
    alogd("path:[%s], arg number is [%d]", argv[0], argc);
    int ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(SampleSmartIPCDemoCmdLineParam));
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

#ifdef SUPPORT_OFFLINE_RAW_SIMULATE
static void *read_bin_file(VI_ATTR_S *mViAttr, char *path, int length)
{
    void *raw_data = NULL;
    long int total_length;
    if (length == 0)
        return NULL;
    VI_PTN_CFG_S* ptn_ctx = &mViAttr->mPtnCfg;
    ptn_ctx->ptn_file = fopen(path, "rb");
    if (ptn_ctx->ptn_file == NULL)
    {
        aloge("open %s fail\n", path);
        return NULL;
    }
    alogd("open %s ok\n", path);
    fseek(ptn_ctx->ptn_file, 0, SEEK_END);
    total_length = ftell(ptn_ctx->ptn_file);
    if (total_length <= 0)
    {
        aloge("maybe raw buf is too large, get buf length err\n");
        fclose(ptn_ctx->ptn_file);
        return NULL;
    }
    rewind(ptn_ctx->ptn_file);
    ptn_ctx->ptn_count_total = total_length/length;
    alogd("%s total_length is %ld, raw conut is %d\n", path, total_length, ptn_ctx->ptn_count_total);
    ptn_ctx->start_frame = 0;
    ptn_ctx->end_frame = ptn_ctx->ptn_count_total - 1;
    raw_data = (void *)malloc((length + 1) * sizeof(char));
    if (!raw_data)
    {
        aloge("malloc raw buf err, size is %d\n", length + 1);
        fclose(ptn_ctx->ptn_file);
        return NULL;
    }
    length = fread(raw_data, 1, length, ptn_ctx->ptn_file);
    if (length == 0)
    {
        aloge("read ptn size is %d\n", length);
		fclose(ptn_ctx->ptn_file);
		free(raw_data);
        return NULL;
    }
    alogd("buf_addr %p, size = %d\n", raw_data, length);
    return raw_data;
}

static int set_video_ptn(int vich, VencStreamContext *mStream, char *RawFilePath,int RawType)
{
    int buf_len = 0;
    unsigned int count = 0;
    void *ptn_vaddr = NULL;
    int pixelformat;
    VI_ATTR_S *mViAttr = &mStream->mViAttr;
    VI_PTN_CFG_S *ptn_ctx = &mViAttr->mPtnCfg;
    char *offline_tuning_path = RawFilePath;
    int rawtype = RawType;
    alogd("rawtype is %d,path is %s\n",rawtype,offline_tuning_path); // pass
    switch (rawtype)
    {
    case MM_PIXEL_FORMAT_RAW_SBGGR8:
        pixelformat = V4L2_PIX_FMT_SBGGR8;
        buf_len = mViAttr->format.width * mViAttr->format.height;
        break;
    case MM_PIXEL_FORMAT_RAW_SGBRG8:
        pixelformat = V4L2_PIX_FMT_SGBRG8;
        buf_len = mViAttr->format.width * mViAttr->format.height;
        break;
    case MM_PIXEL_FORMAT_RAW_SGRBG8:
        pixelformat = V4L2_PIX_FMT_SGRBG8;
        buf_len = mViAttr->format.width * mViAttr->format.height;
        break;
    case MM_PIXEL_FORMAT_RAW_SRGGB8:
        pixelformat = V4L2_PIX_FMT_SRGGB8;
        buf_len = mViAttr->format.width * mViAttr->format.height;
        break;
    case MM_PIXEL_FORMAT_RAW_SBGGR10:
        pixelformat = V4L2_PIX_FMT_SBGGR10;
        buf_len = mViAttr->format.width * mViAttr->format.height * 2;
        break;
    case MM_PIXEL_FORMAT_RAW_SGBRG10:
        pixelformat = V4L2_PIX_FMT_SGBRG10;
        buf_len = mViAttr->format.width * mViAttr->format.height * 2;
        break;
    case MM_PIXEL_FORMAT_RAW_SGRBG10:
        pixelformat = V4L2_PIX_FMT_SGRBG10;
        buf_len = mViAttr->format.width * mViAttr->format.height * 2;
        break;
    case MM_PIXEL_FORMAT_RAW_SRGGB10:
        pixelformat = V4L2_PIX_FMT_SRGGB10;
        buf_len = mViAttr->format.width * mViAttr->format.height * 2;
        break;
    case MM_PIXEL_FORMAT_RAW_SBGGR12:
        pixelformat = V4L2_PIX_FMT_SBGGR12;
        buf_len = mViAttr->format.width * mViAttr->format.height * 2;
        break;
    case MM_PIXEL_FORMAT_RAW_SGBRG12:
        pixelformat = V4L2_PIX_FMT_SGBRG12;
        buf_len = mViAttr->format.width * mViAttr->format.height * 2;
        break;
    case MM_PIXEL_FORMAT_RAW_SGRBG12:
        pixelformat = V4L2_PIX_FMT_SGRBG12;
        buf_len = mViAttr->format.width * mViAttr->format.height * 2;
        break;
    case MM_PIXEL_FORMAT_RAW_SRGGB12:
        pixelformat = V4L2_PIX_FMT_SRGGB12;
        buf_len = mViAttr->format.width * mViAttr->format.height * 2;
        break;
    default:
        pixelformat = V4L2_PIX_FMT_SBGGR10;
        buf_len = mViAttr->format.width * mViAttr->format.height * 2;
        break;
    }
    alogd("buf_length is %d,raw format is %d\n",buf_len,rawtype);
    ptn_vaddr = read_bin_file(mViAttr, offline_tuning_path, buf_len);
    if (!ptn_vaddr)
    {
        aloge("ptn_vaddr is null !!!\n");
        ptn_ctx->ptn_en = 0;
        return -1;
    }
    else
    {
        alogd("read first count to ptn!\n");
    }

    ptn_ctx->ptn_en = 1;
    ptn_ctx->ptn_cfg.ptn_en = 1;
    ptn_ctx->ptn_cfg.ptn_type = 1;
    ptn_ctx->ptn_type = 1;
    ptn_ctx->ptn_frame_temp = 0;
    ptn_ctx->ptn_cfg.ptn_addr = ptn_vaddr;
    ptn_ctx->ptn_cfg.ptn_size = buf_len;
    ptn_ctx->ptn_cfg.ptn_w = mViAttr->format.width;
    ptn_ctx->ptn_cfg.ptn_h = mViAttr->format.height;
    ptn_ctx->ptn_cfg.ptn_fmt = pixelformat;
    ptn_ctx->ptn_count = &count;
    return 0;
}

static void read_next_bin_file(SampleSmartIPCDemoContext *pContext)
{
    int length;
    int start_conut = pContext->mConfigPara.mMainOfflineRawSimulateCntStart;
    int end_count = pContext->mConfigPara.mMainOfflineRawSimulateCntEnd;
    int ptn_type = pContext->mConfigPara.mMainOfflineRawSimulateType;
    VI_ATTR_S *mViAttr = &pContext->mMainStream.mViAttr;
    VI_PTN_CFG_S *ptn_ctx = &mViAttr->mPtnCfg;
    if (ptn_ctx->ptn_file == NULL)
    {
        aloge("ptn_file is null !!!\n");
        return;
    }
    if (ptn_type)
    {
        if (start_conut == end_count)
        {
            fseek(ptn_ctx->ptn_file, -1 * ptn_ctx->ptn_cfg.ptn_size, SEEK_CUR);
            *ptn_ctx->ptn_count = *ptn_ctx->ptn_count - 1;
        }
        else
        {
            if (*ptn_ctx->ptn_read_dir == 1)
            {
                fseek(ptn_ctx->ptn_file, -2 * ptn_ctx->ptn_cfg.ptn_size, SEEK_CUR);
                *ptn_ctx->ptn_count = *ptn_ctx->ptn_count - 2;
                if(*ptn_ctx->ptn_count <= start_conut)
                {
                     *ptn_ctx->ptn_read_dir = 0;
                }
            }
            if (*ptn_ctx->ptn_count >= end_count)
            {
                *ptn_ctx->ptn_read_dir = 1;
            }
        }
    }
    else
    {
        if ((*ptn_ctx->ptn_count >= end_count) || (*ptn_ctx->ptn_count <= start_conut))
        {
            rewind(ptn_ctx->ptn_file);
            fseek(ptn_ctx->ptn_file, start_conut * ptn_ctx->ptn_cfg.ptn_size, SEEK_CUR);
            *ptn_ctx->ptn_count = start_conut;
        }
    }
    length = fread(ptn_ctx->ptn_cfg.ptn_addr, 1, ptn_ctx->ptn_cfg.ptn_size, ptn_ctx->ptn_file);
    if (length != 0)
    {
        if(*ptn_ctx->ptn_count % 10 == 0)
            alogd("read ptn buf[%d] size is %d\n", *ptn_ctx->ptn_count, length);
        (*ptn_ctx->ptn_count)++;
    }
    else
    {
        aloge("read failed,read ptn buf[%d] size is %d\n", *ptn_ctx->ptn_count, length);
    }
}

static void close_video_ptn(VI_PTN_CFG_S *ptn_ctx)
{
    if(ptn_ctx->ptn_file)
    {
        fclose(ptn_ctx->ptn_file);
        ptn_ctx->ptn_file = NULL;
    }
    if(ptn_ctx->ptn_cfg.ptn_addr)
    {
        free(ptn_ctx->ptn_cfg.ptn_addr);
        ptn_ctx->ptn_cfg.ptn_addr = NULL;
    }
}
#endif

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

static VENC_RC_MODE_E getRcModeFromConfig(int rc_mode, PAYLOAD_TYPE_E eEncType)
{
    VENC_RC_MODE_E eRcMode = VENC_RC_MODE_BUTT;

    if (PT_H264 == eEncType)
    {
        switch (rc_mode)
        {
        case 0:
            eRcMode = VENC_RC_MODE_H264CBR;
            break;
        case 1:
            eRcMode = VENC_RC_MODE_H264VBR;
            break;
        case 2:
            eRcMode = VENC_RC_MODE_H264FIXQP;
            break;
        default:
            aloge("not support! use default cbr mode");
            eRcMode = VENC_RC_MODE_H264CBR;
            break;
        }
    }
    else if (PT_H265 == eEncType)
    {
        switch (rc_mode)
        {
        case 0:
            eRcMode = VENC_RC_MODE_H265CBR;
            break;
        case 1:
            eRcMode = VENC_RC_MODE_H265VBR;
            break;
        case 2:
            eRcMode = VENC_RC_MODE_H265FIXQP;
            break;
        default:
            aloge("not support! use default cbr mode");
            eRcMode = VENC_RC_MODE_H265CBR;
            break;
        }
    }
    else if (PT_MJPEG == eEncType)
    {
        switch (rc_mode)
        {
        case 0:
            eRcMode = VENC_RC_MODE_MJPEGCBR;
            break;
        case 2:
            eRcMode = VENC_RC_MODE_MJPEGFIXQP;
            break;
        default:
            aloge("not support! use default cbr mode");
            eRcMode = VENC_RC_MODE_MJPEGCBR;
            break;
        }
    }

    return eRcMode;
}

static int checkInputParams(SampleSmartIPCDemoConfig *pConfig)
{
    // check use same rtsp id
    if (pConfig->mMainEnable && pConfig->mSubEnable)
    {
        if (-1 != pConfig->mMainRtspID && pConfig->mMainRtspID == pConfig->mSubRtspID)
        {
            aloge("fatal error, there are same rtsp ids, MainRtspID:%d, SubRtspID:%d", pConfig->mMainRtspID, pConfig->mSubRtspID);
            return -1;
        }
    }
    if (pConfig->mMainEnable && pConfig->mSubEnable && pConfig->mThreeEnable)
    {
        if (-1 != pConfig->mMainRtspID && (pConfig->mMainRtspID == pConfig->mSubRtspID  ||
            pConfig->mMainRtspID == pConfig->mThreeRtspID || pConfig->mSubRtspID == pConfig->mThreeRtspID))
        {
            aloge("fatal error, there are same rtsp ids, MainRtspID:%d, SubRtspID:%d, ThreeRtspID:%d",
                pConfig->mMainRtspID, pConfig->mSubRtspID, pConfig->mThreeRtspID);
            return -1;
        }
    }
    if (pConfig->mMainEnable && pConfig->mSubEnable && pConfig->mThreeEnable && pConfig->mFourEnable)
    {
        if (-1 != pConfig->mMainRtspID && (pConfig->mMainRtspID == pConfig->mSubRtspID  ||
            pConfig->mMainRtspID == pConfig->mThreeRtspID || pConfig->mMainRtspID == pConfig->mFourRtspID ||
            pConfig->mSubRtspID == pConfig->mThreeRtspID || pConfig->mSubRtspID == pConfig->mFourRtspID ||
            pConfig->mThreeRtspID == pConfig->mFourRtspID))
        {
            aloge("fatal error, there are same rtsp ids, MainRtspID:%d, SubRtspID:%d, ThreeRtspID:%d, FourRtspID:%d",
                pConfig->mMainRtspID, pConfig->mSubRtspID, pConfig->mThreeRtspID, pConfig->mFourRtspID);
            return -1;
        }
    }

    if (pConfig->mMainViStitchMode > DMA_STITCH_NONE) {
        /* mViStitchIspChannelId is ISP0, it will be run when mViStitchMode > DMA_STITCH_NONE */
        if (pConfig->mMainIsp != 1) {
            aloge("when mMainViStitchMode = %d, it must be set mMainIsp to 1, please check!!!\n", pConfig->mMainViStitchMode);
            return -1;
        }
        pConfig->mMainViStitchIspChannelId = 0;
    }

    if (pConfig->mMainDetectMipiDeskEnable) {
        if (pConfig->mMainMipiChannel < 0 || pConfig->mMainMipiChannel > 1) {
            aloge("when mDetectMipiDeskEnable = %d, it must be set mMipiChannel to 0 or 1\n", pConfig->mMainDetectMipiDeskEnable);
            return -1;
        }
    }

    return 0;
}

static unsigned int getMultiSensorOnlineEnable(SampleSmartIPCDemoConfig *pConfig)
{
    int online_cnt = 0;

    if (pConfig->mMainEnable && (pConfig->mMainOnlineEnable || pConfig->mMain2ndOnlineEnable))
    {
        online_cnt++;
    }
    if (pConfig->mSubEnable && (pConfig->mSubOnlineEnable || pConfig->mSub2ndOnlineEnable))
    {
        online_cnt++;
    }
    if (pConfig->mThreeEnable && (pConfig->mThreeOnlineEnable || pConfig->mThree2ndOnlineEnable))
    {
        online_cnt++;
    }
    if (pConfig->mFourEnable && (pConfig->mFourOnlineEnable || pConfig->mFour2ndOnlineEnable))
    {
        online_cnt++;
    }

    if (online_cnt >= 2)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static ERRORTYPE loadSampleConfig(SampleSmartIPCDemoConfig *pConfig, const char *conf_path)
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

        // rtsp
        pConfig->mRtspNetType = GetConfParaInt(&stConfParser, CFG_RtspNetType, 0);
        // common params
        pConfig->mStreamBufSize = GetConfParaInt(&stConfParser, CFG_StreamBufSize, 0);
        pConfig->mVeRecRefBufReduceEnable = GetConfParaInt(&stConfParser, CFG_VeRecRefBufReduceEnable, 0);
        pConfig->mProductMode = GetConfParaInt(&stConfParser, CFG_ProductMode, 0);
        pConfig->mVbrOptEnable = GetConfParaInt(&stConfParser, CFG_VBR_OPT_ENABLE, 0);
        pConfig->mVbrOptRcPriority = GetConfParaInt(&stConfParser, CFG_VbrOptRcPriority, -1);
        pConfig->mVbrOptRcQualityLevel = GetConfParaInt(&stConfParser, CFG_VbrOptRcQualityLevel, -1);

        pConfig->mRcMode = GetConfParaInt(&stConfParser, CFG_RC_MODE, 0);
        pConfig->mInitQp = GetConfParaInt(&stConfParser, CFG_INIT_QP, 0);
        pConfig->mMinIQp = GetConfParaInt(&stConfParser, CFG_MIN_I_QP, 0);
        pConfig->mMaxIQp = GetConfParaInt(&stConfParser, CFG_MAX_I_QP, 0);
        pConfig->mMinPQp = GetConfParaInt(&stConfParser, CFG_MIN_P_QP, 0);
        pConfig->mMaxPQp = GetConfParaInt(&stConfParser, CFG_MAX_P_QP, 0);
        pConfig->mEnMbQpLimit = GetConfParaInt(&stConfParser, CFG_MB_QP_LIMIT, 0);
        pConfig->mMovingTh = GetConfParaInt(&stConfParser, CFG_MOVING_TH, 0);
        pConfig->mQuality = GetConfParaInt(&stConfParser, CFG_QUALITY, 0);
        pConfig->mPBitsCoef = GetConfParaInt(&stConfParser, CFG_P_BITS_COEF, 0);
        pConfig->mIBitsCoef = GetConfParaInt(&stConfParser, CFG_I_BITS_COEF, 0);
        // isp and ve linkage
        pConfig->mIspAndVeLinkageEnable = GetConfParaInt(&stConfParser, CFG_IspAndVeLinkageEnable, 0);
        pConfig->mRegionLinkEnable = GetConfParaInt(&stConfParser, CFG_RegionLinkEnable, 0);
        pConfig->mRegionLinkTexDetectEnable = GetConfParaInt(&stConfParser, CFG_RegionLinkTexDetectEnable, 0);
        pConfig->mRegionLinkMotionDetectEnable = GetConfParaInt(&stConfParser, CFG_RegionLinkMotionDetectEnable, 0);
        pConfig->mRegionLinkMotionDetectInv = GetConfParaInt(&stConfParser, CFG_RegionLinkMotionDetectInv, 0);
        pConfig->mCameraAdaptiveMovingAndStaticEnable = GetConfParaInt(&stConfParser, CFG_CameraAdaptiveMovingAndStaticEnable, 0);
        pConfig->mVencLensMovingMaxQp = GetConfParaInt(&stConfParser, CFG_VencLensMovingMaxQp, 0);
        // wb yuv
        pConfig->mWbYuvEnable = GetConfParaInt(&stConfParser, CFG_WbYuvEnable, 0);
        pConfig->mWbYuvBufNum = GetConfParaInt(&stConfParser, CFG_WbYuvBufNum, 0);
        pConfig->mWbYuvScalerRatio = GetConfParaInt(&stConfParser, CFG_WbYuvScalerRatio, 0);
        pConfig->mWbYuvStartIndex = GetConfParaInt(&stConfParser, CFG_WbYuvStartIndex, 0);
        pConfig->mWbYuvTotalCnt = GetConfParaInt(&stConfParser, CFG_WbYuvTotalCnt, 0);
        pConfig->mWbYuvStreamChn = GetConfParaInt(&stConfParser, CFG_WbYuvStreamChannel, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_WbYuvFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mWbYuvFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        // test code
        pConfig->mViTimeoutResetDisable = GetConfParaInt(&stConfParser, CFG_ViTimeoutResetDisable, 0);
        pConfig->mTestTriggerViTimeout = GetConfParaInt(&stConfParser, CFG_TestTriggerViTimeout, 0);
        // encpp sharp debug
        pConfig->mEncppSharpDebugDisable = GetConfParaInt(&stConfParser, CFG_EncppSharpDebugDisable, 0);
        // tmd speed down
        pConfig->mTdmSpeedDownEnable = GetConfParaInt(&stConfParser, CFG_TdmSpeedDownEnable, 0);
        pConfig->mTdmTxValidNum = GetConfParaInt(&stConfParser, CFG_TdmTxValidNum, 0);
        pConfig->mTdmTxInvalidNum = GetConfParaInt(&stConfParser, CFG_TdmTxInvalidNum, 0);
        pConfig->mTdmTxValidNumOffset = GetConfParaInt(&stConfParser, CFG_TdmTxValidNumOffset, 0);
        // 2dnr and 3dnr
        pConfig->mVe2DNRDisable = GetConfParaInt(&stConfParser, CFG_Ve2DNRDisable, 0);
        pConfig->mVe3DNRDisable = GetConfParaInt(&stConfParser, CFG_Ve3DNRDisable, 0);
        //Sensor Flip Mirror
        pConfig->mSensorFlipMirror = GetConfParaInt(&stConfParser, CFG_SensorFlipMirror, 0);
        pConfig->mFlipSwitchInterval = GetConfParaInt(&stConfParser, CFG_FlipSwitchInterval, 0);
        pConfig->mFlipMainVippEnable = GetConfParaInt(&stConfParser, CFG_FlipMainVippEnable, 0);
        pConfig->mFlipSubVippEnable = GetConfParaInt(&stConfParser, CFG_FlipSubVippEnable, 0);
        //ve and isp colorspace
        pConfig->mIspColorSpace = GetConfParaInt(&stConfParser, CFG_IspColorSpace, 0);
        // others
        pConfig->mTestDuration = GetConfParaInt(&stConfParser, CFG_TestDuration, 0);
        // ir cut switch test
        pConfig->mIrCutSwitchTestEnable = GetConfParaInt(&stConfParser, CFG_IrCutSwitchTestEnable, 0);
        pConfig->mIrCutSwitchMode = GetConfParaInt(&stConfParser, CFG_IrCutSwitchTestMode, 0);
        pConfig->mIrCutSwitchInterval = GetConfParaInt(&stConfParser, CFG_IrCutSwitchInterval, 0);
        pConfig->mIrCutSwitchIsp = GetConfParaInt(&stConfParser, CFG_IrCutSwitchIsp, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_IrCutSwitchIrBinPath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mIrCutSwitchIrBinPath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        ptr = (char*)GetConfParaString(&stConfParser, CFG_IrCutSwitchRgbBinPath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mIrCutSwitchRgbBinPath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        // audio test
        pConfig->audio_test_enable = GetConfParaInt(&stConfParser, CFG_AUDIO_TEST_ENABLE, 0);
        pConfig->capture_en = GetConfParaInt(&stConfParser, CFG_CAPTURE_EN, 0);
        pConfig->capture_sample_rate = GetConfParaInt(&stConfParser, CFG_CAPTURE_SAMPLE_RATE, 16000);
        pConfig->capture_bit_witdh = GetConfParaInt(&stConfParser, CFG_CAPTURE_BIT_WIDTH, 16);
        pConfig->capture_channel_cnt = GetConfParaInt(&stConfParser, CFG_CAPTURE_CHANNEL_CNT, 1);
        pConfig->capture_ans_en = GetConfParaInt(&stConfParser, CFG_CAPTURE_ANS_EN, 1);
        pConfig->capture_agc_en = GetConfParaInt(&stConfParser, CFG_CAPTURE_AGC_EN, 1);
        pConfig->capture_aec_en = GetConfParaInt(&stConfParser, CFG_CAPTURE_AEC_EN, 1);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_CAPTURE_SAVE_FILE, NULL);
        if (ptr != NULL)
            strncpy(pConfig->capture_save_file, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->capture_save_file_duration = GetConfParaInt(&stConfParser, CFG_CAPTURE_SAVE_FILE_DURATION, 600);
        pConfig->capture_save_file_max_cnt = GetConfParaInt(&stConfParser, CFG_CAPTURE_SAVE_FILE_MAX_CNT, 10);
        pConfig->playback_en = GetConfParaInt(&stConfParser, CFG_PLAYBACK_EN, 1);
        pConfig->playback_volume = GetConfParaInt(&stConfParser, CFG_PLAYBACK_volume, 80);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_PLAYBACK_FILE, NULL);
        if (ptr != NULL)
            strncpy(pConfig->playback_file, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        alogd("audio test enable %d, capture %d-%d-%d-%d-%d-%d-%d-%s, playback %d-%d-%s",
            pConfig->audio_test_enable, pConfig->capture_en, pConfig->capture_sample_rate, pConfig->capture_bit_witdh,
            pConfig->capture_channel_cnt, pConfig->capture_ans_en, pConfig->capture_agc_en, pConfig->capture_aec_en,
            pConfig->capture_save_file, pConfig->playback_en, pConfig->playback_volume, pConfig->playback_file);
        // motion search
        pConfig->motionAlarm_on = GetConfParaInt(&stConfParser, CFG_motionAlarm_on, 0);
        pConfig->motionAlarm_result_print_interval = GetConfParaInt(&stConfParser, CFG_motionAlarm_result_print_interval, 0);
        pConfig->motionAlarm_sensitivity = GetConfParaInt(&stConfParser, CFG_motionAlarm_sensitivity, 0);
        pConfig->motionAlarm_support_zone = GetConfParaInt(&stConfParser, CFG_motionAlarm_support_zone, 0);
        pConfig->motionAlarm_left_up_x = GetConfParaInt(&stConfParser, CFG_motionAlarm_left_up_x, 0);
        pConfig->motionAlarm_left_up_y = GetConfParaInt(&stConfParser, CFG_motionAlarm_left_up_y, 0);
        pConfig->motionAlarm_left_bottom_x = GetConfParaInt(&stConfParser, CFG_motionAlarm_left_bottom_x, 0);
        pConfig->motionAlarm_left_bottom_y = GetConfParaInt(&stConfParser, CFG_motionAlarm_left_bottom_y, 0);
        pConfig->motionAlarm_right_bottom_x = GetConfParaInt(&stConfParser, CFG_motionAlarm_right_bottom_x, 0);
        pConfig->motionAlarm_right_bottom_y = GetConfParaInt(&stConfParser, CFG_motionAlarm_right_bottom_y, 0);
        pConfig->motionAlarm_right_up_x = GetConfParaInt(&stConfParser, CFG_motionAlarm_right_up_x, 0);
        pConfig->motionAlarm_right_up_y = GetConfParaInt(&stConfParser, CFG_motionAlarm_right_up_y, 0);
        pConfig->motionAlarm_update_interval = GetConfParaInt(&stConfParser, CFG_motionAlarm_update_interval, 0);
        pConfig->motionAlarm_useDefaultCfgEnable = GetConfParaInt(&stConfParser, CFG_motionAlarm_useDefaultCfgEnable, 0);
        pConfig->motionAlarm_HorizontalRegionNum = GetConfParaInt(&stConfParser, CFG_motionAlarm_HorizontalRegionNum, 0);
        pConfig->motionAlarm_VerticalRegionNum = GetConfParaInt(&stConfParser, CFG_motionAlarm_VerticalRegionNum, 0);
        pConfig->motionAlarm_Threshold_High = GetConfParaInt(&stConfParser, CFG_motionAlarm_Threshold_High, 0);
        pConfig->motionAlarm_Threshold_MediumHigh = GetConfParaInt(&stConfParser, CFG_motionAlarm_Threshold_MediumHigh, 0);
        pConfig->motionAlarm_Threshold_Default = GetConfParaInt(&stConfParser, CFG_motionAlarm_Threshold_Default, 0);
        pConfig->motionAlarm_Threshold_MediumLow = GetConfParaInt(&stConfParser, CFG_motionAlarm_Threshold_MediumLow, 0);
        pConfig->motionAlarm_Threshold_Low = GetConfParaInt(&stConfParser, CFG_motionAlarm_Threshold_Low, 0);
        // main stream
        pConfig->mMainEnable = GetConfParaInt(&stConfParser, CFG_MainEnable, 0);
        pConfig->mMainRtspID = GetConfParaInt(&stConfParser, CFG_MainRtspID, 0);
        pConfig->mMainIsp = GetConfParaInt(&stConfParser, CFG_MainIsp, 0);
        pConfig->mMainIspAlgoFreq = GetConfParaInt(&stConfParser, CFG_MainIspAlGOFREQ, 0);
        pConfig->mMainIspD3dLbcRatio = GetConfParaInt(&stConfParser, CFG_MainIspD3dLbcRatio, 0);
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
        pConfig->mMainOnlineEnable = GetConfParaInt(&stConfParser, CFG_MainOnlineEnable, 0);
        pConfig->mMainOnlineShareBufNum = GetConfParaInt(&stConfParser, CFG_MainOnlineShareBufNum, 0);
        pConfig->mMainEncppEnable = GetConfParaInt(&stConfParser, CFG_MainEncppEnable, 0);
        pConfig->mMainVeRefFrameLbcMode = GetConfParaInt(&stConfParser, CFG_MainVeRefFrameLbcMode, 0);
        pConfig->mMainKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_MainKeyFrameInterval, 0);
        pConfig->mMainViStitchMode = GetConfParaInt(&stConfParser, CFG_MainViStitchMode, 0);
        pConfig->mMainViStitchScalerEn = GetConfParaInt(&stConfParser, CFG_MainViStitchScalerEn, 0);
        pConfig->mMainViStitchSensorAScalerWidth = GetConfParaInt(&stConfParser, CFG_MainViStitchSorASclrWidth, 0);
        pConfig->mMainViStitchSensorAScalerheight = GetConfParaInt(&stConfParser, CFG_MainViStitchSorASclrHeight, 0);
        pConfig->mMainViStitchSensorBScalerWidth = GetConfParaInt(&stConfParser, CFG_MainViStitchSorBSclrWidth, 0);
        pConfig->mMainViStitchSensorBScalerheight = GetConfParaInt(&stConfParser, CFG_MainViStitchSorBSclrHeight, 0);
        pConfig->mMainViSyncCtrlEnable = GetConfParaInt(&stConfParser, CFG_MainViSyncCtrlEnable, 0);
        pConfig->mMainVeRxInputBufmultiplexEnable = GetConfParaInt(&stConfParser, CFG_MainVeRxInputBufMultiplexEnable, 0);

        pConfig->mMainIspTestEnable = GetConfParaInt(&stConfParser, CFG_MainIspTestEnable, 0);
        pConfig->mMainIspTestIntervalMs = GetConfParaInt(&stConfParser, CFG_MainIspTestIntervalMs, 0);
        pConfig->mMainDetectMipiDeskEnable = GetConfParaInt(&stConfParser, CFG_MainDetectMipiEnable, 0);
        pConfig->mMainDetectIntervalMs = GetConfParaInt(&stConfParser, CFG_MainDetectMipiIntrervMs, 0);
        pConfig->mMainMipiChannel = GetConfParaInt(&stConfParser, CFG_MainDetectMipiChannel, 0);

        ptr = (char*)GetConfParaString(&stConfParser, CFG_MainFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mMainFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mMainSaveOneFileDuration = GetConfParaInt(&stConfParser, CFG_MainSaveOneFileDuration, 0);
        pConfig->mMainSaveMaxFileCnt = GetConfParaInt(&stConfParser, CFG_MainSaveMaxFileCnt, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_MainDrawOSDText, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mMainDrawOSDText, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");

        pConfig->mMainViMirror = GetConfParaInt(&stConfParser, CFG_MainViMirror, 0);
        pConfig->mMainViFlip = GetConfParaInt(&stConfParser, CFG_MainViFlip, 0);

        // main recreate config
        pConfig->mMainEncodeDynamicConfig.mVencRecreateEnable = GetConfParaInt(&stConfParser, CFG_MainVencRecreateEnalbe, 0);
        pConfig->mMainEncodeDynamicConfig.mVencRecreateLoopCnt = GetConfParaInt(&stConfParser, CFG_MainVencRecreateLoopCnt, 0);
        pConfig->mMainEncodeDynamicConfig.mVencRecreateInterval = GetConfParaInt(&stConfParser, CFG_MainVencRecreateInterval, 0);
        pConfig->mMainEncodeDynamicConfig.mVideoFrameRate = GetConfParaInt(&stConfParser, CFG_MainRecreateFrameRate, 0);
        pConfig->mMainEncodeDynamicConfig.mVideoBitRate = GetConfParaInt(&stConfParser, CFG_MainRecreateBitRate, 0);
        pConfig->mMainEncodeDynamicConfig.dstWidth = GetConfParaInt(&stConfParser, CFG_MainRecreateDstWidth, 0);
        pConfig->mMainEncodeDynamicConfig.dstHeight = GetConfParaInt(&stConfParser, CFG_MainRecreateDstHeight, 0);
        pConfig->mMainEncodeDynamicConfig.mKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_MainRecreateKeyFrameInv, 0);

        ptr = (char*)GetConfParaString(&stConfParser, CFG_MainRecreateEncoder, NULL);
        if (ptr != NULL)
        {
            if (!strcmp(ptr, "H.264"))
            {
                pConfig->mMainEncodeDynamicConfig.mVideoEncoderFmt = PT_H264;
                alogd("H.264");
            }
            else if (!strcmp(ptr, "H.265"))
            {
                pConfig->mMainEncodeDynamicConfig.mVideoEncoderFmt = PT_H265;
                alogd("H.265");
            }
            else
            {
                aloge("error conf encoder type");
            }
        }
        else
        {
            aloge("fatal error! ptr is NULL!");
        }

        // main tdm raw
        pConfig->mMainIspTdmRawProcessType = GetConfParaInt(&stConfParser, CFG_MainIspTdmRawProcessType, -1);
        pConfig->mMainIspTdmRawWidth = GetConfParaInt(&stConfParser, CFG_MainIspTdmRawWidth, 1920);
        pConfig->mMainIspTdmRawHeight = GetConfParaInt(&stConfParser, CFG_MainIspTdmRawHeight, 1080);
        pConfig->mMainIspTdmRawRxBufNum = GetConfParaInt(&stConfParser, CFG_MainIspTdmRawRxBufNum, 5);
        pConfig->mMainIspTdmRawProcessFrameCntMin = GetConfParaInt(&stConfParser, CFG_MainIspTdmRawProcessFrameCntMin, 0);
        pConfig->mMainIspTdmRawProcessFrameCntMax = GetConfParaInt(&stConfParser, CFG_MainIspTdmRawProcessFrameCntMax, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_MainIspTdmRawFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mMainIspTdmRawFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        //main offline raw simulate
        pConfig->mMainOfflineRawSimulateType = GetConfParaInt(&stConfParser, CFG_MainOfflineRawSimulateType, -1);
        pConfig->mMainOfflineRawType = GetConfParaInt(&stConfParser, CFG_MainOfflineRawType, 5);
        pConfig->mMainOfflineRawSimulateCntStart = GetConfParaInt(&stConfParser, CFG_MainOfflineRawSimulateCntStart, 0);
        pConfig->mMainOfflineRawSimulateCntEnd = GetConfParaInt(&stConfParser, CFG_MainOfflineRawSimulateCntEnd, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_MainOfflineRawFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mMainOfflineRawFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        // main 2nd stream
        pConfig->mMain2ndEnable = GetConfParaInt(&stConfParser, CFG_Main2ndEnable, 0);
        pConfig->mMain2ndVipp = GetConfParaInt(&stConfParser, CFG_Main2ndVipp, 0);
        pConfig->mMain2ndViChn = GetConfParaInt(&stConfParser, CFG_Main2ndViChn, 0);
        pConfig->mMain2ndSrcWidth = GetConfParaInt(&stConfParser, CFG_Main2ndSrcWidth, 0);
        pConfig->mMain2ndSrcHeight = GetConfParaInt(&stConfParser, CFG_Main2ndSrcHeight, 0);
        pConfig->mMain2ndPixelFormat = getPicFormatFromConfig(&stConfParser, CFG_Main2ndPixelFormat);
        pConfig->mMain2ndViBufNum = GetConfParaInt(&stConfParser, CFG_Main2ndViBufNum, 0);
        pConfig->mMain2ndSrcFrameRate = GetConfParaInt(&stConfParser, CFG_Main2ndSrcFrameRate, 0);
        pConfig->mMain2ndVEncChn = GetConfParaInt(&stConfParser, CFG_Main2ndVEncChn, 0);
        pConfig->mMain2ndEncodeType = getEncoderTypeFromConfig(&stConfParser, CFG_Main2ndEncodeType);
        pConfig->mMain2ndEncodeWidth = GetConfParaInt(&stConfParser, CFG_Main2ndEncodeWidth, 0);
        pConfig->mMain2ndEncodeHeight = GetConfParaInt(&stConfParser, CFG_Main2ndEncodeHeight, 0);
        pConfig->mMain2ndEncodeFrameRate = GetConfParaInt(&stConfParser, CFG_Main2ndEncodeFrameRate, 0);
        pConfig->mMain2ndEncodeBitrate = GetConfParaInt(&stConfParser, CFG_Main2ndEncodeBitrate, 0);
        pConfig->mMain2ndEncppSharpAttenCoefPer = 100 * pConfig->mMain2ndEncodeWidth / pConfig->mMainEncodeWidth;
        pConfig->mMain2ndOnlineEnable = GetConfParaInt(&stConfParser, CFG_Main2ndOnlineEnable, 0);
        pConfig->mMain2ndOnlineShareBufNum = GetConfParaInt(&stConfParser, CFG_Main2ndOnlineShareBufNum, 0);
        pConfig->mMain2ndEncppEnable = GetConfParaInt(&stConfParser, CFG_Main2ndEncppEnable, 0);
        pConfig->mMain2ndVeRefFrameLbcMode = GetConfParaInt(&stConfParser, CFG_Main2ndVeRefFrameLbcMode, 0);
        pConfig->mMain2ndKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_Main2ndKeyFrameInterval, 0);
        pConfig->mMain2ndViStitchMode = GetConfParaInt(&stConfParser, CFG_Main2ndViStitchMode, 0);

        pConfig->mMain2ndViStitchScalerEn = GetConfParaInt(&stConfParser, CFG_Main2ndViStitchScalerEn, 0);
        pConfig->mMain2ndViStitchSensorAScalerWidth = GetConfParaInt(&stConfParser, CFG_Main2ndViStitchSorASclrWidth, 0);
        pConfig->mMain2ndViStitchSensorAScalerheight = GetConfParaInt(&stConfParser, CFG_Main2ndViStitchSorASclrHeight, 0);
        pConfig->mMain2ndViStitchSensorBScalerWidth = GetConfParaInt(&stConfParser, CFG_Main2ndViStitchSorBSclrWidth, 0);
        pConfig->mMain2ndViStitchSensorBScalerheight = GetConfParaInt(&stConfParser, CFG_Main2ndViStitchSorBSclrHeight, 0);

        ptr = (char*)GetConfParaString(&stConfParser, CFG_Main2ndFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mMain2ndFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mMain2ndSaveOneFileDuration = GetConfParaInt(&stConfParser, CFG_Main2ndSaveOneFileDuration, 0);
        pConfig->mMain2ndSaveMaxFileCnt = GetConfParaInt(&stConfParser, CFG_Main2ndSaveMaxFileCnt, 0);
        pConfig->mMain2ndVeRxInputBufmultiplexEnable = GetConfParaInt(&stConfParser, CFG_Main2ndVeRxInputBufMultiplexEnable, 0);
        pConfig->mMain2ndTakePicture = GetConfParaInt(&stConfParser, CFG_Main2ndTakePicture, 0);
        pConfig->mMain2ndTakePictureViChn = GetConfParaInt(&stConfParser, CFG_Main2ndTakePictureViChn, 0);
        pConfig->mMain2ndTakePictureVencChn = GetConfParaInt(&stConfParser, CFG_Main2ndTakePictureVencChn, 8);
        pConfig->mMain2ndTakePictureInterval = GetConfParaInt(&stConfParser, CFG_Main2ndTakePictureInterval, 10);
        ptr = (char *)GetConfParaString(&stConfParser, CFG_Main2ndTakePictureFile, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mMain2ndTakePictureFile, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mMain2ndTakePictureFileCnt = GetConfParaInt(&stConfParser, CFG_Main2ndTakePictureFileCnt, 10);
        pConfig->mMain2ndTakePictureOnlyCaptureYuv = GetConfParaInt(&stConfParser, CFG_Main2ndTakePictureOnlyCaptureYuv, 0);
        // main 2nd pdet
        pConfig->mMain2ndPdetEnable = GetConfParaInt(&stConfParser, CFG_Main2ndPdetEnable, 0);
        pConfig->mMain2ndPdetViChn = GetConfParaInt(&stConfParser, CFG_Main2ndPdetViChn, 0);
        pConfig->mMain2ndPdetInputWidth = GetConfParaInt(&stConfParser, CFG_Main2ndPdetInputWidth, 0);
        pConfig->mMain2ndPdetInputHeight = GetConfParaInt(&stConfParser, CFG_Main2ndPdetInputHeight, 0);
        pConfig->mMain2ndPdetConfThres =  GetConfParaDouble(&stConfParser, CFG_Main2ndPdetConfThres, 0);
        pConfig->mMain2ndPdetRunIinterval = GetConfParaInt(&stConfParser, CFG_Main2ndPdetRunIinterval, 0);

        // strategy of pdet and motion detect
        pConfig->mMotionPdetSensitivity = GetConfParaInt(&stConfParser, CFG_MOTION_PDET_SENSITIVITY, 2);
        pConfig->mJudgeMotionNum = GetConfParaInt(&stConfParser, CFG_JUDGE_MOTION_NUM, 20);
        pConfig->mMotionPdetThres =  GetConfParaDouble(&stConfParser, CFG_MITION_PDET_THRES, 0.4);
        pConfig->mMotionDynamicThres =  GetConfParaDouble(&stConfParser, CFG_MITION_DYNAMIC_THRES, 0.3);
        alogd("motiondetect param: {mJudgeMotionNum:%d, mMotionPdetThres:%f, mMotionDynamicThres:%f}",
                pConfig->mJudgeMotionNum, pConfig->mMotionPdetThres, pConfig->mMotionDynamicThres);

        // sub stream
        pConfig->mSubEnable = GetConfParaInt(&stConfParser, CFG_SubEnable, 0);
        pConfig->mSubRtspID = GetConfParaInt(&stConfParser, CFG_SubRtspID, 0);
        pConfig->mSubIsp = GetConfParaInt(&stConfParser, CFG_SubIsp, 0);
        pConfig->mSubIspAlgoFreq = GetConfParaInt(&stConfParser, CFG_SubIspAlGOFREQ, 0);
        pConfig->mSubIspD3dLbcRatio = GetConfParaInt(&stConfParser, CFG_SubIspD3dLbcRatio, 0);
        pConfig->mSubVipp = GetConfParaInt(&stConfParser, CFG_SubVipp, 0);
        pConfig->mSubSrcWidth = GetConfParaInt(&stConfParser, CFG_SubSrcWidth, 0);
        pConfig->mSubSrcHeight = GetConfParaInt(&stConfParser, CFG_SubSrcHeight, 0);
        pConfig->mSubPixelFormat = getPicFormatFromConfig(&stConfParser, CFG_SubPixelFormat);
        pConfig->mSubWdrEnable = GetConfParaInt(&stConfParser, CFG_SubWdrEnable, 0);
        pConfig->mSubViBufNum = GetConfParaInt(&stConfParser, CFG_SubViBufNum, 0);
        pConfig->mSubSrcFrameRate = GetConfParaInt(&stConfParser, CFG_SubSrcFrameRate, 0);
        pConfig->mSubViChn = GetConfParaInt(&stConfParser, CFG_SubViChn, 0);
        pConfig->mSubVEncChn = GetConfParaInt(&stConfParser, CFG_SubVEncChn, 0);
        pConfig->mSubEncodeType = getEncoderTypeFromConfig(&stConfParser, CFG_SubEncodeType);
        pConfig->mSubEncodeWidth = GetConfParaInt(&stConfParser, CFG_SubEncodeWidth, 0);
        pConfig->mSubEncodeHeight = GetConfParaInt(&stConfParser, CFG_SubEncodeHeight, 0);
        pConfig->mSubEncodeFrameRate = GetConfParaInt(&stConfParser, CFG_SubEncodeFrameRate, 0);
        pConfig->mSubEncodeBitrate = GetConfParaInt(&stConfParser, CFG_SubEncodeBitrate, 0);
        pConfig->mSubEncppSharpAttenCoefPer = 100 * pConfig->mSubEncodeWidth / pConfig->mMainEncodeWidth;
        pConfig->mSubOnlineEnable = GetConfParaInt(&stConfParser, CFG_SubOnlineEnable, 0);
        pConfig->mSubOnlineShareBufNum = GetConfParaInt(&stConfParser, CFG_SubOnlineShareBufNum, 0);
        pConfig->mSubEncppEnable = GetConfParaInt(&stConfParser, CFG_SubEncppEnable, 0);
        pConfig->mSubVeRefFrameLbcMode = GetConfParaInt(&stConfParser, CFG_SubVeRefFrameLbcMode, 0);
        pConfig->mSubKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_SubKeyFrameInterval, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_SubFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mSubFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mSubSaveOneFileDuration = GetConfParaInt(&stConfParser, CFG_SubSaveOneFileDuration, 0);
        pConfig->mSubSaveMaxFileCnt = GetConfParaInt(&stConfParser, CFG_SubSaveMaxFileCnt, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_SubDrawOSDText, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mSubDrawOSDText, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mSubViMirror = GetConfParaInt(&stConfParser, CFG_SubViMirror, 0);
        pConfig->mSubViFlip = GetConfParaInt(&stConfParser, CFG_SubViFlip, 0);
        // sub recreate config
        pConfig->mSubEncodeDynamicConfig.mVencRecreateEnable = GetConfParaInt(&stConfParser, CFG_SubVencRecreateEnalbe, 0);
        pConfig->mSubEncodeDynamicConfig.mVencRecreateLoopCnt = GetConfParaInt(&stConfParser, CFG_SubVencRecreateLoopCnt, 0);
        pConfig->mSubEncodeDynamicConfig.mVencRecreateInterval = GetConfParaInt(&stConfParser, CFG_SubVencRecreateInterval, 0);
        pConfig->mSubEncodeDynamicConfig.mVideoFrameRate = GetConfParaInt(&stConfParser, CFG_SubRecreateFrameRate, 0);
        pConfig->mSubEncodeDynamicConfig.mVideoBitRate = GetConfParaInt(&stConfParser, CFG_SubRecreateBitRate, 0);
        pConfig->mSubEncodeDynamicConfig.dstWidth = GetConfParaInt(&stConfParser, CFG_SubRecreateDstWidth, 0);
        pConfig->mSubEncodeDynamicConfig.dstHeight = GetConfParaInt(&stConfParser, CFG_SubRecreateDstHeight, 0);
        pConfig->mSubEncodeDynamicConfig.mKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_SubRecreateKeyFrameInv, 0);

        ptr = (char*)GetConfParaString(&stConfParser, CFG_SubRecreateEncoder, NULL);
        if (ptr != NULL)
        {
            if (!strcmp(ptr, "H.264"))
            {
                pConfig->mSubEncodeDynamicConfig.mVideoEncoderFmt = PT_H264;
                alogd("H.264");
            }
            else if (!strcmp(ptr, "H.265"))
            {
                pConfig->mSubEncodeDynamicConfig.mVideoEncoderFmt = PT_H265;
                alogd("H.265");
            }
            else
            {
                aloge("error conf encoder type");
            }
        }
        else
        {
            aloge("fatal error! ptr is NULL!");
        }

        // sub tdm raw
        pConfig->mSubIspTdmRawProcessType = GetConfParaInt(&stConfParser, CFG_SubIspTdmRawProcessType, -1);
        pConfig->mSubIspTdmRawWidth = GetConfParaInt(&stConfParser, CFG_SubIspTdmRawWidth, 1920);
        pConfig->mSubIspTdmRawHeight = GetConfParaInt(&stConfParser, CFG_SubIspTdmRawHeight, 1080);
        pConfig->mSubIspTdmRawRxBufNum = GetConfParaInt(&stConfParser, CFG_SubIspTdmRawRxBufNum, 5);
        pConfig->mSubIspTdmRawProcessFrameCntMin = GetConfParaInt(&stConfParser, CFG_SubIspTdmRawProcessFrameCntMin, 0);
        pConfig->mSubIspTdmRawProcessFrameCntMax = GetConfParaInt(&stConfParser, CFG_SubIspTdmRawProcessFrameCntMax, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_SubIspTdmRawFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mSubIspTdmRawFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        // sub 2nd stream
        pConfig->mSub2ndEnable = GetConfParaInt(&stConfParser, CFG_Sub2ndEnable, 0);
        pConfig->mSub2ndVipp = GetConfParaInt(&stConfParser, CFG_Sub2ndVipp, 0);
        pConfig->mSub2ndViChn = GetConfParaInt(&stConfParser, CFG_Sub2ndViChn, 0);
        pConfig->mSub2ndSrcWidth = GetConfParaInt(&stConfParser, CFG_Sub2ndSrcWidth, 0);
        pConfig->mSub2ndSrcHeight = GetConfParaInt(&stConfParser, CFG_Sub2ndSrcHeight, 0);
        pConfig->mSub2ndPixelFormat = getPicFormatFromConfig(&stConfParser, CFG_Sub2ndPixelFormat);
        pConfig->mSub2ndViBufNum = GetConfParaInt(&stConfParser, CFG_Sub2ndViBufNum, 0);
        pConfig->mSub2ndSrcFrameRate = GetConfParaInt(&stConfParser, CFG_Sub2ndSrcFrameRate, 0);
        pConfig->mSub2ndVEncChn = GetConfParaInt(&stConfParser, CFG_Sub2ndVEncChn, 0);
        pConfig->mSub2ndEncodeType = getEncoderTypeFromConfig(&stConfParser, CFG_Sub2ndEncodeType);
        pConfig->mSub2ndEncodeWidth = GetConfParaInt(&stConfParser, CFG_Sub2ndEncodeWidth, 0);
        pConfig->mSub2ndEncodeHeight = GetConfParaInt(&stConfParser, CFG_Sub2ndEncodeHeight, 0);
        pConfig->mSub2ndEncodeFrameRate = GetConfParaInt(&stConfParser, CFG_Sub2ndEncodeFrameRate, 0);
        pConfig->mSub2ndEncodeBitrate = GetConfParaInt(&stConfParser, CFG_Sub2ndEncodeBitrate, 0);
        pConfig->mSub2ndEncppSharpAttenCoefPer = 100 * pConfig->mSub2ndEncodeWidth / pConfig->mMainEncodeWidth;
        pConfig->mSub2ndOnlineEnable = GetConfParaInt(&stConfParser, CFG_Sub2ndOnlineEnable, 0);
        pConfig->mSub2ndOnlineShareBufNum = GetConfParaInt(&stConfParser, CFG_Sub2ndOnlineShareBufNum, 0);
        pConfig->mSub2ndEncppEnable = GetConfParaInt(&stConfParser, CFG_Sub2ndEncppEnable, 0);
        pConfig->mSub2ndVeRefFrameLbcMode = GetConfParaInt(&stConfParser, CFG_Sub2ndVeRefFrameLbcMode, 0);
        pConfig->mSub2ndKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_Sub2ndKeyFrameInterval, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_Sub2ndFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mSub2ndFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mSub2ndSaveOneFileDuration = GetConfParaInt(&stConfParser, CFG_Sub2ndSaveOneFileDuration, 0);
        pConfig->mSub2ndSaveMaxFileCnt = GetConfParaInt(&stConfParser, CFG_Sub2ndSaveMaxFileCnt, 0);
        pConfig->mSub2ndTakePicture = GetConfParaInt(&stConfParser, CFG_Sub2ndTakePicture, 0);
        pConfig->mSub2ndTakePictureViChn = GetConfParaInt(&stConfParser, CFG_Sub2ndTakePictureViChn, 0);
        pConfig->mSub2ndTakePictureVencChn = GetConfParaInt(&stConfParser, CFG_Sub2ndTakePictureVencChn, 9);
        pConfig->mSub2ndTakePictureInterval = GetConfParaInt(&stConfParser, CFG_Sub2ndTakePictureInterval, 10);
        ptr = (char *)GetConfParaString(&stConfParser, CFG_Sub2ndTakePictureFile, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mSub2ndTakePictureFile, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mSub2ndTakePictureFileCnt = GetConfParaInt(&stConfParser, CFG_Sub2ndTakePictureFileCnt, 10);
        pConfig->mSub2ndTakePictureOnlyCaptureYuv = GetConfParaInt(&stConfParser, CFG_Sub2ndTakePictureOnlyCaptureYuv, 0);
        alogd("mSub2nd take picture %d interval %d file %s cnt %d",
            pConfig->mSub2ndTakePicture, pConfig->mSub2ndTakePictureInterval,
            pConfig->mSub2ndTakePictureFile, pConfig->mSub2ndTakePictureFileCnt);
        // sub 2nd pdet
        pConfig->mSub2ndPdetEnable = GetConfParaInt(&stConfParser, CFG_Sub2ndPdetEnable, 0);
        pConfig->mSub2ndPdetViChn = GetConfParaInt(&stConfParser, CFG_Sub2ndPdetViChn, 0);

        pConfig->mSub2ndPdetInputWidth = GetConfParaInt(&stConfParser, CFG_Sub2ndPdetInputWidth, 0);
        pConfig->mSub2ndPdetInputHeight = GetConfParaInt(&stConfParser, CFG_Sub2ndPdetInputHeight, 0);
        pConfig->mSub2ndPdetConfThres =  GetConfParaDouble(&stConfParser, CFG_Sub2ndPdetConfThres, 0);
        pConfig->mSub2ndPdetRunIinterval = GetConfParaInt(&stConfParser, CFG_Sub2ndPdetRunIinterval, 0);

        // three stream
        pConfig->mThreeEnable = GetConfParaInt(&stConfParser, CFG_ThreeEnable, 0);
        pConfig->mThreeRtspID = GetConfParaInt(&stConfParser, CFG_ThreeRtspID, 0);
        pConfig->mThreeIsp = GetConfParaInt(&stConfParser, CFG_ThreeIsp, 0);
        pConfig->mThreeIspAlgoFreq = GetConfParaInt(&stConfParser, CFG_ThreeIspAlGOFREQ, 0);
        pConfig->mThreeIspD3dLbcRatio = GetConfParaInt(&stConfParser, CFG_ThreeIspD3dLbcRatio, 0);
        pConfig->mThreeVipp = GetConfParaInt(&stConfParser, CFG_ThreeVipp, 0);
        pConfig->mThreeViChn = GetConfParaInt(&stConfParser, CFG_ThreeViChn, 0);
        pConfig->mThreeSrcWidth = GetConfParaInt(&stConfParser, CFG_ThreeSrcWidth, 0);
        pConfig->mThreeSrcHeight = GetConfParaInt(&stConfParser, CFG_ThreeSrcHeight, 0);
        pConfig->mThreePixelFormat = getPicFormatFromConfig(&stConfParser, CFG_ThreePixelFormat);
        pConfig->mThreeWdrEnable = GetConfParaInt(&stConfParser, CFG_ThreeWdrEnable, 0);
        pConfig->mThreeViBufNum = GetConfParaInt(&stConfParser, CFG_ThreeViBufNum, 0);
        pConfig->mThreeSrcFrameRate = GetConfParaInt(&stConfParser, CFG_ThreeSrcFrameRate, 0);
        pConfig->mThreeVEncChn = GetConfParaInt(&stConfParser, CFG_ThreeVEncChn, 0);
        pConfig->mThreeEncodeType = getEncoderTypeFromConfig(&stConfParser, CFG_ThreeEncodeType);
        pConfig->mThreeEncodeWidth = GetConfParaInt(&stConfParser, CFG_ThreeEncodeWidth, 0);
        pConfig->mThreeEncodeHeight = GetConfParaInt(&stConfParser, CFG_ThreeEncodeHeight, 0);
        pConfig->mThreeEncodeFrameRate = GetConfParaInt(&stConfParser, CFG_ThreeEncodeFrameRate, 0);
        pConfig->mThreeEncodeBitrate = GetConfParaInt(&stConfParser, CFG_ThreeEncodeBitrate, 0);
        pConfig->mThreeOnlineEnable = GetConfParaInt(&stConfParser, CFG_ThreeOnlineEnable, 0);
        pConfig->mThreeOnlineShareBufNum = GetConfParaInt(&stConfParser, CFG_ThreeOnlineShareBufNum, 0);
        pConfig->mThreeEncppEnable = GetConfParaInt(&stConfParser, CFG_ThreeEncppEnable, 0);
        pConfig->mThreeVeRefFrameLbcMode = GetConfParaInt(&stConfParser, CFG_ThreeVeRefFrameLbcMode, 0);
        pConfig->mThreeKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_ThreeKeyFrameInterval, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_ThreeFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mThreeFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mThreeSaveOneFileDuration = GetConfParaInt(&stConfParser, CFG_ThreeSaveOneFileDuration, 0);
        pConfig->mThreeSaveMaxFileCnt = GetConfParaInt(&stConfParser, CFG_ThreeSaveMaxFileCnt, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_ThreeDrawOSDText, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mThreeDrawOSDText, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mThreeViMirror = GetConfParaInt(&stConfParser, CFG_ThreeViMirror, 0);
        pConfig->mThreeViFlip = GetConfParaInt(&stConfParser, CFG_ThreeViFlip, 0);
        // three recreate config
        pConfig->mThreeEncodeDynamicConfig.mVencRecreateEnable = GetConfParaInt(&stConfParser, CFG_ThreeVencRecreateEnalbe, 0);
        pConfig->mThreeEncodeDynamicConfig.mVencRecreateLoopCnt = GetConfParaInt(&stConfParser, CFG_ThreeVencRecreateLoopCnt, 0);
        pConfig->mThreeEncodeDynamicConfig.mVencRecreateInterval = GetConfParaInt(&stConfParser, CFG_ThreeVencRecreateInterval, 0);
        pConfig->mThreeEncodeDynamicConfig.mVideoFrameRate = GetConfParaInt(&stConfParser, CFG_ThreeRecreateFrameRate, 0);
        pConfig->mThreeEncodeDynamicConfig.mVideoBitRate = GetConfParaInt(&stConfParser, CFG_ThreeRecreateBitRate, 0);
        pConfig->mThreeEncodeDynamicConfig.dstWidth = GetConfParaInt(&stConfParser, CFG_ThreeRecreateDstWidth, 0);
        pConfig->mThreeEncodeDynamicConfig.dstHeight = GetConfParaInt(&stConfParser, CFG_ThreeRecreateDstHeight, 0);
        pConfig->mThreeEncodeDynamicConfig.mKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_ThreeRecreateKeyFrameInv, 0);

        ptr = (char*)GetConfParaString(&stConfParser, CFG_ThreeRecreateEncoder, NULL);
        if (ptr != NULL)
        {
            if (!strcmp(ptr, "H.264"))
            {
                pConfig->mThreeEncodeDynamicConfig.mVideoEncoderFmt = PT_H264;
                alogd("H.264");
            }
            else if (!strcmp(ptr, "H.265"))
            {
                pConfig->mThreeEncodeDynamicConfig.mVideoEncoderFmt = PT_H265;
                alogd("H.265");
            }
            else
            {
                aloge("error conf encoder type");
            }
        }
        else
        {
            aloge("fatal error! ptr is NULL!");
        }

        // three tdm raw
        pConfig->mThreeIspTdmRawProcessType = GetConfParaInt(&stConfParser, CFG_ThreeIspTdmRawProcessType, -1);
        pConfig->mThreeIspTdmRawWidth = GetConfParaInt(&stConfParser, CFG_ThreeIspTdmRawWidth, 1920);
        pConfig->mThreeIspTdmRawHeight = GetConfParaInt(&stConfParser, CFG_ThreeIspTdmRawHeight, 1080);
        pConfig->mThreeIspTdmRawRxBufNum = GetConfParaInt(&stConfParser, CFG_ThreeIspTdmRawRxBufNum, 5);
        pConfig->mThreeIspTdmRawProcessFrameCntMin = GetConfParaInt(&stConfParser, CFG_ThreeIspTdmRawProcessFrameCntMin, 0);
        pConfig->mThreeIspTdmRawProcessFrameCntMax = GetConfParaInt(&stConfParser, CFG_ThreeIspTdmRawProcessFrameCntMax, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_ThreeIspTdmRawFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mThreeIspTdmRawFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        // three 2nd stream
        pConfig->mThree2ndEnable = GetConfParaInt(&stConfParser, CFG_Three2ndEnable, 0);
        pConfig->mThree2ndVipp = GetConfParaInt(&stConfParser, CFG_Three2ndVipp, 0);
        pConfig->mThree2ndViChn = GetConfParaInt(&stConfParser, CFG_Three2ndViChn, 0);
        pConfig->mThree2ndSrcWidth = GetConfParaInt(&stConfParser, CFG_Three2ndSrcWidth, 0);
        pConfig->mThree2ndSrcHeight = GetConfParaInt(&stConfParser, CFG_Three2ndSrcHeight, 0);
        pConfig->mThree2ndPixelFormat = getPicFormatFromConfig(&stConfParser, CFG_Three2ndPixelFormat);
        pConfig->mThree2ndViBufNum = GetConfParaInt(&stConfParser, CFG_Three2ndViBufNum, 0);
        pConfig->mThree2ndSrcFrameRate = GetConfParaInt(&stConfParser, CFG_Three2ndSrcFrameRate, 0);
        pConfig->mThree2ndVEncChn = GetConfParaInt(&stConfParser, CFG_Three2ndVEncChn, 0);
        pConfig->mThree2ndEncodeType = getEncoderTypeFromConfig(&stConfParser, CFG_Three2ndEncodeType);
        pConfig->mThree2ndEncodeWidth = GetConfParaInt(&stConfParser, CFG_Three2ndEncodeWidth, 0);
        pConfig->mThree2ndEncodeHeight = GetConfParaInt(&stConfParser, CFG_Three2ndEncodeHeight, 0);
        pConfig->mThree2ndEncodeFrameRate = GetConfParaInt(&stConfParser, CFG_Three2ndEncodeFrameRate, 0);
        pConfig->mThree2ndEncodeBitrate = GetConfParaInt(&stConfParser, CFG_Three2ndEncodeBitrate, 0);
        pConfig->mThree2ndEncppSharpAttenCoefPer = 100 * pConfig->mThree2ndEncodeWidth / pConfig->mThreeEncodeWidth;
        pConfig->mThree2ndOnlineEnable = GetConfParaInt(&stConfParser, CFG_Three2ndOnlineEnable, 0);
        pConfig->mThree2ndOnlineShareBufNum = GetConfParaInt(&stConfParser, CFG_Three2ndOnlineShareBufNum, 0);
        pConfig->mThree2ndEncppEnable = GetConfParaInt(&stConfParser, CFG_Three2ndEncppEnable, 0);
        pConfig->mThree2ndVeRefFrameLbcMode = GetConfParaInt(&stConfParser, CFG_Three2ndVeRefFrameLbcMode, 0);
        pConfig->mThree2ndKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_Three2ndKeyFrameInterval, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_Three2ndFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mThree2ndFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mThree2ndSaveOneFileDuration = GetConfParaInt(&stConfParser, CFG_Three2ndSaveOneFileDuration, 0);
        pConfig->mThree2ndSaveMaxFileCnt = GetConfParaInt(&stConfParser, CFG_Three2ndSaveMaxFileCnt, 0);
        pConfig->mThree2ndTakePicture = GetConfParaInt(&stConfParser, CFG_Three2ndTakePicture, 0);
        pConfig->mThree2ndTakePictureViChn = GetConfParaInt(&stConfParser, CFG_Three2ndTakePictureViChn, 0);
        pConfig->mThree2ndTakePictureVencChn = GetConfParaInt(&stConfParser, CFG_Three2ndTakePictureVencChn, 10);
        pConfig->mThree2ndTakePictureInterval = GetConfParaInt(&stConfParser, CFG_Three2ndTakePictureInterval, 10);
        ptr = (char *)GetConfParaString(&stConfParser, CFG_Three2ndTakePictureFile, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mThree2ndTakePictureFile, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mThree2ndTakePictureFileCnt = GetConfParaInt(&stConfParser, CFG_Three2ndTakePictureFileCnt, 10);
        pConfig->mThree2ndTakePictureOnlyCaptureYuv = GetConfParaInt(&stConfParser, CFG_Three2ndTakePictureOnlyCaptureYuv, 0);
        alogd("three2nd take picture %d interval %d file %s cnt %d",
            pConfig->mThree2ndTakePicture, pConfig->mThree2ndTakePictureInterval,
            pConfig->mThree2ndTakePictureFile, pConfig->mThree2ndTakePictureFileCnt);
        // three 2nd pdet
        pConfig->mThree2ndPdetEnable = GetConfParaInt(&stConfParser, CFG_Three2ndPdetEnable, 0);
        pConfig->mThree2ndPdetViChn = GetConfParaInt(&stConfParser, CFG_Three2ndPdetViChn, 0);

        pConfig->mThree2ndPdetInputWidth = GetConfParaInt(&stConfParser, CFG_Three2ndPdetInputWidth, 0);
        pConfig->mThree2ndPdetInputHeight = GetConfParaInt(&stConfParser, CFG_Three2ndPdetInputHeight, 0);
        pConfig->mThree2ndPdetConfThres =  GetConfParaDouble(&stConfParser, CFG_Three2ndPdetConfThres, 0);
        pConfig->mThree2ndPdetRunIinterval = GetConfParaInt(&stConfParser, CFG_Three2ndPdetRunIinterval, 0);

        // four stream
        pConfig->mFourEnable = GetConfParaInt(&stConfParser, CFG_FourEnable, 0);
        pConfig->mFourRtspID = GetConfParaInt(&stConfParser, CFG_FourRtspID, 0);
        pConfig->mFourIsp = GetConfParaInt(&stConfParser, CFG_FourIsp, 0);
        pConfig->mFourIspAlgoFreq = GetConfParaInt(&stConfParser, CFG_FourIspAlGOFREQ, 0);
        pConfig->mFourIspD3dLbcRatio = GetConfParaInt(&stConfParser, CFG_FourIspD3dLbcRatio, 0);
        pConfig->mFourVipp = GetConfParaInt(&stConfParser, CFG_FourVipp, 0);
        pConfig->mFourViChn = GetConfParaInt(&stConfParser, CFG_FourViChn, 0);
        pConfig->mFourSrcWidth = GetConfParaInt(&stConfParser, CFG_FourSrcWidth, 0);
        pConfig->mFourSrcHeight = GetConfParaInt(&stConfParser, CFG_FourSrcHeight, 0);
        pConfig->mFourPixelFormat = getPicFormatFromConfig(&stConfParser, CFG_FourPixelFormat);
        pConfig->mFourWdrEnable = GetConfParaInt(&stConfParser, CFG_FourWdrEnable, 0);
        pConfig->mFourViBufNum = GetConfParaInt(&stConfParser, CFG_FourViBufNum, 0);
        pConfig->mFourSrcFrameRate = GetConfParaInt(&stConfParser, CFG_FourSrcFrameRate, 0);
        pConfig->mFourVEncChn = GetConfParaInt(&stConfParser, CFG_FourVEncChn, 0);
        pConfig->mFourEncodeType = getEncoderTypeFromConfig(&stConfParser, CFG_FourEncodeType);
        pConfig->mFourEncodeWidth = GetConfParaInt(&stConfParser, CFG_FourEncodeWidth, 0);
        pConfig->mFourEncodeHeight = GetConfParaInt(&stConfParser, CFG_FourEncodeHeight, 0);
        pConfig->mFourEncodeFrameRate = GetConfParaInt(&stConfParser, CFG_FourEncodeFrameRate, 0);
        pConfig->mFourEncodeBitrate = GetConfParaInt(&stConfParser, CFG_FourEncodeBitrate, 0);
        pConfig->mFourOnlineEnable = GetConfParaInt(&stConfParser, CFG_FourOnlineEnable, 0);
        pConfig->mFourOnlineShareBufNum = GetConfParaInt(&stConfParser, CFG_FourOnlineShareBufNum, 0);
        pConfig->mFourEncppEnable = GetConfParaInt(&stConfParser, CFG_FourEncppEnable, 0);
        pConfig->mFourVeRefFrameLbcMode = GetConfParaInt(&stConfParser, CFG_FourVeRefFrameLbcMode, 0);
        pConfig->mFourKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_FourKeyFrameInterval, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_FourFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mFourFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mFourSaveOneFileDuration = GetConfParaInt(&stConfParser, CFG_FourSaveOneFileDuration, 0);
        pConfig->mFourSaveMaxFileCnt = GetConfParaInt(&stConfParser, CFG_FourSaveMaxFileCnt, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_FourDrawOSDText, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mFourDrawOSDText, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mFourViMirror = GetConfParaInt(&stConfParser, CFG_FourViMirror, 0);
        pConfig->mFourViFlip = GetConfParaInt(&stConfParser, CFG_FourViFlip, 0);
        // four recreate config
        pConfig->mFourEncodeDynamicConfig.mVencRecreateEnable = GetConfParaInt(&stConfParser, CFG_FourVencRecreateEnalbe, 0);
        pConfig->mFourEncodeDynamicConfig.mVencRecreateLoopCnt = GetConfParaInt(&stConfParser, CFG_FourVencRecreateLoopCnt, 0);
        pConfig->mFourEncodeDynamicConfig.mVencRecreateInterval = GetConfParaInt(&stConfParser, CFG_FourVencRecreateInterval, 0);
        pConfig->mFourEncodeDynamicConfig.mVideoFrameRate = GetConfParaInt(&stConfParser, CFG_FourRecreateFrameRate, 0);
        pConfig->mFourEncodeDynamicConfig.mVideoBitRate = GetConfParaInt(&stConfParser, CFG_FourRecreateBitRate, 0);
        pConfig->mFourEncodeDynamicConfig.dstWidth = GetConfParaInt(&stConfParser, CFG_FourRecreateDstWidth, 0);
        pConfig->mFourEncodeDynamicConfig.dstHeight = GetConfParaInt(&stConfParser, CFG_FourRecreateDstHeight, 0);
        pConfig->mFourEncodeDynamicConfig.mKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_FourRecreateKeyFrameInv, 0);

        ptr = (char*)GetConfParaString(&stConfParser, CFG_FourRecreateEncoder, NULL);
        if (ptr != NULL)
        {
            if (!strcmp(ptr, "H.264"))
            {
                pConfig->mFourEncodeDynamicConfig.mVideoEncoderFmt = PT_H264;
                alogd("H.264");
            }
            else if (!strcmp(ptr, "H.265"))
            {
                pConfig->mFourEncodeDynamicConfig.mVideoEncoderFmt = PT_H265;
                alogd("H.265");
            }
            else
            {
                aloge("error conf encoder type");
            }
        }
        else
        {
            aloge("fatal error! ptr is NULL!");
        }

        // four tdm raw
        pConfig->mFourIspTdmRawProcessType = GetConfParaInt(&stConfParser, CFG_FourIspTdmRawProcessType, -1);
        pConfig->mFourIspTdmRawWidth = GetConfParaInt(&stConfParser, CFG_FourIspTdmRawWidth, 1920);
        pConfig->mFourIspTdmRawHeight = GetConfParaInt(&stConfParser, CFG_FourIspTdmRawHeight, 1080);
        pConfig->mFourIspTdmRawRxBufNum = GetConfParaInt(&stConfParser, CFG_FourIspTdmRawRxBufNum, 5);
        pConfig->mFourIspTdmRawProcessFrameCntMin = GetConfParaInt(&stConfParser, CFG_FourIspTdmRawProcessFrameCntMin, 0);
        pConfig->mFourIspTdmRawProcessFrameCntMax = GetConfParaInt(&stConfParser, CFG_FourIspTdmRawProcessFrameCntMax, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_FourIspTdmRawFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mFourIspTdmRawFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        // four 2nd stream
        pConfig->mFour2ndEnable = GetConfParaInt(&stConfParser, CFG_Four2ndEnable, 0);
        pConfig->mFour2ndVipp = GetConfParaInt(&stConfParser, CFG_Four2ndVipp, 0);
        pConfig->mFour2ndViChn = GetConfParaInt(&stConfParser, CFG_Four2ndViChn, 0);
        pConfig->mFour2ndSrcWidth = GetConfParaInt(&stConfParser, CFG_Four2ndSrcWidth, 0);
        pConfig->mFour2ndSrcHeight = GetConfParaInt(&stConfParser, CFG_Four2ndSrcHeight, 0);
        pConfig->mFour2ndPixelFormat = getPicFormatFromConfig(&stConfParser, CFG_Four2ndPixelFormat);
        pConfig->mFour2ndViBufNum = GetConfParaInt(&stConfParser, CFG_Four2ndViBufNum, 0);
        pConfig->mFour2ndSrcFrameRate = GetConfParaInt(&stConfParser, CFG_Four2ndSrcFrameRate, 0);
        pConfig->mFour2ndVEncChn = GetConfParaInt(&stConfParser, CFG_Four2ndVEncChn, 0);
        pConfig->mFour2ndEncodeType = getEncoderTypeFromConfig(&stConfParser, CFG_Four2ndEncodeType);
        pConfig->mFour2ndEncodeWidth = GetConfParaInt(&stConfParser, CFG_Four2ndEncodeWidth, 0);
        pConfig->mFour2ndEncodeHeight = GetConfParaInt(&stConfParser, CFG_Four2ndEncodeHeight, 0);
        pConfig->mFour2ndEncodeFrameRate = GetConfParaInt(&stConfParser, CFG_Four2ndEncodeFrameRate, 0);
        pConfig->mFour2ndEncodeBitrate = GetConfParaInt(&stConfParser, CFG_Four2ndEncodeBitrate, 0);
        pConfig->mFour2ndEncppSharpAttenCoefPer = 100 * pConfig->mFour2ndEncodeWidth / pConfig->mFourEncodeWidth;
        pConfig->mFour2ndOnlineEnable = GetConfParaInt(&stConfParser, CFG_Four2ndOnlineEnable, 0);
        pConfig->mFour2ndOnlineShareBufNum = GetConfParaInt(&stConfParser, CFG_Four2ndOnlineShareBufNum, 0);
        pConfig->mFour2ndEncppEnable = GetConfParaInt(&stConfParser, CFG_Four2ndEncppEnable, 0);
        pConfig->mFour2ndVeRefFrameLbcMode = GetConfParaInt(&stConfParser, CFG_Four2ndVeRefFrameLbcMode, 0);
        pConfig->mFour2ndKeyFrameInterval = GetConfParaInt(&stConfParser, CFG_Four2ndKeyFrameInterval, 0);
        ptr = (char*)GetConfParaString(&stConfParser, CFG_Four2ndFilePath, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mFour2ndFilePath, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mFour2ndSaveOneFileDuration = GetConfParaInt(&stConfParser, CFG_Four2ndSaveOneFileDuration, 0);
        pConfig->mFour2ndSaveMaxFileCnt = GetConfParaInt(&stConfParser, CFG_Four2ndSaveMaxFileCnt, 0);
        pConfig->mFour2ndTakePicture = GetConfParaInt(&stConfParser, CFG_Four2ndTakePicture, 0);
        pConfig->mFour2ndTakePictureViChn = GetConfParaInt(&stConfParser, CFG_Four2ndTakePictureViChn, 0);
        pConfig->mFour2ndTakePictureVencChn = GetConfParaInt(&stConfParser, CFG_Four2ndTakePictureVencChn, 11);
        pConfig->mFour2ndTakePictureInterval = GetConfParaInt(&stConfParser, CFG_Four2ndTakePictureInterval, 10);
        ptr = (char *)GetConfParaString(&stConfParser, CFG_Four2ndTakePictureFile, NULL);
        if (ptr != NULL)
            strncpy(pConfig->mFour2ndTakePictureFile, ptr, MAX_FILE_PATH_SIZE);
        else
            aloge("fatal error! ptr is NULL!");
        pConfig->mFour2ndTakePictureFileCnt = GetConfParaInt(&stConfParser, CFG_Four2ndTakePictureFileCnt, 10);
        pConfig->mFour2ndTakePictureOnlyCaptureYuv = GetConfParaInt(&stConfParser, CFG_Four2ndTakePictureOnlyCaptureYuv, 0);
        alogd("four2nd take picture %d interval %d file %s cnt %d",
            pConfig->mFour2ndTakePicture, pConfig->mFour2ndTakePictureInterval,
            pConfig->mFour2ndTakePictureFile, pConfig->mFour2ndTakePictureFileCnt);
        // four 2nd pdet
        pConfig->mFour2ndPdetEnable = GetConfParaInt(&stConfParser, CFG_Four2ndPdetEnable, 0);
        pConfig->mFour2ndPdetViChn = GetConfParaInt(&stConfParser, CFG_Four2ndPdetViChn, 0);

        pConfig->mFour2ndPdetInputWidth = GetConfParaInt(&stConfParser, CFG_Four2ndPdetInputWidth, 0);
        pConfig->mFour2ndPdetInputHeight = GetConfParaInt(&stConfParser, CFG_Four2ndPdetInputHeight, 0);
        pConfig->mFour2ndPdetConfThres =  GetConfParaDouble(&stConfParser, CFG_Four2ndPdetConfThres, 0);
        pConfig->mFour2ndPdetRunIinterval = GetConfParaInt(&stConfParser, CFG_Four2ndPdetRunIinterval, 0);

        alogd("MainEn:%d, Main2ndEn:%d, SubEn:%d, Sub2ndEn:%d, IspAndVeLinkageEn:%d, AdaptEn:%d, LensMoveMaxQp:%d, WbYuvEn:%d, TestDuration:%d",
            pConfig->mMainEnable, pConfig->mMain2ndEnable, pConfig->mSubEnable, pConfig->mSub2ndEnable,
            pConfig->mIspAndVeLinkageEnable, pConfig->mCameraAdaptiveMovingAndStaticEnable, pConfig->mVencLensMovingMaxQp,
            pConfig->mWbYuvEnable, pConfig->mTestDuration);

        destroyConfParser(&stConfParser);
    }
    else
    {
        alogw("user not set config file, use default configs.");
    }

    if (0 != checkInputParams(pConfig))
    {
        aloge("fatal error, checkInputParams failed!");
        return FAILURE;
    }

    pConfig->mMultiSensorOnlineEnable = getMultiSensorOnlineEnable(pConfig);
    alogd("MultiSensorOnlineEnable:%d", pConfig->mMultiSensorOnlineEnable);

    return SUCCESS;
}

static int getBitRate(SampleSmartIPCDemoContext *pContext, VENC_CHN mVEncChn)
{
    int nBitRate = 0;

    if (mVEncChn == pContext->mMainStream.mVEncChn)
    {
        nBitRate = pContext->mConfigPara.mMainEncodeBitrate;
    }
    else if (mVEncChn == pContext->mMain2ndStream.mVEncChn)
    {
        nBitRate = pContext->mConfigPara.mMain2ndEncodeBitrate;
    }
    else if (mVEncChn == pContext->mSubStream.mVEncChn)
    {
        nBitRate = pContext->mConfigPara.mSubEncodeBitrate;
    }
    else if (mVEncChn == pContext->mSub2ndStream.mVEncChn)
    {
        nBitRate = pContext->mConfigPara.mSub2ndEncodeBitrate;
    }
    else if (mVEncChn == pContext->mThreeStream.mVEncChn)
    {
        nBitRate = pContext->mConfigPara.mThreeEncodeBitrate;
    }
    else if (mVEncChn == pContext->mThree2ndStream.mVEncChn)
    {
        nBitRate = pContext->mConfigPara.mThree2ndEncodeBitrate;
    }
    else if (mVEncChn == pContext->mFourStream.mVEncChn)
    {
        nBitRate = pContext->mConfigPara.mFourEncodeBitrate;
    }
    else if (mVEncChn == pContext->mFour2ndStream.mVEncChn)
    {
        nBitRate = pContext->mConfigPara.mFour2ndEncodeBitrate;
    }
    else
    {
        nBitRate = 1*1024*1024;
        aloge("fatal error! VencChn[%d] is not match, set nBitRate = %d bps !", mVEncChn, nBitRate);
    }

    return nBitRate;
}

static VencStreamContext *getStreamContext(SampleSmartIPCDemoContext *pContext, VENC_CHN mVEncChn)
{
    VencStreamContext *pStreamContext = NULL;

    if (mVEncChn == pContext->mMainStream.mVEncChn)
    {
        pStreamContext = &pContext->mMainStream;
    }
    else if ((mVEncChn == pContext->mMain2ndStream.mVEncChn)
        || (mVEncChn == pContext->mMain2ndStream.mTakePictureVeChn))
    {
        pStreamContext = &pContext->mMain2ndStream;
    }
    else if (mVEncChn == pContext->mSubStream.mVEncChn)
    {
        pStreamContext = &pContext->mSubStream;
    }
    else if ((mVEncChn == pContext->mSub2ndStream.mVEncChn)
        || (mVEncChn == pContext->mSub2ndStream.mTakePictureVeChn))
    {
        pStreamContext = &pContext->mSub2ndStream;
    }
    else if (mVEncChn == pContext->mThreeStream.mVEncChn)
    {
        pStreamContext = &pContext->mThreeStream;
    }
    else if ((mVEncChn == pContext->mThree2ndStream.mVEncChn)
        || (mVEncChn == pContext->mThree2ndStream.mTakePictureVeChn))
    {
        pStreamContext = &pContext->mThree2ndStream;
    }
    else if (mVEncChn == pContext->mFourStream.mVEncChn)
    {
        pStreamContext = &pContext->mFourStream;
    }
    else if ((mVEncChn == pContext->mFour2ndStream.mVEncChn)
        || (mVEncChn == pContext->mFour2ndStream.mTakePictureVeChn))
    {
        pStreamContext = &pContext->mFour2ndStream;
    }
    else
    {
        aloge("fatal error! VencChn[%d] is not match, set pStreamContext = NULL!", mVEncChn);
        pStreamContext = NULL;
    }

    return pStreamContext;
}

static ERRORTYPE MPPCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext*)cookie;
    ERRORTYPE ret = 0;

    if (MOD_ID_VIU == pChn->mModId)
    {
        switch(event)
        {
            case MPP_EVENT_VI_TIMEOUT:
            {
                aloge("receive vi timeout. vipp:%d, chn:%d", pChn->mDevId, pChn->mChnId);
                message_t stCmdMsg;
                InitMessage(&stCmdMsg);
                stCmdMsg.command = Vi_Timeout;
                stCmdMsg.para0 = pChn->mDevId;
                putMessageWithData(&pContext->mMsgQueue, &stCmdMsg);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if (MOD_ID_VENC == pChn->mModId)
    {
        VENC_CHN mVEncChn = pChn->mChnId;

        VencStreamContext *pStreamContext = getStreamContext(pContext, mVEncChn);
        if (NULL == pStreamContext)
        {
            aloge("fatal error! VenChn[%d] pStreamContext is NULL!", mVEncChn);
            return -1;
        }

        switch(event)
        {
            /*case MPP_EVENT_LINKAGE_ISP2VE_PARAM:
            {
                Isp2VeLinkageParam stIsp2Ve;
                memset(&stIsp2Ve, 0, sizeof(Isp2VeLinkageParam));
                stIsp2Ve.mIspAndVeLinkageEnable = pStreamContext->mIspAndVeLinkageEnable;
                stIsp2Ve.mCameraAdaptiveMovingAndStaticEnable = pStreamContext->mCameraAdaptiveMovingAndStaticEnable;
                stIsp2Ve.mVEncChn = mVEncChn;
                stIsp2Ve.mVipp = pStreamContext->mVipp;
                stIsp2Ve.pIsp2VeParam = (VencIsp2VeParam *)pEventData;
                stIsp2Ve.nEncppSharpAttenCoefPer = pStreamContext->mEncppSharpAttenCoefPer;
                stIsp2Ve.mEncppSharpDebugDisable = pContext->mConfigPara.mEncppSharpDebugDisable;
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
                stVe2Isp.mIspAndVeLinkageEnable = pStreamContext->mIspAndVeLinkageEnable;
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

#ifdef SUPPORT_AUIDO_TEST
static void write_wav_header(FILE *fp, SampleSmartIPCDemoConfig *config)
{
    struct WaveHeader{
        int riff_id;
        int riff_sz;
        int riff_fmt;
        int fmt_id;
        int fmt_sz;
        short audio_fmt;
        short num_chn;
        int sample_rate;
        int byte_rate;
        short block_align;
        short bits_per_sample;
        int data_id;
        int data_sz;
    };

    struct WaveHeader header;
    memset(&header, 0, sizeof(header));
    memcpy(&header.riff_id, "RIFF", 4);
    int pcm_size = ftell(fp);
    header.riff_sz = pcm_size - 8;
    memcpy(&header.riff_fmt, "WAVE", 4);
    memcpy(&header.fmt_id, "fmt ", 4);
    header.fmt_sz = 16;
    header.audio_fmt = 1;       // s16le
    header.num_chn = config->capture_channel_cnt;
    header.sample_rate = config->capture_sample_rate;

    if(config->capture_bit_witdh == 24) // the data captured from driver is 32bits when set 24 bitdepth
    {
        header.byte_rate = config->capture_sample_rate * config->capture_channel_cnt * 32/8;
        header.block_align = config->capture_channel_cnt * 32/8;
        header.bits_per_sample = 32;
    }
    else
    {
        header.byte_rate = config->capture_sample_rate * config->capture_channel_cnt * config->capture_bit_witdh/8;
        header.block_align = config->capture_channel_cnt * config->capture_bit_witdh/8;
        header.bits_per_sample = config->capture_bit_witdh;
    }
    memcpy(&header.data_id, "data", 4);
    header.data_sz = pcm_size;

    fseek(fp, 0, SEEK_SET);
    fwrite(&header, 1, sizeof(struct WaveHeader), fp);
}

static int parser_wav_header(FILE *pWavFile, int *pChnNum, int *pSampleRate, int *pBitsPerSample)
{
    struct WaveHeader{
        int riff_id;
        int riff_sz;
        int riff_fmt;
        int fmt_id;
        int fmt_sz;
        short audio_fmt;
        short num_chn;
        int sample_rate;
        int byte_rate;
        short block_align;
        short bits_per_sample;
//        int data_id;
//        int data_sz;
    } header;
    struct RIFFSubChunkHeader
    {
        int mSubChunkId;
        int mSubChunkSize;
    } stSubChunkHeader;

    int nHeaderSize;
    int nRdLen = fread(&header, 1, sizeof(struct WaveHeader), pWavFile);
    if(nRdLen != sizeof(struct WaveHeader))
    {
        aloge("fatal error! read file wrong!");
    }
    if(header.fmt_sz != 16)
    {
        alogd("Be careful! wav header fmt_sz[%d]!= 16, need adjust filepos by fseek [%d]bytes", header.fmt_sz, header.fmt_sz - 16);
        fseek(pWavFile, header.fmt_sz - 16, SEEK_CUR);
    }
    fread(&stSubChunkHeader, 1, sizeof(struct RIFFSubChunkHeader), pWavFile);
    //find data sub-chunk
    while(1)
    {
        if(0 != strncmp((char*)&stSubChunkHeader.mSubChunkId, "data", 4))
        {
            alogd("Be careful! wav header has chunk:[0x%x], size:%d, skip", stSubChunkHeader.mSubChunkId, stSubChunkHeader.mSubChunkSize);
            fseek(pWavFile, stSubChunkHeader.mSubChunkSize, SEEK_CUR);
            //read next sub-chunk
            fread(&stSubChunkHeader, 1, sizeof(struct RIFFSubChunkHeader), pWavFile);
        }
        else
        {
            break;
        }
    }
    if(pChnNum)
    {
        *pChnNum = header.num_chn;
    }
    if(pSampleRate)
    {
        *pSampleRate = header.sample_rate;
    }
    if(pBitsPerSample)
    {
        *pBitsPerSample = header.bits_per_sample;
    }
    nHeaderSize = ftell(pWavFile);
    return nHeaderSize;
}

static void *audio_capture_task(void *arg)
{
    FILE *fp = NULL;
    ERRORTYPE ret;
    int file_cnt = 0;
    unsigned long long pre_timestamp = 0;
    SampleSmartIPCDemoContext *sample_context = (SampleSmartIPCDemoContext *)arg;
    SampleSmartIPCDemoConfig *config = &sample_context->mConfigPara;
    struct audio_test_context *context = &sample_context->audio_test_context;
    char strThreadName[32];
    sprintf(strThreadName, "audiocap-ai%d-%d", context->ai_dev, context->ai_chn);
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    context->capture_task_running = 1;
    while (1) {
        if (context->capture_task_exit)
            break;

        AUDIO_FRAME_S frame;
        memset(&frame, 0, sizeof(AUDIO_FRAME_S));
        ret = AW_MPI_AI_GetFrame(context->ai_dev, context->ai_chn, &frame, NULL, 20);
        if (ret != SUCCESS) {
            usleep(10*1000);
            continue;
        }

        if (strlen(config->capture_save_file)) {
            if (!pre_timestamp) {
                char file[MAX_FILE_PATH_SIZE] = {0};
                sprintf(file, "%s%d.wav", config->capture_save_file, file_cnt);
                fp = fopen(file, "wb");
                if (!fp)
                    aloge("open file %s fail!", file);
                alogd("open file %s", file);
                if (fp)
                   fseek(fp, 44, SEEK_SET);
                file_cnt++;
                pre_timestamp = frame.mTimeStamp;
            } else {
                if ((frame.mTimeStamp - pre_timestamp) >= config->capture_save_file_duration*1000*1000) {
                    alogd("frame timestamp %lld pre %lld file duration %d", frame.mTimeStamp, pre_timestamp, config->capture_save_file_duration);
                    if (fp) {
                        write_wav_header(fp, config);
                        fclose(fp);
                    }
                    char file[MAX_FILE_PATH_SIZE] = {0};
                    if (file_cnt >= config->capture_save_file_max_cnt)
                        file_cnt = 0;
                    sprintf(file, "%s%d.wav", config->capture_save_file, file_cnt);
                    fp = fopen(file, "wb");
                    if (!fp)
                        aloge("open file %s fail!", file);
                    alogd("open file %s", file);
                    if (fp)
                       fseek(fp, 44, SEEK_SET);
                    file_cnt++;
                    pre_timestamp = frame.mTimeStamp;
                }
            }

            if (fp)
                fwrite(frame.mpAddr, 1, frame.mLen, fp);
        }

        AW_MPI_AI_ReleaseFrame(context->ai_dev, context->ai_chn, &frame, NULL);
    }

    if (fp)
        fclose(fp);
    return (void *)NULL;
}

static void audio_capture_init(SampleSmartIPCDemoContext *sample_context)
{
    SampleSmartIPCDemoConfig *config = &sample_context->mConfigPara;
    struct audio_test_context *context = &sample_context->audio_test_context;

    memset(&context->ai_chn_attr, 0, sizeof(context->ai_chn_attr));
    context->ai_chn_attr.enSamplerate =
        map_SampleRate_to_AUDIO_SAMPLE_RATE_E(config->capture_sample_rate);
    context->ai_chn_attr.enBitwidth =
        map_BitWidth_to_AUDIO_BIT_WIDTH_E(config->capture_bit_witdh);
    context->ai_chn_attr.enSoundmode =
        (config->capture_channel_cnt == 1) ? AUDIO_SOUND_MODE_MONO : AUDIO_SOUND_MODE_STEREO;
    context->ai_chn_attr.mMicNum = 1;
    context->ai_chn_attr.mChnCnt = config->capture_channel_cnt;
    context->ai_chn_attr.ai_ans_en = config->capture_ans_en;
    if (context->ai_chn_attr.ai_ans_en)
        context->ai_chn_attr.ai_ans_mode = 3;
    context->ai_chn_attr.ai_agc_en = config->capture_agc_en;
    if (context->ai_chn_attr.ai_agc_en) {
        context->ai_chn_attr.ai_agc_float_cfg.fTargetDb = 0;
        context->ai_chn_attr.ai_agc_float_cfg.fMaxGainDb = 30;
    }
    context->ai_chn_attr.ai_aec_en = config->capture_aec_en;

    context->ai_dev = 0;
    AW_MPI_AI_SetPubAttr(context->ai_dev, &context->ai_chn_attr);

    context->ai_chn = 0;
    AI_CHN_ATTR_S ai_chn_attr;
    memset(&ai_chn_attr, 0, sizeof(ai_chn_attr));
    ai_chn_attr.nFrameSize = 1024;
    AW_MPI_AI_CreateChn(context->ai_dev, context->ai_chn, &ai_chn_attr);
    AW_MPI_AI_EnableChn(context->ai_dev, context->ai_chn);

    pthread_create(&context->capture_task_trd, NULL, audio_capture_task, (void *)sample_context);
}

static void audio_capture_exit(SampleSmartIPCDemoContext *sample_context)
{
    struct audio_test_context *context = &sample_context->audio_test_context;

    context->capture_task_exit = 1;
    if (context->capture_task_running)
        pthread_join(context->capture_task_trd, NULL);
    AW_MPI_AI_DisableChn(context->ai_dev, context->ai_chn);
    AW_MPI_AI_DestroyChn(context->ai_dev, context->ai_chn);
}

static void *audio_playback_task(void *arg)
{
    SampleSmartIPCDemoContext *sample_context = (SampleSmartIPCDemoContext *)arg;
    SampleSmartIPCDemoConfig *config = &sample_context->mConfigPara;
    struct audio_test_context *context = &sample_context->audio_test_context;

    char strThreadName[32];
    sprintf(strThreadName, "audioplay-ao%d-%d", context->ao_dev, context->ao_chn);
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    context->playback_task_running = 1;

    FILE *fp = fopen(config->playback_file, "rb");
    if (!fp) {
        aloge("open file %s fail!", config->playback_file);
        return (void *)NULL;
    }
    int chn, sample_rate, bits_per_sample;
    int header_size = parser_wav_header(fp, &chn, &sample_rate, &bits_per_sample);
    int want_read_len = chn * bits_per_sample / 8 * 1024;
    char *buf = malloc(want_read_len);
    if (!buf)
        aloge("alloc buf len %d fail!", want_read_len);
    while (1) {
        if (context->playback_task_exit)
            break;

        if (feof(fp)) {
            fseek(fp, header_size, SEEK_SET);
        }

        AUDIO_FRAME_S frame;
        memset(&frame, 0, sizeof(frame));
        frame.mBitwidth = map_AUDIO_BIT_WIDTH_E_to_BitWidth(bits_per_sample);
        frame.mSamplerate = map_AUDIO_SAMPLE_RATE_E_to_SampleRate(sample_rate);
        frame.mSoundmode = (chn == 1) ? AUDIO_SOUND_MODE_MONO : AUDIO_SOUND_MODE_STEREO;
        int read_len = fread(buf, 1, want_read_len, fp);
        frame.mLen = read_len;
        frame.mpAddr = buf;
        AW_MPI_AO_SendFrameSync(context->ao_dev, context->ao_chn, &frame);
    }
    if (buf)
        free(buf);
    fclose(fp);
    return (void *)NULL;
}

static void audio_playback_init(SampleSmartIPCDemoContext *sample_context)
{
    SampleSmartIPCDemoConfig *config = &sample_context->mConfigPara;
    struct audio_test_context *context = &sample_context->audio_test_context;

    context->ao_dev = 0;
    context->ao_chn = 0;
    AW_MPI_AO_CreateChn(context->ao_dev, context->ao_chn);
    MPPCallbackInfo cbInfo;
    cbInfo.cookie = (void*)sample_context;
    cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
    AW_MPI_AO_RegisterCallback(context->ao_dev, context->ao_chn, &cbInfo);
    AW_MPI_AO_StartChn(context->ao_dev, context->ao_chn);
    AW_MPI_AO_SetDevVolume(context->ao_dev, config->playback_volume);

    pthread_create(&context->playback_task_trd, 0, audio_playback_task, (void *)sample_context);
}

static void audio_playback_exit(SampleSmartIPCDemoContext *sample_context)
{
    SampleSmartIPCDemoConfig *config = &sample_context->mConfigPara;
    struct audio_test_context *context = &sample_context->audio_test_context;

    context->playback_task_exit = 1;
    if (context->playback_task_running)
        pthread_join(context->playback_task_trd, NULL);
    AW_MPI_AO_StopChn(context->ao_dev, context->ao_chn);
    AW_MPI_AO_DestroyChn(context->ao_dev, context->ao_chn);
}

static void audio_test_init(SampleSmartIPCDemoContext *sample_context)
{
    if(sample_context->mConfigPara.capture_en)
        audio_capture_init(sample_context);
    if (sample_context->mConfigPara.playback_en)
        audio_playback_init(sample_context);
}

static void audio_test_exit(SampleSmartIPCDemoContext *sample_context)
{
    if(sample_context->mConfigPara.capture_en)
        audio_capture_exit(sample_context);
    if (sample_context->mConfigPara.playback_en)
        audio_playback_exit(sample_context);
}
#endif

#ifdef SUPPORT_MOTION_SEARCH
static int ConvertUsrCoordinate(SIZE_S *size, bool isXAxis, int value)
{
    if (isXAxis)
        return (value * size->Width / 10000);
    else
        return (value * size->Height / 10000);
}

static void ConvertMotionCoordinateZone2RawPic(VencStreamContext *stream, MotionDetectZoneInfo *pZoneInfo)
{
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext *)stream->priv;
    int width = 0, height = 0;
    switch (stream->mVEncChnAttr.VeAttr.Type) {
    case PT_H264:
        width = stream->mVEncChnAttr.VeAttr.AttrH264e.PicWidth;
        height = stream->mVEncChnAttr.VeAttr.AttrH264e.PicHeight;
        break;
    case PT_H265:
        width = stream->mVEncChnAttr.VeAttr.AttrH265e.mPicWidth;
        height = stream->mVEncChnAttr.VeAttr.AttrH265e.mPicHeight;
        break;
    case PT_MJPEG:
        width = stream->mVEncChnAttr.VeAttr.AttrMjpeg.mPicWidth;
        height = stream->mVEncChnAttr.VeAttr.AttrMjpeg.mPicHeight;
        break;
    default:
        alogw("unsupport encoder type 0x%x", stream->mVEncChnAttr.VeAttr.Type);
        return;
    }
    SIZE_S pic_size = {width, height};
    pZoneInfo->left_up_x = ConvertUsrCoordinate(&pic_size, TRUE, pContext->mConfigPara.motionAlarm_left_up_x);
    pZoneInfo->left_up_y = ConvertUsrCoordinate(&pic_size, FALSE, pContext->mConfigPara.motionAlarm_left_up_y);
    pZoneInfo->left_bottom_x = ConvertUsrCoordinate(&pic_size, TRUE, pContext->mConfigPara.motionAlarm_left_bottom_x);
    pZoneInfo->left_bottom_y = ConvertUsrCoordinate(&pic_size, FALSE, pContext->mConfigPara.motionAlarm_left_bottom_y);
    pZoneInfo->right_up_x = ConvertUsrCoordinate(&pic_size, TRUE, pContext->mConfigPara.motionAlarm_right_up_x);
    pZoneInfo->right_up_y = ConvertUsrCoordinate(&pic_size, FALSE, pContext->mConfigPara.motionAlarm_right_up_y);
    pZoneInfo->right_bottom_x = ConvertUsrCoordinate(&pic_size, TRUE, pContext->mConfigPara.motionAlarm_right_bottom_x);
    pZoneInfo->right_bottom_y = ConvertUsrCoordinate(&pic_size, FALSE, pContext->mConfigPara.motionAlarm_right_bottom_y);
}

static void ConvertMotionCoordinateZone2RegionNum(VencStreamContext *stream, MotionDetectZoneInfo *pZoneInfo, int *totalRegionNum)
{
    int width = 0, height = 0;
    switch (stream->mVEncChnAttr.VeAttr.Type) {
    case PT_H264:
        width = stream->mVEncChnAttr.VeAttr.AttrH264e.PicWidth;
        height = stream->mVEncChnAttr.VeAttr.AttrH264e.PicHeight;
        break;
    case PT_H265:
        width = stream->mVEncChnAttr.VeAttr.AttrH265e.mPicWidth;
        height = stream->mVEncChnAttr.VeAttr.AttrH265e.mPicHeight;
        break;
    case PT_MJPEG:
        width = stream->mVEncChnAttr.VeAttr.AttrMjpeg.mPicWidth;
        height = stream->mVEncChnAttr.VeAttr.AttrMjpeg.mPicHeight;
        break;
    default:
        alogw("unsupport encoder type 0x%x", stream->mVEncChnAttr.VeAttr.Type);
        return;
    }
    int usr_width = pZoneInfo->right_bottom_x - pZoneInfo->left_up_x;
    int usr_height = pZoneInfo->right_bottom_y - pZoneInfo->left_up_y;
    *totalRegionNum = (usr_width*usr_height) * stream->mMotionResult.total_region_num / (width*height);
}

static void ConvertMotionSensitivity2Threshold(VencStreamContext *stream, int totalRegionNum, int *threshold)
{
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext *)stream->priv;
    int sensitivity = pContext->mConfigPara.motionAlarm_sensitivity;
    switch (sensitivity)
    {
        case 100:
        {
            *threshold = pContext->mConfigPara.motionAlarm_Threshold_High;
            break;
        }
        case 80:
        {
            *threshold = pContext->mConfigPara.motionAlarm_Threshold_MediumHigh;
            break;
        }
        case 40:
        {
            *threshold = pContext->mConfigPara.motionAlarm_Threshold_MediumLow;
            break;
        }
        case 20:
        {
            *threshold = pContext->mConfigPara.motionAlarm_Threshold_Low;
            break;
        }
        case 60:
        default:
        {
            *threshold = pContext->mConfigPara.motionAlarm_Threshold_Default;
            break;
        }
    }
    if (totalRegionNum < *threshold)
    {
        alogv("fatal error! invalid threshold %d, it should be <= %d", *threshold, totalRegionNum);
        *threshold = totalRegionNum;
    }
}

static void MotionSearchJudgeResult(VencStreamContext *stream, int *result)
{
    int totalMotionRegionNum = 0;
    int i = 0;
    int threshold = 0;
    int sensitivity = 0;
    int totalRegionNum = stream->mMotionResult.total_region_num;
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext *)stream->priv;

    MotionDetectRectInfo tmp_data;
    memset(&tmp_data, 0, sizeof(MotionDetectRectInfo));

    if (pContext->mConfigPara.motionAlarm_support_zone)
    {
        for (i = 0; i < totalRegionNum; i++)
        {
            if (stream->mMotionResult.region[i].is_motion)
            {
                // 转换用户配置的zone坐标到原图坐标
                MotionDetectZoneInfo zoneInfo;
                memset(&zoneInfo, 0, sizeof(MotionDetectZoneInfo));
                ConvertMotionCoordinateZone2RawPic(stream, &zoneInfo);
                // 移动侦测告警区域，统一当作矩形处理
                if ((zoneInfo.left_up_x <= stream->mMotionResult.region[i].pix_x_bgn) &&
                    (zoneInfo.left_up_y <= stream->mMotionResult.region[i].pix_y_bgn) &&
                    (zoneInfo.right_bottom_x >= stream->mMotionResult.region[i].pix_x_end) &&
                    (zoneInfo.right_bottom_y >= stream->mMotionResult.region[i].pix_y_end))
                {
                    alogv("area_%d:[(%d,%d),(%d,%d)] usrZone:[(%d,%d),(%d,%d)]", i,
                        stream->mMotionResult.region[i].pix_x_bgn, stream->mMotionResult.region[i].pix_y_bgn,
                        stream->mMotionResult.region[i].pix_x_end, stream->mMotionResult.region[i].pix_y_end,
                        zoneInfo.left_up_x, zoneInfo.left_up_y, zoneInfo.right_bottom_x, zoneInfo.right_bottom_y);
                    totalMotionRegionNum++;
                }
                ConvertMotionCoordinateZone2RegionNum(stream, &zoneInfo, &totalRegionNum);
            }
        }
    }
    else
    {
        totalMotionRegionNum = stream->mMotionResult.motion_region_num;
    }

    // 灵敏度：上报移动侦测事件时，根据灵敏度高低设置不同阈值，侦测区域内的标记motion的region个数超过阈值就上报，否则不上报。
    ConvertMotionSensitivity2Threshold(stream, totalRegionNum, &threshold);

    if (0 < totalMotionRegionNum)
    {
        alogv("detect motion, total region:%d, threshold:%d", totalMotionRegionNum, threshold);
    }

    if (threshold <= totalMotionRegionNum)
    {
        *result = 1;
    }
    else
    {
        *result = 0;
    }
}
#endif

static void configMainStream(VencStreamContext *pStreamContext, SampleSmartIPCDemoConfig *pConfig)
{
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;
    if (!pStreamContext || !pConfig)
    {
        aloge("fatal error! invalid input params! %p,%p", pStreamContext, pConfig);
    }

    pStreamContext->mIsp = pConfig->mMainIsp;
    pStreamContext->mIspAlgoFreq = pConfig->mMainIspAlgoFreq;
    pStreamContext->mVipp = pConfig->mMainVipp;
    pStreamContext->mViChn = pConfig->mMainViChn;
    pStreamContext->mVEncChn = pConfig->mMainVEncChn;

    if (pConfig->mMainOnlineEnable)
    {
        pStreamContext->mViAttr.mOnlineEnable = 1;
        pStreamContext->mViAttr.mOnlineShareBufNum = pConfig->mMainOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable = 1;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum = pConfig->mMainOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable = pConfig->mMultiSensorOnlineEnable;
        if (1 == pConfig->mMainViStitchMode || 2 == pConfig->mMainViStitchMode || 3 == pConfig->mMainViStitchMode)
        {
            pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable = 1;
            pStreamContext->mVEncChnAttr.VeAttr.mImageStitchingEnable = 1;
            alogd("VencChn[%d] MultiSensorOnlineEnable:%d ImageStitchingEnable:%d", pStreamContext->mVEncChn,
                pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable,
                pStreamContext->mVEncChnAttr.VeAttr.mImageStitchingEnable);
        }
    }
    alogd("main vipp%d ve_online_en:%d, dma_buf_num:%d, venc ch%d OnlineEnable:%d, OnlineShareBufNum:%d",
        pStreamContext->mVipp, pStreamContext->mViAttr.mOnlineEnable, pStreamContext->mViAttr.mOnlineShareBufNum,
        pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable, pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum);

    pStreamContext->mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pStreamContext->mViAttr.memtype = V4L2_MEMORY_MMAP;
    pStreamContext->mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pConfig->mMainPixelFormat);
    pStreamContext->mViAttr.format.field = V4L2_FIELD_NONE;
    if (pConfig->mIspColorSpace){
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709_PART_RANGE;
    }
    else
    {
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709;
    }
    pStreamContext->mViAttr.format.width = pConfig->mMainSrcWidth;
    pStreamContext->mViAttr.format.height = pConfig->mMainSrcHeight;
    pStreamContext->mViAttr.fps = pConfig->mMainSrcFrameRate;
    pStreamContext->mViAttr.use_current_win = 0;
    pStreamContext->mViAttr.nbufs = pConfig->mMainViBufNum;
    pStreamContext->mViAttr.nplanes = 2;
    pStreamContext->mViAttr.wdr_mode = pConfig->mMainWdrEnable;
    pStreamContext->mViAttr.drop_frame_num = 0;
    if (pConfig->mMainViStitchMode < DMA_STITCH_NONE || pConfig->mMainViStitchMode > DMA_STITCH_MODE_MAX) {
        aloge("stitch_mode =%d, but it should be set to [0, 3], it will set it by default:0\n", pConfig->mMainViStitchMode);
        pConfig->mMainViStitchMode = DMA_STITCH_NONE;
    }
    pStreamContext->mViAttr.large_dma_merge_en = pConfig->mMainViStitchMode;
    pStreamContext->mViAttr.mStitchModeSensorScalerCfg.scaler_en = pConfig->mMainViStitchScalerEn;
    if (pStreamContext->mViAttr.mStitchModeSensorScalerCfg.scaler_en && (pStreamContext->mViAttr.large_dma_merge_en == DMA_STITCH_VERTICAL ||
            pStreamContext->mViAttr.large_dma_merge_en == DMA_STITCH_HORIZONTAL)) {
        if (pConfig->mMainViStitchSensorAScalerWidth && pConfig->mMainViStitchSensorAScalerheight &&
            pConfig->mMainViStitchSensorBScalerWidth && pConfig->mMainViStitchSensorBScalerheight) {
            pStreamContext->mViAttr.mStitchModeSensorScalerCfg.sensorA_scaler_cfg.width = pConfig->mMainViStitchSensorAScalerWidth;
            pStreamContext->mViAttr.mStitchModeSensorScalerCfg.sensorA_scaler_cfg.height = pConfig->mMainViStitchSensorAScalerheight;
            pStreamContext->mViAttr.mStitchModeSensorScalerCfg.sensorB_scaler_cfg.width = pConfig->mMainViStitchSensorBScalerWidth;
            pStreamContext->mViAttr.mStitchModeSensorScalerCfg.sensorB_scaler_cfg.height = pConfig->mMainViStitchSensorBScalerheight;
        } else {
            aloge("(MainStream) Invaild width and height for SensorA/B after scaler, please check!!!\n");
        }
    }
    if (-1 != pConfig->mMainIspTdmRawProcessType)
    {
        pStreamContext->mViAttr.tdm_rxbuf_cnt = pConfig->mMainIspTdmRawRxBufNum;
    }
    pStreamContext->mViAttr.tdm_speed_down_en = pConfig->mTdmSpeedDownEnable;
    if (pStreamContext->mViAttr.tdm_speed_down_en)
    {
        pStreamContext->mViAttr.tdm_tx_valid_num = pConfig->mTdmTxValidNum;
        pStreamContext->mViAttr.tdm_tx_invalid_num = pConfig->mTdmTxInvalidNum;
        pStreamContext->mViAttr.tdm_tx_valid_num_offset = pConfig->mTdmTxValidNumOffset;
    }
    alogd("main vipp%d tdm_speed_down_en:%d, vaild_num:%d, invaild_num:%d, valid_num_offset:%d", pStreamContext->mVipp, pStreamContext->mViAttr.tdm_speed_down_en,
        pStreamContext->mViAttr.tdm_tx_valid_num, pStreamContext->mViAttr.tdm_tx_invalid_num, pStreamContext->mViAttr.tdm_tx_valid_num_offset);

    pStreamContext->mVEncChnAttr.VeAttr.mVippID = pConfig->mMainVipp;
    pStreamContext->mVEncChnAttr.VeAttr.Type = pConfig->mMainEncodeType;
    pStreamContext->mVEncChnAttr.VeAttr.MaxKeyInterval = pConfig->mMainKeyFrameInterval;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicWidth = pConfig->mMainSrcWidth;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicHeight = pConfig->mMainSrcHeight;
    pStreamContext->mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pStreamContext->mVEncChnAttr.VeAttr.PixelFormat = pConfig->mMainPixelFormat;
    pStreamContext->mVEncChnAttr.VeAttr.mColorSpace = pStreamContext->mViAttr.format.colorspace;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRefFrameLbcMode = pConfig->mMainVeRefFrameLbcMode;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRecRefBufReduceEnable = pConfig->mVeRecRefBufReduceEnable;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRxInputBufmultiplexEnable = pConfig->mMainVeRxInputBufmultiplexEnable;

    pStreamContext->mVEncChnAttr.VeAttr.mVbrOptEnable = pConfig->mVbrOptEnable;

    pStreamContext->mVEncChnAttr.RcAttr.mProductMode = pConfig->mProductMode;
    //pStreamContext->mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

    pStreamContext->mEncppSharpAttenCoefPer = 100;
    alogd("main EncppSharpAttenCoefPer: %d%%", pStreamContext->mEncppSharpAttenCoefPer);
    pStreamContext->mIspAndVeLinkageEnable = pConfig->mIspAndVeLinkageEnable;
    pStreamContext->mCameraAdaptiveMovingAndStaticEnable = pConfig->mCameraAdaptiveMovingAndStaticEnable;

    pStreamContext->mVEncChnAttr.EncppAttr.eEncppSharpSetting = pConfig->mMainEncppEnable?VencEncppSharp_FollowISPConfig:VencEncppSharp_Disable;

    if (pConfig->mMainSrcFrameRate)
    {
        vbvThreshSize = pConfig->mMainEncodeBitrate/8/pConfig->mMainSrcFrameRate*15;
    }
    vbvBufSize = pConfig->mMainEncodeBitrate/8*VE_VBV_CACHE_TIME + vbvThreshSize;
    pStreamContext->mVbvBufSize = vbvBufSize;
    alogd("main vbvThreshSize: %d, vbvBufSize: %d", vbvThreshSize, vbvBufSize);

    VencRateCtrlConfig stRcConfig;
    memset(&stRcConfig, 0, sizeof(VencRateCtrlConfig));
    stRcConfig.mEncodeType = pConfig->mMainEncodeType;
    stRcConfig.mEncodeWidth = pConfig->mMainEncodeWidth;
    stRcConfig.mEncodeHeight = pConfig->mMainEncodeHeight;
    stRcConfig.mSrcFrameRate = pConfig->mMainSrcFrameRate;
    stRcConfig.mDstFrameRate = pConfig->mMainEncodeFrameRate;
    stRcConfig.mEncodeBitrate = pConfig->mMainEncodeBitrate;
    stRcConfig.mRcMode = getRcModeFromConfig(pConfig->mRcMode, pConfig->mMainEncodeType);
    stRcConfig.mMaxIQp = pConfig->mMaxIQp;
    stRcConfig.mMinIQp = pConfig->mMinIQp;
    stRcConfig.mMaxPQp = pConfig->mMaxPQp;
    stRcConfig.mMinPQp = pConfig->mMinPQp;
    stRcConfig.mInitQp = pConfig->mInitQp;
    stRcConfig.mEnMbQpLimit = pConfig->mEnMbQpLimit;
    stRcConfig.mQuality = pConfig->mQuality;
    stRcConfig.mMovingTh = pConfig->mMovingTh;
    stRcConfig.mIBitsCoef = pConfig->mIBitsCoef;
    stRcConfig.mPBitsCoef = pConfig->mPBitsCoef;
    stRcConfig.mVbvBufSize = vbvBufSize;
    stRcConfig.mVbvThreshSize = vbvThreshSize;
    configVencRateCtrlParam(&stRcConfig, &pStreamContext->mVEncChnAttr, &pStreamContext->mVEncRcParam);

    pStreamContext->mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pStreamContext->mVEncChnAttr.GopAttr.mGopSize = 2;

    pStreamContext->mVEncFrameRateConfig.SrcFrmRate = pConfig->mMainSrcFrameRate;
    pStreamContext->mVEncFrameRateConfig.DstFrmRate = pConfig->mMainEncodeFrameRate;

#ifdef ENABLE_VENC_ADVANCED_PARAM
    if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type || PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        configIFrmMbRcMoveStatus(&pStreamContext->mVEncRcParam);
        configBitsClipParam(&pStreamContext->mVEncRcParam);
    }
#endif
}

static void configMain2ndStream(VencStreamContext *pStreamContext, SampleSmartIPCDemoConfig *pConfig)
{
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;
    if (!pStreamContext || !pConfig)
    {
        aloge("fatal error! invalid input params! %p,%p", pStreamContext, pConfig);
    }

    pStreamContext->mIsp = pConfig->mMainIsp;
    pStreamContext->mIspAlgoFreq = pConfig->mMainIspAlgoFreq;
    pStreamContext->mVipp = pConfig->mMain2ndVipp;
    pStreamContext->mViChn = pConfig->mMain2ndViChn;
    pStreamContext->mVEncChn = pConfig->mMain2ndVEncChn;

    if (pConfig->mMain2ndOnlineEnable)
    {
        pStreamContext->mViAttr.mOnlineEnable = 1;
        pStreamContext->mViAttr.mOnlineShareBufNum = pConfig->mMain2ndOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable = 1;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum = pConfig->mMain2ndOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable = pConfig->mMultiSensorOnlineEnable;
        if (1 == pConfig->mMain2ndViStitchMode)
        {
            pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable = 1;
            pStreamContext->mVEncChnAttr.VeAttr.mImageStitchingEnable = 1;
            alogd("VencChn[%d] MultiSensorOnlineEnable:%d ImageStitchingEnable:%d", pStreamContext->mVEncChn,
                pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable,
                pStreamContext->mVEncChnAttr.VeAttr.mImageStitchingEnable);
        }
    }
    alogd("main2nd vipp%d ve_online_en:%d, dma_buf_num:%d, venc ch%d OnlineEnable:%d, OnlineShareBufNum:%d",
        pStreamContext->mVipp, pStreamContext->mViAttr.mOnlineEnable, pStreamContext->mViAttr.mOnlineShareBufNum,
        pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable, pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum);

    pStreamContext->mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pStreamContext->mViAttr.memtype = V4L2_MEMORY_MMAP;
    pStreamContext->mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pConfig->mMain2ndPixelFormat);
    pStreamContext->mViAttr.format.field = V4L2_FIELD_NONE;
    if (pConfig->mIspColorSpace){
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709_PART_RANGE;
    }
    else
    {
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709;
    }
    pStreamContext->mViAttr.format.width = pConfig->mMain2ndSrcWidth;
    pStreamContext->mViAttr.format.height = pConfig->mMain2ndSrcHeight;
    pStreamContext->mViAttr.fps = pConfig->mMain2ndSrcFrameRate;
    pStreamContext->mViAttr.use_current_win = 1;
    pStreamContext->mViAttr.nbufs = pConfig->mMain2ndViBufNum;
    pStreamContext->mViAttr.nplanes = 2;
    pStreamContext->mViAttr.wdr_mode = 0;
    pStreamContext->mViAttr.drop_frame_num = 0;
    if (pConfig->mMain2ndViStitchMode < DMA_STITCH_NONE || pConfig->mMain2ndViStitchMode > DMA_STITCH_MODE_MAX) {
        aloge("stitch_mode =%d, but it should be set to [0, 3], it will set it by default:0\n", pConfig->mMain2ndViStitchMode);
        pConfig->mMain2ndViStitchMode = DMA_STITCH_NONE;
    }
    pStreamContext->mViAttr.large_dma_merge_en = pConfig->mMain2ndViStitchMode;
    pStreamContext->mViAttr.mStitchModeSensorScalerCfg.scaler_en = pConfig->mMain2ndViStitchScalerEn;
    if (pStreamContext->mViAttr.mStitchModeSensorScalerCfg.scaler_en && (pStreamContext->mViAttr.large_dma_merge_en == DMA_STITCH_VERTICAL ||
            pStreamContext->mViAttr.large_dma_merge_en == DMA_STITCH_HORIZONTAL)) {
        if (pConfig->mMain2ndViStitchSensorAScalerWidth && pConfig->mMain2ndViStitchSensorAScalerheight &&
            pConfig->mMain2ndViStitchSensorBScalerWidth && pConfig->mMain2ndViStitchSensorBScalerheight) {
            pStreamContext->mViAttr.mStitchModeSensorScalerCfg.sensorA_scaler_cfg.width = pConfig->mMain2ndViStitchSensorAScalerWidth;
            pStreamContext->mViAttr.mStitchModeSensorScalerCfg.sensorA_scaler_cfg.height = pConfig->mMain2ndViStitchSensorAScalerheight;
            pStreamContext->mViAttr.mStitchModeSensorScalerCfg.sensorB_scaler_cfg.width = pConfig->mMain2ndViStitchSensorBScalerWidth;
            pStreamContext->mViAttr.mStitchModeSensorScalerCfg.sensorB_scaler_cfg.height = pConfig->mMain2ndViStitchSensorBScalerheight;
        } else {
            aloge("(Main2ndStream) Invaild width and height for SensorA/B after scaler, please check!!!\n");
        }
    }
    pStreamContext->mViAttr.tdm_speed_down_en = pConfig->mTdmSpeedDownEnable;
    if (pStreamContext->mViAttr.tdm_speed_down_en)
    {
        pStreamContext->mViAttr.tdm_tx_valid_num = pConfig->mTdmTxValidNum;
        pStreamContext->mViAttr.tdm_tx_invalid_num = pConfig->mTdmTxInvalidNum;
        pStreamContext->mViAttr.tdm_tx_valid_num_offset = pConfig->mTdmTxValidNumOffset;
    }
    alogd("main2nd vipp%d tdm_speed_down_en:%d, vaild_num:%d, invaild_num:%d, valid_num_offset:%d", pStreamContext->mVipp, pStreamContext->mViAttr.tdm_speed_down_en,
        pStreamContext->mViAttr.tdm_tx_valid_num, pStreamContext->mViAttr.tdm_tx_invalid_num, pStreamContext->mViAttr.tdm_tx_valid_num_offset);

    pStreamContext->mVEncChnAttr.VeAttr.mVippID = pConfig->mMain2ndVipp;
    pStreamContext->mVEncChnAttr.VeAttr.Type = pConfig->mMain2ndEncodeType;
    pStreamContext->mVEncChnAttr.VeAttr.MaxKeyInterval = pConfig->mMain2ndKeyFrameInterval;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicWidth = pConfig->mMain2ndSrcWidth;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicHeight = pConfig->mMain2ndSrcHeight;
    pStreamContext->mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pStreamContext->mVEncChnAttr.VeAttr.PixelFormat = pConfig->mMain2ndPixelFormat;
    pStreamContext->mVEncChnAttr.VeAttr.mColorSpace = pStreamContext->mViAttr.format.colorspace;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRefFrameLbcMode = pConfig->mMain2ndVeRefFrameLbcMode;;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRecRefBufReduceEnable = pConfig->mVeRecRefBufReduceEnable;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRxInputBufmultiplexEnable = pConfig->mMain2ndVeRxInputBufmultiplexEnable;

    pStreamContext->mVEncChnAttr.VeAttr.mVbrOptEnable = pConfig->mVbrOptEnable;

    pStreamContext->mVEncChnAttr.RcAttr.mProductMode = pConfig->mProductMode;
    //pStreamContext->mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

    pStreamContext->mEncppSharpAttenCoefPer = pConfig->mMain2ndEncppSharpAttenCoefPer;
    alogd("main2nd EncppSharpAttenCoefPer: %d%%", pStreamContext->mEncppSharpAttenCoefPer);
    pStreamContext->mIspAndVeLinkageEnable = pConfig->mIspAndVeLinkageEnable;
    pStreamContext->mCameraAdaptiveMovingAndStaticEnable = pConfig->mCameraAdaptiveMovingAndStaticEnable;

    pStreamContext->mVEncChnAttr.EncppAttr.eEncppSharpSetting = pConfig->mMain2ndEncppEnable?VencEncppSharp_FollowISPConfig:VencEncppSharp_Disable;

    if (pConfig->mMain2ndSrcFrameRate)
    {
        vbvThreshSize = pConfig->mMain2ndEncodeBitrate/8/pConfig->mMain2ndSrcFrameRate*15;
    }
    vbvBufSize = pConfig->mMain2ndEncodeBitrate/8*VE_VBV_CACHE_TIME + vbvThreshSize;
    pStreamContext->mVbvBufSize = vbvBufSize;
    alogd("main2nd vbvThreshSize: %d, vbvBufSize: %d", vbvThreshSize, vbvBufSize);

    VencRateCtrlConfig stRcConfig;
    memset(&stRcConfig, 0, sizeof(VencRateCtrlConfig));
    stRcConfig.mEncodeType = pConfig->mMain2ndEncodeType;
    stRcConfig.mEncodeWidth = pConfig->mMain2ndEncodeWidth;
    stRcConfig.mEncodeHeight = pConfig->mMain2ndEncodeHeight;
    stRcConfig.mSrcFrameRate = pConfig->mMain2ndSrcFrameRate;
    stRcConfig.mDstFrameRate = pConfig->mMain2ndEncodeFrameRate;
    stRcConfig.mEncodeBitrate = pConfig->mMain2ndEncodeBitrate;
    stRcConfig.mRcMode = getRcModeFromConfig(pConfig->mRcMode, pConfig->mMain2ndEncodeType);
    stRcConfig.mMaxIQp = pConfig->mMaxIQp;
    stRcConfig.mMinIQp = pConfig->mMinIQp;
    stRcConfig.mMaxPQp = pConfig->mMaxPQp;
    stRcConfig.mMinPQp = pConfig->mMinPQp;
    stRcConfig.mInitQp = pConfig->mInitQp;
    stRcConfig.mEnMbQpLimit = pConfig->mEnMbQpLimit;
    stRcConfig.mQuality = pConfig->mQuality;
    stRcConfig.mMovingTh = pConfig->mMovingTh;
    stRcConfig.mIBitsCoef = pConfig->mIBitsCoef;
    stRcConfig.mPBitsCoef = pConfig->mPBitsCoef;
    stRcConfig.mVbvBufSize = vbvBufSize;
    stRcConfig.mVbvThreshSize = vbvThreshSize;
    configVencRateCtrlParam(&stRcConfig, &pStreamContext->mVEncChnAttr, &pStreamContext->mVEncRcParam);

    pStreamContext->mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pStreamContext->mVEncChnAttr.GopAttr.mGopSize = 2;
    pStreamContext->mVEncFrameRateConfig.SrcFrmRate = pConfig->mMain2ndSrcFrameRate;
    pStreamContext->mVEncFrameRateConfig.DstFrmRate = pConfig->mMain2ndEncodeFrameRate;

#ifdef ENABLE_VENC_ADVANCED_PARAM
    if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type || PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        configIFrmMbRcMoveStatus(&pStreamContext->mVEncRcParam);
        configBitsClipParam(&pStreamContext->mVEncRcParam);
    }
#endif

    pStreamContext->mTakePicture = pConfig->mMain2ndTakePicture;
    pStreamContext->mTakePictureViChn = pConfig->mMain2ndTakePictureViChn;
    pStreamContext->mTakePictureVeChn = pConfig->mMain2ndTakePictureVencChn;
    pStreamContext->mTakePictureInterval = pConfig->mMain2ndTakePictureInterval;
    strncpy(pStreamContext->mTakePictureFile, pConfig->mMain2ndTakePictureFile, MAX_FILE_PATH_SIZE);
    pStreamContext->mTakePictureFileCnt = pConfig->mMain2ndTakePictureFileCnt;
    pStreamContext->mTakePictureOnlyCaptureYuv = pConfig->mMain2ndTakePictureOnlyCaptureYuv;

#ifdef SUPPORT_PERSON_DETECT
    pStreamContext->mPdetEnable = pConfig->mMain2ndPdetEnable;
    pStreamContext->mPdetViChn = pConfig->mMain2ndPdetViChn;
    pStreamContext->mPdetInfo.pdet_input_w = AWALIGN(pConfig->mMain2ndPdetInputWidth, 32);
    pStreamContext->mPdetInfo.pdet_input_h = AWALIGN(pConfig->mMain2ndPdetInputHeight, 32);
    pStreamContext->mPdetInfo.pdet_input_c = 3;
    pStreamContext->mPdetInfo.pdet_conf_thres = pConfig->mMain2ndPdetConfThres;

    if (pStreamContext->mPdetEnable)
    {
        if (pConfig->motionAlarm_on)
        {
            pConfig->motionAlarm_on = 0;
            alogw("four2nd disable VE MotionDetect and use SW MotionDetect when enable pdet !");
        }
        else
        {
            alogw("four2nd VE MotionDetect is disabled, use SW MotionDetect when enable pdet !");
        }
    }

    pStreamContext->mDewbInfo.mInputInfo.mWidth = pConfig->mMain2ndSrcWidth;
    pStreamContext->mDewbInfo.mInputInfo.mHeight = pConfig->mMain2ndSrcHeight;
    pStreamContext->mDewbInfo.mOutputInfo.mWidth = pConfig->mMain2ndPdetInputWidth;
    pStreamContext->mDewbInfo.mOutputInfo.mHeight = pConfig->mMain2ndPdetInputHeight;
    if ((MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420 == pConfig->mMain2ndPixelFormat)
        || (MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pConfig->mMain2ndPixelFormat))
    {
        pStreamContext->mDewbInfo.mInputInfo.ePixFmt = DISP_FORMAT_YUV420_SP_UVUV;
    }
    else
    {
        aloge("fatal error! Pdet DE input not support this PixFmt!\n");
    }

    pStreamContext->mDewbInfo.mOutputInfo.ePixFmtTpye = DISP_CSC_TYPE_RGB;
    pStreamContext->mDewbInfo.mOutputInfo.ePixFmt = DISP_FORMAT_RGB_888;

    alogd("main2nd pdet input w[%d] h[%d], chn[%d], thres[%f], de input w[%d] h[%d], de output w [%d] h[%d]",
        pStreamContext->mPdetInfo.pdet_input_w, pStreamContext->mPdetInfo.pdet_input_h,
        pStreamContext->mPdetInfo.pdet_input_c, pStreamContext->mPdetInfo.pdet_conf_thres,
        pStreamContext->mDewbInfo.mInputInfo.mWidth, pStreamContext->mDewbInfo.mInputInfo.mHeight,
        pStreamContext->mDewbInfo.mOutputInfo.mWidth, pStreamContext->mDewbInfo.mOutputInfo.mHeight);

    // motion_pdet main2nd
    pStreamContext->mMotionPdetParam.pDewbInfo = &pStreamContext->mDewbInfo;
    pStreamContext->mMotionPdetParam.pPdetInfo = &pStreamContext->mPdetInfo;

    MotionDetectContext *pMotionDetectContext = &pStreamContext->mMotionPdetParam.mMotionDetectContext;
    pMotionDetectContext->mMdStaCfg.nPicWidth = AWALIGN(pConfig->mMain2ndSrcWidth, 16);
    pMotionDetectContext->mMdStaCfg.nPicStride = AWALIGN(pConfig->mMain2ndSrcWidth, 16);
    pMotionDetectContext->mMdStaCfg.nPicHeight = AWALIGN(pConfig->mMain2ndSrcHeight, 16);
    pMotionDetectContext->mMdDynCfg.nHorRegionNum = pMotionDetectContext->mMdStaCfg.nPicWidth / 16;
    pMotionDetectContext->mMdDynCfg.nVerRegionNum = pMotionDetectContext->mMdStaCfg.nPicHeight / 16;
    pMotionDetectContext->mMdDynCfg.bDefaultCfgDis  = 1;
    pMotionDetectContext->mMdDynCfg.bMorphologEn    = 0;
    pMotionDetectContext->mMdDynCfg.nLargeMadTh     = 10;
    pMotionDetectContext->mMdDynCfg.nBackgroundWeight  = 0;
    pMotionDetectContext->mMdDynCfg.fLargeMadRatioTh  = 10.0;

    pMotionDetectContext->mJudgeMotionNum = pConfig->mJudgeMotionNum;
    pMotionDetectContext->mMotionDynamicThres = pConfig->mMotionDynamicThres;
    pMotionDetectContext->mMotionPdetThres = pConfig->mMotionPdetThres;

    alogd("motion_pdet param main2nd pdet input w[%d] h[%d], chn[%d], thres[%f], de input w[%d] h[%d], de output w [%d] h[%d]",
        pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_w, pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_h,
        pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_c, pStreamContext->mMotionPdetParam.pPdetInfo->pdet_conf_thres,
        pStreamContext->mMotionPdetParam.pDewbInfo->mInputInfo.mWidth, pStreamContext->mMotionPdetParam.pDewbInfo->mInputInfo.mHeight,
        pStreamContext->mMotionPdetParam.pDewbInfo->mOutputInfo.mWidth, pStreamContext->mMotionPdetParam.pDewbInfo->mOutputInfo.mHeight);

    MOTION_DETECT_STATIC_CFG_S *pMdStaCfg = &pStreamContext->mMotionPdetParam.mMotionDetectContext.mMdStaCfg;
    MOTION_DETECT_DYNAMIC_CFG_S *pMdDynCfg = &pStreamContext->mMotionPdetParam.mMotionDetectContext.mMdDynCfg;

    alogd("motion_pdet static cfg {nPicHeight:%d, nPicWidth:%d, nPicStride:%d}", pMdStaCfg->nPicHeight, pMdStaCfg->nPicWidth, pMdStaCfg->nPicStride);
    alogd("motion_pdet dynamic cfg {%d %d}", pMdDynCfg->nHorRegionNum, pMdDynCfg->nVerRegionNum);
#endif
}

static void configSubStream(VencStreamContext *pStreamContext, SampleSmartIPCDemoConfig *pConfig)
{
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;
    if (!pStreamContext || !pConfig)
    {
        aloge("fatal error! invalid input params! %p,%p", pStreamContext, pConfig);
    }

    pStreamContext->mIsp = pConfig->mSubIsp;
    pStreamContext->mIspAlgoFreq = pConfig->mSubIspAlgoFreq;
    pStreamContext->mVipp = pConfig->mSubVipp;
    pStreamContext->mViChn = pConfig->mSubViChn;
    pStreamContext->mVEncChn = pConfig->mSubVEncChn;

    if (pConfig->mSubOnlineEnable)
    {
        pStreamContext->mViAttr.mOnlineEnable = 1;
        pStreamContext->mViAttr.mOnlineShareBufNum = pConfig->mSubOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable = 1;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum = pConfig->mSubOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable = pConfig->mMultiSensorOnlineEnable;
    }
    alogd("sub vipp%d ve_online_en:%d, dma_buf_num:%d, venc ch%d OnlineEnable:%d, OnlineShareBufNum:%d",
        pStreamContext->mVipp, pStreamContext->mViAttr.mOnlineEnable, pStreamContext->mViAttr.mOnlineShareBufNum,
        pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable, pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum);

    pStreamContext->mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pStreamContext->mViAttr.memtype = V4L2_MEMORY_MMAP;
    pStreamContext->mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pConfig->mSubPixelFormat);
    pStreamContext->mViAttr.format.field = V4L2_FIELD_NONE;
    if (pConfig->mIspColorSpace){
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709_PART_RANGE;
    }
    else
    {
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709;
    }
    pStreamContext->mViAttr.format.width = pConfig->mSubSrcWidth;
    pStreamContext->mViAttr.format.height = pConfig->mSubSrcHeight;
    pStreamContext->mViAttr.fps = pConfig->mSubSrcFrameRate;
    pStreamContext->mViAttr.use_current_win = 1;
    pStreamContext->mViAttr.nbufs = pConfig->mSubViBufNum;
    pStreamContext->mViAttr.nplanes = 2;
    pStreamContext->mViAttr.wdr_mode = pConfig->mSubWdrEnable;
    pStreamContext->mViAttr.drop_frame_num = 0;
    if (-1 != pConfig->mSubIspTdmRawProcessType)
    {
        pStreamContext->mViAttr.tdm_rxbuf_cnt = pConfig->mSubIspTdmRawRxBufNum;
    }
    pStreamContext->mViAttr.tdm_speed_down_en = pConfig->mTdmSpeedDownEnable;
    alogd("sub vipp%d tdm_speed_down_en:%d", pStreamContext->mVipp, pStreamContext->mViAttr.tdm_speed_down_en);
#ifdef SOFTWARE_TDM_FOR_ISP
    pStreamContext->mViAttr.bSensorSwitch = TRUE;
    alogd("sub vipp%d enable sensorSwitch for v821 3-sensors project", pStreamContext->mVipp);
#endif

    pStreamContext->mVEncChnAttr.VeAttr.mVippID = pConfig->mSubVipp;
    pStreamContext->mVEncChnAttr.VeAttr.Type = pConfig->mSubEncodeType;
    pStreamContext->mVEncChnAttr.VeAttr.MaxKeyInterval = pConfig->mSubKeyFrameInterval;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicWidth = pConfig->mSubSrcWidth;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicHeight = pConfig->mSubSrcHeight;
    pStreamContext->mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pStreamContext->mVEncChnAttr.VeAttr.PixelFormat = pConfig->mSubPixelFormat;
    pStreamContext->mVEncChnAttr.VeAttr.mColorSpace = pStreamContext->mViAttr.format.colorspace;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRefFrameLbcMode = pConfig->mSubVeRefFrameLbcMode;;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRecRefBufReduceEnable = pConfig->mVeRecRefBufReduceEnable;

    pStreamContext->mVEncChnAttr.VeAttr.mVbrOptEnable = pConfig->mVbrOptEnable;

    pStreamContext->mVEncChnAttr.RcAttr.mProductMode = pConfig->mProductMode;
    //pStreamContext->mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

    pStreamContext->mEncppSharpAttenCoefPer = pConfig->mSubEncppSharpAttenCoefPer;
    alogd("sub EncppSharpAttenCoefPer: %d%%", pStreamContext->mEncppSharpAttenCoefPer);
    pStreamContext->mIspAndVeLinkageEnable = pConfig->mIspAndVeLinkageEnable;
    pStreamContext->mCameraAdaptiveMovingAndStaticEnable = pConfig->mCameraAdaptiveMovingAndStaticEnable;

    pStreamContext->mVEncChnAttr.EncppAttr.eEncppSharpSetting = pConfig->mSubEncppEnable?VencEncppSharp_FollowISPConfig:VencEncppSharp_Disable;

    if (pConfig->mSubSrcFrameRate)
    {
        vbvThreshSize = pConfig->mSubEncodeBitrate/8/pConfig->mSubSrcFrameRate*15;
    }
    vbvBufSize = pConfig->mSubEncodeBitrate/8*VE_VBV_CACHE_TIME + vbvThreshSize;
    pStreamContext->mVbvBufSize = vbvBufSize;
    alogd("sub vbvThreshSize: %d, vbvBufSize: %d", vbvThreshSize, vbvBufSize);

    VencRateCtrlConfig stRcConfig;
    memset(&stRcConfig, 0, sizeof(VencRateCtrlConfig));
    stRcConfig.mEncodeType = pConfig->mSubEncodeType;
    stRcConfig.mEncodeWidth = pConfig->mSubEncodeWidth;
    stRcConfig.mEncodeHeight = pConfig->mSubEncodeHeight;
    stRcConfig.mSrcFrameRate = pConfig->mSubSrcFrameRate;
    stRcConfig.mDstFrameRate = pConfig->mSubEncodeFrameRate;
    stRcConfig.mEncodeBitrate = pConfig->mSubEncodeBitrate;
    stRcConfig.mRcMode = getRcModeFromConfig(pConfig->mRcMode, pConfig->mSubEncodeType);
    stRcConfig.mMaxIQp = pConfig->mMaxIQp;
    stRcConfig.mMinIQp = pConfig->mMinIQp;
    stRcConfig.mMaxPQp = pConfig->mMaxPQp;
    stRcConfig.mMinPQp = pConfig->mMinPQp;
    stRcConfig.mInitQp = pConfig->mInitQp;
    stRcConfig.mEnMbQpLimit = pConfig->mEnMbQpLimit;
    stRcConfig.mQuality = pConfig->mQuality;
    stRcConfig.mMovingTh = pConfig->mMovingTh;
    stRcConfig.mIBitsCoef = pConfig->mIBitsCoef;
    stRcConfig.mPBitsCoef = pConfig->mPBitsCoef;
    stRcConfig.mVbvBufSize = vbvBufSize;
    stRcConfig.mVbvThreshSize = vbvThreshSize;
    configVencRateCtrlParam(&stRcConfig, &pStreamContext->mVEncChnAttr, &pStreamContext->mVEncRcParam);

    pStreamContext->mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pStreamContext->mVEncChnAttr.GopAttr.mGopSize = 2;
    pStreamContext->mVEncFrameRateConfig.SrcFrmRate = pConfig->mSubSrcFrameRate;
    pStreamContext->mVEncFrameRateConfig.DstFrmRate = pConfig->mSubEncodeFrameRate;

#ifdef ENABLE_VENC_ADVANCED_PARAM
    if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type || PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        configIFrmMbRcMoveStatus(&pStreamContext->mVEncRcParam);
        configBitsClipParam(&pStreamContext->mVEncRcParam);
    }
#endif
}

static void configSub2ndStream(VencStreamContext *pStreamContext, SampleSmartIPCDemoConfig *pConfig)
{
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;
    if (!pStreamContext || !pConfig)
    {
        aloge("fatal error! invalid input params! %p,%p", pStreamContext, pConfig);
    }

    pStreamContext->mIsp = pConfig->mSubIsp;
    pStreamContext->mIspAlgoFreq = pConfig->mSubIspAlgoFreq;
    pStreamContext->mVipp = pConfig->mSub2ndVipp;
    pStreamContext->mViChn = pConfig->mSub2ndViChn;
    pStreamContext->mVEncChn = pConfig->mSub2ndVEncChn;

    if (pConfig->mSub2ndOnlineEnable)
    {
        pStreamContext->mViAttr.mOnlineEnable = 1;
        pStreamContext->mViAttr.mOnlineShareBufNum = pConfig->mSub2ndOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable = 1;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum = pConfig->mSub2ndOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable = pConfig->mMultiSensorOnlineEnable;
    }
    alogd("sub2nd vipp%d ve_online_en:%d, dma_buf_num:%d, venc ch%d OnlineEnable:%d, OnlineShareBufNum:%d",
        pStreamContext->mVipp, pStreamContext->mViAttr.mOnlineEnable, pStreamContext->mViAttr.mOnlineShareBufNum,
        pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable, pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum);

    pStreamContext->mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pStreamContext->mViAttr.memtype = V4L2_MEMORY_MMAP;
    pStreamContext->mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pConfig->mSub2ndPixelFormat);
    pStreamContext->mViAttr.format.field = V4L2_FIELD_NONE;
    if (pConfig->mIspColorSpace){
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709_PART_RANGE;
    }
    else
    {
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709;
    }
    pStreamContext->mViAttr.format.width = pConfig->mSub2ndSrcWidth;
    pStreamContext->mViAttr.format.height = pConfig->mSub2ndSrcHeight;
    pStreamContext->mViAttr.fps = pConfig->mSub2ndSrcFrameRate;
    pStreamContext->mViAttr.use_current_win = 1;
    pStreamContext->mViAttr.nbufs = pConfig->mSub2ndViBufNum;
    pStreamContext->mViAttr.nplanes = 2;
    pStreamContext->mViAttr.wdr_mode = 0;
    pStreamContext->mViAttr.drop_frame_num = 0;
    pStreamContext->mViAttr.tdm_speed_down_en = pConfig->mTdmSpeedDownEnable;
    alogd("sub2nd vipp%d tdm_speed_down_en:%d", pStreamContext->mVipp, pStreamContext->mViAttr.tdm_speed_down_en);
#ifdef SOFTWARE_TDM_FOR_ISP
    pStreamContext->mViAttr.bSensorSwitch = TRUE;
    alogd("sub2nd vipp%d enable sensorSwitch for v821 3-sensors project", pStreamContext->mVipp);
#endif
    pStreamContext->mVEncChnAttr.VeAttr.mVippID = pConfig->mSub2ndVipp;
    pStreamContext->mVEncChnAttr.VeAttr.Type = pConfig->mSub2ndEncodeType;
    pStreamContext->mVEncChnAttr.VeAttr.MaxKeyInterval = pConfig->mSub2ndKeyFrameInterval;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicWidth = pConfig->mSub2ndSrcWidth;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicHeight = pConfig->mSub2ndSrcHeight;
    pStreamContext->mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pStreamContext->mVEncChnAttr.VeAttr.PixelFormat = pConfig->mSub2ndPixelFormat;
    pStreamContext->mVEncChnAttr.VeAttr.mColorSpace = pStreamContext->mViAttr.format.colorspace;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRefFrameLbcMode = pConfig->mSub2ndVeRefFrameLbcMode;;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRecRefBufReduceEnable = pConfig->mVeRecRefBufReduceEnable;

    pStreamContext->mVEncChnAttr.VeAttr.mVbrOptEnable = pConfig->mVbrOptEnable;

    pStreamContext->mVEncChnAttr.RcAttr.mProductMode = pConfig->mProductMode;
    //pStreamContext->mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

    pStreamContext->mEncppSharpAttenCoefPer = pConfig->mSub2ndEncppSharpAttenCoefPer;
    alogd("sub2nd EncppSharpAttenCoefPer: %d%%", pStreamContext->mEncppSharpAttenCoefPer);
    pStreamContext->mIspAndVeLinkageEnable = pConfig->mIspAndVeLinkageEnable;
    pStreamContext->mCameraAdaptiveMovingAndStaticEnable = pConfig->mCameraAdaptiveMovingAndStaticEnable;

    pStreamContext->mVEncChnAttr.EncppAttr.eEncppSharpSetting = pConfig->mSub2ndEncppEnable?VencEncppSharp_FollowISPConfig:VencEncppSharp_Disable;

    if (pConfig->mSub2ndSrcFrameRate)
    {
        vbvThreshSize = pConfig->mSub2ndEncodeBitrate/8/pConfig->mSub2ndSrcFrameRate*15;
    }
    vbvBufSize = pConfig->mSub2ndEncodeBitrate/8*VE_VBV_CACHE_TIME + vbvThreshSize;
    pStreamContext->mVbvBufSize = vbvBufSize;
    alogd("sub2nd vbvThreshSize: %d, vbvBufSize: %d", vbvThreshSize, vbvBufSize);

    VencRateCtrlConfig stRcConfig;
    memset(&stRcConfig, 0, sizeof(VencRateCtrlConfig));
    stRcConfig.mEncodeType = pConfig->mSub2ndEncodeType;
    stRcConfig.mEncodeWidth = pConfig->mSub2ndEncodeWidth;
    stRcConfig.mEncodeHeight = pConfig->mSub2ndEncodeHeight;
    stRcConfig.mSrcFrameRate = pConfig->mSub2ndSrcFrameRate;
    stRcConfig.mDstFrameRate = pConfig->mSub2ndEncodeFrameRate;
    stRcConfig.mEncodeBitrate = pConfig->mSub2ndEncodeBitrate;
    stRcConfig.mRcMode = getRcModeFromConfig(pConfig->mRcMode, pConfig->mSub2ndEncodeType);
    stRcConfig.mMaxIQp = pConfig->mMaxIQp;
    stRcConfig.mMinIQp = pConfig->mMinIQp;
    stRcConfig.mMaxPQp = pConfig->mMaxPQp;
    stRcConfig.mMinPQp = pConfig->mMinPQp;
    stRcConfig.mInitQp = pConfig->mInitQp;
    stRcConfig.mEnMbQpLimit = pConfig->mEnMbQpLimit;
    stRcConfig.mQuality = pConfig->mQuality;
    stRcConfig.mMovingTh = pConfig->mMovingTh;
    stRcConfig.mIBitsCoef = pConfig->mIBitsCoef;
    stRcConfig.mPBitsCoef = pConfig->mPBitsCoef;
    stRcConfig.mVbvBufSize = vbvBufSize;
    stRcConfig.mVbvThreshSize = vbvThreshSize;
    configVencRateCtrlParam(&stRcConfig, &pStreamContext->mVEncChnAttr, &pStreamContext->mVEncRcParam);

    pStreamContext->mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pStreamContext->mVEncChnAttr.GopAttr.mGopSize = 2;
    pStreamContext->mVEncFrameRateConfig.SrcFrmRate = pConfig->mSub2ndSrcFrameRate;
    pStreamContext->mVEncFrameRateConfig.DstFrmRate = pConfig->mSub2ndEncodeFrameRate;

#ifdef ENABLE_VENC_ADVANCED_PARAM
    if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type || PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        configIFrmMbRcMoveStatus(&pStreamContext->mVEncRcParam);
        configBitsClipParam(&pStreamContext->mVEncRcParam);
    }
#endif
    pStreamContext->mTakePicture = pConfig->mSub2ndTakePicture;
    pStreamContext->mTakePictureViChn = pConfig->mSub2ndTakePictureViChn;
    pStreamContext->mTakePictureVeChn = pConfig->mSub2ndTakePictureVencChn;
    pStreamContext->mTakePictureInterval = pConfig->mSub2ndTakePictureInterval;
    strncpy(pStreamContext->mTakePictureFile, pConfig->mSub2ndTakePictureFile, MAX_FILE_PATH_SIZE);
    pStreamContext->mTakePictureFileCnt = pConfig->mSub2ndTakePictureFileCnt;
    pStreamContext->mTakePictureOnlyCaptureYuv = pConfig->mSub2ndTakePictureOnlyCaptureYuv;

#ifdef SUPPORT_PERSON_DETECT
    pStreamContext->mPdetEnable = pConfig->mSub2ndPdetEnable;
    pStreamContext->mPdetViChn = pConfig->mSub2ndPdetViChn;
    pStreamContext->mPdetInfo.pdet_input_w = AWALIGN(pConfig->mSub2ndPdetInputWidth, 32);
    pStreamContext->mPdetInfo.pdet_input_h = AWALIGN(pConfig->mSub2ndPdetInputHeight, 32);
    pStreamContext->mPdetInfo.pdet_input_c = 3;
    pStreamContext->mPdetInfo.pdet_conf_thres = pConfig->mSub2ndPdetConfThres;

    if (pStreamContext->mPdetEnable)
    {
        if (pConfig->motionAlarm_on)
        {
            pConfig->motionAlarm_on = 0;
            alogw("four2nd disable VE MotionDetect and use SW MotionDetect when enable pdet !");
        }
        else
        {
            alogw("four2nd VE MotionDetect is disabled, use SW MotionDetect when enable pdet !");
        }
    }

    pStreamContext->mDewbInfo.mInputInfo.mWidth = pConfig->mSub2ndSrcWidth;
    pStreamContext->mDewbInfo.mInputInfo.mHeight = pConfig->mSub2ndSrcHeight;
    pStreamContext->mDewbInfo.mOutputInfo.mWidth = pConfig->mSub2ndPdetInputWidth;
    pStreamContext->mDewbInfo.mOutputInfo.mHeight = pConfig->mSub2ndPdetInputHeight;
    if ((MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420 == pConfig->mSub2ndPixelFormat)
        || (MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pConfig->mSub2ndPixelFormat))
    {
        pStreamContext->mDewbInfo.mInputInfo.ePixFmt = DISP_FORMAT_YUV420_SP_UVUV;
    }
    else
    {
        aloge("fatal error! Pdet DE input not support this PixFmt!\n");
    }
    pStreamContext->mDewbInfo.mOutputInfo.ePixFmtTpye = DISP_CSC_TYPE_RGB;

    alogd("sub2nd pdet input w[%d] h[%d], chn[%d], thres[%f], de input w[%d] h[%d], de output w [%d] h[%d]",
        pStreamContext->mPdetInfo.pdet_input_w, pStreamContext->mPdetInfo.pdet_input_h,
        pStreamContext->mPdetInfo.pdet_input_c, pStreamContext->mPdetInfo.pdet_conf_thres,
        pStreamContext->mDewbInfo.mInputInfo.mWidth, pStreamContext->mDewbInfo.mInputInfo.mHeight,
        pStreamContext->mDewbInfo.mOutputInfo.mWidth, pStreamContext->mDewbInfo.mOutputInfo.mHeight);

    // motion_pdet sub2nd
    pStreamContext->mMotionPdetParam.pDewbInfo = &pStreamContext->mDewbInfo;
    pStreamContext->mMotionPdetParam.pPdetInfo = &pStreamContext->mPdetInfo;

    MotionDetectContext *pMotionDetectContext = &pStreamContext->mMotionPdetParam.mMotionDetectContext;
    pMotionDetectContext->mMdStaCfg.nPicWidth = AWALIGN(pConfig->mSub2ndSrcWidth, 16);
    pMotionDetectContext->mMdStaCfg.nPicStride = AWALIGN(pConfig->mSub2ndSrcWidth, 16);
    pMotionDetectContext->mMdStaCfg.nPicHeight = AWALIGN(pConfig->mSub2ndSrcHeight, 16);
    pMotionDetectContext->mMdDynCfg.nHorRegionNum = pMotionDetectContext->mMdStaCfg.nPicWidth / 16;
    pMotionDetectContext->mMdDynCfg.nVerRegionNum = pMotionDetectContext->mMdStaCfg.nPicHeight / 16;
    pMotionDetectContext->mMdDynCfg.bDefaultCfgDis  = 1;
    pMotionDetectContext->mMdDynCfg.bMorphologEn    = 0;
    pMotionDetectContext->mMdDynCfg.nLargeMadTh     = 10;
    pMotionDetectContext->mMdDynCfg.nBackgroundWeight  = 0;
    pMotionDetectContext->mMdDynCfg.fLargeMadRatioTh  = 10.0;

    pStreamContext->mMotionPdetParam.mMotionDetectContext.mJudgeMotionNum = pConfig->mJudgeMotionNum;
    pStreamContext->mMotionPdetParam.mMotionDetectContext.mMotionDynamicThres = pConfig->mMotionDynamicThres;
    pStreamContext->mMotionPdetParam.mMotionDetectContext.mMotionPdetThres = pConfig->mMotionPdetThres;

    alogd("motion_pdet param sub2nd pdet input w[%d] h[%d], chn[%d], thres[%f], de input w[%d] h[%d], de output w [%d] h[%d]",
        pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_w, pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_h,
        pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_c, pStreamContext->mMotionPdetParam.pPdetInfo->pdet_conf_thres,
        pStreamContext->mMotionPdetParam.pDewbInfo->mInputInfo.mWidth, pStreamContext->mMotionPdetParam.pDewbInfo->mInputInfo.mHeight,
        pStreamContext->mMotionPdetParam.pDewbInfo->mOutputInfo.mWidth, pStreamContext->mMotionPdetParam.pDewbInfo->mOutputInfo.mHeight);

    MOTION_DETECT_STATIC_CFG_S *pMdStaCfg = &pStreamContext->mMotionPdetParam.mMotionDetectContext.mMdStaCfg;
    MOTION_DETECT_DYNAMIC_CFG_S *pMdDynCfg = &pStreamContext->mMotionPdetParam.mMotionDetectContext.mMdDynCfg;

    alogd("motion_pdet static cfg {nPicHeight:%d, nPicWidth:%d, nPicStride:%d}", pMdStaCfg->nPicHeight, pMdStaCfg->nPicWidth, pMdStaCfg->nPicStride);
    alogd("motion_pdet dynamic cfg {%d %d}", pMdDynCfg->nHorRegionNum, pMdDynCfg->nVerRegionNum);
#endif
}

static void configThreeStream(VencStreamContext *pStreamContext, SampleSmartIPCDemoConfig *pConfig)
{
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;
    if (!pStreamContext || !pConfig)
    {
        aloge("fatal error! invalid input params! %p,%p", pStreamContext, pConfig);
    }

    pStreamContext->mIsp = pConfig->mThreeIsp;
    pStreamContext->mIspAlgoFreq = pConfig->mThreeIspAlgoFreq;
    pStreamContext->mVipp = pConfig->mThreeVipp;
    pStreamContext->mViChn = pConfig->mThreeViChn; //0;
    pStreamContext->mVEncChn = pConfig->mThreeVEncChn; //0;

    if (pConfig->mThreeOnlineEnable)
    {
        pStreamContext->mViAttr.mOnlineEnable = 1;
        pStreamContext->mViAttr.mOnlineShareBufNum = pConfig->mThreeOnlineShareBufNum;//BK_TWO_BUFFER;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable = 1;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum = pConfig->mThreeOnlineShareBufNum;//BK_TWO_BUFFER;
        pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable = pConfig->mMultiSensorOnlineEnable;
    }
    alogd("three vipp%d ve_online_en:%d, dma_buf_num:%d, venc ch%d OnlineEnable:%d, OnlineShareBufNum:%d",
        pStreamContext->mVipp, pStreamContext->mViAttr.mOnlineEnable, pStreamContext->mViAttr.mOnlineShareBufNum,
        pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable, pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum);

    pStreamContext->mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pStreamContext->mViAttr.memtype = V4L2_MEMORY_MMAP;
    pStreamContext->mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pConfig->mThreePixelFormat);
    pStreamContext->mViAttr.format.field = V4L2_FIELD_NONE;
    if (pConfig->mIspColorSpace){
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709_PART_RANGE;
    }
    else
    {
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709;
    }
    pStreamContext->mViAttr.format.width = pConfig->mThreeSrcWidth;
    pStreamContext->mViAttr.format.height = pConfig->mThreeSrcHeight;
    pStreamContext->mViAttr.fps = pConfig->mThreeSrcFrameRate;
    pStreamContext->mViAttr.use_current_win = 0;
    pStreamContext->mViAttr.nbufs = pConfig->mThreeViBufNum;
    pStreamContext->mViAttr.nplanes = 2;
    pStreamContext->mViAttr.wdr_mode = pConfig->mThreeWdrEnable;
    pStreamContext->mViAttr.drop_frame_num = 0;
    if (-1 != pConfig->mThreeIspTdmRawProcessType)
    {
        pStreamContext->mViAttr.tdm_rxbuf_cnt = pConfig->mThreeIspTdmRawRxBufNum;
    }
    pStreamContext->mViAttr.tdm_speed_down_en = pConfig->mTdmSpeedDownEnable;
    alogd("three vipp%d tdm_speed_down_en:%d", pStreamContext->mVipp, pStreamContext->mViAttr.tdm_speed_down_en);

    pStreamContext->mVEncChnAttr.VeAttr.mVippID = pConfig->mThreeVipp;
    pStreamContext->mVEncChnAttr.VeAttr.Type = pConfig->mThreeEncodeType;
    pStreamContext->mVEncChnAttr.VeAttr.MaxKeyInterval = pConfig->mThreeKeyFrameInterval;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicWidth = pConfig->mThreeSrcWidth;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicHeight = pConfig->mThreeSrcHeight;
    pStreamContext->mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pStreamContext->mVEncChnAttr.VeAttr.PixelFormat = pConfig->mThreePixelFormat;
    pStreamContext->mVEncChnAttr.VeAttr.mColorSpace = pStreamContext->mViAttr.format.colorspace;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRefFrameLbcMode = pConfig->mThreeVeRefFrameLbcMode;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRecRefBufReduceEnable = pConfig->mVeRecRefBufReduceEnable;

    pStreamContext->mVEncChnAttr.VeAttr.mVbrOptEnable = pConfig->mVbrOptEnable;

    pStreamContext->mVEncChnAttr.RcAttr.mProductMode = pConfig->mProductMode;
    //pStreamContext->mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

    pStreamContext->mEncppSharpAttenCoefPer = 100;
    alogd("three EncppSharpAttenCoefPer: %d%%", pStreamContext->mEncppSharpAttenCoefPer);
    pStreamContext->mIspAndVeLinkageEnable = pConfig->mIspAndVeLinkageEnable;
    pStreamContext->mCameraAdaptiveMovingAndStaticEnable = pConfig->mCameraAdaptiveMovingAndStaticEnable;

    pStreamContext->mVEncChnAttr.EncppAttr.eEncppSharpSetting = pConfig->mThreeEncppEnable?VencEncppSharp_FollowISPConfig:VencEncppSharp_Disable;

    if (pConfig->mThreeSrcFrameRate)
    {
        vbvThreshSize = pConfig->mThreeEncodeBitrate/8/pConfig->mThreeSrcFrameRate*15;
    }
    vbvBufSize = pConfig->mThreeEncodeBitrate/8*VE_VBV_CACHE_TIME + vbvThreshSize;
    pStreamContext->mVbvBufSize = vbvBufSize;
    alogd("three vbvThreshSize: %d, vbvBufSize: %d", vbvThreshSize, vbvBufSize);

    VencRateCtrlConfig stRcConfig;
    memset(&stRcConfig, 0, sizeof(VencRateCtrlConfig));
    stRcConfig.mEncodeType = pConfig->mThreeEncodeType;
    stRcConfig.mEncodeWidth = pConfig->mThreeEncodeWidth;
    stRcConfig.mEncodeHeight = pConfig->mThreeEncodeHeight;
    stRcConfig.mSrcFrameRate = pConfig->mThreeSrcFrameRate;
    stRcConfig.mDstFrameRate = pConfig->mThreeEncodeFrameRate;
    stRcConfig.mEncodeBitrate = pConfig->mThreeEncodeBitrate;
    stRcConfig.mRcMode = getRcModeFromConfig(pConfig->mRcMode, pConfig->mThreeEncodeType);
    stRcConfig.mMaxIQp = pConfig->mMaxIQp;
    stRcConfig.mMinIQp = pConfig->mMinIQp;
    stRcConfig.mMaxPQp = pConfig->mMaxPQp;
    stRcConfig.mMinPQp = pConfig->mMinPQp;
    stRcConfig.mInitQp = pConfig->mInitQp;
    stRcConfig.mEnMbQpLimit = pConfig->mEnMbQpLimit;
    stRcConfig.mQuality = pConfig->mQuality;
    stRcConfig.mMovingTh = pConfig->mMovingTh;
    stRcConfig.mIBitsCoef = pConfig->mIBitsCoef;
    stRcConfig.mPBitsCoef = pConfig->mPBitsCoef;
    stRcConfig.mVbvBufSize = vbvBufSize;
    stRcConfig.mVbvThreshSize = vbvThreshSize;
    configVencRateCtrlParam(&stRcConfig, &pStreamContext->mVEncChnAttr, &pStreamContext->mVEncRcParam);

    pStreamContext->mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pStreamContext->mVEncChnAttr.GopAttr.mGopSize = 2;

    pStreamContext->mVEncFrameRateConfig.SrcFrmRate = pConfig->mThreeSrcFrameRate;
    pStreamContext->mVEncFrameRateConfig.DstFrmRate = pConfig->mThreeEncodeFrameRate;

#ifdef ENABLE_VENC_ADVANCED_PARAM
    if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type || PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        configIFrmMbRcMoveStatus(&pStreamContext->mVEncRcParam);
        configBitsClipParam(&pStreamContext->mVEncRcParam);
    }
#endif
}

static void configThree2ndStream(VencStreamContext *pStreamContext, SampleSmartIPCDemoConfig *pConfig)
{
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;
    if (!pStreamContext || !pConfig)
    {
        aloge("fatal error! invalid input params! %p,%p", pStreamContext, pConfig);
    }

    pStreamContext->mIsp = pConfig->mThreeIsp;
    pStreamContext->mIspAlgoFreq = pConfig->mThreeIspAlgoFreq;
    pStreamContext->mVipp = pConfig->mThree2ndVipp;
    pStreamContext->mViChn = pConfig->mThree2ndViChn;
    pStreamContext->mVEncChn = pConfig->mThree2ndVEncChn;

    if (pConfig->mThree2ndOnlineEnable)
    {
        pStreamContext->mViAttr.mOnlineEnable = 1;
        pStreamContext->mViAttr.mOnlineShareBufNum = pConfig->mThree2ndOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable = 1;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum = pConfig->mThree2ndOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable = pConfig->mMultiSensorOnlineEnable;
    }
    alogd("three2nd vipp%d ve_online_en:%d, dma_buf_num:%d, venc ch%d OnlineEnable:%d, OnlineShareBufNum:%d",
        pStreamContext->mVipp, pStreamContext->mViAttr.mOnlineEnable, pStreamContext->mViAttr.mOnlineShareBufNum,
        pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable, pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum);

    pStreamContext->mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pStreamContext->mViAttr.memtype = V4L2_MEMORY_MMAP;
    pStreamContext->mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pConfig->mThree2ndPixelFormat);
    pStreamContext->mViAttr.format.field = V4L2_FIELD_NONE;
    if (pConfig->mIspColorSpace){
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709_PART_RANGE;
    }
    else
    {
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709;
    }
    pStreamContext->mViAttr.format.width = pConfig->mThree2ndSrcWidth;
    pStreamContext->mViAttr.format.height = pConfig->mThree2ndSrcHeight;
    pStreamContext->mViAttr.fps = pConfig->mThree2ndSrcFrameRate;
    pStreamContext->mViAttr.use_current_win = 1;
    pStreamContext->mViAttr.nbufs = pConfig->mThree2ndViBufNum;
    pStreamContext->mViAttr.nplanes = 2;
    pStreamContext->mViAttr.wdr_mode = 0;
    pStreamContext->mViAttr.drop_frame_num = 0;
    pStreamContext->mViAttr.tdm_speed_down_en = pConfig->mTdmSpeedDownEnable;
    alogd("three2nd vipp%d tdm_speed_down_en:%d", pStreamContext->mVipp, pStreamContext->mViAttr.tdm_speed_down_en);

    pStreamContext->mVEncChnAttr.VeAttr.mVippID = pConfig->mThree2ndVipp;
    pStreamContext->mVEncChnAttr.VeAttr.Type = pConfig->mThree2ndEncodeType;
    pStreamContext->mVEncChnAttr.VeAttr.MaxKeyInterval = pConfig->mThree2ndKeyFrameInterval;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicWidth = pConfig->mThree2ndSrcWidth;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicHeight = pConfig->mThree2ndSrcHeight;
    pStreamContext->mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pStreamContext->mVEncChnAttr.VeAttr.PixelFormat = pConfig->mThree2ndPixelFormat;
    pStreamContext->mVEncChnAttr.VeAttr.mColorSpace = pStreamContext->mViAttr.format.colorspace;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRefFrameLbcMode = pConfig->mThree2ndVeRefFrameLbcMode;;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRecRefBufReduceEnable = pConfig->mVeRecRefBufReduceEnable;

    pStreamContext->mVEncChnAttr.VeAttr.mVbrOptEnable = pConfig->mVbrOptEnable;

    pStreamContext->mVEncChnAttr.RcAttr.mProductMode = pConfig->mProductMode;
    //pStreamContext->mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

    pStreamContext->mEncppSharpAttenCoefPer = pConfig->mThree2ndEncppSharpAttenCoefPer;
    alogd("three2nd EncppSharpAttenCoefPer: %d%%", pStreamContext->mEncppSharpAttenCoefPer);
    pStreamContext->mIspAndVeLinkageEnable = pConfig->mIspAndVeLinkageEnable;
    pStreamContext->mCameraAdaptiveMovingAndStaticEnable = pConfig->mCameraAdaptiveMovingAndStaticEnable;

    pStreamContext->mVEncChnAttr.EncppAttr.eEncppSharpSetting = pConfig->mThree2ndEncppEnable?VencEncppSharp_FollowISPConfig:VencEncppSharp_Disable;

    if (pConfig->mThree2ndSrcFrameRate)
    {
        vbvThreshSize = pConfig->mThree2ndEncodeBitrate/8/pConfig->mThree2ndSrcFrameRate*15;
    }
    vbvBufSize = pConfig->mThree2ndEncodeBitrate/8*VE_VBV_CACHE_TIME + vbvThreshSize;
    pStreamContext->mVbvBufSize = vbvBufSize;
    alogd("three2nd vbvThreshSize: %d, vbvBufSize: %d", vbvThreshSize, vbvBufSize);

    VencRateCtrlConfig stRcConfig;
    memset(&stRcConfig, 0, sizeof(VencRateCtrlConfig));
    stRcConfig.mEncodeType = pConfig->mThree2ndEncodeType;
    stRcConfig.mEncodeWidth = pConfig->mThree2ndEncodeWidth;
    stRcConfig.mEncodeHeight = pConfig->mThree2ndEncodeHeight;
    stRcConfig.mSrcFrameRate = pConfig->mThree2ndSrcFrameRate;
    stRcConfig.mDstFrameRate = pConfig->mThree2ndEncodeFrameRate;
    stRcConfig.mEncodeBitrate = pConfig->mThree2ndEncodeBitrate;
    stRcConfig.mRcMode = getRcModeFromConfig(pConfig->mRcMode, pConfig->mThree2ndEncodeType);
    stRcConfig.mMaxIQp = pConfig->mMaxIQp;
    stRcConfig.mMinIQp = pConfig->mMinIQp;
    stRcConfig.mMaxPQp = pConfig->mMaxPQp;
    stRcConfig.mMinPQp = pConfig->mMinPQp;
    stRcConfig.mInitQp = pConfig->mInitQp;
    stRcConfig.mEnMbQpLimit = pConfig->mEnMbQpLimit;
    stRcConfig.mQuality = pConfig->mQuality;
    stRcConfig.mMovingTh = pConfig->mMovingTh;
    stRcConfig.mIBitsCoef = pConfig->mIBitsCoef;
    stRcConfig.mPBitsCoef = pConfig->mPBitsCoef;
    stRcConfig.mVbvBufSize = vbvBufSize;
    stRcConfig.mVbvThreshSize = vbvThreshSize;
    configVencRateCtrlParam(&stRcConfig, &pStreamContext->mVEncChnAttr, &pStreamContext->mVEncRcParam);

    pStreamContext->mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pStreamContext->mVEncChnAttr.GopAttr.mGopSize = 2;
    pStreamContext->mVEncFrameRateConfig.SrcFrmRate = pConfig->mThree2ndSrcFrameRate;
    pStreamContext->mVEncFrameRateConfig.DstFrmRate = pConfig->mThree2ndEncodeFrameRate;

#ifdef ENABLE_VENC_ADVANCED_PARAM
    if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type || PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        configIFrmMbRcMoveStatus(&pStreamContext->mVEncRcParam);
        configBitsClipParam(&pStreamContext->mVEncRcParam);
    }
#endif
    pStreamContext->mTakePicture = pConfig->mThree2ndTakePicture;
    pStreamContext->mTakePictureViChn = pConfig->mThree2ndTakePictureViChn;
    pStreamContext->mTakePictureVeChn = pConfig->mThree2ndTakePictureVencChn;
    pStreamContext->mTakePictureInterval = pConfig->mThree2ndTakePictureInterval;
    strncpy(pStreamContext->mTakePictureFile, pConfig->mThree2ndTakePictureFile, MAX_FILE_PATH_SIZE);
    pStreamContext->mTakePictureFileCnt = pConfig->mThree2ndTakePictureFileCnt;
    pStreamContext->mTakePictureOnlyCaptureYuv = pConfig->mThree2ndTakePictureOnlyCaptureYuv;

#ifdef SUPPORT_PERSON_DETECT
    pStreamContext->mPdetEnable = pConfig->mThree2ndPdetEnable;
    pStreamContext->mPdetViChn = pConfig->mThree2ndPdetViChn;
    pStreamContext->mPdetInfo.pdet_input_w = AWALIGN(pConfig->mThree2ndPdetInputWidth, 32);
    pStreamContext->mPdetInfo.pdet_input_h = AWALIGN(pConfig->mThree2ndPdetInputHeight, 32);
    pStreamContext->mPdetInfo.pdet_input_c = 3;
    pStreamContext->mPdetInfo.pdet_conf_thres = pConfig->mThree2ndPdetConfThres;

    if (pStreamContext->mPdetEnable)
    {
        if (pConfig->motionAlarm_on)
        {
            pConfig->motionAlarm_on = 0;
            alogw("four2nd disable VE MotionDetect and use SW MotionDetect when enable pdet !");
        }
        else
        {
            alogw("four2nd VE MotionDetect is disabled, use SW MotionDetect when enable pdet !");
        }
    }

    pStreamContext->mDewbInfo.mInputInfo.mWidth = pConfig->mThree2ndSrcWidth;
    pStreamContext->mDewbInfo.mInputInfo.mHeight = pConfig->mThree2ndSrcHeight;
    pStreamContext->mDewbInfo.mOutputInfo.mWidth = pConfig->mThree2ndPdetInputWidth;
    pStreamContext->mDewbInfo.mOutputInfo.mHeight = pConfig->mThree2ndPdetInputHeight;
    if ((MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420 == pConfig->mThree2ndPixelFormat)
        || (MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pConfig->mThree2ndPixelFormat))
    {
        pStreamContext->mDewbInfo.mInputInfo.ePixFmt = DISP_FORMAT_YUV420_SP_UVUV;
    }
    else
    {
        aloge("fatal error! Pdet DE input not support this PixFmt!\n");
    }

    pStreamContext->mDewbInfo.mOutputInfo.ePixFmtTpye = DISP_CSC_TYPE_RGB;

    alogd("three2nd pdet input w[%d] h[%d], chn[%d], thres[%f], de input w[%d] h[%d], de output w [%d] h[%d]",
        pStreamContext->mPdetInfo.pdet_input_w, pStreamContext->mPdetInfo.pdet_input_h,
        pStreamContext->mPdetInfo.pdet_input_c, pStreamContext->mPdetInfo.pdet_conf_thres,
        pStreamContext->mDewbInfo.mInputInfo.mWidth, pStreamContext->mDewbInfo.mInputInfo.mHeight,
        pStreamContext->mDewbInfo.mOutputInfo.mWidth, pStreamContext->mDewbInfo.mOutputInfo.mHeight);

    // motion_pdet three2nd
    pStreamContext->mMotionPdetParam.pDewbInfo = &pStreamContext->mDewbInfo;
    pStreamContext->mMotionPdetParam.pPdetInfo = &pStreamContext->mPdetInfo;

    MotionDetectContext *pMotionDetectContext = &pStreamContext->mMotionPdetParam.mMotionDetectContext;
    pMotionDetectContext->mMdStaCfg.nPicWidth = AWALIGN(pConfig->mThree2ndSrcWidth, 16);
    pMotionDetectContext->mMdStaCfg.nPicStride = AWALIGN(pConfig->mThree2ndSrcWidth, 16);
    pMotionDetectContext->mMdStaCfg.nPicHeight = AWALIGN(pConfig->mThree2ndSrcHeight, 16);
    pMotionDetectContext->mMdDynCfg.nHorRegionNum = pMotionDetectContext->mMdStaCfg.nPicWidth / 16;
    pMotionDetectContext->mMdDynCfg.nVerRegionNum = pMotionDetectContext->mMdStaCfg.nPicHeight / 16;
    pMotionDetectContext->mMdDynCfg.bDefaultCfgDis  = 1;
    pMotionDetectContext->mMdDynCfg.bMorphologEn    = 0;
    pMotionDetectContext->mMdDynCfg.nLargeMadTh     = 10;
    pMotionDetectContext->mMdDynCfg.nBackgroundWeight  = 0;
    pMotionDetectContext->mMdDynCfg.fLargeMadRatioTh  = 10.0;

    pStreamContext->mMotionPdetParam.mMotionDetectContext.mJudgeMotionNum = pConfig->mJudgeMotionNum;
    pStreamContext->mMotionPdetParam.mMotionDetectContext.mMotionDynamicThres = pConfig->mMotionDynamicThres;
    pStreamContext->mMotionPdetParam.mMotionDetectContext.mMotionPdetThres = pConfig->mMotionPdetThres;

    alogd("motion_pdet param three2nd pdet input w[%d] h[%d], chn[%d], thres[%f], de input w[%d] h[%d], de output w [%d] h[%d]",
        pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_w, pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_h,
        pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_c, pStreamContext->mMotionPdetParam.pPdetInfo->pdet_conf_thres,
        pStreamContext->mMotionPdetParam.pDewbInfo->mInputInfo.mWidth, pStreamContext->mMotionPdetParam.pDewbInfo->mInputInfo.mHeight,
        pStreamContext->mMotionPdetParam.pDewbInfo->mOutputInfo.mWidth, pStreamContext->mMotionPdetParam.pDewbInfo->mOutputInfo.mHeight);

    MOTION_DETECT_STATIC_CFG_S *pMdStaCfg = &pStreamContext->mMotionPdetParam.mMotionDetectContext.mMdStaCfg;
    MOTION_DETECT_DYNAMIC_CFG_S *pMdDynCfg = &pStreamContext->mMotionPdetParam.mMotionDetectContext.mMdDynCfg;

    alogd("motion_pdet static cfg {nPicHeight:%d, nPicWidth:%d, nPicStride:%d}", pMdStaCfg->nPicHeight, pMdStaCfg->nPicWidth, pMdStaCfg->nPicStride);
    alogd("motion_pdet dynamic cfg {%d %d}", pMdDynCfg->nHorRegionNum, pMdDynCfg->nVerRegionNum);
#endif
}

static void configFourStream(VencStreamContext *pStreamContext, SampleSmartIPCDemoConfig *pConfig)
{
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;
    if (!pStreamContext || !pConfig)
    {
        aloge("fatal error! invalid input params! %p,%p", pStreamContext, pConfig);
    }

    pStreamContext->mIsp = pConfig->mFourIsp;
    pStreamContext->mIspAlgoFreq = pConfig->mFourIspAlgoFreq;
    pStreamContext->mVipp = pConfig->mFourVipp;
    pStreamContext->mViChn = pConfig->mFourViChn;
    pStreamContext->mVEncChn = pConfig->mFourVEncChn;

    if (pConfig->mFourOnlineEnable)
    {
        pStreamContext->mViAttr.mOnlineEnable = 1;
        pStreamContext->mViAttr.mOnlineShareBufNum = pConfig->mFourOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable = 1;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum = pConfig->mFourOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable = pConfig->mMultiSensorOnlineEnable;
    }
    alogd("four vipp%d ve_online_en:%d, dma_buf_num:%d, venc ch%d OnlineEnable:%d, OnlineShareBufNum:%d",
        pStreamContext->mVipp, pStreamContext->mViAttr.mOnlineEnable, pStreamContext->mViAttr.mOnlineShareBufNum,
        pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable, pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum);

    pStreamContext->mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pStreamContext->mViAttr.memtype = V4L2_MEMORY_MMAP;
    pStreamContext->mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pConfig->mFourPixelFormat);
    pStreamContext->mViAttr.format.field = V4L2_FIELD_NONE;
    if (pConfig->mIspColorSpace){
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709_PART_RANGE;
    }
    else
    {
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709;
    }
    pStreamContext->mViAttr.format.width = pConfig->mFourSrcWidth;
    pStreamContext->mViAttr.format.height = pConfig->mFourSrcHeight;
    pStreamContext->mViAttr.fps = pConfig->mFourSrcFrameRate;
    pStreamContext->mViAttr.use_current_win = 0;
    pStreamContext->mViAttr.nbufs = pConfig->mFourViBufNum;
    pStreamContext->mViAttr.nplanes = 2;
    pStreamContext->mViAttr.wdr_mode = pConfig->mFourWdrEnable;
    pStreamContext->mViAttr.drop_frame_num = 0;
    if (-1 != pConfig->mFourIspTdmRawProcessType)
    {
        pStreamContext->mViAttr.tdm_rxbuf_cnt = pConfig->mFourIspTdmRawRxBufNum;
    }
    pStreamContext->mViAttr.tdm_speed_down_en = pConfig->mTdmSpeedDownEnable;
    alogd("four vipp%d tdm_speed_down_en:%d", pStreamContext->mVipp, pStreamContext->mViAttr.tdm_speed_down_en);

    pStreamContext->mVEncChnAttr.VeAttr.mVippID = pConfig->mFourVipp;
    pStreamContext->mVEncChnAttr.VeAttr.Type = pConfig->mFourEncodeType;
    pStreamContext->mVEncChnAttr.VeAttr.MaxKeyInterval = pConfig->mFourKeyFrameInterval;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicWidth = pConfig->mFourSrcWidth;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicHeight = pConfig->mFourSrcHeight;
    pStreamContext->mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pStreamContext->mVEncChnAttr.VeAttr.PixelFormat = pConfig->mFourPixelFormat;
    pStreamContext->mVEncChnAttr.VeAttr.mColorSpace = pStreamContext->mViAttr.format.colorspace;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRefFrameLbcMode = pConfig->mFourVeRefFrameLbcMode;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRecRefBufReduceEnable = pConfig->mVeRecRefBufReduceEnable;

    pStreamContext->mVEncChnAttr.VeAttr.mVbrOptEnable = pConfig->mVbrOptEnable;

    pStreamContext->mVEncChnAttr.RcAttr.mProductMode = pConfig->mProductMode;
    //pStreamContext->mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

    pStreamContext->mEncppSharpAttenCoefPer = 100;
    alogd("four EncppSharpAttenCoefPer: %d%%", pStreamContext->mEncppSharpAttenCoefPer);
    pStreamContext->mIspAndVeLinkageEnable = pConfig->mIspAndVeLinkageEnable;
    pStreamContext->mCameraAdaptiveMovingAndStaticEnable = pConfig->mCameraAdaptiveMovingAndStaticEnable;

    pStreamContext->mVEncChnAttr.EncppAttr.eEncppSharpSetting = pConfig->mFourEncppEnable?VencEncppSharp_FollowISPConfig:VencEncppSharp_Disable;

    if (pConfig->mFourSrcFrameRate)
    {
        vbvThreshSize = pConfig->mFourEncodeBitrate/8/pConfig->mFourSrcFrameRate*15;
    }
    vbvBufSize = pConfig->mFourEncodeBitrate/8*VE_VBV_CACHE_TIME + vbvThreshSize;
    pStreamContext->mVbvBufSize = vbvBufSize;
    alogd("four vbvThreshSize: %d, vbvBufSize: %d", vbvThreshSize, vbvBufSize);

    VencRateCtrlConfig stRcConfig;
    memset(&stRcConfig, 0, sizeof(VencRateCtrlConfig));
    stRcConfig.mEncodeType = pConfig->mFourEncodeType;
    stRcConfig.mEncodeWidth = pConfig->mFourEncodeWidth;
    stRcConfig.mEncodeHeight = pConfig->mFourEncodeHeight;
    stRcConfig.mSrcFrameRate = pConfig->mFourSrcFrameRate;
    stRcConfig.mDstFrameRate = pConfig->mFourEncodeFrameRate;
    stRcConfig.mEncodeBitrate = pConfig->mFourEncodeBitrate;
    stRcConfig.mRcMode = getRcModeFromConfig(pConfig->mRcMode, pConfig->mFourEncodeType);
    stRcConfig.mMaxIQp = pConfig->mMaxIQp;
    stRcConfig.mMinIQp = pConfig->mMinIQp;
    stRcConfig.mMaxPQp = pConfig->mMaxPQp;
    stRcConfig.mMinPQp = pConfig->mMinPQp;
    stRcConfig.mInitQp = pConfig->mInitQp;
    stRcConfig.mEnMbQpLimit = pConfig->mEnMbQpLimit;
    stRcConfig.mQuality = pConfig->mQuality;
    stRcConfig.mMovingTh = pConfig->mMovingTh;
    stRcConfig.mIBitsCoef = pConfig->mIBitsCoef;
    stRcConfig.mPBitsCoef = pConfig->mPBitsCoef;
    stRcConfig.mVbvBufSize = vbvBufSize;
    stRcConfig.mVbvThreshSize = vbvThreshSize;
    configVencRateCtrlParam(&stRcConfig, &pStreamContext->mVEncChnAttr, &pStreamContext->mVEncRcParam);

    pStreamContext->mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pStreamContext->mVEncChnAttr.GopAttr.mGopSize = 2;

    pStreamContext->mVEncFrameRateConfig.SrcFrmRate = pConfig->mFourSrcFrameRate;
    pStreamContext->mVEncFrameRateConfig.DstFrmRate = pConfig->mFourEncodeFrameRate;

#ifdef ENABLE_VENC_ADVANCED_PARAM
    if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type || PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        configIFrmMbRcMoveStatus(&pStreamContext->mVEncRcParam);
        configBitsClipParam(&pStreamContext->mVEncRcParam);
    }
#endif
}

static void configFour2ndStream(VencStreamContext *pStreamContext, SampleSmartIPCDemoConfig *pConfig)
{
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;
    if (!pStreamContext || !pConfig)
    {
        aloge("fatal error! invalid input params! %p,%p", pStreamContext, pConfig);
    }

    pStreamContext->mIsp = pConfig->mFourIsp;
    pStreamContext->mIspAlgoFreq = pConfig->mFourIspAlgoFreq;
    pStreamContext->mVipp = pConfig->mFour2ndVipp;
    pStreamContext->mViChn = pConfig->mFour2ndViChn;
    pStreamContext->mVEncChn = pConfig->mFour2ndVEncChn;

    if (pConfig->mFour2ndOnlineEnable)
    {
        pStreamContext->mViAttr.mOnlineEnable = 1;
        pStreamContext->mViAttr.mOnlineShareBufNum = pConfig->mFour2ndOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable = 1;
        pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum = pConfig->mFour2ndOnlineShareBufNum;
        pStreamContext->mVEncChnAttr.VeAttr.mMultiSensorOnlineEnable = pConfig->mMultiSensorOnlineEnable;
    }
    alogd("four2nd vipp%d ve_online_en:%d, dma_buf_num:%d, venc ch%d OnlineEnable:%d, OnlineShareBufNum:%d",
        pStreamContext->mVipp, pStreamContext->mViAttr.mOnlineEnable, pStreamContext->mViAttr.mOnlineShareBufNum,
        pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.VeAttr.mOnlineEnable, pStreamContext->mVEncChnAttr.VeAttr.mOnlineShareBufNum);

    pStreamContext->mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pStreamContext->mViAttr.memtype = V4L2_MEMORY_MMAP;
    pStreamContext->mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pConfig->mFour2ndPixelFormat);
    pStreamContext->mViAttr.format.field = V4L2_FIELD_NONE;
    if (pConfig->mIspColorSpace){
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709_PART_RANGE;
    }
    else
    {
        pStreamContext->mViAttr.format.colorspace = V4L2_COLORSPACE_REC709;
    }
    pStreamContext->mViAttr.format.width = pConfig->mFour2ndSrcWidth;
    pStreamContext->mViAttr.format.height = pConfig->mFour2ndSrcHeight;
    pStreamContext->mViAttr.fps = pConfig->mFour2ndSrcFrameRate;
    pStreamContext->mViAttr.use_current_win = 1;
    pStreamContext->mViAttr.nbufs = pConfig->mFour2ndViBufNum;
    pStreamContext->mViAttr.nplanes = 2;
    pStreamContext->mViAttr.wdr_mode = 0;
    pStreamContext->mViAttr.drop_frame_num = 0;
    pStreamContext->mViAttr.tdm_speed_down_en = pConfig->mTdmSpeedDownEnable;
    alogd("four2nd vipp%d tdm_speed_down_en:%d", pStreamContext->mVipp, pStreamContext->mViAttr.tdm_speed_down_en);

    pStreamContext->mVEncChnAttr.VeAttr.mVippID = pConfig->mFour2ndVipp;
    pStreamContext->mVEncChnAttr.VeAttr.Type = pConfig->mFour2ndEncodeType;
    pStreamContext->mVEncChnAttr.VeAttr.MaxKeyInterval = pConfig->mFour2ndKeyFrameInterval;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicWidth = pConfig->mFour2ndSrcWidth;
    pStreamContext->mVEncChnAttr.VeAttr.SrcPicHeight = pConfig->mFour2ndSrcHeight;
    pStreamContext->mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pStreamContext->mVEncChnAttr.VeAttr.PixelFormat = pConfig->mFour2ndPixelFormat;
    pStreamContext->mVEncChnAttr.VeAttr.mColorSpace = pStreamContext->mViAttr.format.colorspace;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRefFrameLbcMode = pConfig->mFour2ndVeRefFrameLbcMode;;
    pStreamContext->mVEncChnAttr.VeAttr.mVeRecRefBufReduceEnable = pConfig->mVeRecRefBufReduceEnable;

    pStreamContext->mVEncChnAttr.VeAttr.mVbrOptEnable = pConfig->mVbrOptEnable;

    pStreamContext->mVEncChnAttr.RcAttr.mProductMode = pConfig->mProductMode;
    //pStreamContext->mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

    pStreamContext->mEncppSharpAttenCoefPer = pConfig->mFour2ndEncppSharpAttenCoefPer;
    alogd("four2nd EncppSharpAttenCoefPer: %d%%", pStreamContext->mEncppSharpAttenCoefPer);
    pStreamContext->mIspAndVeLinkageEnable = pConfig->mIspAndVeLinkageEnable;
    pStreamContext->mCameraAdaptiveMovingAndStaticEnable = pConfig->mCameraAdaptiveMovingAndStaticEnable;

    pStreamContext->mVEncChnAttr.EncppAttr.eEncppSharpSetting = pConfig->mFour2ndEncppEnable?VencEncppSharp_FollowISPConfig:VencEncppSharp_Disable;

    if (pConfig->mFour2ndSrcFrameRate)
    {
        vbvThreshSize = pConfig->mFour2ndEncodeBitrate/8/pConfig->mFour2ndSrcFrameRate*15;
    }
    vbvBufSize = pConfig->mFour2ndEncodeBitrate/8*VE_VBV_CACHE_TIME + vbvThreshSize;
    pStreamContext->mVbvBufSize = vbvBufSize;
    alogd("four2nd vbvThreshSize: %d, vbvBufSize: %d", vbvThreshSize, vbvBufSize);

    VencRateCtrlConfig stRcConfig;
    memset(&stRcConfig, 0, sizeof(VencRateCtrlConfig));
    stRcConfig.mEncodeType = pConfig->mFour2ndEncodeType;
    stRcConfig.mEncodeWidth = pConfig->mFour2ndEncodeWidth;
    stRcConfig.mEncodeHeight = pConfig->mFour2ndEncodeHeight;
    stRcConfig.mSrcFrameRate = pConfig->mFour2ndSrcFrameRate;
    stRcConfig.mDstFrameRate = pConfig->mFour2ndEncodeFrameRate;
    stRcConfig.mEncodeBitrate = pConfig->mFour2ndEncodeBitrate;
    stRcConfig.mRcMode = getRcModeFromConfig(pConfig->mRcMode, pConfig->mFour2ndEncodeType);
    stRcConfig.mMaxIQp = pConfig->mMaxIQp;
    stRcConfig.mMinIQp = pConfig->mMinIQp;
    stRcConfig.mMaxPQp = pConfig->mMaxPQp;
    stRcConfig.mMinPQp = pConfig->mMinPQp;
    stRcConfig.mInitQp = pConfig->mInitQp;
    stRcConfig.mEnMbQpLimit = pConfig->mEnMbQpLimit;
    stRcConfig.mQuality = pConfig->mQuality;
    stRcConfig.mMovingTh = pConfig->mMovingTh;
    stRcConfig.mIBitsCoef = pConfig->mIBitsCoef;
    stRcConfig.mPBitsCoef = pConfig->mPBitsCoef;
    stRcConfig.mVbvBufSize = vbvBufSize;
    stRcConfig.mVbvThreshSize = vbvThreshSize;
    configVencRateCtrlParam(&stRcConfig, &pStreamContext->mVEncChnAttr, &pStreamContext->mVEncRcParam);

    pStreamContext->mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pStreamContext->mVEncChnAttr.GopAttr.mGopSize = 2;
    pStreamContext->mVEncFrameRateConfig.SrcFrmRate = pConfig->mFour2ndSrcFrameRate;
    pStreamContext->mVEncFrameRateConfig.DstFrmRate = pConfig->mFour2ndEncodeFrameRate;

#ifdef ENABLE_VENC_ADVANCED_PARAM
    if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type || PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        configIFrmMbRcMoveStatus(&pStreamContext->mVEncRcParam);
        configBitsClipParam(&pStreamContext->mVEncRcParam);
    }
#endif
    pStreamContext->mTakePicture = pConfig->mFour2ndTakePicture;
    pStreamContext->mTakePictureViChn = pConfig->mFour2ndTakePictureViChn;
    pStreamContext->mTakePictureVeChn = pConfig->mFour2ndTakePictureVencChn;
    pStreamContext->mTakePictureInterval = pConfig->mFour2ndTakePictureInterval;
    strncpy(pStreamContext->mTakePictureFile, pConfig->mFour2ndTakePictureFile, MAX_FILE_PATH_SIZE);
    pStreamContext->mTakePictureFileCnt = pConfig->mFour2ndTakePictureFileCnt;
    pStreamContext->mTakePictureOnlyCaptureYuv = pConfig->mFour2ndTakePictureOnlyCaptureYuv;

#ifdef SUPPORT_PERSON_DETECT
    pStreamContext->mPdetEnable = pConfig->mFour2ndPdetEnable;
    pStreamContext->mPdetViChn = pConfig->mFour2ndPdetViChn;
    pStreamContext->mPdetInfo.pdet_input_w = AWALIGN(pConfig->mFour2ndPdetInputWidth, 32);
    pStreamContext->mPdetInfo.pdet_input_h = AWALIGN(pConfig->mFour2ndPdetInputHeight, 32);
    pStreamContext->mPdetInfo.pdet_input_c = 3;
    pStreamContext->mPdetInfo.pdet_conf_thres = pConfig->mFour2ndPdetConfThres;

    if (pStreamContext->mPdetEnable)
    {
        if (pConfig->motionAlarm_on)
        {
            pConfig->motionAlarm_on = 0;
            alogw("four2nd disable VE MotionDetect and use SW MotionDetect when enable pdet !");
        }
        else
        {
            alogw("four2nd VE MotionDetect is disabled, use SW MotionDetect when enable pdet !");
        }
    }

    pStreamContext->mDewbInfo.mInputInfo.mWidth = pConfig->mFour2ndSrcWidth;
    pStreamContext->mDewbInfo.mInputInfo.mHeight = pConfig->mFour2ndSrcHeight;
    pStreamContext->mDewbInfo.mOutputInfo.mWidth = pConfig->mFour2ndPdetInputWidth;
    pStreamContext->mDewbInfo.mOutputInfo.mHeight = pConfig->mFour2ndPdetInputHeight;
    if ((MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420 == pConfig->mFour2ndPixelFormat)
        || (MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pConfig->mFour2ndPixelFormat))
    {
        pStreamContext->mDewbInfo.mInputInfo.ePixFmt = DISP_FORMAT_YUV420_SP_UVUV;
    }
    else
    {
        aloge("fatal error! Pdet DE input not support this PixFmt!\n");
    }

    pStreamContext->mDewbInfo.mOutputInfo.ePixFmtTpye = DISP_CSC_TYPE_RGB;

    alogd("four2nd pdet input w[%d] h[%d], chn[%d], thres[%f], de input w[%d] h[%d], de output w [%d] h[%d]",
        pStreamContext->mPdetInfo.pdet_input_w, pStreamContext->mPdetInfo.pdet_input_h,
        pStreamContext->mPdetInfo.pdet_input_c, pStreamContext->mPdetInfo.pdet_conf_thres,
        pStreamContext->mDewbInfo.mInputInfo.mWidth, pStreamContext->mDewbInfo.mInputInfo.mHeight,
        pStreamContext->mDewbInfo.mOutputInfo.mWidth, pStreamContext->mDewbInfo.mOutputInfo.mHeight);

    // motion_pdet four2nd
    pStreamContext->mMotionPdetParam.pDewbInfo = &pStreamContext->mDewbInfo;
    pStreamContext->mMotionPdetParam.pPdetInfo = &pStreamContext->mPdetInfo;

    MotionDetectContext *pMotionDetectContext = &pStreamContext->mMotionPdetParam.mMotionDetectContext;
    pMotionDetectContext->mMdStaCfg.nPicWidth = AWALIGN(pConfig->mFour2ndSrcWidth, 16);
    pMotionDetectContext->mMdStaCfg.nPicStride = AWALIGN(pConfig->mFour2ndSrcWidth, 16);
    pMotionDetectContext->mMdStaCfg.nPicHeight = AWALIGN(pConfig->mFour2ndSrcHeight, 16);
    pMotionDetectContext->mMdDynCfg.nHorRegionNum = pMotionDetectContext->mMdStaCfg.nPicWidth / 16;
    pMotionDetectContext->mMdDynCfg.nVerRegionNum = pMotionDetectContext->mMdStaCfg.nPicHeight / 16;
    pMotionDetectContext->mMdDynCfg.bDefaultCfgDis  = 1;
    pMotionDetectContext->mMdDynCfg.bMorphologEn    = 0;
    pMotionDetectContext->mMdDynCfg.nLargeMadTh     = 10;
    pMotionDetectContext->mMdDynCfg.nBackgroundWeight  = 0;
    pMotionDetectContext->mMdDynCfg.fLargeMadRatioTh  = 10.0;

    pStreamContext->mMotionPdetParam.mMotionDetectContext.mJudgeMotionNum = pConfig->mJudgeMotionNum;
    pStreamContext->mMotionPdetParam.mMotionDetectContext.mMotionDynamicThres = pConfig->mMotionDynamicThres;
    pStreamContext->mMotionPdetParam.mMotionDetectContext.mMotionPdetThres = pConfig->mMotionPdetThres;

    alogd("motion_pdet param four2nd pdet input w[%d] h[%d], chn[%d], thres[%f], de input w[%d] h[%d], de output w [%d] h[%d]",
        pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_w, pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_h,
        pStreamContext->mMotionPdetParam.pPdetInfo->pdet_input_c, pStreamContext->mMotionPdetParam.pPdetInfo->pdet_conf_thres,
        pStreamContext->mMotionPdetParam.pDewbInfo->mInputInfo.mWidth, pStreamContext->mMotionPdetParam.pDewbInfo->mInputInfo.mHeight,
        pStreamContext->mMotionPdetParam.pDewbInfo->mOutputInfo.mWidth, pStreamContext->mMotionPdetParam.pDewbInfo->mOutputInfo.mHeight);

    MOTION_DETECT_STATIC_CFG_S *pMdStaCfg = &pStreamContext->mMotionPdetParam.mMotionDetectContext.mMdStaCfg;
    MOTION_DETECT_DYNAMIC_CFG_S *pMdDynCfg = &pStreamContext->mMotionPdetParam.mMotionDetectContext.mMdDynCfg;

    alogd("motion_pdet static cfg {nPicHeight:%d, nPicWidth:%d, nPicStride:%d}", pMdStaCfg->nPicHeight, pMdStaCfg->nPicWidth, pMdStaCfg->nPicStride);
    alogd("motion_pdet dynamic cfg {%d %d}", pMdDynCfg->nHorRegionNum, pMdDynCfg->nVerRegionNum);

#endif
}

static void* getWbYuvThread(void *pThreadData)
{
    int result = 0;
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext*)pThreadData;
    VencStreamContext *pStreamContext = NULL;

    if (pContext->mConfigPara.mMainVEncChn == pContext->mConfigPara.mWbYuvStreamChn)
    {
        pStreamContext = &pContext->mMainStream;
    }
    else if (pContext->mConfigPara.mMain2ndVEncChn == pContext->mConfigPara.mWbYuvStreamChn)
    {
        pStreamContext = &pContext->mMain2ndStream;
    }
    else if (pContext->mConfigPara.mSubVEncChn == pContext->mConfigPara.mWbYuvStreamChn)
    {
        pStreamContext = &pContext->mSubStream;
    }
    else if (pContext->mConfigPara.mSub2ndVEncChn == pContext->mConfigPara.mWbYuvStreamChn)
    {
        pStreamContext = &pContext->mSub2ndStream;
    }
    else if (pContext->mConfigPara.mThreeVEncChn == pContext->mConfigPara.mWbYuvStreamChn)
    {
        pStreamContext = &pContext->mThreeStream;
    }
    else if (pContext->mConfigPara.mThree2ndVEncChn == pContext->mConfigPara.mWbYuvStreamChn)
    {
        pStreamContext = &pContext->mThree2ndStream;
    }
    else if (pContext->mConfigPara.mFourVEncChn == pContext->mConfigPara.mWbYuvStreamChn)
    {
        pStreamContext = &pContext->mFourStream;
    }
    else if (pContext->mConfigPara.mFour2ndVEncChn == pContext->mConfigPara.mWbYuvStreamChn)
    {
        pStreamContext = &pContext->mFour2ndStream;
    }
    else
    {
        aloge("fatal error, WbYuvStreamChn:%d is not support for wb yuv!", pContext->mConfigPara.mWbYuvStreamChn);
        return NULL;
    }

    char strThreadName[32];
    sprintf(strThreadName, "venc%d-wbyuv", pStreamContext->mVEncChn);
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    FILE* fp_wb_yuv = fopen(pContext->mConfigPara.mWbYuvFilePath, "wb");
    if (NULL == fp_wb_yuv)
    {
        aloge("fatal error! why open file[%s] fail? errno is %d", pContext->mConfigPara.mWbYuvFilePath, errno);
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

void requestKeyFrameFunc(int nVeChn, int *result)
{
    int ret = AW_MPI_VENC_RequestIDR(nVeChn, TRUE);
    if (ret)
    {
        aloge("fatal error! VeChn[%d] req IDR failed, ret=%d", nVeChn, ret);
    }
    else
    {
        alogd("***** VeChn[%d] req IDR *****", nVeChn);
    }
    *result = ret;
}

#ifdef SUPPORT_PERSON_DETECT
static int configMotionPdet(VencStreamContext *pStreamContext, MotionPdetParam *pMotionPdetParam)
{
    int ret;
    float ratio_w_0 = 0;
    float ratio_h_0 = 0;
    float ratio_w_1 = 0;
    float ratio_h_1 = 0;

    unsigned int sub_x = 0;
    unsigned int sub_y = 0;
    unsigned int sub_Width = 0;
    unsigned int sub_Height = 0;

    SampleDewbInfo *pDewbInfo = &pStreamContext->mDewbInfo;
    SamplePdetInfo *pPdetInfo = &pStreamContext->mPdetInfo;
    PdetOrlInfo *pPdetOrlInfo = &pMotionPdetParam->mPdetOrlInfo;
    PdetOsdInfo *pPdetOsdInfo = &pMotionPdetParam->mPdetOsdInfo;
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext *)pStreamContext->priv;

    if (!pDewbInfo || !pPdetInfo || !pPdetOrlInfo)
    {
        aloge("pDewbInfo, pPdetInfo or pPdetOrlInfo is null");
        return -1;
    }

    if (!pContext)
    {
        aloge("pContext is null");
        return -2;
    }

    VencStreamContext *pMasterStreamContext = NULL;
    VencStreamContext *pSubStreamContext = NULL;
    if (pContext->mConfigPara.mMain2ndVEncChn == pStreamContext->mVEncChn)
    {
        pMasterStreamContext = &pContext->mMainStream;
        pSubStreamContext = &pContext->mMain2ndStream;
    }
    else if (pContext->mConfigPara.mSub2ndVEncChn == pStreamContext->mVEncChn)
    {
        pMasterStreamContext = &pContext->mSubStream;
        pSubStreamContext = &pContext->mSub2ndStream;
    }
    else if (pContext->mConfigPara.mThree2ndVEncChn == pStreamContext->mVEncChn)
    {
        pMasterStreamContext = &pContext->mThreeStream;
        pSubStreamContext = &pContext->mThree2ndStream;
    }
    else if (pContext->mConfigPara.mFour2ndVEncChn == pStreamContext->mVEncChn)
    {
        pMasterStreamContext = &pContext->mFourStream;
        pSubStreamContext = &pContext->mFour2ndStream;
    }
    else
    {
        aloge("fatal error! no support this mVEnChn[%d]", pStreamContext->mVEncChn);
    }

    // orl config
    ratio_w_0 = pSubStreamContext->mViAttr.format.width * 1.0 / pDewbInfo->mOutputInfo.mWidth;
    ratio_h_0 = pSubStreamContext->mViAttr.format.height * 1.0 / pDewbInfo->mOutputInfo.mHeight;
    ratio_w_1 = pMasterStreamContext->mViAttr.format.width * 1.0 / pSubStreamContext->mViAttr.format.width;
    ratio_h_1 = pMasterStreamContext->mViAttr.format.height * 1.0 / pSubStreamContext->mViAttr.format.height;
    pPdetOrlInfo->mOrlWidthRatio = ratio_w_0 * ratio_w_1;
    pPdetOrlInfo->mOrlHeightRatio = ratio_h_0 * ratio_h_1;
    pPdetOrlInfo->mVipp  = pMasterStreamContext->mVipp;
    pPdetOrlInfo->mViChn = pMasterStreamContext->mViChn;

    // osd config
    pPdetOsdInfo->mOsdWidthRatio = ratio_w_0 * ratio_w_1;
    pPdetOsdInfo->mOsdHeightRatio = ratio_h_0 * ratio_h_1;
    pPdetOsdInfo->mVEncChn = pMasterStreamContext->mVEncChn;
    pPdetOsdInfo->mStreamOsdConfig.mOverlayDrawStreamOSDBase = 100;

    pMotionPdetParam->pDewbInfo = pDewbInfo;
    pMotionPdetParam->pPdetInfo = pPdetInfo;

    alogd("mOrlWidthRatio:%f, mOrlHeightRatio:%f, mVipp:%d, mViChn:%d",
           pPdetOrlInfo->mOrlWidthRatio, pPdetOrlInfo->mOrlHeightRatio, pPdetOrlInfo->mVipp, pPdetOrlInfo->mViChn);
    return 0;
}

#ifdef ENABLE_VENC_GET_DIRMVINFO
static int judgeDirMvResult(VencStreamContext *pStreamContext)
{
    int ret = -1;
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext *)pStreamContext->priv;

    alogd("AW_MPI_VENC_GetDirMvInfoResult \n");
    if (pStreamContext->mVEncChn == pContext->mConfigPara.mMain2ndVEncChn)
    {
        alogv("test get dir mv info result, vencChn:%d \n", pStreamContext->mVEncChn);
        int i = 0, j = 0;

        VencDirMvInfoResult *pDirMvInfoResult = &pStreamContext->mDirMvInfoResult;

        memset(pDirMvInfoResult, 0, sizeof(VencDirMvInfoResult));
        ret = AW_MPI_VENC_GetDirMvInfoResult(pStreamContext->mVEncChn, pDirMvInfoResult);
        if (ret == SUCCESS)
        {
            for (i = 0; i < DIR_MV_REGION_NUM; i++)
            {
                alogd("line_abs_mv_sum: [%d, %d]\n", pDirMvInfoResult->line_abs_mv_sum[i].mvX,  pDirMvInfoResult->line_abs_mv_sum[i].mvY);
                alogd("col_abs_mv_sum:  [%d, %d]\n", pDirMvInfoResult->col_abs_mv_sum[i].mvX,   pDirMvInfoResult->col_abs_mv_sum[i].mvY);
                alogd("line_dir_mv_sum: [%d, %d]\n", pDirMvInfoResult->line_dir_mv_sum[i].mvX,  pDirMvInfoResult->line_dir_mv_sum[i].mvY);
                alogd("col_dir_mv_sum:  [%d, %d]\n", pDirMvInfoResult->col_dir_mv_sum[i].mvX,   pDirMvInfoResult->col_dir_mv_sum[i].mvY);
            }
#if 0
            for (i = 0; i < DIR_MV_REGION_NUM; i++)
            {
                for (j = 0; j < DIR_MV_REGION_NUM; j++)
                {
                    alogd("(%4d %4d) ", pDirMvInfoResult->region_dir_mv[i*DIR_MV_REGION_NUM+j].mvX, pDirMvInfoResult->region_dir_mv[i*DIR_MV_REGION_NUM+j].mvY);
                }
                alogd("\n");
            }
#endif
        }
        else
        {
            aloge("AW_MPI_VENC_GetDirMvInfoResult fail\n");
        }
    }

    return ret;
}
#endif

static void *getPdetRunThread(void *pThreadData)
{
    VencStreamContext *pStreamContext = (VencStreamContext *)pThreadData;
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext *)pStreamContext->priv;
    SampleSmartIPCDemoConfig *pConfigPara = &pContext->mConfigPara;

    MotionPdetParam *pMotionPdetParam = &pStreamContext->mMotionPdetParam;
    MotionDetectRectInfo *pMotionLastData = &pMotionPdetParam->mMotionLastData;
    MotionDetectContext *pMotionDetectContext = &pMotionPdetParam->mMotionDetectContext;

    configMotionPdet(pStreamContext, pMotionPdetParam);

    ERRORTYPE ret = 0;

    char strThreadName[32];
    sprintf(strThreadName, "getPdetRun%d-%d", pStreamContext->mVipp, pStreamContext->mPdetViChn);
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    /* config motion_pdet sensitivity */
    motion_pdet_set_sensitivity(pMotionPdetParam, pConfigPara->mMotionPdetSensitivity);

    while (1)
    {
        cdx_sem_down(&pStreamContext->mSemPdetFrameReady);
        if (pContext->mbExitFlag)
        {
            ret = cdx_sem_get_val(&pStreamContext->mSemPdetFrameReady);
            alogd("sem val [%d]", ret);
            if (ret)
                alogv("error! cdx_sem_get_val fail.");
            break;
        }

        pthread_mutex_lock(&pStreamContext->mPdetLock);
        unsigned int tm0 = getSysTickMs();
        motion_pdet_run(pMotionPdetParam, pMotionLastData);
        unsigned int tm1 = getSysTickMs();
        alogv("motion_pdet_run cost time: %dms", tm1-tm0);

        pthread_mutex_unlock(&pStreamContext->mPdetLock);
    }

    return NULL;
}

static void *getPdetCSIThread(void *pThreadData)
{
    VencStreamContext *pStreamContext = (VencStreamContext *)pThreadData;
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext *)pStreamContext->priv;
    SampleSmartIPCDemoConfig *pConfig = &pContext->mConfigPara;
    MotionPdetParam *pMotionPdetParam = &pStreamContext->mMotionPdetParam;
    SampleDewbInfo *pDewbInfo = &pStreamContext->mDewbInfo;
    SamplePdetInfo *pPdetInfo = &pStreamContext->mPdetInfo;
    long long Timeout = 0;
    ERRORTYPE ret = 0;
    unsigned int tm0 = 0, tm1 = 0;
    int frame_cnt = 0;
    int pdet_run_intv = pStreamContext->mViAttr.fps * 2;

    char strThreadName[32];
    sprintf(strThreadName, "getPdetCSI%d-%d", pStreamContext->mVipp, pStreamContext->mPdetViChn);
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    VIDEO_FRAME_INFO_S video_frame;
    memset(&video_frame, 0, sizeof(video_frame));

    if (pStreamContext->mPdetRunInterval > 0)
    {
        pdet_run_intv = pStreamContext->mPdetRunInterval;
    }

    Timeout = (pdet_run_intv * 1000000 / pStreamContext->mViAttr.fps) * 1000;   // / 4 * 1024(1000) get lock wait time, unit: ns
    while (!pContext->mbExitFlag)
    {
        if (pContext->mViresetFlag)
            continue;

        ret = AW_MPI_VI_GetFrame(pStreamContext->mVipp, pStreamContext->mPdetViChn, &video_frame, 2000);
        if (ret != 0)
        {
            aloge("get frame fail ret[%d]", ret);
            continue;
        }

#ifdef ENABLE_VENC_GET_DIRMVINFO
        judgeDirMvResult(pStreamContext);
#endif

        if (0 == frame_cnt % pdet_run_intv)
        {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            ts.tv_nsec += Timeout;
            ret = pthread_mutex_timedlock(&pStreamContext->mPdetLock, &ts);
            if (0 == ret)
            {
                ret = cdx_sem_get_val(&pStreamContext->mSemPdetFrameReady);
                if (0 == ret && !pContext->mbExitFlag)
                {
                    pDewbInfo->mInputInfo.mIonFd = video_frame.VFrame.dma_fd[0];
                    ret = motion_pdet_prepare(pMotionPdetParam, video_frame.VFrame.mpVirAddr[0]);
                    if (ret != 0)
                    {
                        aloge("error! motion_pdet_prepare fail!");
                        AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mPdetViChn, &video_frame);
                        pthread_mutex_unlock(&pStreamContext->mPdetLock);
                        continue;
                    }

                    AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mPdetViChn, &video_frame);

                    pthread_mutex_unlock(&pStreamContext->mPdetLock);
                    cdx_sem_up(&pStreamContext->mSemPdetFrameReady);
                }
                else
                {
                    pthread_mutex_unlock(&pStreamContext->mPdetLock);
                    alogv("Not scheduled in time, the system loading needs to be checked, or application want exit?");
                    AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mPdetViChn, &video_frame);
                }
            }
            else
            {
                alogv("pdet deal last frame now, not attach lock");
                frame_cnt--;
                AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mPdetViChn, &video_frame);
            }
        }
        else
        {
            AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mPdetViChn, &video_frame);
        }
        frame_cnt++;
    }
    return NULL;
}

#endif

#ifdef SUPPORT_TAKE_PICTURE
static int startJpegPicture(VencStreamContext *stream)
{
    //call mpi_venc to encode jpeg.
    int result = SUCCESS;
    ERRORTYPE ret = 0;
    unsigned int nVbvBufSize = 0;
    unsigned int vbvThreshSize = 0;
    unsigned int picWidth = stream->mViAttr.format.width;
    unsigned int picHeight = stream->mViAttr.format.height;
    unsigned int mPictureNum = 1;

    if (stream->mTakePictureOnlyCaptureYuv) {
        alogd("stream takepicture only capture yuv");
        result = SUCCESS;
        goto _out0;
    }

    // calc vbv buf size and vbv threshold size
    unsigned int minVbvBufSize = picWidth * picHeight * 3/2;
    vbvThreshSize = picWidth * picHeight;
    nVbvBufSize = (picWidth * picHeight * 3/2 /10 * mPictureNum) + vbvThreshSize;
    if(nVbvBufSize < minVbvBufSize)
    {
        nVbvBufSize = minVbvBufSize;
    }
    if(nVbvBufSize > 16*1024*1024)
    {
        alogd("Be careful! vbvSize[%d]MB is too large, decrease to threshSize[%d]MB + 1MB", nVbvBufSize/(1024*1024), vbvThreshSize/(1024*1024));
        nVbvBufSize = vbvThreshSize + 1*1024*1024;
    }
    nVbvBufSize = AWALIGN(nVbvBufSize, 1024);

    VENC_CHN_ATTR_S venc_chn_attr;
    memset(&venc_chn_attr, 0, sizeof(VENC_CHN_ATTR_S));
    venc_chn_attr.VeAttr.Type = PT_JPEG;
    venc_chn_attr.VeAttr.mVippID = stream->mVipp;
    venc_chn_attr.VeAttr.AttrJpeg.MaxPicWidth = 0;
    venc_chn_attr.VeAttr.AttrJpeg.MaxPicHeight = 0;
    venc_chn_attr.VeAttr.AttrJpeg.BufSize = nVbvBufSize;
    venc_chn_attr.VeAttr.AttrJpeg.mThreshSize = vbvThreshSize;
    venc_chn_attr.VeAttr.AttrJpeg.bByFrame = TRUE;
    venc_chn_attr.VeAttr.AttrJpeg.PicWidth = picWidth;
    venc_chn_attr.VeAttr.AttrJpeg.PicHeight = picHeight;
    venc_chn_attr.VeAttr.AttrJpeg.bSupportDCF = FALSE;
    venc_chn_attr.VeAttr.MaxKeyInterval = 1;
    venc_chn_attr.VeAttr.SrcPicWidth = picWidth;
    venc_chn_attr.VeAttr.SrcPicHeight = picHeight;
    venc_chn_attr.VeAttr.Field = VIDEO_FIELD_FRAME;
    venc_chn_attr.VeAttr.PixelFormat = map_V4L2_PIX_FMT_to_PIXEL_FORMAT_E(stream->mViAttr.format.pixelformat);
    venc_chn_attr.VeAttr.OutputFormat = VENC_OUTPUT_SAME_AS_INPUT;
    venc_chn_attr.VeAttr.mColorSpace = V4L2_COLORSPACE_JPEG;
    alogd("pixfmt:0x%x, output_format:0x%x, colorSpace:0x%x", venc_chn_attr.VeAttr.PixelFormat, venc_chn_attr.VeAttr.OutputFormat, venc_chn_attr.VeAttr.mColorSpace);

    ret = AW_MPI_VENC_CreateChn(stream->mTakePictureVeChn, &venc_chn_attr);
    if (SUCCESS != ret)
    {
        aloge("fatal error! create venc channel[%d] ret[0x%x]!", stream->mTakePictureVeChn, ret);
        result = FAILURE;
        goto _out0;
    }

    MPPCallbackInfo cbInfo;
    cbInfo.cookie = (void*)stream->priv;
    cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
    AW_MPI_VENC_RegisterCallback(stream->mTakePictureVeChn, &cbInfo);

    VENC_PARAM_JPEG_S jpeg_param;
    memset(&jpeg_param, 0, sizeof(VENC_PARAM_JPEG_S));
    jpeg_param.Qfactor = 90;
    AW_MPI_VENC_SetJpegParam(stream->mTakePictureVeChn, &jpeg_param);

    AW_MPI_VENC_ForbidDiscardingFrame(stream->mTakePictureVeChn, TRUE);

    ret = AW_MPI_VENC_StartRecvPic(stream->mTakePictureVeChn);
    if (SUCCESS != ret)
    {
        aloge("fatal error:%x jpegEnc AW_MPI_VENC_StartRecvPic",ret);
        result = FAILURE;
    }

    VENC_EXIFINFO_S exif_info;
    memset(&exif_info, 0, sizeof(VENC_EXIFINFO_S));
    time_t t;
    struct tm *tm_t;
    time(&t);
    tm_t = localtime(&t);
    snprintf((char *)exif_info.DateTime, MM_DATA_TIME_LENGTH, "%4d:%02d:%02d %02d:%02d:%02d",
        tm_t->tm_year + 1900, tm_t->tm_mon + 1, tm_t->tm_mday,
        tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec);

    exif_info.ThumbWidth = 320;
    exif_info.ThumbHeight = 240;
    exif_info.thumb_quality = 60;
    exif_info.Orientation = 0;
    exif_info.FNumber.num = 26;
    exif_info.FNumber.den = 10;
    exif_info.MeteringMode = METERING_MODE_AVERAGE;
    exif_info.FocalLength.num = 228;
    exif_info.FocalLength.den = 100;
    exif_info.WhiteBalance = 0;
    exif_info.FocalLengthIn35mmFilm = 18;
    strcpy((char*)exif_info.ImageName, "aw-photo");
    AW_MPI_VENC_SetJpegExifInfo(stream->mTakePictureVeChn, &exif_info);

    alogd("VeChn[%d] start jpeg take picture", stream->mTakePictureVeChn);

_out0:
    return result;
}

static int stopJpegPicture(VencStreamContext *stream)
{
    int result = SUCCESS;

    if (stream->mTakePictureOnlyCaptureYuv)
        return result;

    AW_MPI_VENC_StopRecvPic(stream->mTakePictureVeChn);
    //can't reset channel now, because in non-tunnel mode, mpi_venc will force release out frames, but we don't want
    //those out frames to be released before we return them.
    //AW_MPI_VENC_ResetChn(mChn);
    AW_MPI_VENC_DestroyChn(stream->mTakePictureVeChn);

    alogd("VeChn[%d] stop jpeg take picture", stream->mTakePictureVeChn);

    return result;
}

#ifdef SUPPORT_RECREATE_JPEG
static void *takePictureThread(void *pThreadData)
{
    VencStreamContext *pStreamContext = (VencStreamContext*)pThreadData;
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext*)pStreamContext->priv;
    ERRORTYPE ret = 0;
    unsigned int cur_time = 0;
    char strThreadName[32];
    sprintf(strThreadName, "takepic-vi%d-%d-ve%d", pStreamContext->mVipp, pStreamContext->mTakePictureViChn, pStreamContext->mTakePictureVeChn);
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    VIDEO_FRAME_INFO_S video_frame;
    memset(&video_frame, 0, sizeof(video_frame));
    cur_time = getSysTickMs();
    while (!pContext->mbExitFlag)
    {
        if (pContext->mViresetFlag)
            continue;
        if (getSysTickMs() >= cur_time + pStreamContext->mTakePictureInterval * 1000)
        {
            cur_time = getSysTickMs();

            ViVirChnAttrS mVirviChnAttr;
            memset(&mVirviChnAttr, 0, sizeof(ViVirChnAttrS));
            mVirviChnAttr.mCacheFrameNum = 1;
            AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, &mVirviChnAttr);
            AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);

            if (SUCCESS != startJpegPicture(pStreamContext))
            {
                aloge("fatal error! VeChn%d startJpegPicture fail!", pStreamContext->mTakePictureVeChn);
                goto err0;
            }
retry:
            ret = AW_MPI_VI_GetFrame(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, &video_frame, 2000);
            if (ret != 0)
            {
                if (!pContext->mbExitFlag)
                    goto retry;
                goto err1;
            }

            if (pStreamContext->mTakePictureOnlyCaptureYuv)
            {
                char strFilePath[MAX_FILE_PATH_SIZE];
                if (pStreamContext->mTakePictureFileCurCnt >= pStreamContext->mTakePictureFileCnt)
                {
                    pStreamContext->mTakePictureFileCurCnt = 0;
                }
                snprintf(strFilePath, MAX_FILE_PATH_SIZE, "%s_%d_%dx%d.yuv",
                    pStreamContext->mTakePictureFile, pStreamContext->mTakePictureFileCurCnt,
                    video_frame.VFrame.mWidth, video_frame.VFrame.mHeight);
                FILE *fpPic = fopen(strFilePath, "wb");
                if (fpPic)
                {
                    if (video_frame.VFrame.mpVirAddr[0])
                        fwrite(video_frame.VFrame.mpVirAddr[0], 1, video_frame.VFrame.mWidth*video_frame.VFrame.mHeight, fpPic);
                    if (video_frame.VFrame.mpVirAddr[1])
                        fwrite(video_frame.VFrame.mpVirAddr[1], 1, video_frame.VFrame.mWidth*video_frame.VFrame.mHeight / 2, fpPic);
                    fclose(fpPic);
                    pStreamContext->mTakePictureFileCurCnt++;
                    alogd("vipp%d ch%d capture yuv %s size %dx%d format 0x%x", pStreamContext->mVipp, pStreamContext->mTakePictureViChn,
                        strFilePath, video_frame.VFrame.mWidth, video_frame.VFrame.mHeight, video_frame.VFrame.mPixelFormat);
                }
                else
                {
                    alogw("fatal error! vipp%d ch%d open file %s fail! errno(%d)", pStreamContext->mVipp, pStreamContext->mTakePictureViChn, strFilePath, errno);
                }
                AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, &video_frame);
            }
            else
            {
                ret = AW_MPI_VENC_SendFrameSync(pStreamContext->mTakePictureVeChn, &video_frame, 10 * 1000);
                if (ret != 0)
                {
                    aloge("fatal error! VeChn%d send frame sync failed! ret[0x%x]", pStreamContext->mTakePictureVeChn, ret);
                    AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, &video_frame);
                    break;
                }
                alogd("VeChn%d send frame sync done", pStreamContext->mTakePictureVeChn);
                AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, &video_frame);

                VENC_STREAM_S venc_stream;
                VENC_PACK_S pack;
                memset(&venc_stream, 0, sizeof(venc_stream));
                memset(&pack, 0, sizeof(pack));
                venc_stream.mPackCount = 1;
                venc_stream.mpPack = &pack;

                ret = AW_MPI_VENC_GetStream(pStreamContext->mTakePictureVeChn, &venc_stream, 1000);
                if (ret != SUCCESS)
                {
                    aloge("fatal error! VeChn%d why get stream fail?", pStreamContext->mTakePictureVeChn);
                }
                else
                {
                    char strFilePath[MAX_FILE_PATH_SIZE];
                    if (pStreamContext->mTakePictureFileCurCnt >= pStreamContext->mTakePictureFileCnt)
                        pStreamContext->mTakePictureFileCurCnt = 0;
                    snprintf(strFilePath, MAX_FILE_PATH_SIZE, "%s_%d.jpg", pStreamContext->mTakePictureFile, pStreamContext->mTakePictureFileCurCnt);
                    FILE *fpPic = fopen(strFilePath, "wb");
                    if(fpPic != NULL)
                    {
                        if(venc_stream.mpPack[0].mpAddr0 != NULL && venc_stream.mpPack[0].mLen0 > 0)
                            fwrite(venc_stream.mpPack[0].mpAddr0, 1, venc_stream.mpPack[0].mLen0, fpPic);
                        if(venc_stream.mpPack[0].mpAddr1 != NULL && venc_stream.mpPack[0].mLen1 > 0)
                            fwrite(venc_stream.mpPack[0].mpAddr1, 1, venc_stream.mpPack[0].mLen1, fpPic);
                        if(venc_stream.mpPack[0].mpAddr2 != NULL && venc_stream.mpPack[0].mLen2 > 0)
                            fwrite(venc_stream.mpPack[0].mpAddr2, 1, venc_stream.mpPack[0].mLen2, fpPic);
                        pStreamContext->mTakePictureFileCurCnt++;
                        fclose(fpPic);
                        alogd("VeChn%d store jpeg in file[%s]", pStreamContext->mTakePictureVeChn, strFilePath);
                    }
                    else
                    {
                        aloge("fatal error! VeChn%d open file[%s] fail! errno(%d)", pStreamContext->mTakePictureVeChn, strFilePath, errno);
                    }
                }

                ret = AW_MPI_VENC_ReleaseStream(pStreamContext->mTakePictureVeChn, &venc_stream);
                if (ret != SUCCESS)
                {
                    aloge("fatal error! VeChn%d why release stream fail(0x%x)?", pStreamContext->mTakePictureVeChn, ret);
                }
            }
err1:
            stopJpegPicture(pStreamContext);
err0:
            AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
            AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
        }

        usleep(20000);
    }

    return NULL;
}

#else
static void *takePictureThread(void *pThreadData)
{
    VencStreamContext *pStreamContext = (VencStreamContext*)pThreadData;
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext*)pStreamContext->priv;
    ERRORTYPE ret = 0;
    unsigned int cur_time = 0;
    char strThreadName[32];
    sprintf(strThreadName, "takepic-vi%d-%d-ve%d", pStreamContext->mVipp, pStreamContext->mTakePictureViChn, pStreamContext->mTakePictureVeChn);
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    if (SUCCESS != startJpegPicture(pStreamContext))
    {
        aloge("fatal error! VeChn%d startJpegPicture fail!", pStreamContext->mTakePictureVeChn);
        return NULL;
    }

    VIDEO_FRAME_INFO_S video_frame;
    memset(&video_frame, 0, sizeof(video_frame));

    cur_time = getSysTickMs();

    while (!pContext->mbExitFlag)
    {
        if (pContext->mViresetFlag)
            continue;
        ret = AW_MPI_VI_GetFrame(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, &video_frame, 2000);
        if (ret != 0)
        {
            continue;
        }
        if (getSysTickMs() >= cur_time + pStreamContext->mTakePictureInterval * 1000)
        {
            cur_time = getSysTickMs();
            if (pStreamContext->mTakePictureOnlyCaptureYuv)
            {
                char strFilePath[MAX_FILE_PATH_SIZE];
                if (pStreamContext->mTakePictureFileCurCnt >= pStreamContext->mTakePictureFileCnt)
                {
                    pStreamContext->mTakePictureFileCurCnt = 0;
                }
                snprintf(strFilePath, MAX_FILE_PATH_SIZE, "%s_%d_%dx%d.yuv",
                    pStreamContext->mTakePictureFile, pStreamContext->mTakePictureFileCurCnt,
                    video_frame.VFrame.mWidth, video_frame.VFrame.mHeight);
                FILE *fpPic = fopen(strFilePath, "wb");
                if (fpPic)
                {
                    if (video_frame.VFrame.mpVirAddr[0])
                        fwrite(video_frame.VFrame.mpVirAddr[0], 1, video_frame.VFrame.mWidth*video_frame.VFrame.mHeight, fpPic);
                    if (video_frame.VFrame.mpVirAddr[1])
                        fwrite(video_frame.VFrame.mpVirAddr[1], 1, video_frame.VFrame.mWidth*video_frame.VFrame.mHeight / 2, fpPic);
                    fclose(fpPic);
                    pStreamContext->mTakePictureFileCurCnt++;
                    alogd("vipp%d ch%d capture yuv %s size %dx%d format 0x%x", pStreamContext->mVipp, pStreamContext->mTakePictureViChn,
                        strFilePath, video_frame.VFrame.mWidth, video_frame.VFrame.mHeight, video_frame.VFrame.mPixelFormat);
                }
                else
                {
                    alogw("fatal error! vipp%d ch%d open file %s fail! errno(%d)", pStreamContext->mVipp, pStreamContext->mTakePictureViChn, strFilePath, errno);
                }
                AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, &video_frame);
            }
            else
            {
                ret = AW_MPI_VENC_SendFrameSync(pStreamContext->mTakePictureVeChn, &video_frame, 10 * 1000);
                if (ret != 0)
                {
                    aloge("fatal error! VeChn%d send frame sync failed! ret[0x%x]", pStreamContext->mTakePictureVeChn, ret);
                    AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, &video_frame);
                    break;
                }
                alogd("VeChn%d send frame sync done", pStreamContext->mTakePictureVeChn);
                AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, &video_frame);

                VENC_STREAM_S venc_stream;
                VENC_PACK_S pack;
                memset(&venc_stream, 0, sizeof(venc_stream));
                memset(&pack, 0, sizeof(pack));
                venc_stream.mPackCount = 1;
                venc_stream.mpPack = &pack;

                ret = AW_MPI_VENC_GetStream(pStreamContext->mTakePictureVeChn, &venc_stream, 1000);
                if (ret != SUCCESS)
                {
                    aloge("fatal error! VeChn%d why get stream fail?", pStreamContext->mTakePictureVeChn);
                }
                else
                {
                    char strFilePath[MAX_FILE_PATH_SIZE];
                    if (pStreamContext->mTakePictureFileCurCnt >= pStreamContext->mTakePictureFileCnt)
                        pStreamContext->mTakePictureFileCurCnt = 0;
                    snprintf(strFilePath, MAX_FILE_PATH_SIZE, "%s_%d.jpg", pStreamContext->mTakePictureFile, pStreamContext->mTakePictureFileCurCnt);
                    FILE *fpPic = fopen(strFilePath, "wb");
                    if(fpPic != NULL)
                    {
                        if(venc_stream.mpPack[0].mpAddr0 != NULL && venc_stream.mpPack[0].mLen0 > 0)
                            fwrite(venc_stream.mpPack[0].mpAddr0, 1, venc_stream.mpPack[0].mLen0, fpPic);
                        if(venc_stream.mpPack[0].mpAddr1 != NULL && venc_stream.mpPack[0].mLen1 > 0)
                            fwrite(venc_stream.mpPack[0].mpAddr1, 1, venc_stream.mpPack[0].mLen1, fpPic);
                        if(venc_stream.mpPack[0].mpAddr2 != NULL && venc_stream.mpPack[0].mLen2 > 0)
                            fwrite(venc_stream.mpPack[0].mpAddr2, 1, venc_stream.mpPack[0].mLen2, fpPic);
                        pStreamContext->mTakePictureFileCurCnt++;
                        fclose(fpPic);
                        alogd("VeChn%d store jpeg in file[%s]", pStreamContext->mTakePictureVeChn, strFilePath);
                    }
                    else
                    {
                        aloge("fatal error! VeChn%d open file[%s] fail! errno(%d)", pStreamContext->mTakePictureVeChn, strFilePath, errno);
                    }
                }

                ret = AW_MPI_VENC_ReleaseStream(pStreamContext->mTakePictureVeChn, &venc_stream);
                if (ret != SUCCESS)
                {
                    aloge("fatal error! VeChn%d why release stream fail(0x%x)?", pStreamContext->mTakePictureVeChn, ret);
                }
            }
        }
        else
        {
            AW_MPI_VI_ReleaseFrame(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, &video_frame);
        }
    }
    stopJpegPicture(pStreamContext);

    return NULL;
}
#endif
#endif

static int vencRecreate(VencStreamContext *pStreamContext, VencRecreateDynamicConfig *pNewDynamicConfig)
{
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext *)pStreamContext->priv;
    ERRORTYPE ret = 0;
    if (NULL == pStreamContext)
    {
        aloge("fatal error, pStreamContext is NULL!");
        return -1;
    }

    alogd("venc_recreate process VencChn[%d] enter", pStreamContext->mVEncChn);

    //(1)check if need recreate vencLib!
    // old config
    unsigned int nOldBitRate = GetBitRateFromVENC_CHN_ATTR_S(&pStreamContext->mVEncChnAttr);
    int nOldDstFrameRate = pStreamContext->mVEncFrameRateConfig.DstFrmRate;
    SIZE_S stOldDstEncodeSize = {0};
    ret = GetEncodeDstSizeFromVENC_CHN_ATTR_S(&pStreamContext->mVEncChnAttr, &stOldDstEncodeSize);
    if (ret != SUCCESS)
    {
        aloge("fatal error! get encode dst size fail! check code!");
    }
    int nOldKeyInterval = pStreamContext->mVEncChnAttr.VeAttr.MaxKeyInterval;
    PAYLOAD_TYPE_E nOldEncodeType = pStreamContext->mVEncChnAttr.VeAttr.Type;

    // new config
    unsigned int nNewBitRate = pNewDynamicConfig->mVideoBitRate;
    int nNewDstFrameRate = pNewDynamicConfig->mVideoFrameRate;
    SIZE_S stNewDstEncodeSize = {pNewDynamicConfig->dstWidth, pNewDynamicConfig->dstHeight};
    int nNewKeyInterval = pNewDynamicConfig->mKeyFrameInterval;
    PAYLOAD_TYPE_E nNewEncodeType = pNewDynamicConfig->mVideoEncoderFmt;

    BOOL bNeedRecreate = FALSE;
    BOOL bNeedResetVipp = FALSE;
    do
    {
        if (nOldBitRate != nNewBitRate)
        {
            alogd("veChn[%d] bitRate change:[%d]->[%d]", pStreamContext->mVEncChn, nOldBitRate, nNewBitRate);
            bNeedRecreate = TRUE;
        }
        if (nOldDstFrameRate != nNewDstFrameRate)
        {
            alogd("veChn[%d] frameRate change:[%d]->[%d]", pStreamContext->mVEncChn, nOldDstFrameRate, nNewDstFrameRate);
            bNeedRecreate = TRUE;
        }
        if (stOldDstEncodeSize.Width != stNewDstEncodeSize.Width || stOldDstEncodeSize.Height != stNewDstEncodeSize.Height)
        {
            alogd("veChn[%d] resolution change:[%dx%d]->[%dx%d]", pStreamContext->mVEncChn,
                stOldDstEncodeSize.Width, stOldDstEncodeSize.Height, stNewDstEncodeSize.Width, stNewDstEncodeSize.Height);
            bNeedRecreate = TRUE;
            bNeedResetVipp = TRUE;
        }
        if (nOldKeyInterval != nNewKeyInterval)
        {
            alogd("veChn[%d] keyFrameInterval change:[%d]->[%d]", pStreamContext->mVEncChn, nOldKeyInterval, nNewKeyInterval);
            bNeedRecreate = TRUE;
        }
        if (nOldEncodeType!= nNewEncodeType)
        {
            alogd("veChn[%d] EncodeType change:[%d]->[%d]", pStreamContext->mVEncChn, nOldEncodeType, nNewEncodeType);
            bNeedRecreate = TRUE;
        }
    } while(0);

    //(2)recreate vencLib
    if (bNeedResetVipp)
    {
        //a) destroy vipp
        ret = AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if (ret != SUCCESS)
        {
            aloge("fatal error! vipp[%d] disable virChn[%d] fail[0x%x]!", pStreamContext->mVipp, pStreamContext->mViChn, ret);
        }
        ret = AW_MPI_VENC_StopRecvPic(pStreamContext->mVEncChn);
        if (ret != SUCCESS)
        {
            aloge("fatal error! vencChn[%d] stop fail[0x%x]!", pStreamContext->mVEncChn, ret);
        }
        //must return all inputFrames before unbind.
        ret = AW_MPI_VENC_ResetChn(pStreamContext->mVEncChn);
        if (ret != SUCCESS)
        {
            aloge("fatal error! vencChn[%d] ret fail[0x%x]!", pStreamContext->mVEncChn, ret);
        }
        //unbind virViChn and vencChn
        MPP_CHN_S stVirViChn = {MOD_ID_VIU, pStreamContext->mVipp, pStreamContext->mViChn};
        MPP_CHN_S stVencChn = {MOD_ID_VENC, 0, pStreamContext->mVEncChn};
        ret = AW_MPI_SYS_UnBind(&stVirViChn, &stVencChn);
        if (ret != SUCCESS)
        {
            aloge("fatal error! virViChn[%d-%d] && vencChn[%d] unbind fail[0x%x]!",
                pStreamContext->mVipp, pStreamContext->mViChn, pStreamContext->mVEncChn, ret);
        }
        ret = AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if (ret != SUCCESS)
        {
            aloge("fatal error! vipp[%d] destroy virViChn[%d] fail[0x%x]!", pStreamContext->mVipp, pStreamContext->mViChn, ret);
        }
        ret = AW_MPI_VI_DisableVipp(pStreamContext->mVipp);
        if (ret != SUCCESS)
        {
            aloge("fatal error! disableVipp[%d] fail!", pStreamContext->mVipp);
        }
#if ISP_RUN
        AW_MPI_ISP_Stop(pStreamContext->mIsp);
#endif
        ret = AW_MPI_VI_DestroyVipp(pStreamContext->mVipp);
        if (ret != SUCCESS)
        {
            aloge("fatal error! AW_MPI_VI DestroyVipp[%d] failed", pStreamContext->mVipp);
        }

        //b) create vipp
        //update viAttr's resolution
        pStreamContext->mViAttr.format.width = stNewDstEncodeSize.Width;
        pStreamContext->mViAttr.format.height = stNewDstEncodeSize.Height;
        pStreamContext->mViAttr.fps = nNewDstFrameRate;
        ret = AW_MPI_VI_CreateVipp(pStreamContext->mVipp);
        if (ret != SUCCESS)
        {
            aloge("fatal error! AW_MPI_VI CreateVipp[%d] failed", pStreamContext->mVipp);
        }

        ret = AW_MPI_VI_SetVippAttr(pStreamContext->mVipp, &pStreamContext->mViAttr);
        if (ret != SUCCESS)
        {
            aloge("fatal error! AW_MPI_VI SetVippAttr[%d] failed", pStreamContext->mVipp);
        }
        ret = AW_MPI_VI_GetVippAttr(pStreamContext->mVipp, &pStreamContext->mViAttr);
        if (ret != SUCCESS)
        {
            aloge("fatal error! AW_MPI_VI GetVippAttr[%d] failed", pStreamContext->mVipp);
        }
#if ISP_RUN
        AW_MPI_ISP_Run(pStreamContext->mIsp);
        AW_MPI_ISP_SetIspAlgoFreq(pStreamContext->mIsp, pStreamContext->mIspAlgoFreq);
#endif
        ret = AW_MPI_VI_EnableVipp(pStreamContext->mVipp);
        if (ret != SUCCESS)
        {
            aloge("fatal error! enableVipp[%d] fail!", pStreamContext->mVipp);
        }
        ret = AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mViChn, NULL);
        if (ret != SUCCESS)
        {
            aloge("fatal error! vipp[%d] createVirChn[%d] fail!", pStreamContext->mVipp, pStreamContext->mViChn);
        }
        //bind vi to venc.
        ret = AW_MPI_SYS_Bind(&stVirViChn, &stVencChn);
        if (ret != SUCCESS)
        {
            aloge("fatal error! bind viChn[%d-%d] to vencChn[%d] fail!", pStreamContext->mVipp, pStreamContext->mViChn, pStreamContext->mVEncChn);
        }
        //start virViChn.
        ret = AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if (ret != SUCCESS)
        {
            aloge("fatal error! virViChn[%d-%d] enable error!", pStreamContext->mVipp, pStreamContext->mViChn);
        }
    }

    if (bNeedRecreate)
    {
        AW_MPI_VENC_StopRecvPic(pStreamContext->mVEncChn);
        //must return all outFrames before destroy encLib.
        AW_MPI_VENC_DestroyEncoder(pStreamContext->mVEncChn);
        //create encoder
        AW_MPI_VENC_CreateEncoder(pStreamContext->mVEncChn);
        //config VencConfig again.
        VENC_CHN_ATTR_S *pVencChnAttr = &pStreamContext->mVEncChnAttr;

        unsigned int vbvThreshSize = 0;
        unsigned int vbvBufSize = 0;
        if (nNewDstFrameRate)
        {
            vbvThreshSize = nNewBitRate/8/nNewDstFrameRate*15;
        }
        vbvBufSize = nNewBitRate/8*VE_VBV_CACHE_TIME + vbvThreshSize;
        pStreamContext->mVbvBufSize = vbvBufSize;

        pVencChnAttr->VeAttr.Type = nNewEncodeType;
        pVencChnAttr->VeAttr.MaxKeyInterval = nNewKeyInterval;
        pVencChnAttr->VeAttr.SrcPicWidth = pStreamContext->mViAttr.format.width;
        pVencChnAttr->VeAttr.SrcPicHeight = pStreamContext->mViAttr.format.height;
        if (PT_H264 == pVencChnAttr->VeAttr.Type)
        {
            pVencChnAttr->VeAttr.AttrH264e.BufSize = AWALIGN(vbvBufSize, 1024);
            pVencChnAttr->VeAttr.AttrH264e.mThreshSize = vbvThreshSize;
            pVencChnAttr->VeAttr.AttrH264e.PicWidth  = stNewDstEncodeSize.Width;
            pVencChnAttr->VeAttr.AttrH264e.PicHeight = stNewDstEncodeSize.Height;
            switch (pVencChnAttr->RcAttr.mRcMode)
            {
                case VENC_RC_MODE_H264VBR:
                {
                    pVencChnAttr->RcAttr.mAttrH264Vbr.mMaxBitRate = nNewBitRate;
                    pVencChnAttr->RcAttr.mAttrH264Vbr.mSrcFrmRate = nNewDstFrameRate;
                    pVencChnAttr->RcAttr.mAttrH264Vbr.mDstFrmRate = nNewDstFrameRate;
                    break;
                }
                case VENC_RC_MODE_H264CBR:
                {
                    pVencChnAttr->RcAttr.mAttrH264Cbr.mBitRate = nNewBitRate;
                    pVencChnAttr->RcAttr.mAttrH264Cbr.mSrcFrmRate = nNewDstFrameRate;
                    pVencChnAttr->RcAttr.mAttrH264Cbr.mDstFrmRate = nNewDstFrameRate;
                    break;
                }
                default:
                {
                    aloge("fatal error! wrong rcMode[%d], check code!", pVencChnAttr->RcAttr.mRcMode);
                    break;
                }
            }
        }
        else if (PT_H265 == pVencChnAttr->VeAttr.Type)
        {
            pVencChnAttr->VeAttr.AttrH265e.mBufSize = AWALIGN(vbvBufSize, 1024);
            pVencChnAttr->VeAttr.AttrH265e.mThreshSize = vbvThreshSize;
            pVencChnAttr->VeAttr.AttrH265e.mPicWidth = stNewDstEncodeSize.Width;
            pVencChnAttr->VeAttr.AttrH265e.mPicHeight = stNewDstEncodeSize.Height;
            switch (pVencChnAttr->RcAttr.mRcMode)
            {
                case VENC_RC_MODE_H265VBR:
                {
                    pVencChnAttr->RcAttr.mAttrH265Vbr.mMaxBitRate = nNewBitRate;
                    pVencChnAttr->RcAttr.mAttrH265Vbr.mSrcFrmRate = nNewDstFrameRate;
                    pVencChnAttr->RcAttr.mAttrH265Vbr.mDstFrmRate = nNewDstFrameRate;
                    break;
                }
                case VENC_RC_MODE_H265CBR:
                {
                    pVencChnAttr->RcAttr.mAttrH265Cbr.mBitRate = nNewBitRate;
                    pVencChnAttr->RcAttr.mAttrH265Cbr.mSrcFrmRate = nNewDstFrameRate;
                    pVencChnAttr->RcAttr.mAttrH265Cbr.mDstFrmRate = nNewDstFrameRate;
                    break;
                }
                default:
                {
                    aloge("fatal error! wrong rcMode[%d], check code!", pVencChnAttr->RcAttr.mRcMode);
                    break;
                }
            }
        }
        else
        {
            aloge("fatal error! main stream wrong venc type:%d", pVencChnAttr->VeAttr.Type);
        }
        pStreamContext->mVEncFrameRateConfig.SrcFrmRate = nNewDstFrameRate;
        pStreamContext->mVEncFrameRateConfig.DstFrmRate = nNewDstFrameRate;

        AW_MPI_VENC_SetChnAttr(pStreamContext->mVEncChn, pVencChnAttr);
        //AW_MPI_VENC_SetFrameRate(pStreamContext->mVEncChn, &pStreamContext->mVEncFrameRateConfig);

        ret = AW_MPI_VENC_StartRecvPic(pStreamContext->mVEncChn);
        if (ret != SUCCESS)
        {
            aloge("fatal error! venc[%d] start fail!", pStreamContext->mVEncChn);
        }

#ifdef SUPPORT_MOTION_SEARCH
        if (pStreamContext->bMotionSearchEnable)
        {
            AW_MPI_VENC_SetMotionSearchParam(pStreamContext->mVEncChn, &pStreamContext->mMotionParam);
            alogd("enable MotionSearch");
        }
#endif

        pNewDynamicConfig->mVideoEncoderFmt = nOldEncodeType;
        pNewDynamicConfig->mVideoFrameRate = nOldDstFrameRate;
        pNewDynamicConfig->mVideoBitRate = nOldBitRate;
        pNewDynamicConfig->dstWidth = stOldDstEncodeSize.Width;
        pNewDynamicConfig->dstHeight = stOldDstEncodeSize.Height;
        pNewDynamicConfig->mKeyFrameInterval = nOldKeyInterval;
    }

    pNewDynamicConfig->mVencRecreateFlag = VENC_RECREATE_DONE;
    alogd("venc_recreate process VencChn[%d] done", pStreamContext->mVEncChn);

    return ret;
}

static void* getVencStreamThread(void *pThreadData)
{
    VencStreamContext *pStreamContext = (VencStreamContext*)pThreadData;
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext*)pStreamContext->priv;
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

#ifdef SUPPORT_SAVE_STREAM
    if (pStreamContext->mRecordHandler >= 0)
    {
        RECORD_CONFIG_S mRecConfig;
        memset(&mRecConfig, 0, sizeof(RECORD_CONFIG_S));
        mRecConfig.mVeChn = pStreamContext->mVEncChn;
        mRecConfig.mRecordHandler = pStreamContext->mRecordHandler;
        if (pContext->mConfigPara.mMainVEncChn == pStreamContext->mVEncChn)
        {
            mRecConfig.mMaxFileCnt = pContext->mConfigPara.mMainSaveMaxFileCnt;
            mRecConfig.mMaxFileDuration = pContext->mConfigPara.mMainSaveOneFileDuration*1000*1000;
            snprintf(mRecConfig.mFilePath, MAX_RECORD_FILE_PATH_LEN, "%s", pContext->mConfigPara.mMainFilePath);
        }
        else if (pContext->mConfigPara.mMain2ndVEncChn == pStreamContext->mVEncChn)
        {
            mRecConfig.mMaxFileCnt = pContext->mConfigPara.mMain2ndSaveMaxFileCnt;
            mRecConfig.mMaxFileDuration = pContext->mConfigPara.mMain2ndSaveOneFileDuration*1000*1000;
            snprintf(mRecConfig.mFilePath, MAX_RECORD_FILE_PATH_LEN, "%s", pContext->mConfigPara.mMain2ndFilePath);
        }
        else if (pContext->mConfigPara.mSubVEncChn == pStreamContext->mVEncChn)
        {
            mRecConfig.mMaxFileCnt = pContext->mConfigPara.mSubSaveMaxFileCnt;
            mRecConfig.mMaxFileDuration = pContext->mConfigPara.mSubSaveOneFileDuration*1000*1000;
            snprintf(mRecConfig.mFilePath, MAX_RECORD_FILE_PATH_LEN, "%s", pContext->mConfigPara.mSubFilePath);
        }
        else if (pContext->mConfigPara.mSub2ndVEncChn == pStreamContext->mVEncChn)
        {
            mRecConfig.mMaxFileCnt = pContext->mConfigPara.mSub2ndSaveMaxFileCnt;
            mRecConfig.mMaxFileDuration = pContext->mConfigPara.mSub2ndSaveOneFileDuration*1000*1000;
            snprintf(mRecConfig.mFilePath, MAX_RECORD_FILE_PATH_LEN, "%s", pContext->mConfigPara.mSub2ndFilePath);
        }
        else if (pContext->mConfigPara.mThreeVEncChn == pStreamContext->mVEncChn)
        {
            mRecConfig.mMaxFileCnt = pContext->mConfigPara.mThreeSaveMaxFileCnt;
            mRecConfig.mMaxFileDuration = pContext->mConfigPara.mThreeSaveOneFileDuration*1000*1000;
            snprintf(mRecConfig.mFilePath, MAX_RECORD_FILE_PATH_LEN, "%s", pContext->mConfigPara.mThreeFilePath);
        }
        else if (pContext->mConfigPara.mThree2ndVEncChn == pStreamContext->mVEncChn)
        {
            mRecConfig.mMaxFileCnt = pContext->mConfigPara.mThree2ndSaveMaxFileCnt;
            mRecConfig.mMaxFileDuration = pContext->mConfigPara.mThree2ndSaveOneFileDuration*1000*1000;
            snprintf(mRecConfig.mFilePath, MAX_RECORD_FILE_PATH_LEN, "%s", pContext->mConfigPara.mThree2ndFilePath);
        }
        else if (pContext->mConfigPara.mFourVEncChn == pStreamContext->mVEncChn)
        {
            mRecConfig.mMaxFileCnt = pContext->mConfigPara.mFourSaveMaxFileCnt;
            mRecConfig.mMaxFileDuration = pContext->mConfigPara.mFourSaveOneFileDuration*1000*1000;
            snprintf(mRecConfig.mFilePath, MAX_RECORD_FILE_PATH_LEN, "%s", pContext->mConfigPara.mFourFilePath);
        }
        else if (pContext->mConfigPara.mFour2ndVEncChn == pStreamContext->mVEncChn)
        {
            mRecConfig.mMaxFileCnt = pContext->mConfigPara.mFour2ndSaveMaxFileCnt;
            mRecConfig.mMaxFileDuration = pContext->mConfigPara.mFour2ndSaveOneFileDuration*1000*1000;
            snprintf(mRecConfig.mFilePath, MAX_RECORD_FILE_PATH_LEN, "%s", pContext->mConfigPara.mFour2ndFilePath);
        }
        else
        {
            aloge("fatal error! invalid venc chn %d", pStreamContext->mVEncChn);
        }
        mRecConfig.requestKeyFrame = requestKeyFrameFunc;
        record_set_config(&mRecConfig);
    }
#endif

    int nEncodeWidth = 0;
    int nEncodeHeight = 0;
    unsigned int buf_size = 0;
    unsigned char *stream_buf = NULL;
    int rtsp_test_enable = 0;
    VencRecreateDynamicConfig *pVencRecreateCfg = NULL;
    uint64_t pVencRecreateInv = 0;
    if (pContext->mConfigPara.mMainVEncChn == pStreamContext->mVEncChn)
    {
        nEncodeWidth = pContext->mConfigPara.mMainEncodeWidth;
        nEncodeHeight = pContext->mConfigPara.mMainEncodeHeight;
        if (pContext->mConfigPara.mMainRtspID >= 0)
            rtsp_test_enable = 1;

        if (1 == pContext->mConfigPara.mMainEncodeDynamicConfig.mVencRecreateEnable)
        {
            pVencRecreateCfg = &pContext->mConfigPara.mMainEncodeDynamicConfig;
            pVencRecreateInv = pVencRecreateCfg->mVencRecreateInterval * 1000 * 1000;
        }
    }
    else if (pContext->mConfigPara.mMain2ndVEncChn == pStreamContext->mVEncChn)
    {
        nEncodeWidth = pContext->mConfigPara.mMain2ndEncodeWidth;
        nEncodeHeight = pContext->mConfigPara.mMain2ndEncodeHeight;
    }
    else if (pContext->mConfigPara.mSubVEncChn == pStreamContext->mVEncChn)
    {
        nEncodeWidth = pContext->mConfigPara.mSubEncodeWidth;
        nEncodeHeight = pContext->mConfigPara.mSubEncodeHeight;
        if (pContext->mConfigPara.mSubRtspID >= 0)
            rtsp_test_enable = 1;

        if (1 == pContext->mConfigPara.mSubEncodeDynamicConfig.mVencRecreateEnable)
        {
            pVencRecreateCfg = &pContext->mConfigPara.mSubEncodeDynamicConfig;
            pVencRecreateInv = pVencRecreateCfg->mVencRecreateInterval * 1000 * 1000;
        }
    }
    else if (pContext->mConfigPara.mSub2ndVEncChn == pStreamContext->mVEncChn)
    {
        nEncodeWidth = pContext->mConfigPara.mSub2ndEncodeWidth;
        nEncodeHeight = pContext->mConfigPara.mSub2ndEncodeHeight;
    }
    else if (pContext->mConfigPara.mThreeVEncChn == pStreamContext->mVEncChn)
    {
        nEncodeWidth = pContext->mConfigPara.mThreeEncodeWidth;
        nEncodeHeight = pContext->mConfigPara.mThreeEncodeHeight;
        if (pContext->mConfigPara.mThreeRtspID >= 0)
            rtsp_test_enable = 1;

        if (1 == pContext->mConfigPara.mThreeEncodeDynamicConfig.mVencRecreateEnable)
        {
            pVencRecreateCfg = &pContext->mConfigPara.mThreeEncodeDynamicConfig;
            pVencRecreateInv = pVencRecreateCfg->mVencRecreateInterval * 1000 * 1000;
        }
    }
    else if (pContext->mConfigPara.mThree2ndVEncChn == pStreamContext->mVEncChn)
    {
        nEncodeWidth = pContext->mConfigPara.mThree2ndEncodeWidth;
        nEncodeHeight = pContext->mConfigPara.mThree2ndEncodeHeight;
    }
    else if (pContext->mConfigPara.mFourVEncChn == pStreamContext->mVEncChn)
    {
        nEncodeWidth = pContext->mConfigPara.mFourEncodeWidth;
        nEncodeHeight = pContext->mConfigPara.mFourEncodeHeight;
        if (pContext->mConfigPara.mFourRtspID >= 0)
            rtsp_test_enable = 1;

        if (1 == pContext->mConfigPara.mFourEncodeDynamicConfig.mVencRecreateEnable)
        {
            pVencRecreateCfg = &pContext->mConfigPara.mFourEncodeDynamicConfig;
            pVencRecreateInv = pVencRecreateCfg->mVencRecreateInterval * 1000 * 1000;
        }
    }
    else if (pContext->mConfigPara.mFour2ndVEncChn == pStreamContext->mVEncChn)
    {
        nEncodeWidth = pContext->mConfigPara.mFour2ndEncodeWidth;
        nEncodeHeight = pContext->mConfigPara.mFour2ndEncodeHeight;
    }
    else
    {
        aloge("fatal error! invalid venc chn %d", pStreamContext->mVEncChn);
        return NULL;
    }

    if (pContext->mConfigPara.mStreamBufSize > 0)
    {
        buf_size = pContext->mConfigPara.mStreamBufSize;
    }
    else if (pContext->mConfigPara.mStreamBufSize == -1)
    {
        buf_size = pStreamContext->mVbvBufSize;
    }
    else
    {
        buf_size = nEncodeWidth * nEncodeHeight * 3 / 2 / 10;
    }
    if (0 >= buf_size)
    {
        aloge("fatal error! VencChn[%d] buf_size %d is invalid param!", pStreamContext->mVEncChn, buf_size);
        return NULL;
    }
    stream_buf = (unsigned char *)malloc(buf_size);
    if (NULL == stream_buf)
    {
        aloge("malloc stream_buf failed, size=%d", buf_size);
        return NULL;
    }
    memset(stream_buf, 0, buf_size);
    alogd("VencChn[%d] stream_buf:%p, size=%d", pStreamContext->mVEncChn, stream_buf, buf_size);

#ifdef SUPPORT_RTSP_TEST
    if (rtsp_test_enable)
    {
        if (pContext->mConfigPara.mMainVEncChn == pStreamContext->mVEncChn)
        {
            rtsp_start(pContext->mConfigPara.mMainRtspID);
        }
        else if (pContext->mConfigPara.mSubVEncChn == pStreamContext->mVEncChn)
        {
            rtsp_start(pContext->mConfigPara.mSubRtspID);
        }
        else if (pContext->mConfigPara.mThreeVEncChn == pStreamContext->mVEncChn)
        {
            rtsp_start(pContext->mConfigPara.mThreeRtspID);
        }
        else if (pContext->mConfigPara.mFourVEncChn == pStreamContext->mVEncChn)
        {
            rtsp_start(pContext->mConfigPara.mFourRtspID);
        }
    }
#endif

#ifdef SUPPORT_MOTION_SEARCH
    int motionAlarm_result_print_interval = 0;
    if (0 < pContext->mConfigPara.motionAlarm_result_print_interval)
        motionAlarm_result_print_interval = pContext->mConfigPara.motionAlarm_result_print_interval;
    else
        motionAlarm_result_print_interval = 50;
#endif

    int mActualFrameRate = 0;
    int mTempFrameCnt = 0;
    int mTempCurrentTime = 0;
    int mTempLastTime = 0;
    uint64_t mLastPts = 0;
    pStreamContext->mStreamDataCnt = 0;

    while (!pContext->mbExitFlag)
    {
        if ((NULL != pVencRecreateCfg) && (1 == pVencRecreateCfg->mVencRecreateEnable))
        {
            do
            {
                if ((0 == pVencRecreateCfg->mVencRecreateLoopCnt)
                    || (VENC_RECREATE_ACTIVE != pVencRecreateCfg->mVencRecreateFlag))
                {
                    break;
                }

                ret = vencRecreate(pStreamContext, pVencRecreateCfg);
                if (SUCCESS != ret)
                {
                    aloge("fatal error! why venc recreate fail[%d]?", ret);
                }
#ifdef SUPPORT_SAVE_STREAM
                if (VENC_RECREATE_DONE == pVencRecreateCfg->mVencRecreateFlag)
                {
                    pVencRecreateCfg->mVencRecreateLoopCnt = (-1 != pVencRecreateCfg->mVencRecreateLoopCnt)
                        ? (pVencRecreateCfg->mVencRecreateLoopCnt - 1) : pVencRecreateCfg->mVencRecreateLoopCnt;
                    getVencSpsPpsInfo(pStreamContext);
                }
#endif
            } while(0);
        }

        memset(stVencStream.mpPack, 0, sizeof(VENC_PACK_S));
        ret = AW_MPI_VENC_GetStream(pStreamContext->mVEncChn, &stVencStream, 4000);
        if(SUCCESS == ret)
        {
            pStreamContext->mStreamDataCnt++;

#ifdef SUPPORT_MOTION_SEARCH
#ifdef MOTION_SEARCH_DYNAMIC_ADJUST_UPDATE_INTV
            if (pStreamContext->bMotionSearchEnable && 200 == pStreamContext->mStreamDataCnt) // for test
            {
                int dst_update_interval = 3;
                AW_MPI_VENC_GetMotionSearchParam(pStreamContext->mVEncChn, &pStreamContext->mMotionParam);
                if (pStreamContext->mMotionParam.update_interval != dst_update_interval)
                {
                    alogw("VencChn[%d] change update_interval %d -> %d", pStreamContext->mVEncChn, pStreamContext->mMotionParam.update_interval, dst_update_interval);
                    pStreamContext->mMotionParam.update_interval = dst_update_interval;
                    AW_MPI_VENC_SetMotionSearchParam(pStreamContext->mVEncChn, &pStreamContext->mMotionParam);
                }
            }
#endif
#endif
            nStreamLen = stVencStream.mpPack[0].mLen0 + stVencStream.mpPack[0].mLen1;
            if (nStreamLen <= 0 || nStreamLen > buf_size)
            {
                aloge("fatal error! VencStream length error, drop it, StreamLen:%d, buf_size:%d, [%d,%d]!",
                    nStreamLen, buf_size, stVencStream.mpPack[0].mLen0, stVencStream.mpPack[0].mLen1);
                AW_MPI_VENC_ReleaseStream(pStreamContext->mVEncChn, &stVencStream);
                continue;
            }

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
                        int sensor_fps = 0;
                        AW_MPI_ISP_GetSensorFps(pStreamContext->mIsp, &sensor_fps);
                        alogv("sensor_fps:%d, venc fps:%d", sensor_fps, pStreamContext->mVEncFrameRateConfig.DstFrmRate);
                        if (sensor_fps == pStreamContext->mVEncFrameRateConfig.DstFrmRate)
                        {
                            alogw("VencChn[%d], actualFrameRate %d < dstFrmRate %d", pStreamContext->mVEncChn, mActualFrameRate, pStreamContext->mVEncFrameRateConfig.DstFrmRate);
                        }
                    }
                    mTempLastTime = mTempCurrentTime;
                    mTempFrameCnt = 0;
                }
            }

            uint64_t pts = stVencStream.mpPack->mPTS;
            int len = 0;
            char keyframe = 0;
            char stream_buf_empty = 0;

            if ((NULL != pVencRecreateCfg) && (1 == pVencRecreateCfg->mVencRecreateEnable))
            {
                if (0 == mLastPts)
                {
                    mLastPts = pts;
                }
                else if ((pts - mLastPts) >= pVencRecreateInv)
                {
                    pVencRecreateCfg->mVencRecreateFlag = VENC_RECREATE_ACTIVE;
                    mLastPts = pts;
                }
            }

            if (stVencStream.mpPack != NULL && stVencStream.mpPack->mLen0 > 0)
            {
                if (PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                {
                    if (H264E_NALU_ISLICE == stVencStream.mpPack->mDataType.enH264EType)
                    {
                        if (NULL == pStreamContext->mSpsPpsInfo.pBuffer)
                        {
                            alogd("SpsPpsInfo.pBuffer = NULL!!\n");
                        }
                        if (0 < pStreamContext->mSpsPpsInfo.nLength && buf_size >= pStreamContext->mSpsPpsInfo.nLength)
                        {
                            /* Get sps/pps first */
                            memcpy(stream_buf, pStreamContext->mSpsPpsInfo.pBuffer, pStreamContext->mSpsPpsInfo.nLength);
                            len += pStreamContext->mSpsPpsInfo.nLength;
                        }
                        else
                        {
                            stream_buf_empty = 1;
                            aloge("fatal error! VeChn[%d] stream buf size %d is too small, h264 spspps len=%d !", pStreamContext->mVEncChn, buf_size, pStreamContext->mSpsPpsInfo.nLength);
                        }
                        keyframe = 1;
                        alogv("***** VeChn[%d] H264 got I frame, Seq %d *****", pStreamContext->mVEncChn, stVencStream.mSeq);
                    }
                    else
                    {
                        alogv("VeChn[%d] H264 got P frame, Seq %d", pStreamContext->mVEncChn, stVencStream.mSeq);
                    }
                }
                else if (PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                {
                    if (H265E_NALU_ISLICE == stVencStream.mpPack->mDataType.enH265EType)
                    {
                        if (NULL == pStreamContext->mSpsPpsInfo.pBuffer)
                        {
                            alogd("SpsPpsInfo.pBuffer = NULL!!\n");
                        }
                        if (0 < pStreamContext->mSpsPpsInfo.nLength && buf_size >= pStreamContext->mSpsPpsInfo.nLength)
                        {
                            /* Get sps/pps first */
                            memcpy(stream_buf, pStreamContext->mSpsPpsInfo.pBuffer, pStreamContext->mSpsPpsInfo.nLength);
                            len += pStreamContext->mSpsPpsInfo.nLength;
                        }
                        else
                        {
                            stream_buf_empty = 1;
                            aloge("fatal error! VeChn[%d] stream buf size %d is too small, h265 spspps len=%d !", pStreamContext->mVEncChn, buf_size, pStreamContext->mSpsPpsInfo.nLength);
                        }
                        keyframe = 1;
                        alogv("***** VeChn[%d] H265 got I frame, Seq %d *****", pStreamContext->mVEncChn, stVencStream.mSeq);
                    }
                    else
                    {
                        alogv("VeChn[%d] H265 got P frame, Seq %d", pStreamContext->mVEncChn, stVencStream.mSeq);
                    }
                }
                else
                {
                    aloge("fatal error! vencType:0x%x is wrong!", pStreamContext->mVEncChnAttr.VeAttr.Type);
                }

                if (stVencStream.mpPack->mLen0 > 0)
                {
                    if (buf_size >= (len + stVencStream.mpPack->mLen0))
                    {
                        memcpy(stream_buf + len, stVencStream.mpPack->mpAddr0, stVencStream.mpPack->mLen0);
                        len += stVencStream.mpPack->mLen0;
                    }
                    else
                    {
                        stream_buf_empty = 1;
                        aloge("fatal error! VeChn[%d] stream buf size %d is too small, len=%d !", pStreamContext->mVEncChn, buf_size, len + stVencStream.mpPack->mLen0);
                    }
                }
                if (stVencStream.mpPack->mLen1 > 0)
                {
                    if (buf_size >= (len + stVencStream.mpPack->mLen1))
                    {
                        memcpy(stream_buf + len, stVencStream.mpPack->mpAddr1, stVencStream.mpPack->mLen1);
                        len += stVencStream.mpPack->mLen1;
                    }
                    else
                    {
                        stream_buf_empty = 1;
                        aloge("fatal error! VeChn[%d] stream buf size %d is too small, len=%d !", pStreamContext->mVEncChn, buf_size, len + stVencStream.mpPack->mLen1);
                    }
                }
            }
#ifdef SUPPORT_MOTION_SEARCH
            if (pStreamContext->bMotionSearchEnable && pStreamContext->mMotionResult.region)
            {
                ret = AW_MPI_VENC_GetMotionSearchResult(pStreamContext->mVEncChn, &pStreamContext->mMotionResult);
                if (SUCCESS == ret)
                {
                    int num = 0;
                    int i = 0;
                    for(i=0; i< pStreamContext->mMotionResult.total_region_num; i++)
                    {
                        if(pStreamContext->mMotionResult.region[i].is_motion)
                        {
                            alogv("area_%d:[(%d,%d),(%d,%d)]", i,
                                pStreamContext->mMotionResult.region[i].pix_x_bgn, pStreamContext->mMotionResult.region[i].pix_y_bgn,
                                pStreamContext->mMotionResult.region[i].pix_x_end, pStreamContext->mMotionResult.region[i].pix_y_end);
                            num++;
                        }
                    }
                    if (num != pStreamContext->mMotionResult.motion_region_num)
                    {
                        aloge("num(%d) != motion_region_num(%d)", num, pStreamContext->mMotionResult.motion_region_num);
                    }
                    int result = 0;
                    MotionSearchJudgeResult(pStreamContext, &result);
                    if ((motionAlarm_result_print_interval) && (pStreamContext->mStreamDataCnt % motionAlarm_result_print_interval) == 0)
                    {
                        alogd("VeChn[%d] detect motion area: total:%d, motion:%d, result:%d", pStreamContext->mVEncChn, pStreamContext->mMotionResult.total_region_num, num, result);
                    }
                }
            }
#endif
            ret = AW_MPI_VENC_ReleaseStream(pStreamContext->mVEncChn, &stVencStream);
            if(ret != SUCCESS)
            {
                aloge("fatal error! venc_chn[%d] releaseStream fail", pStreamContext->mVEncChn);
            }

            if ((stream_buf != NULL) && len > 0 && (0 == stream_buf_empty))
            {
#ifdef SUPPORT_SAVE_STREAM
                if (pStreamContext->mRecordHandler >= 0)
                {
                    RECORD_FRAME_S mRecFrm;
                    memset(&mRecFrm, 0, sizeof(RECORD_FRAME_S));
                    mRecFrm.mpFrm = stream_buf;
                    mRecFrm.mFrmSize = len;
                    mRecFrm.mFrameType = (1 == keyframe) ? RECORD_FRAME_TYPE_I : RECORD_FRAME_TYPE_P;
                    mRecFrm.mPts = pts;
                    if ((NULL != pVencRecreateCfg) && (VENC_RECREATE_DONE == pVencRecreateCfg->mVencRecreateFlag))
                    {
                        mRecFrm.mChangeFd = 1;
                        pVencRecreateCfg->mVencRecreateFlag = VENC_RECREATE_IDLE;
                    }
                    //alogd("record[%d] FrameType=%d", pStreamContext->mRecordHandler, mRecFrm.mFrameType);
                    int start_time = getSysTickMs();
                    record_send_data(pStreamContext->mRecordHandler, &mRecFrm);
                    int end_time = getSysTickMs();
                    if (100 < end_time - start_time)
                    {
                        alogw("venc_chn[%d] record_send_data cost %dms > 100ms", pStreamContext->mVEncChn, end_time - start_time);
                    }
                }
#endif

#ifdef SUPPORT_RTSP_TEST
                if (rtsp_test_enable)
                {
                    /* get current fps and set to rtsp to avoid rtsp checkDurationTime warn */
                    int sensor_fps = 0;
                    AW_MPI_ISP_GetSensorFps(pStreamContext->mIsp, &sensor_fps);

                    RtspSendDataParam stRtspParam;
                    memset(&stRtspParam, 0, sizeof(RtspSendDataParam));
                    stRtspParam.buf = stream_buf;
                    stRtspParam.size = len;
                    stRtspParam.frame_type = (1 == keyframe) ? RTSP_FRAME_DATA_TYPE_I : RTSP_FRAME_DATA_TYPE_P;
                    stRtspParam.pts = pts;
                    stRtspParam.frame_rate = sensor_fps;
                    alogv("VencChn[%d] rtsp id[%d] RtspParam %p %d %d %lld, %dfps",
                        pStreamContext->mVEncChn, pContext->mConfigPara.mMainRtspID,
                        stRtspParam.buf, stRtspParam.size, stRtspParam.frame_type, stRtspParam.pts, stRtspParam.frame_rate);
                    int start_time = getSysTickMs();
                    if (pContext->mConfigPara.mMainVEncChn == pStreamContext->mVEncChn)
                    {
                        rtsp_sendData(pContext->mConfigPara.mMainRtspID, &stRtspParam);
                    }
                    else if (pContext->mConfigPara.mSubVEncChn == pStreamContext->mVEncChn)
                    {
                        rtsp_sendData(pContext->mConfigPara.mSubRtspID, &stRtspParam);
                    }
                    else if (pContext->mConfigPara.mThreeVEncChn == pStreamContext->mVEncChn)
                    {
                        rtsp_sendData(pContext->mConfigPara.mThreeRtspID, &stRtspParam);
                    }
                    else if (pContext->mConfigPara.mFourVEncChn == pStreamContext->mVEncChn)
                    {
                        rtsp_sendData(pContext->mConfigPara.mFourRtspID, &stRtspParam);
                    }
                    int end_time = getSysTickMs();
                    if (100 < end_time - start_time)
                    {
                        alogw("venc_chn[%d] rtsp_sendData cost %dms > 100ms", pStreamContext->mVEncChn, end_time - start_time);
                    }
                }
#endif
            }
        }
        else
        {
            alogw("fatal error! vencChn[%d] getStream failed! check code! ret=0x%x", pStreamContext->mVEncChn, ret);
            continue;
        }
    }

    if (stream_buf)
    {
        free(stream_buf);
        stream_buf = NULL;
    }

    alogd("exit");

    return (void*)result;
}

#ifdef SUPPORT_OFFLINE_RAW_SIMULATE
static void* SendRawThread(void* pThreadData)
{
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext*)pThreadData;
    VencStreamContext *pStreamContext = &pContext->mMainStream;
    VI_ATTR_S *mViAttr = &pStreamContext->mViAttr;
    VI_PTN_CFG_S *ptn_ctx = &mViAttr->mPtnCfg;
    unsigned int ptn_current_count = 0;
    int ptn_current_dir = 0;
    int ptn_framenum_temp = 0;
    int ptn_framenum = 0;
    char strThreadName[32];
    sprintf(strThreadName, "isp%d-sendraw", pStreamContext->mIsp);
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);
    ptn_ctx->ptn_count = &ptn_current_count;
    *ptn_ctx->ptn_count = pContext->mConfigPara.mMainOfflineRawSimulateCntStart;
    ptn_ctx->ptn_read_dir = (unsigned char *)&ptn_current_dir;
    if(ptn_ctx->ptn_file)
    {
        rewind(ptn_ctx->ptn_file);
        fseek(ptn_ctx->ptn_file, ptn_current_count * ptn_ctx->ptn_cfg.ptn_size, SEEK_CUR);
    }

    while (!pContext->mbExitFlag)
    {
        ptn_framenum = AW_MPI_VI_GetPtnFrameNum(pStreamContext->mVipp);
        if (ptn_framenum != ptn_framenum_temp)
        {
            ptn_framenum_temp = ptn_framenum;
            if (ptn_ctx->ptn_en)
            {
                read_next_bin_file(pContext);
                AW_MPI_VI_SetNextPtn(pStreamContext->mVipp,&pStreamContext->mViAttr);
                continue;
            }
        }
        else
        {
            usleep(10*1000);
        }
    }
    return NULL;
}
#endif

static void getVencSpsPpsInfo(VencStreamContext *pStreamContext)
{
    int ret = 0;
    memset(&pStreamContext->mSpsPpsInfo, 0, sizeof(VencHeaderData));
    if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        ret = AW_MPI_VENC_GetH264SpsPpsInfo(pStreamContext->mVEncChn, &pStreamContext->mSpsPpsInfo);
        if(ret != SUCCESS)
        {
            aloge("fatal error! get spspps fail[0x%x]!", ret);
        }
    }
    else if(PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        ret = AW_MPI_VENC_GetH265SpsPpsInfo(pStreamContext->mVEncChn, &pStreamContext->mSpsPpsInfo);
        if(ret != SUCCESS)
        {
            aloge("fatal error! get spspps fail[0x%x]!", ret);
        }
    }
    else
    {
        aloge("fatal error! vencType:0x%x is wrong!", pStreamContext->mVEncChnAttr.VeAttr.Type);
    }
}

static ERRORTYPE resetCamera(SampleSmartIPCDemoContext *pContext, int vipp, BOOL enable)
{
    VencStreamContext *pStreamContext = NULL;
    int mVipp = -1;
    int mIsp = -1;
    int mIspAlgoFreq = 0;
    int mViChn = -1;
    int mPdetViChn = -1;
    int mTakePicViChn = -1;
    int mVEncChn = -1;
    VI_ATTR_S mViAttr;
    int mPdetEnable = 0;
    int mTakePicEnable = 0;

    alogd("enter, vipp%d", vipp);

    if (vipp == pContext->mMainStream.mVipp)
    {
        alogd("MainStream");
        pStreamContext = &pContext->mMainStream;
        mVipp = pStreamContext->mVipp;
        mIsp = pStreamContext->mIsp;
        mIspAlgoFreq = pStreamContext->mIspAlgoFreq;
        mViChn = pStreamContext->mViChn;
        mVEncChn = pStreamContext->mVEncChn;
    }
    else if (vipp == pContext->mMain2ndStream.mVipp)
    {
        alogd("Main2ndStream");
        pStreamContext = &pContext->mMain2ndStream;
        mVipp = pStreamContext->mVipp;
        mIsp = pStreamContext->mIsp;
        mIspAlgoFreq = pStreamContext->mIspAlgoFreq;
        mViChn = pStreamContext->mViChn;
        mPdetEnable = pStreamContext->mPdetEnable;
        mPdetViChn = pStreamContext->mPdetViChn;
        mTakePicEnable = pStreamContext->mTakePicture;
        mTakePicViChn = pStreamContext->mTakePictureViChn;
        mVEncChn = pStreamContext->mVEncChn;
    }
    else if (vipp == pContext->mSubStream.mVipp)
    {
        alogd("SubStream");
        pStreamContext = &pContext->mSubStream;
        mVipp = pStreamContext->mVipp;
        mIsp = pStreamContext->mIsp;
        mIspAlgoFreq = pStreamContext->mIspAlgoFreq;
        mViChn = pStreamContext->mViChn;
        mVEncChn = pStreamContext->mVEncChn;
    }
    else if (vipp == pContext->mSub2ndStream.mVipp)
    {
        alogd("Sub2ndStream");
        pStreamContext = &pContext->mSub2ndStream;
        mVipp = pStreamContext->mVipp;
        mIsp = pStreamContext->mIsp;
        mIspAlgoFreq = pStreamContext->mIspAlgoFreq;
        mViChn = pStreamContext->mViChn;
        mPdetEnable = pStreamContext->mPdetEnable;
        mPdetViChn = pStreamContext->mPdetViChn;
        mTakePicEnable = pStreamContext->mTakePicture;
        mTakePicViChn = pStreamContext->mTakePictureViChn;
        mVEncChn = pStreamContext->mVEncChn;
    }
    else if (vipp == pContext->mThreeStream.mVipp)
    {
        alogd("ThreeStream");
        pStreamContext = &pContext->mThreeStream;
        mVipp = pStreamContext->mVipp;
        mIsp = pStreamContext->mIsp;
        mIspAlgoFreq = pStreamContext->mIspAlgoFreq;
        mViChn = pStreamContext->mViChn;
        mVEncChn = pStreamContext->mVEncChn;
    }
    else if (vipp == pContext->mThree2ndStream.mVipp)
    {
        alogd("Three2ndStream");
        pStreamContext = &pContext->mThree2ndStream;
        mVipp = pStreamContext->mVipp;
        mIsp = pStreamContext->mIsp;
        mIspAlgoFreq = pStreamContext->mIspAlgoFreq;
        mViChn = pStreamContext->mViChn;
        mPdetEnable = pStreamContext->mPdetEnable;
        mPdetViChn = pStreamContext->mPdetViChn;
        mTakePicEnable = pStreamContext->mTakePicture;
        mTakePicViChn = pStreamContext->mTakePictureViChn;
        mVEncChn = pStreamContext->mVEncChn;
    }
    else if (vipp == pContext->mFourStream.mVipp)
    {
        alogd("FourStream");
        pStreamContext = &pContext->mFourStream;
        mVipp = pStreamContext->mVipp;
        mIsp = pStreamContext->mIsp;
        mIspAlgoFreq = pStreamContext->mIspAlgoFreq;
        mViChn = pStreamContext->mViChn;
        mVEncChn = pStreamContext->mVEncChn;
    }
    else if (vipp == pContext->mFour2ndStream.mVipp)
    {
        alogd("Four2ndStream");
        pStreamContext = &pContext->mFour2ndStream;
        mVipp = pStreamContext->mVipp;
        mIsp = pStreamContext->mIsp;
        mIspAlgoFreq = pStreamContext->mIspAlgoFreq;
        mViChn = pStreamContext->mViChn;
        mPdetEnable = pStreamContext->mPdetEnable;
        mPdetViChn = pStreamContext->mPdetViChn;
        mTakePicEnable = pStreamContext->mTakePicture;
        mTakePicViChn = pStreamContext->mTakePictureViChn;
        mVEncChn = pStreamContext->mVEncChn;
    }
    else
    {
        alogd("invalid vipp %d", vipp);
        return -1;
    }

    if (FALSE == enable)
    {
        if (0 == mPdetEnable)
            alogd("vipp%d viChn%d stop vi&venc", mVipp, mViChn);
        else
            alogd("vipp%d viChn%d PdetViChn%d stop vi&venc", mVipp, mViChn, mPdetViChn);
        AW_MPI_VI_DisableVirChn(mVipp, mViChn);
        if (mPdetEnable)
            AW_MPI_VI_DisableVirChn(mVipp, mPdetViChn);
#ifndef SUPPORT_RECREATE_JPEG
        if (mTakePicEnable)
        {
            AW_MPI_VI_DisableVirChn(mVipp, mTakePicViChn);
            AW_MPI_VI_DestroyVirChn(mVipp, mTakePicViChn);
        }
#endif
        if (-1 != mVEncChn)
        {
            AW_MPI_VENC_StopRecvPic(mVEncChn);
            //AW_MPI_VENC_DestroyEncoder(mVEncChn);
            //AW_MPI_VENC_ResetChn(mVEncChn);
            MPP_CHN_S ViChn = {MOD_ID_VIU, mVipp, mViChn};
            MPP_CHN_S VeChn = {MOD_ID_VENC, 0, mVEncChn};
            AW_MPI_SYS_UnBind(&ViChn, &VeChn);
        }
        AW_MPI_VI_DestroyVirChn(mVipp, mViChn);
        if (mPdetEnable)
            AW_MPI_VI_DestroyVirChn(mVipp, mPdetViChn);
        AW_MPI_VI_DisableVipp(mVipp);
        AW_MPI_ISP_Stop(mIsp);
        AW_MPI_VI_DestroyVipp(mVipp);
    }
    else
    {
        if (0 == mPdetEnable)
            alogd("vipp%d viChn%d prepare vi&venc", mVipp, mViChn);
        else
            alogd("vipp%d viChn%d PdetViChn%d prepare vi&venc", mVipp, mViChn, mPdetViChn);
        AW_MPI_VI_CreateVipp(mVipp);
        AW_MPI_VI_SetVippAttr(mVipp, &pStreamContext->mViAttr);
        AW_MPI_ISP_Run(mIsp);
        AW_MPI_ISP_SetIspAlgoFreq(mIsp, mIspAlgoFreq);
        AW_MPI_VI_CreateVirChn(mVipp, mViChn, NULL);
        if (mPdetEnable)
            AW_MPI_VI_CreateVirChn(mVipp, mPdetViChn, NULL);
        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VI_RegisterCallback(mVipp, &cbInfo);
        AW_MPI_VI_EnableVipp(mVipp);
        if (-1 != mVEncChn)
        {
            MPP_CHN_S ViChn = {MOD_ID_VIU, mVipp, mViChn};
            MPP_CHN_S VeChn = {MOD_ID_VENC, 0, mVEncChn};
            AW_MPI_SYS_Bind(&ViChn, &VeChn);
        }
        alogd("vipp%d start vi&venc", mVipp);
        AW_MPI_VI_EnableVirChn(mVipp, mViChn);

#ifndef SUPPORT_RECREATE_JPEG
        if (mTakePicEnable)
        {
            AW_MPI_VI_CreateVirChn(mVipp, mTakePicViChn, NULL);
            AW_MPI_VI_EnableVirChn(mVipp, mTakePicViChn);
        }
#endif

        if (mPdetEnable)
            AW_MPI_VI_EnableVirChn(mVipp, mPdetViChn);
        if (-1 != mVEncChn)
        {
            AW_MPI_VENC_StartRecvPic(mVEncChn);
        }
    }

    alogd("ok, vipp%d", mVipp);

    return SUCCESS;
}

#ifdef SUPPORT_IR_CUT_SWITCH_TEST
void write_to_file(const char *file, const char *value) {
    FILE *fp = fopen(file, "w");
    if (fp == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "%s\n", value);
    fclose(fp);
}

static void *CGIRCUTThread(void *pThreadData)
{
    int irlight_weight = 0, envlv = 0,ir_change_flag = 0,mode = 0,count = 0,ir_flag = 0;
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext*)pThreadData;
    int interval = pContext->mConfigPara.mIrCutSwitchInterval;
    int switchmode = pContext->mConfigPara.mIrCutSwitchMode;

    char strThreadName[32];
    sprintf(strThreadName, "isp%d-ircut", pContext->mConfigPara.mIrCutSwitchIsp);
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    if (0 == interval)
        interval = 10;

    write_to_file(IRCUT_DIR "/ircut_ctr", "enable");
    write_to_file(IRCUT_DIR "/ircut_pin_drv", "3");
    while (!pContext->mbExitFlag)
    {
        if (switchmode == 0)
        {
            irlight_weight = AW_MPI_ISP_GetIrAwbWinCntWeight(pContext->mConfigPara.mIrCutSwitchIsp);
            envlv = AW_MPI_ISP_GetEnvLV(pContext->mConfigPara.mIrCutSwitchIsp);
            alogd("Enlv = %d,  irlight_weight= %d\n", envlv, irlight_weight);

            if (mode == 0) //daymode
            {
                if (envlv < DAY2IR_EVLV_TH)
                {
                    count++;
                    if(count >= CHANGE_COUNT_TH) {
                        alogd("change to ir mode\n");
                        ir_change_flag = 1;
                        mode = 1;
                    }
                } else {
                    count = 0;
                }
            } else {
                if (envlv > IR2DAY_EVLV_TH && irlight_weight < IR2DAY_IRWEIGHT_TH)  //switch to day mode when envlv > IR2DAY_EVLV_TH && irlight_weight < IR2DAY_IRWEIGHT_TH
                {
                    count++;
                    if(count >= CHANGE_COUNT_TH) {
                        alogd("change to day mode\n");
                        ir_change_flag = 1;
                        mode = 0;
                    }
                } else {
                    count = 0;
                }
            }

            if (ir_change_flag == 1){
                if (mode == 1)
                {
                    /*IR*/
                    AW_MPI_ISP_ReadIspCfgBin(pContext->mConfigPara.mIrCutSwitchIsp, 1, pContext->mConfigPara.mIrCutSwitchIrBinPath);
                    usleep(100*1000);
                    system("cd /sys/kernel/debug/sunxi_pinctrl && echo 42000540.pinctrl > dev_name && echo PL6 > sunxi_pin && echo PL6 1 > function && echo PL6 1 > data"); //open irlight
                    write_to_file(IRCUT_DIR "/ircut_status", "open");  //open ircut
                }
                else
                {
                    /*RGB*/
                    write_to_file(IRCUT_DIR "/ircut_status", "close");//close ircut
                    system("cd /sys/kernel/debug/sunxi_pinctrl && echo 42000540.pinctrl > dev_name && echo PL6 > sunxi_pin && echo PL6 1 > function && echo PL6 0 > data"); //close irlight
                    usleep(100*1000);
                    AW_MPI_ISP_ReadIspCfgBin(pContext->mConfigPara.mIrCutSwitchIsp, 0, pContext->mConfigPara.mIrCutSwitchRgbBinPath);
                }
                ir_change_flag = 0;
            }
            usleep(interval*1000);
        }
        else if (switchmode == 1)
        {
            if (ir_flag == 0)
            {
                /*IR*/
                AW_MPI_ISP_ReadIspCfgBin(pContext->mConfigPara.mIrCutSwitchIsp, 1, pContext->mConfigPara.mIrCutSwitchIrBinPath);
                usleep(100*1000);
                system("cd /sys/kernel/debug/sunxi_pinctrl && echo 42000540.pinctrl > dev_name && echo PL6 > sunxi_pin && echo PL6 1 > function && echo PL6 1 > data");
                write_to_file(IRCUT_DIR "/ircut_status", "open");
                ir_flag = 1;
            }
            else
            {
                /*RGB*/
                write_to_file(IRCUT_DIR "/ircut_status", "close");
                system("cd /sys/kernel/debug/sunxi_pinctrl && echo 42000540.pinctrl > dev_name && echo PL6 > sunxi_pin && echo PL6 1 > function && echo PL6 0 > data");
                usleep(100*1000);
                AW_MPI_ISP_ReadIspCfgBin(pContext->mConfigPara.mIrCutSwitchIsp, 0, pContext->mConfigPara.mIrCutSwitchRgbBinPath);
                ir_flag = 0;
            }
           sleep(interval);
        }
        else if (switchmode == 2) {
            /*FORCE IR*/
            AW_MPI_ISP_ReadIspCfgBin(pContext->mConfigPara.mIrCutSwitchIsp, 1, pContext->mConfigPara.mIrCutSwitchIrBinPath);
            usleep(100*1000);
            system("cd /sys/kernel/debug/sunxi_pinctrl && echo 42000540.pinctrl > dev_name && echo PL6 > sunxi_pin && echo PL6 1 > function && echo PL6 1 > data");
            write_to_file(IRCUT_DIR "/ircut_status", "open");
            break;
        }
        else {
            /*FORCE RGB*/
            write_to_file(IRCUT_DIR "/ircut_status", "close");
            system("cd /sys/kernel/debug/sunxi_pinctrl && echo 42000540.pinctrl > dev_name && echo PL6 > sunxi_pin && echo PL6 1 > function && echo PL6 0 > data");
            usleep(100*1000);
            AW_MPI_ISP_ReadIspCfgBin(pContext->mConfigPara.mIrCutSwitchIsp, 0, pContext->mConfigPara.mIrCutSwitchRgbBinPath);
            break;
        }
    }

    return NULL;
}
#endif

#ifdef SUPPORT_SENSOR_FLIP_MIRROR
static void* FlipSwitchThread(void *pThreadData) {
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext*)pThreadData;

    int flip_states[4] = {1, 2, 3, 0};
    int state_index = 0;
    int interval = pContext->mConfigPara.mFlipSwitchInterval;
    if (0 == interval)
        interval = 10;

    while (!pContext->mbExitFlag) {
        if(pContext->mConfigPara.mSensorFlipMirror == 4) {
            int current_state = flip_states[state_index];
            if (pContext->mConfigPara.mFlipMainVippEnable) {
                AW_MPI_VI_SetVippMirror(pContext->mMainStream.mVipp, (current_state & 0x1) ? 1 : 0);
                AW_MPI_VI_SetVippFlip(pContext->mMainStream.mVipp, (current_state & 0x2) ? 1 : 0);
            }
            if (pContext->mConfigPara.mFlipSubVippEnable) {
                AW_MPI_VI_SetVippMirror(pContext->mSubStream.mVipp, (current_state & 0x1)? 1 : 0);
                AW_MPI_VI_SetVippFlip(pContext->mSubStream.mVipp, (current_state & 0x2)? 1 : 0);
            }
            alogd("Flip and mirror state switched to: %d", current_state);
            state_index = (state_index + 1) % 4;
            sleep(interval);
        }
        else
        {
            if (pContext->mConfigPara.mFlipMainVippEnable) {
                AW_MPI_VI_SetVippMirror(pContext->mMainStream.mVipp, (pContext->mConfigPara.mSensorFlipMirror & 0x1) ? 1 : 0);
                AW_MPI_VI_SetVippFlip(pContext->mMainStream.mVipp, (pContext->mConfigPara.mSensorFlipMirror & 0x2) ? 1 : 0);
            }
            if (pContext->mConfigPara.mFlipSubVippEnable) {
                AW_MPI_VI_SetVippMirror(pContext->mSubStream.mVipp, (pContext->mConfigPara.mSensorFlipMirror & 0x1) ? 1 : 0);
                AW_MPI_VI_SetVippFlip(pContext->mSubStream.mVipp, (pContext->mConfigPara.mSensorFlipMirror & 0x2) ? 1 : 0);
            }
            break;
        }
    }
    return NULL;
}
#endif

static void *MsgQueueThread(void *pThreadData)
{
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext*)pThreadData;
    message_t stCmdMsg;
    SmartIPCMsgType cmd;
    int nCmdPara;

    char strThreadName[32];
    sprintf(strThreadName, "MsgQueue");
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    alogd("msg queue thread start run!");
    while (1)
    {
        if (0 == get_message(&pContext->mMsgQueue, &stCmdMsg))
        {
            cmd = stCmdMsg.command;
            nCmdPara = stCmdMsg.para0;

            switch (cmd)
            {
                case Vi_Timeout:
                {
                    if (0 == pContext->mConfigPara.mViTimeoutResetDisable)
                    {
                        int vipp = nCmdPara;
                        alogd("vipp[%d] got Vi Timeout, reset Camera", vipp);
                        pContext->mViresetFlag = 1;
                        if (pContext->mConfigPara.mMainEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mMainVipp, FALSE);
                        }
                        if (pContext->mConfigPara.mMain2ndEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mMain2ndVipp, FALSE);
                        }
                        if (pContext->mConfigPara.mSubEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mSubVipp, FALSE);
                        }
                        if (pContext->mConfigPara.mSub2ndEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mSub2ndVipp, FALSE);
                        }
                        if (pContext->mConfigPara.mThreeEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mThreeVipp, FALSE);
                        }
                        if (pContext->mConfigPara.mThree2ndEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mThree2ndVipp, FALSE);
                        }
                        if (pContext->mConfigPara.mFourEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mFourVipp, FALSE);
                        }
                        if (pContext->mConfigPara.mFour2ndEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mFour2ndVipp, FALSE);
                        }
                        usleep(100*1000);
                        if (pContext->mConfigPara.mMainEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mMainVipp, TRUE);
                        }
                        if (pContext->mConfigPara.mMain2ndEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mMain2ndVipp, TRUE);
                        }
                        if (pContext->mConfigPara.mSubEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mSubVipp, TRUE);
                        }
                        if (pContext->mConfigPara.mSub2ndEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mSub2ndVipp, TRUE);
                        }
                        if (pContext->mConfigPara.mThreeEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mThreeVipp, TRUE);
                        }
                        if (pContext->mConfigPara.mThree2ndEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mThree2ndVipp, TRUE);
                        }
                        if (pContext->mConfigPara.mFourEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mFourVipp, TRUE);
                        }
                        if (pContext->mConfigPara.mFour2ndEnable)
                        {
                            resetCamera(pContext, pContext->mConfigPara.mFour2ndVipp, TRUE);
                        }
                        pContext->mViresetFlag = 0;
                        alogd("vipp[%d] got Vi Timeout, reset Camera done", vipp);
                    }
                    break;
                }
                case MsgQueue_Stop:
                {
                    goto _Exit;
                }
                default:
                {
                    break;
                }
            }
        }
        else
        {
            TMessage_WaitQueueNotEmpty(&pContext->mMsgQueue, 0);
        }
    }
_Exit:
    alogd("msg queue thread exit!");
    return NULL;
}

// just for stress test
static void *testTriggerThread(void *pThreadData)
{
    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext*)pThreadData;
    int vipp = 0;
    int interval_ms = 1000;

    char strThreadName[32];
    sprintf(strThreadName, "testTrigger");
    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    if (pContext->mConfigPara.mTestTriggerViTimeout > 0)
    {
        interval_ms = pContext->mConfigPara.mTestTriggerViTimeout;
    }

    alogd("test thread start run!");
    while (!pContext->mbExitFlag)
    {
        usleep(interval_ms*1000);
        alogd("test vi timeout. vipp:%d, interval_ms:%d", vipp, interval_ms);

        message_t stCmdMsg;
        InitMessage(&stCmdMsg);
        stCmdMsg.command = Vi_Timeout;
        stCmdMsg.para0 = vipp;
        putMessageWithData(&pContext->mMsgQueue, &stCmdMsg);
    }

    alogd("test thread exit!");
    return NULL;
}

static void setVencVui(VencStreamContext *pStreamContext)
{
    if (PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        VENC_PARAM_H264_VUI_S h264_vui;
        memset(&h264_vui, 0, sizeof(VENC_PARAM_H264_VUI_S));
        AW_MPI_VENC_GetH264Vui(pStreamContext->mVEncChn, &h264_vui);
        h264_vui.VuiTimeInfo.timing_info_present_flag = 1;
        h264_vui.VuiTimeInfo.fixed_frame_rate_flag = 1;
        h264_vui.VuiTimeInfo.num_units_in_tick = 1000;
        h264_vui.VuiTimeInfo.time_scale = h264_vui.VuiTimeInfo.num_units_in_tick
            * pStreamContext->mVEncFrameRateConfig.DstFrmRate * 2;
        AW_MPI_VENC_SetH264Vui(pStreamContext->mVEncChn, &h264_vui);
    }
    else if (PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        VENC_PARAM_H265_VUI_S h265_vui;
        memset(&h265_vui, 0, sizeof(VENC_PARAM_H265_VUI_S));
        AW_MPI_VENC_GetH265Vui(pStreamContext->mVEncChn, &h265_vui);
        h265_vui.VuiTimeInfo.timing_info_present_flag = 1;
        h265_vui.VuiTimeInfo.num_units_in_tick = 1000;
        /* Notices: the protocol syntax states that h265 does not need to be multiplied by 2. */
        h265_vui.VuiTimeInfo.time_scale = h265_vui.VuiTimeInfo.num_units_in_tick
            * pStreamContext->mVEncFrameRateConfig.DstFrmRate;
        h265_vui.VuiTimeInfo.num_ticks_poc_diff_one_minus1 = h265_vui.VuiTimeInfo.num_units_in_tick;
        AW_MPI_VENC_SetH265Vui(pStreamContext->mVEncChn, &h265_vui);
    }
}

static void setRegionDetectLink(int nVEncChn, int nRegionLinkEn, int nRegionLinkTexDetectEn, int nRegionLinkMotionDetectEn, int nRegionLinkMotionDetectInv)
{
    sRegionLinkParam stRegionLinkParam;
    memset(&stRegionLinkParam, 0, sizeof(sRegionLinkParam));
    AW_MPI_VENC_GetRegionDetectLink(nVEncChn, &stRegionLinkParam);
    if (stRegionLinkParam.mStaticParam.region_link_en != nRegionLinkEn)
    {
        alogd("VencChn[%d] region_link_en: %d -> %d", nVEncChn, stRegionLinkParam.mStaticParam.region_link_en, nRegionLinkEn);
        stRegionLinkParam.mStaticParam.region_link_en = nRegionLinkEn;
    }
    if (stRegionLinkParam.mStaticParam.tex_detect_en != nRegionLinkTexDetectEn)
    {
        alogd("VencChn[%d] tex_detect_en: %d -> %d", nVEncChn, stRegionLinkParam.mStaticParam.tex_detect_en, nRegionLinkTexDetectEn);
        stRegionLinkParam.mStaticParam.tex_detect_en = nRegionLinkTexDetectEn;
    }
    if (stRegionLinkParam.mStaticParam.motion_detect_en != nRegionLinkMotionDetectEn)
    {
        alogd("VencChn[%d] motion_detect_en: %d -> %d", nVEncChn, stRegionLinkParam.mStaticParam.motion_detect_en, nRegionLinkMotionDetectEn);
        stRegionLinkParam.mStaticParam.motion_detect_en = nRegionLinkMotionDetectEn;
    }
    if (stRegionLinkParam.mStaticParam.updateInterVal != nRegionLinkMotionDetectInv)
    {
        alogd("VencChn[%d] updateInterVal: %d -> %d", nVEncChn, stRegionLinkParam.mStaticParam.updateInterVal, nRegionLinkMotionDetectInv);
        stRegionLinkParam.mStaticParam.updateInterVal = nRegionLinkMotionDetectInv;
    }
    AW_MPI_VENC_SetRegionDetectLink(nVEncChn, &stRegionLinkParam);
}

#ifdef SUPPORT_PERSON_DETECT
static int pdetprepare(VencStreamContext *pStreamContext)
{
    int ret = 0;
    SamplePdetInfo *pPdetInfo = &pStreamContext->mPdetInfo;
    SampleDewbInfo *pDewbInfo = &pStreamContext->mDewbInfo;

    cdx_sem_init(&pStreamContext->mSemPdetFrameReady, 0);
    pthread_mutex_init(&pStreamContext->mPdetLock, NULL);
    ret = De_Init(pDewbInfo);
    if (ret != 0)
    {
        aloge("fatal error! de init fail.\n");
        ret = -1;
        goto _de_stop;
    }

    ret = pdet_init(pPdetInfo);
    if (ret < 0)
    {
        aloge("fatal error! pdet alog init fail.\n");
        ret = -1;
        goto _deinit_pdet;
    }

    return ret;

_deinit_pdet:
    De_Deinit(pDewbInfo);
_de_stop:
    return ret;
}
#endif

static int prepare(SampleSmartIPCDemoContext *pContext)
{
    int ret = 0;

    if (pContext->mConfigPara.mMainEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mMainStream;
        pStreamContext->priv = (void*)pContext;
        configMainStream(pStreamContext, &pContext->mConfigPara);
        AW_MPI_VI_CreateVipp(pStreamContext->mVipp);
        /* open video0/4 when stitch_mode >= DMA_STITCH_HORIZONTAL for set some ioctl to video0/4 */
        if (pStreamContext->mViAttr.large_dma_merge_en >= DMA_STITCH_HORIZONTAL) {
            alogd("It will open video%d when stitch_mode >= DMA_STITCH_HORIZONTAL\n", pStreamContext->mVipp - 1);
            AW_MPI_VI_CreateVipp(pStreamContext->mVipp - 1);
        }
#ifdef SUPPORT_OFFLINE_RAW_SIMULATE
        if (pContext->mConfigPara.mMainOfflineRawSimulateType >= 0)
        {
            set_video_ptn(pStreamContext->mVipp, &pContext->mMainStream, pContext->mConfigPara.mMainOfflineRawFilePath, pContext->mConfigPara.mMainOfflineRawType);
        }
#endif

        AW_MPI_VI_SetVippAttr(pStreamContext->mVipp, &pStreamContext->mViAttr);

        if (100 <= pContext->mConfigPara.mMainIspD3dLbcRatio && 400 >= pContext->mConfigPara.mMainIspD3dLbcRatio)
        {
            alogd("Isp[%d] set D3dLbcRatio %d", pStreamContext->mIsp, pContext->mConfigPara.mMainIspD3dLbcRatio);
            AW_MPI_ISP_SetD3dLbcRatio(pStreamContext->mIsp, pContext->mConfigPara.mMainIspD3dLbcRatio);
        }

        if (pStreamContext->mViAttr.large_dma_merge_en > DMA_STITCH_NONE) {
            if (pStreamContext->mViAttr.large_dma_merge_en == DMA_STITCH_2IN1_LINNER) {
                /* It must be set StitchMode and StatsSyncMode for DMA_STITCH_2IN1_LINNER */
                AW_MPI_ISP_SetStitchMode(pStreamContext->mIsp, STITCH_2IN1_LINNER);
                AW_MPI_ISP_AWB_SetStatsSyncMode(pStreamContext->mIsp, ISP0_ISP1_COMBINE);
            }
            AW_MPI_ISP_Run(pContext->mConfigPara.mMainViStitchIspChannelId);
            if (pContext->mConfigPara.mMainViStitchMode > DMA_STITCH_2IN1_LINNER)
            {
                AW_MPI_ISP_SetIspAlgoFreq(pContext->mConfigPara.mMainViStitchIspChannelId, pStreamContext->mIspAlgoFreq);
            }
        }
        AW_MPI_ISP_Run(pStreamContext->mIsp);
        AW_MPI_ISP_SetIspAlgoFreq(pStreamContext->mIsp, pStreamContext->mIspAlgoFreq);

        if (-1 != pContext->mConfigPara.mMainIspTdmRawProcessType)
        {
            tdm_raw_process_config_param tdm_config_param;
            memset(&tdm_config_param, 0, sizeof(tdm_raw_process_config_param));
            tdm_config_param.type = pContext->mConfigPara.mMainIspTdmRawProcessType;
            tdm_config_param.width = pContext->mConfigPara.mMainIspTdmRawWidth;
            tdm_config_param.height = pContext->mConfigPara.mMainIspTdmRawHeight;
            tdm_config_param.frame_cnt_min = pContext->mConfigPara.mMainIspTdmRawProcessFrameCntMin;
            tdm_config_param.frame_cnt_max = pContext->mConfigPara.mMainIspTdmRawProcessFrameCntMax;
            strncpy(tdm_config_param.tdm_raw_file_path, pContext->mConfigPara.mMainIspTdmRawFilePath, TDM_RAW_PROCESS_FILE_PATH_MAX_LEN);
            tdm_raw_process_open(pStreamContext->mIsp, &tdm_config_param);
            tdm_raw_process_start(pStreamContext->mIsp);
        }

        AW_MPI_VI_EnableVipp(pStreamContext->mVipp);

        AW_MPI_VI_SetVippMirror(pStreamContext->mVipp, pContext->mConfigPara.mMainViMirror);
        AW_MPI_VI_SetVippFlip(pStreamContext->mVipp, pContext->mConfigPara.mMainViFlip);

        alogd("main set vipp%d Mirror=%d Flip=%d after enable vipp", pStreamContext->mVipp,
            pContext->mConfigPara.mMainViMirror, pContext->mConfigPara.mMainViFlip);

        // must be called after AW_MPI_VI_EnableVipp
        if (pContext->mConfigPara.mMainViSyncCtrlEnable)
        {
            struct csi_sync_ctrl csi_sync_cfg;
            memset(&csi_sync_cfg, 0, sizeof(struct csi_sync_ctrl));
            csi_sync_cfg.type = 0;
            csi_sync_cfg.prs_sync_en = 1;
            csi_sync_cfg.prs_sync_scr_sel = 0;
            csi_sync_cfg.prs_sync_bench_sel = 1;
            csi_sync_cfg.prs_sync_input_vsync_en = 1;
            csi_sync_cfg.prs_sync_singal_via_by = 0;
            csi_sync_cfg.prs_sync_singal_scr_sel = 1;
            csi_sync_cfg.prs_sync_pulse_cfg = 25;
            csi_sync_cfg.prs_sync_dist = 1;

            /**
             * sc2336: 0.3ms, 0x22C4
             * gc2083: 0.5ms, 0x3600
             */
            char *sensor_name = AW_MPI_ISP_GetSensorName(pStreamContext->mIsp);
            if(!strcmp(sensor_name, "sc2336_mipi"))
            {
                csi_sync_cfg.prs_sync_wait_n = 0x22C4; //0.3ms
                csi_sync_cfg.prs_sync_wait_m = 0x22C4; //0.3ms
            }
            else if(!strcmp(sensor_name, "gc2083_mipi") || !strcmp(sensor_name, "gc2083_mipi_2"))
            {
                csi_sync_cfg.prs_sync_wait_n = 0x3600; //0.5ms
                csi_sync_cfg.prs_sync_wait_m = 0x3600; //0.5ms
            }
            else
            {
                csi_sync_cfg.prs_sync_wait_n = 0x22C4; //0.3ms
                csi_sync_cfg.prs_sync_wait_m = 0x22C4; //0.3ms
                alogw("fatal error! main vipp%d sensor_name %s is not support SyncCtrl!",
                    pContext->mMainStream.mVipp, sensor_name);
            }

            alogd("main vipp%d, sensor_name %s sync ctrl enable", pContext->mMainStream.mVipp, sensor_name);
            AW_MPI_VI_SetSyncCtrl(pContext->mMainStream.mVipp, &csi_sync_cfg);
        }

#ifdef SUPPORT_SAVE_STREAM
        if (strlen(pContext->mConfigPara.mMainFilePath) > 0)
        {
            pStreamContext->mRecordHandler = pStreamContext->mVEncChn;
        }
#endif

        AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mViChn, NULL);
        AW_MPI_VENC_CreateChn(pStreamContext->mVEncChn, &pStreamContext->mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pStreamContext->mVEncChn, &pStreamContext->mVEncRcParam);

#ifdef ENABLE_VENC_GET_DIRMVINFO
        pStreamContext->mDirMvParam.dir_mv_en = 1;
        pStreamContext->mDirMvParam.update_interval = 1;
        ret = AW_MPI_VENC_SetDirMvParam(pStreamContext->mVEncChn, &pStreamContext->mDirMvParam);
        if (ret == SUCCESS)
        {
            alogd("AW_MPI_VENC_SetDirMvParam success, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
        else
        {
            aloge("AW_MPI_VENC_SetDirMvParam fail, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
#endif

#ifdef ENABLE_VENC_ADVANCED_PARAM
        /* set framerate in AW_MPI_VENC_CreateChn */
        //AW_MPI_VENC_SetFrameRate(pStreamContext->mVEncChn, &pStreamContext->mVEncFrameRateConfig);
        if (0 == pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 1);
        }
        if (0 == pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 1);
        }
        setVencSuperFrameCfg(pStreamContext->mVEncChn, pContext->mConfigPara.mMainEncodeBitrate, pContext->mConfigPara.mMainEncodeFrameRate);
#else
        if (pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 0);
        }
        if (pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 0);
        }
        alogd("VencChn[%d] use the default venc params for product mode %d", pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.RcAttr.mProductMode);
#endif
        setVencVui(pStreamContext);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pStreamContext->mVEncChn, &cbInfo);
        AW_MPI_VI_RegisterCallback(pStreamContext->mVipp, &cbInfo);

        if (pStreamContext->mIspAndVeLinkageEnable)
        {
            VENC_IspVeLinkAttr stIspVeLinkAttr;
            memset(&stIspVeLinkAttr, 0, sizeof(VENC_IspVeLinkAttr));
            stIspVeLinkAttr.bEnableIsp2Ve = TRUE;
            stIspVeLinkAttr.bEnableVe2Isp = TRUE; //main camera main stream enable Ve2isp
            stIspVeLinkAttr.nVipp = pStreamContext->mVipp;
            AW_MPI_VENC_EnableIspVeLink(pStreamContext->mVEncChn, &stIspVeLinkAttr);
            alogd("VencChn[%d] ispVeLink:%d-%d-%d", pStreamContext->mVEncChn, stIspVeLinkAttr.bEnableIsp2Ve, stIspVeLinkAttr.bEnableVe2Isp,
                stIspVeLinkAttr.nVipp);

            if (stIspVeLinkAttr.bEnableVe2Isp == TRUE)
            {
                setRegionDetectLink(pStreamContext->mVEncChn, pContext->mConfigPara.mRegionLinkEnable,
                    pContext->mConfigPara.mRegionLinkTexDetectEnable, pContext->mConfigPara.mRegionLinkMotionDetectEnable,
                    pContext->mConfigPara.mRegionLinkMotionDetectInv);
            }
        }

        if (pContext->mConfigPara.mVbrOptEnable)
        {
            setRcVbrOptParam(pStreamContext->mVEncChn, pContext->mConfigPara.mVbrOptRcPriority, pContext->mConfigPara.mVbrOptRcQualityLevel);
        }

        if (pStreamContext->mCameraAdaptiveMovingAndStaticEnable)
        {
            setVencLensMovingMaxQp(pStreamContext->mVEncChn, pContext->mConfigPara.mVencLensMovingMaxQp);
        }

        MPP_CHN_S ViChn = {MOD_ID_VIU, pStreamContext->mVipp, pStreamContext->mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pStreamContext->mVEncChn};
        AW_MPI_SYS_Bind(&ViChn, &VeChn);
        alogd("mainStream vipp:%d viChn:%d veChn:%d", pStreamContext->mVipp, pStreamContext->mViChn, pStreamContext->mVEncChn);
    }
    if (pContext->mConfigPara.mMain2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mMain2ndStream;
        pStreamContext->priv = (void*)pContext;
        configMain2ndStream(pStreamContext, &pContext->mConfigPara);
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            cdx_sem_init(&pStreamContext->mSemPdetFrameReady, 0);
            pthread_mutex_init(&pStreamContext->mPdetLock, NULL);
            if (0 != motion_pdet_init(&pStreamContext->mMotionPdetParam))
            {
                aloge("fatal error! motion_pdet_init failed!");
            }
        }
#endif
        AW_MPI_VI_CreateVipp(pStreamContext->mVipp);
        if (pStreamContext->mViAttr.large_dma_merge_en >= DMA_STITCH_HORIZONTAL) {
            alogd("It will open video%d when stitch_mode >= DMA_STITCH_HORIZONTAL\n", pStreamContext->mVipp - 1);
            AW_MPI_VI_CreateVipp(pStreamContext->mVipp - 1);
        }
        AW_MPI_VI_SetVippAttr(pStreamContext->mVipp, &pStreamContext->mViAttr);

        AW_MPI_VI_EnableVipp(pStreamContext->mVipp);

        //AW_MPI_VI_SetVippMirror(pStreamContext->mVipp, 1);
        //AW_MPI_VI_SetVippFlip(pStreamContext->mVipp, 1);

#ifdef SUPPORT_SAVE_STREAM
        if (strlen(pContext->mConfigPara.mMain2ndFilePath) > 0)
        {
            pStreamContext->mRecordHandler = pStreamContext->mVEncChn;
        }
#endif

        AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mViChn, NULL);
        AW_MPI_VENC_CreateChn(pStreamContext->mVEncChn, &pStreamContext->mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pStreamContext->mVEncChn, &pStreamContext->mVEncRcParam);

#ifdef ENABLE_VENC_GET_DIRMVINFO
        pStreamContext->mDirMvParam.dir_mv_en = 1;
        pStreamContext->mDirMvParam.update_interval = 1;
        ret = AW_MPI_VENC_SetDirMvParam(pStreamContext->mVEncChn, &pStreamContext->mDirMvParam);
        if (ret == SUCCESS)
        {
            alogd("AW_MPI_VENC_SetDirMvParam success, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
        else
        {
            aloge("AW_MPI_VENC_SetDirMvParam fail, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
#endif

#ifdef ENABLE_VENC_ADVANCED_PARAM
        /* set framerate in AW_MPI_VENC_CreateChn */
        //AW_MPI_VENC_SetFrameRate(pStreamContext->mVEncChn, &pStreamContext->mVEncFrameRateConfig);
        if (0 == pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 1);
        }
        if (0 == pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 1);
        }
        setVencSuperFrameCfg(pStreamContext->mVEncChn, pContext->mConfigPara.mMain2ndEncodeBitrate, pContext->mConfigPara.mMain2ndEncodeFrameRate);
#else
        if (pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 0);
        }
        if (pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 0);
        }
        alogd("VencChn[%d] use the default venc params for product mode %d", pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.RcAttr.mProductMode);
#endif
        setVencVui(pStreamContext);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pStreamContext->mVEncChn, &cbInfo);
        AW_MPI_VI_RegisterCallback(pStreamContext->mVipp, &cbInfo);

        if (pStreamContext->mIspAndVeLinkageEnable)
        {
            VENC_IspVeLinkAttr stIspVeLinkAttr;
            memset(&stIspVeLinkAttr, 0, sizeof(VENC_IspVeLinkAttr));
            stIspVeLinkAttr.bEnableIsp2Ve = TRUE;
            stIspVeLinkAttr.bEnableVe2Isp = FALSE;  //main camera sub stream disable Ve2isp
            stIspVeLinkAttr.nVipp = pStreamContext->mVipp;
            AW_MPI_VENC_EnableIspVeLink(pStreamContext->mVEncChn, &stIspVeLinkAttr);
            alogd("VencChn[%d] ispVeLink:%d-%d-%d", pStreamContext->mVEncChn, stIspVeLinkAttr.bEnableIsp2Ve, stIspVeLinkAttr.bEnableVe2Isp,
                stIspVeLinkAttr.nVipp);

            if (stIspVeLinkAttr.bEnableVe2Isp == TRUE)
            {
                setRegionDetectLink(pStreamContext->mVEncChn, pContext->mConfigPara.mRegionLinkEnable,
                    pContext->mConfigPara.mRegionLinkTexDetectEnable, pContext->mConfigPara.mRegionLinkMotionDetectEnable,
                    pContext->mConfigPara.mRegionLinkMotionDetectInv);
            }
        }

        if (pContext->mConfigPara.mVbrOptEnable)
        {
            setRcVbrOptParam(pStreamContext->mVEncChn, pContext->mConfigPara.mVbrOptRcPriority, pContext->mConfigPara.mVbrOptRcQualityLevel);
        }

        if (pStreamContext->mCameraAdaptiveMovingAndStaticEnable)
        {
            setVencLensMovingMaxQp(pStreamContext->mVEncChn, pContext->mConfigPara.mVencLensMovingMaxQp);
        }

        MPP_CHN_S ViChn = {MOD_ID_VIU, pStreamContext->mVipp, pStreamContext->mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pStreamContext->mVEncChn};
        AW_MPI_SYS_Bind(&ViChn, &VeChn);
        alogd("main2ndStream vipp:%d viChn:%d veChn:%d", pStreamContext->mVipp, pStreamContext->mViChn, pStreamContext->mVEncChn);
    }
    if (pContext->mConfigPara.mSubEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mSubStream;
        pStreamContext->priv = (void*)pContext;
        configSubStream(pStreamContext, &pContext->mConfigPara);
        AW_MPI_VI_CreateVipp(pStreamContext->mVipp);
        AW_MPI_VI_SetVippAttr(pStreamContext->mVipp, &pStreamContext->mViAttr);

        if (100 <= pContext->mConfigPara.mSubIspD3dLbcRatio && 400 >= pContext->mConfigPara.mSubIspD3dLbcRatio)
        {
            alogd("Isp[%d] set D3dLbcRatio %d", pStreamContext->mIsp, pContext->mConfigPara.mSubIspD3dLbcRatio);
            AW_MPI_ISP_SetD3dLbcRatio(pStreamContext->mIsp, pContext->mConfigPara.mSubIspD3dLbcRatio);
        }

        AW_MPI_ISP_Run(pStreamContext->mIsp);
        AW_MPI_ISP_SetIspAlgoFreq(pStreamContext->mIsp, pStreamContext->mIspAlgoFreq);

        if (-1 != pContext->mConfigPara.mSubIspTdmRawProcessType)
        {
            tdm_raw_process_config_param tdm_config_param;
            memset(&tdm_config_param, 0, sizeof(tdm_raw_process_config_param));
            tdm_config_param.type = pContext->mConfigPara.mSubIspTdmRawProcessType;
            tdm_config_param.width = pContext->mConfigPara.mSubIspTdmRawWidth;
            tdm_config_param.height = pContext->mConfigPara.mSubIspTdmRawHeight;
            tdm_config_param.frame_cnt_min = pContext->mConfigPara.mSubIspTdmRawProcessFrameCntMin;
            tdm_config_param.frame_cnt_max = pContext->mConfigPara.mSubIspTdmRawProcessFrameCntMax;
            strncpy(tdm_config_param.tdm_raw_file_path, pContext->mConfigPara.mSubIspTdmRawFilePath, TDM_RAW_PROCESS_FILE_PATH_MAX_LEN);
            tdm_raw_process_open(pStreamContext->mIsp, &tdm_config_param);
            tdm_raw_process_start(pStreamContext->mIsp);
        }

        AW_MPI_VI_EnableVipp(pStreamContext->mVipp);

        AW_MPI_VI_SetVippMirror(pStreamContext->mVipp, pContext->mConfigPara.mSubViMirror);
        AW_MPI_VI_SetVippFlip(pStreamContext->mVipp, pContext->mConfigPara.mSubViFlip);

        alogd("sub set vipp%d Mirror=%d Flip=%d after enable vipp", pStreamContext->mVipp,
            pContext->mConfigPara.mSubViMirror, pContext->mConfigPara.mSubViFlip);

#ifdef SUPPORT_SAVE_STREAM
        if (strlen(pContext->mConfigPara.mSubFilePath) > 0)
        {
            pStreamContext->mRecordHandler = pStreamContext->mVEncChn;
        }
#endif

        AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mViChn, NULL);
        AW_MPI_VENC_CreateChn(pStreamContext->mVEncChn, &pStreamContext->mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pStreamContext->mVEncChn, &pStreamContext->mVEncRcParam);

#ifdef ENABLE_VENC_GET_DIRMVINFO
        pStreamContext->mDirMvParam.dir_mv_en = 1;
        pStreamContext->mDirMvParam.update_interval = 1;
        ret = AW_MPI_VENC_SetDirMvParam(pStreamContext->mVEncChn, &pStreamContext->mDirMvParam);
        if (ret == SUCCESS)
        {
            alogd("AW_MPI_VENC_SetDirMvParam success, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
        else
        {
            aloge("AW_MPI_VENC_SetDirMvParam fail, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
#endif

#ifdef ENABLE_VENC_ADVANCED_PARAM
        /* set framerate in AW_MPI_VENC_CreateChn */
        //AW_MPI_VENC_SetFrameRate(pStreamContext->mVEncChn, &pStreamContext->mVEncFrameRateConfig);
        if (0 == pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 1);
        }
        if (0 == pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 1);
        }
        setVencSuperFrameCfg(pStreamContext->mVEncChn, pContext->mConfigPara.mSubEncodeBitrate, pContext->mConfigPara.mSubEncodeFrameRate);
#else
        if (pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 0);
        }
        if (pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 0);
        }
        alogd("VencChn[%d] use the default venc params for product mode %d", pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.RcAttr.mProductMode);
#endif
        setVencVui(pStreamContext);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pStreamContext->mVEncChn, &cbInfo);
        AW_MPI_VI_RegisterCallback(pStreamContext->mVipp, &cbInfo);

        if (pStreamContext->mIspAndVeLinkageEnable)
        {
            VENC_IspVeLinkAttr stIspVeLinkAttr;
            memset(&stIspVeLinkAttr, 0, sizeof(VENC_IspVeLinkAttr));
            stIspVeLinkAttr.bEnableIsp2Ve = TRUE;
            stIspVeLinkAttr.bEnableVe2Isp = TRUE; //sub camera main stream enable Ve2isp
            stIspVeLinkAttr.nVipp = pStreamContext->mVipp;
            AW_MPI_VENC_EnableIspVeLink(pStreamContext->mVEncChn, &stIspVeLinkAttr);
            alogd("VencChn[%d] ispVeLink:%d-%d-%d", pStreamContext->mVEncChn, stIspVeLinkAttr.bEnableIsp2Ve, stIspVeLinkAttr.bEnableVe2Isp,
                stIspVeLinkAttr.nVipp);

            if (stIspVeLinkAttr.bEnableVe2Isp == TRUE)
            {
                setRegionDetectLink(pStreamContext->mVEncChn, pContext->mConfigPara.mRegionLinkEnable,
                    pContext->mConfigPara.mRegionLinkTexDetectEnable, pContext->mConfigPara.mRegionLinkMotionDetectEnable,
                    pContext->mConfigPara.mRegionLinkMotionDetectInv);
            }
        }

        if (pContext->mConfigPara.mVbrOptEnable)
        {
            setRcVbrOptParam(pStreamContext->mVEncChn, pContext->mConfigPara.mVbrOptRcPriority, pContext->mConfigPara.mVbrOptRcQualityLevel);
        }

        if (pStreamContext->mCameraAdaptiveMovingAndStaticEnable)
        {
            setVencLensMovingMaxQp(pStreamContext->mVEncChn, pContext->mConfigPara.mVencLensMovingMaxQp);
        }

        MPP_CHN_S ViChn = {MOD_ID_VIU, pStreamContext->mVipp, pStreamContext->mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pStreamContext->mVEncChn};
        AW_MPI_SYS_Bind(&ViChn, &VeChn);
        alogd("subStream vipp:%d viChn:%d veChn:%d", pStreamContext->mVipp, pStreamContext->mViChn, pStreamContext->mVEncChn);
    }
    if (pContext->mConfigPara.mSub2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mSub2ndStream;
        pStreamContext->priv = (void*)pContext;
        configSub2ndStream(pStreamContext, &pContext->mConfigPara);
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            cdx_sem_init(&pStreamContext->mSemPdetFrameReady, 0);
            pthread_mutex_init(&pStreamContext->mPdetLock, NULL);
            if (0 != motion_pdet_init(&pStreamContext->mMotionPdetParam))
            {
                aloge("fatal error! motion_pdet_init failed!");
            }
        }
#endif
        AW_MPI_VI_CreateVipp(pStreamContext->mVipp);

        AW_MPI_VI_SetVippAttr(pStreamContext->mVipp, &pStreamContext->mViAttr);

        AW_MPI_VI_EnableVipp(pStreamContext->mVipp);

        //AW_MPI_VI_SetVippMirror(pStreamContext->mVipp, 1);
        //AW_MPI_VI_SetVippFlip(pStreamContext->mVipp, 1);

#ifndef SUPPORT_SAVE_STREAM
        if (strlen(pContext->mConfigPara.mSub2ndFilePath) > 0)
        {
            pStreamContext->mFile = fopen(pContext->mConfigPara.mSub2ndFilePath, "wb");
            if(NULL == pStreamContext->mFile)
            {
                aloge("fatal error! why open file[%s] fail? errno is %d", pContext->mConfigPara.mSub2ndFilePath, errno);
            }
        }
        else
        {
            pStreamContext->mFile = NULL;
        }
#else
        if (strlen(pContext->mConfigPara.mSub2ndFilePath) > 0)
        {
            pStreamContext->mRecordHandler = pStreamContext->mVEncChn;
        }
#endif

        AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mViChn, NULL);
        AW_MPI_VENC_CreateChn(pStreamContext->mVEncChn, &pStreamContext->mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pStreamContext->mVEncChn, &pStreamContext->mVEncRcParam);

#ifdef ENABLE_VENC_GET_DIRMVINFO
        pStreamContext->mDirMvParam.dir_mv_en = 1;
        pStreamContext->mDirMvParam.update_interval = 1;
        ret = AW_MPI_VENC_SetDirMvParam(pStreamContext->mVEncChn, &pStreamContext->mDirMvParam);
        if (ret == SUCCESS)
        {
            alogd("AW_MPI_VENC_SetDirMvParam success, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
        else
        {
            aloge("AW_MPI_VENC_SetDirMvParam fail, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
#endif

#ifdef ENABLE_VENC_ADVANCED_PARAM
        /* set framerate in AW_MPI_VENC_CreateChn */
        //AW_MPI_VENC_SetFrameRate(pStreamContext->mVEncChn, &pStreamContext->mVEncFrameRateConfig);
        if (0 == pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 1);
        }
        if (0 == pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 1);
        }
        setVencSuperFrameCfg(pStreamContext->mVEncChn, pContext->mConfigPara.mSub2ndEncodeBitrate, pContext->mConfigPara.mSub2ndEncodeFrameRate);
#else
        if (pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 0);
        }
        if (pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 0);
        }
        alogd("VencChn[%d] use the default venc params for product mode %d", pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.RcAttr.mProductMode);
#endif
        setVencVui(pStreamContext);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pStreamContext->mVEncChn, &cbInfo);
        AW_MPI_VI_RegisterCallback(pStreamContext->mVipp, &cbInfo);

        if (pStreamContext->mIspAndVeLinkageEnable)
        {
            VENC_IspVeLinkAttr stIspVeLinkAttr;
            memset(&stIspVeLinkAttr, 0, sizeof(VENC_IspVeLinkAttr));
            stIspVeLinkAttr.bEnableIsp2Ve = TRUE;
            stIspVeLinkAttr.bEnableVe2Isp = FALSE; //sub camera sub stream disable Ve2isp
            stIspVeLinkAttr.nVipp = pStreamContext->mVipp;
            AW_MPI_VENC_EnableIspVeLink(pStreamContext->mVEncChn, &stIspVeLinkAttr);
            alogd("VencChn[%d] ispVeLink:%d-%d-%d", pStreamContext->mVEncChn, stIspVeLinkAttr.bEnableIsp2Ve, stIspVeLinkAttr.bEnableVe2Isp,
                stIspVeLinkAttr.nVipp);

            if (stIspVeLinkAttr.bEnableVe2Isp == TRUE)
            {
                setRegionDetectLink(pStreamContext->mVEncChn, pContext->mConfigPara.mRegionLinkEnable,
                    pContext->mConfigPara.mRegionLinkTexDetectEnable, pContext->mConfigPara.mRegionLinkMotionDetectEnable,
                    pContext->mConfigPara.mRegionLinkMotionDetectInv);
            }
        }

        if (pContext->mConfigPara.mVbrOptEnable)
        {
            setRcVbrOptParam(pStreamContext->mVEncChn, pContext->mConfigPara.mVbrOptRcPriority, pContext->mConfigPara.mVbrOptRcQualityLevel);
        }

        if (pStreamContext->mCameraAdaptiveMovingAndStaticEnable)
        {
            setVencLensMovingMaxQp(pStreamContext->mVEncChn, pContext->mConfigPara.mVencLensMovingMaxQp);
        }

        MPP_CHN_S ViChn = {MOD_ID_VIU, pStreamContext->mVipp, pStreamContext->mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pStreamContext->mVEncChn};
        AW_MPI_SYS_Bind(&ViChn, &VeChn);
        alogd("sub2ndStream vipp:%d viChn:%d veChn:%d", pStreamContext->mVipp, pStreamContext->mViChn, pStreamContext->mVEncChn);
    }
    if (pContext->mConfigPara.mThreeEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mThreeStream;
        pStreamContext->priv = (void*)pContext;
        configThreeStream(pStreamContext, &pContext->mConfigPara);
        if (pStreamContext->mVipp != pContext->mSubStream.mVipp)
        {
            AW_MPI_VI_CreateVipp(pStreamContext->mVipp);
            AW_MPI_VI_SetVippAttr(pStreamContext->mVipp, &pStreamContext->mViAttr);
        }

        if (100 <= pContext->mConfigPara.mThreeIspD3dLbcRatio && 400 >= pContext->mConfigPara.mThreeIspD3dLbcRatio)
        {
            alogd("Isp[%d] set D3dLbcRatio %d", pStreamContext->mIsp, pContext->mConfigPara.mThreeIspD3dLbcRatio);
            AW_MPI_ISP_SetD3dLbcRatio(pStreamContext->mIsp, pContext->mConfigPara.mThreeIspD3dLbcRatio);
        }

        AW_MPI_ISP_Run(pStreamContext->mIsp);
        AW_MPI_ISP_SetIspAlgoFreq(pStreamContext->mIsp, pStreamContext->mIspAlgoFreq);

        if (-1 != pContext->mConfigPara.mThreeIspTdmRawProcessType)
        {
            tdm_raw_process_config_param tdm_config_param;
            memset(&tdm_config_param, 0, sizeof(tdm_raw_process_config_param));
            tdm_config_param.type = pContext->mConfigPara.mThreeIspTdmRawProcessType;
            tdm_config_param.width = pContext->mConfigPara.mThreeIspTdmRawWidth;
            tdm_config_param.height = pContext->mConfigPara.mThreeIspTdmRawHeight;
            tdm_config_param.frame_cnt_min = pContext->mConfigPara.mThreeIspTdmRawProcessFrameCntMin;
            tdm_config_param.frame_cnt_max = pContext->mConfigPara.mThreeIspTdmRawProcessFrameCntMax;
            strncpy(tdm_config_param.tdm_raw_file_path, pContext->mConfigPara.mThreeIspTdmRawFilePath, TDM_RAW_PROCESS_FILE_PATH_MAX_LEN);
            tdm_raw_process_open(pStreamContext->mIsp, &tdm_config_param);
            tdm_raw_process_start(pStreamContext->mIsp);
        }

        if (pStreamContext->mVipp != pContext->mSubStream.mVipp)
        {
            AW_MPI_VI_EnableVipp(pStreamContext->mVipp);

            AW_MPI_VI_SetVippMirror(pStreamContext->mVipp, pContext->mConfigPara.mThreeViMirror);
            AW_MPI_VI_SetVippFlip(pStreamContext->mVipp, pContext->mConfigPara.mThreeViFlip);

            alogd("three set vipp%d Mirror=%d Flip=%d after enable vipp", pStreamContext->mVipp,
                pContext->mConfigPara.mThreeViMirror, pContext->mConfigPara.mThreeViFlip);
        }

#ifdef SUPPORT_SAVE_STREAM
        if (strlen(pContext->mConfigPara.mThreeFilePath) > 0)
        {
            pStreamContext->mRecordHandler = pStreamContext->mVEncChn;
        }
#endif

        AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mViChn, NULL);
        AW_MPI_VENC_CreateChn(pStreamContext->mVEncChn, &pStreamContext->mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pStreamContext->mVEncChn, &pStreamContext->mVEncRcParam);

#ifdef ENABLE_VENC_GET_DIRMVINFO
        pStreamContext->mDirMvParam.dir_mv_en = 1;
        pStreamContext->mDirMvParam.update_interval = 1;
        ret = AW_MPI_VENC_SetDirMvParam(pStreamContext->mVEncChn, &pStreamContext->mDirMvParam);
        if (ret == SUCCESS)
        {
            alogd("AW_MPI_VENC_SetDirMvParam success, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
        else
        {
            aloge("AW_MPI_VENC_SetDirMvParam fail, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
#endif

#ifdef ENABLE_VENC_ADVANCED_PARAM
        /* set framerate in AW_MPI_VENC_CreateChn */
        //AW_MPI_VENC_SetFrameRate(pStreamContext->mVEncChn, &pStreamContext->mVEncFrameRateConfig);
        if (0 == pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 1);
        }
        if (0 == pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 1);
        }
        setVencSuperFrameCfg(pStreamContext->mVEncChn, pContext->mConfigPara.mThreeEncodeBitrate, pContext->mConfigPara.mThreeEncodeFrameRate);
#else
        if (pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 0);
        }
        if (pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 0);
        }
        alogd("VencChn[%d] use the default venc params for product mode %d", pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.RcAttr.mProductMode);
#endif
        setVencVui(pStreamContext);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pStreamContext->mVEncChn, &cbInfo);
        if (pStreamContext->mVipp != pContext->mSubStream.mVipp)
        {
            AW_MPI_VI_RegisterCallback(pStreamContext->mVipp, &cbInfo);
        }

        if (pStreamContext->mIspAndVeLinkageEnable)
        {
            VENC_IspVeLinkAttr stIspVeLinkAttr;
            memset(&stIspVeLinkAttr, 0, sizeof(VENC_IspVeLinkAttr));
            stIspVeLinkAttr.bEnableIsp2Ve = TRUE;
            stIspVeLinkAttr.bEnableVe2Isp = TRUE; //3rd camera main stream enable Ve2isp
            stIspVeLinkAttr.nVipp = pStreamContext->mVipp;
        #ifdef SOFTWARE_TDM_FOR_ISP
            stIspVeLinkAttr.nVirtualSource = 1;
        #endif
            AW_MPI_VENC_EnableIspVeLink(pStreamContext->mVEncChn, &stIspVeLinkAttr);
            alogd("VencChn[%d] ispVeLink:%d-%d-%d-%d", pStreamContext->mVEncChn, stIspVeLinkAttr.bEnableIsp2Ve, stIspVeLinkAttr.bEnableVe2Isp,
                stIspVeLinkAttr.nVipp, stIspVeLinkAttr.nVirtualSource);

            if (stIspVeLinkAttr.bEnableVe2Isp == TRUE)
            {
                setRegionDetectLink(pStreamContext->mVEncChn, pContext->mConfigPara.mRegionLinkEnable,
                    pContext->mConfigPara.mRegionLinkTexDetectEnable, pContext->mConfigPara.mRegionLinkMotionDetectEnable,
                    pContext->mConfigPara.mRegionLinkMotionDetectInv);
            }
        }

        if (pContext->mConfigPara.mVbrOptEnable)
        {
            setRcVbrOptParam(pStreamContext->mVEncChn, pContext->mConfigPara.mVbrOptRcPriority, pContext->mConfigPara.mVbrOptRcQualityLevel);
        }

        if (pStreamContext->mCameraAdaptiveMovingAndStaticEnable)
        {
            setVencLensMovingMaxQp(pStreamContext->mVEncChn, pContext->mConfigPara.mVencLensMovingMaxQp);
        }

        MPP_CHN_S ViChn = {MOD_ID_VIU, pStreamContext->mVipp, pStreamContext->mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pStreamContext->mVEncChn};
        AW_MPI_SYS_Bind(&ViChn, &VeChn);
        alogd("subStream vipp:%d viChn:%d veChn:%d", pStreamContext->mVipp, pStreamContext->mViChn, pStreamContext->mVEncChn);
    }
    if (pContext->mConfigPara.mThree2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mThree2ndStream;
        pStreamContext->priv = (void*)pContext;
        configThree2ndStream(pStreamContext, &pContext->mConfigPara);
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            cdx_sem_init(&pStreamContext->mSemPdetFrameReady, 0);
            pthread_mutex_init(&pStreamContext->mPdetLock, NULL);
            if (0 != motion_pdet_init(&pStreamContext->mMotionPdetParam))
            {
                aloge("fatal error! motion_pdet_init failed!");
            }
        }
#endif

        AW_MPI_VI_CreateVipp(pStreamContext->mVipp);

#ifndef SOFTWARE_TDM_FOR_ISP
        AW_MPI_VI_SetVippAttr(pStreamContext->mVipp, &pStreamContext->mViAttr);

        AW_MPI_VI_EnableVipp(pStreamContext->mVipp);

        //AW_MPI_VI_SetVippMirror(pStreamContext->mVipp, 1);
        //AW_MPI_VI_SetVippFlip(pStreamContext->mVipp, 1);

#endif
#ifndef SUPPORT_SAVE_STREAM
        if (strlen(pContext->mConfigPara.mThree2ndFilePath) > 0)
        {
            pStreamContext->mFile = fopen(pContext->mConfigPara.mThree2ndFilePath, "wb");
            if(NULL == pStreamContext->mFile)
            {
                aloge("fatal error! why open file[%s] fail? errno is %d", pContext->mConfigPara.mThree2ndFilePath, errno);
            }
        }
        else
        {
            pStreamContext->mFile = NULL;
        }
#else
        if (strlen(pContext->mConfigPara.mThree2ndFilePath) > 0)
        {
            pStreamContext->mRecordHandler = pStreamContext->mVEncChn;
        }
#endif

        AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mViChn, NULL);
        AW_MPI_VENC_CreateChn(pStreamContext->mVEncChn, &pStreamContext->mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pStreamContext->mVEncChn, &pStreamContext->mVEncRcParam);

#ifdef ENABLE_VENC_GET_DIRMVINFO
        pStreamContext->mDirMvParam.dir_mv_en = 1;
        pStreamContext->mDirMvParam.update_interval = 1;
        ret = AW_MPI_VENC_SetDirMvParam(pStreamContext->mVEncChn, &pStreamContext->mDirMvParam);
        if (ret == SUCCESS)
        {
            alogd("AW_MPI_VENC_SetDirMvParam success, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
        else
        {
            aloge("AW_MPI_VENC_SetDirMvParam fail, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
#endif

#ifdef ENABLE_VENC_ADVANCED_PARAM
        /* set framerate in AW_MPI_VENC_CreateChn */
        //AW_MPI_VENC_SetFrameRate(pStreamContext->mVEncChn, &pStreamContext->mVEncFrameRateConfig);
        if (0 == pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 1);
        }
        if (0 == pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 1);
        }
        setVencSuperFrameCfg(pStreamContext->mVEncChn, pContext->mConfigPara.mThree2ndEncodeBitrate, pContext->mConfigPara.mThree2ndEncodeFrameRate);
#else
        if (pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 0);
        }
        if (pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 0);
        }
        alogd("VencChn[%d] use the default venc params for product mode %d", pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.RcAttr.mProductMode);
#endif
        setVencVui(pStreamContext);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pStreamContext->mVEncChn, &cbInfo);
        AW_MPI_VI_RegisterCallback(pStreamContext->mVipp, &cbInfo);

        if (pStreamContext->mIspAndVeLinkageEnable)
        {
            VENC_IspVeLinkAttr stIspVeLinkAttr;
            memset(&stIspVeLinkAttr, 0, sizeof(VENC_IspVeLinkAttr));
            stIspVeLinkAttr.bEnableIsp2Ve = TRUE;
            stIspVeLinkAttr.bEnableVe2Isp = FALSE; //3rd camera sub stream disable Ve2isp
            stIspVeLinkAttr.nVipp = pStreamContext->mVipp;
            AW_MPI_VENC_EnableIspVeLink(pStreamContext->mVEncChn, &stIspVeLinkAttr);
            alogd("VencChn[%d] ispVeLink:%d-%d-%d", pStreamContext->mVEncChn, stIspVeLinkAttr.bEnableIsp2Ve, stIspVeLinkAttr.bEnableVe2Isp,
                stIspVeLinkAttr.nVipp);

            if (stIspVeLinkAttr.bEnableVe2Isp == TRUE)
            {
                setRegionDetectLink(pStreamContext->mVEncChn, pContext->mConfigPara.mRegionLinkEnable,
                    pContext->mConfigPara.mRegionLinkTexDetectEnable, pContext->mConfigPara.mRegionLinkMotionDetectEnable,
                    pContext->mConfigPara.mRegionLinkMotionDetectInv);
            }
        }

        if (pContext->mConfigPara.mVbrOptEnable)
        {
            setRcVbrOptParam(pStreamContext->mVEncChn, pContext->mConfigPara.mVbrOptRcPriority, pContext->mConfigPara.mVbrOptRcQualityLevel);
        }

        if (pStreamContext->mCameraAdaptiveMovingAndStaticEnable)
        {
            setVencLensMovingMaxQp(pStreamContext->mVEncChn, pContext->mConfigPara.mVencLensMovingMaxQp);
        }

        MPP_CHN_S ViChn = {MOD_ID_VIU, pStreamContext->mVipp, pStreamContext->mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pStreamContext->mVEncChn};
        AW_MPI_SYS_Bind(&ViChn, &VeChn);
        alogd("sub2ndStream vipp:%d viChn:%d veChn:%d", pStreamContext->mVipp, pStreamContext->mViChn, pStreamContext->mVEncChn);
    }
    if (pContext->mConfigPara.mFourEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mFourStream;
        pStreamContext->priv = (void*)pContext;
        configFourStream(pStreamContext, &pContext->mConfigPara);
        AW_MPI_VI_CreateVipp(pStreamContext->mVipp);
        AW_MPI_VI_SetVippAttr(pStreamContext->mVipp, &pStreamContext->mViAttr);

        if (100 <= pContext->mConfigPara.mFourIspD3dLbcRatio && 400 >= pContext->mConfigPara.mFourIspD3dLbcRatio)
        {
            alogd("Isp[%d] set D3dLbcRatio %d", pStreamContext->mIsp, pContext->mConfigPara.mFourIspD3dLbcRatio);
            AW_MPI_ISP_SetD3dLbcRatio(pStreamContext->mIsp, pContext->mConfigPara.mFourIspD3dLbcRatio);
        }

        AW_MPI_ISP_Run(pStreamContext->mIsp);
        AW_MPI_ISP_SetIspAlgoFreq(pStreamContext->mIsp, pStreamContext->mIspAlgoFreq);

        if (-1 != pContext->mConfigPara.mFourIspTdmRawProcessType)
        {
            tdm_raw_process_config_param tdm_config_param;
            memset(&tdm_config_param, 0, sizeof(tdm_raw_process_config_param));
            tdm_config_param.type = pContext->mConfigPara.mFourIspTdmRawProcessType;
            tdm_config_param.width = pContext->mConfigPara.mFourIspTdmRawWidth;
            tdm_config_param.height = pContext->mConfigPara.mFourIspTdmRawHeight;
            tdm_config_param.frame_cnt_min = pContext->mConfigPara.mFourIspTdmRawProcessFrameCntMin;
            tdm_config_param.frame_cnt_max = pContext->mConfigPara.mFourIspTdmRawProcessFrameCntMax;
            strncpy(tdm_config_param.tdm_raw_file_path, pContext->mConfigPara.mFourIspTdmRawFilePath, TDM_RAW_PROCESS_FILE_PATH_MAX_LEN);
            tdm_raw_process_open(pStreamContext->mIsp, &tdm_config_param);
            tdm_raw_process_start(pStreamContext->mIsp);
        }

        AW_MPI_VI_EnableVipp(pStreamContext->mVipp);

        AW_MPI_VI_SetVippMirror(pStreamContext->mVipp, pContext->mConfigPara.mFourViMirror);
        AW_MPI_VI_SetVippFlip(pStreamContext->mVipp, pContext->mConfigPara.mFourViFlip);

        alogd("four set vipp%d Mirror=%d Flip=%d after enable vipp", pStreamContext->mVipp,
            pContext->mConfigPara.mFourViMirror, pContext->mConfigPara.mFourViFlip);

#ifdef SUPPORT_SAVE_STREAM
        if (strlen(pContext->mConfigPara.mFourFilePath) > 0)
        {
            pStreamContext->mRecordHandler = pStreamContext->mVEncChn;
        }
#endif

        AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mViChn, NULL);
        AW_MPI_VENC_CreateChn(pStreamContext->mVEncChn, &pStreamContext->mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pStreamContext->mVEncChn, &pStreamContext->mVEncRcParam);

#ifdef ENABLE_VENC_GET_DIRMVINFO
        pStreamContext->mDirMvParam.dir_mv_en = 1;
        pStreamContext->mDirMvParam.update_interval = 1;
        ret = AW_MPI_VENC_SetDirMvParam(pStreamContext->mVEncChn, &pStreamContext->mDirMvParam);
        if (ret == SUCCESS)
        {
            alogd("AW_MPI_VENC_SetDirMvParam success, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
        else
        {
            aloge("AW_MPI_VENC_SetDirMvParam fail, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
#endif

#ifdef ENABLE_VENC_ADVANCED_PARAM
        /* set framerate in AW_MPI_VENC_CreateChn */
        //AW_MPI_VENC_SetFrameRate(pStreamContext->mVEncChn, &pStreamContext->mVEncFrameRateConfig);
        if (0 == pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 1);
        }
        if (0 == pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 1);
        }
        setVencSuperFrameCfg(pStreamContext->mVEncChn, pContext->mConfigPara.mFourEncodeBitrate, pContext->mConfigPara.mFourEncodeFrameRate);
#else
        if (pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 0);
        }
        if (pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 0);
        }
        alogd("VencChn[%d] use the default venc params for product mode %d", pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.RcAttr.mProductMode);
#endif
        setVencVui(pStreamContext);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pStreamContext->mVEncChn, &cbInfo);
        AW_MPI_VI_RegisterCallback(pStreamContext->mVipp, &cbInfo);

        if (pStreamContext->mIspAndVeLinkageEnable)
        {
            VENC_IspVeLinkAttr stIspVeLinkAttr;
            memset(&stIspVeLinkAttr, 0, sizeof(VENC_IspVeLinkAttr));
            stIspVeLinkAttr.bEnableIsp2Ve = TRUE;
            stIspVeLinkAttr.bEnableVe2Isp = TRUE; //4th camera main stream enable Ve2isp
            stIspVeLinkAttr.nVipp = pStreamContext->mVipp;
            AW_MPI_VENC_EnableIspVeLink(pStreamContext->mVEncChn, &stIspVeLinkAttr);
            alogd("VencChn[%d] ispVeLink:%d-%d-%d", pStreamContext->mVEncChn, stIspVeLinkAttr.bEnableIsp2Ve, stIspVeLinkAttr.bEnableVe2Isp,
                stIspVeLinkAttr.nVipp);

            if (stIspVeLinkAttr.bEnableVe2Isp == TRUE)
            {
                setRegionDetectLink(pStreamContext->mVEncChn, pContext->mConfigPara.mRegionLinkEnable,
                    pContext->mConfigPara.mRegionLinkTexDetectEnable, pContext->mConfigPara.mRegionLinkMotionDetectEnable,
                    pContext->mConfigPara.mRegionLinkMotionDetectInv);
            }
        }

        if (pContext->mConfigPara.mVbrOptEnable)
        {
            setRcVbrOptParam(pStreamContext->mVEncChn, pContext->mConfigPara.mVbrOptRcPriority, pContext->mConfigPara.mVbrOptRcQualityLevel);
        }

        if (pStreamContext->mCameraAdaptiveMovingAndStaticEnable)
        {
            setVencLensMovingMaxQp(pStreamContext->mVEncChn, pContext->mConfigPara.mVencLensMovingMaxQp);
        }

        MPP_CHN_S ViChn = {MOD_ID_VIU, pStreamContext->mVipp, pStreamContext->mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pStreamContext->mVEncChn};
        AW_MPI_SYS_Bind(&ViChn, &VeChn);
        alogd("subStream vipp:%d viChn:%d veChn:%d", pStreamContext->mVipp, pStreamContext->mViChn, pStreamContext->mVEncChn);
    }
    if (pContext->mConfigPara.mFour2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mFour2ndStream;
        pStreamContext->priv = (void*)pContext;
        configFour2ndStream(pStreamContext, &pContext->mConfigPara);
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            cdx_sem_init(&pStreamContext->mSemPdetFrameReady, 0);
            pthread_mutex_init(&pStreamContext->mPdetLock, NULL);
            if (0 != motion_pdet_init(&pStreamContext->mMotionPdetParam))
            {
                aloge("fatal error! motion_pdet_init failed!");
            }
        }
#endif
        AW_MPI_VI_CreateVipp(pStreamContext->mVipp);
        AW_MPI_VI_SetVippAttr(pStreamContext->mVipp, &pStreamContext->mViAttr);

        AW_MPI_VI_EnableVipp(pStreamContext->mVipp);

        //AW_MPI_VI_SetVippMirror(pStreamContext->mVipp, 1);
        //AW_MPI_VI_SetVippFlip(pStreamContext->mVipp, 1);

#ifndef SUPPORT_SAVE_STREAM
        if (strlen(pContext->mConfigPara.mFour2ndFilePath) > 0)
        {
            pStreamContext->mFile = fopen(pContext->mConfigPara.mFour2ndFilePath, "wb");
            if(NULL == pStreamContext->mFile)
            {
                aloge("fatal error! why open file[%s] fail? errno is %d", pContext->mConfigPara.mFour2ndFilePath, errno);
            }
        }
        else
        {
            pStreamContext->mFile = NULL;
        }
#else
        if (strlen(pContext->mConfigPara.mFour2ndFilePath) > 0)
        {
            pStreamContext->mRecordHandler = pStreamContext->mVEncChn;
        }
#endif

        AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mViChn, NULL);
        AW_MPI_VENC_CreateChn(pStreamContext->mVEncChn, &pStreamContext->mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pStreamContext->mVEncChn, &pStreamContext->mVEncRcParam);

#ifdef ENABLE_VENC_GET_DIRMVINFO
        pStreamContext->mDirMvParam.dir_mv_en = 1;
        pStreamContext->mDirMvParam.update_interval = 1;
        ret = AW_MPI_VENC_SetDirMvParam(pStreamContext->mVEncChn, &pStreamContext->mDirMvParam);
        if (ret == SUCCESS)
        {
            alogd("AW_MPI_VENC_SetDirMvParam success, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
        else
        {
            aloge("AW_MPI_VENC_SetDirMvParam fail, mVEncChn:%d\n", pStreamContext->mVEncChn);
        }
#endif

#ifdef ENABLE_VENC_ADVANCED_PARAM
        /* set framerate in AW_MPI_VENC_CreateChn */
        //AW_MPI_VENC_SetFrameRate(pStreamContext->mVEncChn, &pStreamContext->mVEncFrameRateConfig);
        if (0 == pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 1);
        }
        if (0 == pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 1);
        }
        setVencSuperFrameCfg(pStreamContext->mVEncChn, pContext->mConfigPara.mFour2ndEncodeBitrate, pContext->mConfigPara.mFour2ndEncodeFrameRate);
#else
        if (pContext->mConfigPara.mVe2DNRDisable)
        {
            setVenc2Dnr(pStreamContext->mVEncChn, 0);
        }
        if (pContext->mConfigPara.mVe3DNRDisable)
        {
            setVenc3Dnr(pStreamContext->mVEncChn, 0);
        }
        alogd("VencChn[%d] use the default venc params for product mode %d", pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.RcAttr.mProductMode);
#endif
        setVencVui(pStreamContext);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pStreamContext->mVEncChn, &cbInfo);
        AW_MPI_VI_RegisterCallback(pStreamContext->mVipp, &cbInfo);

        if (pStreamContext->mIspAndVeLinkageEnable)
        {
            VENC_IspVeLinkAttr stIspVeLinkAttr;
            memset(&stIspVeLinkAttr, 0, sizeof(VENC_IspVeLinkAttr));
            stIspVeLinkAttr.bEnableIsp2Ve = TRUE;
            stIspVeLinkAttr.bEnableVe2Isp = FALSE; //4th camera sub stream disable Ve2isp
            stIspVeLinkAttr.nVipp = pStreamContext->mVipp;
            AW_MPI_VENC_EnableIspVeLink(pStreamContext->mVEncChn, &stIspVeLinkAttr);
            alogd("VencChn[%d] ispVeLink:%d-%d-%d", pStreamContext->mVEncChn, stIspVeLinkAttr.bEnableIsp2Ve, stIspVeLinkAttr.bEnableVe2Isp,
                stIspVeLinkAttr.nVipp);

            if (stIspVeLinkAttr.bEnableVe2Isp == TRUE)
            {
                setRegionDetectLink(pStreamContext->mVEncChn, pContext->mConfigPara.mRegionLinkEnable,
                    pContext->mConfigPara.mRegionLinkTexDetectEnable, pContext->mConfigPara.mRegionLinkMotionDetectEnable,
                    pContext->mConfigPara.mRegionLinkMotionDetectInv);
            }
        }

        if (pContext->mConfigPara.mVbrOptEnable)
        {
            setRcVbrOptParam(pStreamContext->mVEncChn, pContext->mConfigPara.mVbrOptRcPriority, pContext->mConfigPara.mVbrOptRcQualityLevel);
        }

        if (pStreamContext->mCameraAdaptiveMovingAndStaticEnable)
        {
            setVencLensMovingMaxQp(pStreamContext->mVEncChn, pContext->mConfigPara.mVencLensMovingMaxQp);
        }

        MPP_CHN_S ViChn = {MOD_ID_VIU, pStreamContext->mVipp, pStreamContext->mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pStreamContext->mVEncChn};
        AW_MPI_SYS_Bind(&ViChn, &VeChn);
        alogd("sub2ndStream vipp:%d viChn:%d veChn:%d", pStreamContext->mVipp, pStreamContext->mViChn, pStreamContext->mVEncChn);
    }

    return ret;
}

static int start(SampleSmartIPCDemoContext *pContext)
{
    int result = 0;

    if (pContext->mConfigPara.mMainEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mMainStream;

        AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);

        if (pContext->mConfigPara.mMainIspTestEnable) {
            if (pContext->mConfigPara.mMainIspTestIntervalMs == 0) {
                alogw("mIspTestIntervalMs is 0, it will set to 1000\n");
                pContext->mConfigPara.mMainIspTestIntervalMs = 1000;
            }
            memset(&pStreamContext->mIspTestCfg, 0, sizeof(IspApiTestCtrlConfig));
            pStreamContext->mIspTestCfg.mTetstIspChannelId = pStreamContext->mIsp;
            pStreamContext->mIspTestCfg.mTestIntervalMs = pContext->mConfigPara.mMainIspTestIntervalMs;
            pStreamContext->mIspTestCfg.mIspApiTask.mtaskSchedule = TEST_ISP_ALL;
            pStreamContext->mIspTestCfg.mTestAeCfg.mRes.Width = pStreamContext->mViAttr.format.width;
            pStreamContext->mIspTestCfg.mTestAeCfg.mRes.Height = pStreamContext->mViAttr.format.height;
            ispApiTestInit(&pStreamContext->mIspTestCfg);
        }

        if (pContext->mConfigPara.mMainDetectMipiDeskEnable) {
            memset(&pStreamContext->mDetMipiDeskCtrlCfg, 0, sizeof(DetMipiDeskCtrlConfig));
            pStreamContext->mDetMipiDeskCtrlCfg.mDetectIntervalMs = pContext->mConfigPara.mMainDetectIntervalMs;
            pStreamContext->mDetMipiDeskCtrlCfg.mdetMipiDeskEnable =pContext->mConfigPara.mMainDetectMipiDeskEnable;
            if (pContext->mConfigPara.mMainMipiChannel) {
                pStreamContext->mDetMipiDeskCtrlCfg.mMipiDeskCfg.mPayLoadAddr = MIPIB_PAYLOAD_REGS;
            } else {
                pStreamContext->mDetMipiDeskCtrlCfg.mMipiDeskCfg.mPayLoadAddr = MIPIA_PAYLOAD_REGS;
            }
            pStreamContext->mDetMipiDeskCtrlCfg.mMipiDeskCfg.mPhyDeskValueMax = MIPI_PHY_DESKEW_CLK_DLY_MAX;
            detectMipiDeskTestInit(&pStreamContext->mDetMipiDeskCtrlCfg);
        }

#ifdef SUPPORT_OFFLINE_RAW_SIMULATE
        if (pContext->mConfigPara.mMainOfflineRawSimulateType >= 0)
        {
            result = pthread_create(&pContext->SendRawThreadId, NULL, SendRawThread, pContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
        }
#endif


        getVencSpsPpsInfo(pStreamContext);
        AW_MPI_VENC_StartRecvPic(pStreamContext->mVEncChn);

#ifdef SUPPORT_STREAM_OSD_TEST
        if (pContext->mConfigPara.mMainDrawOSDText)
        {
            CreateStreamOSD(&pContext->mStreamOsdConfig, pStreamContext->mVEncChn, pContext->mConfigPara.mMainDrawOSDText, 16, 32, 0);
        }
#endif

#ifdef SUPPORT_RTSP_TEST
        if (pContext->mConfigPara.mMainRtspID >= 0)
        {
            RtspServerAttr rtsp_attr;
            memset(&rtsp_attr, 0, sizeof(RtspServerAttr));
            rtsp_attr.net_type = pContext->mConfigPara.mRtspNetType;

            if (PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_H264;
            else if (PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_H265;
            else
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_LAST;

            rtsp_attr.frame_rate = pContext->mConfigPara.mMainEncodeFrameRate;

            result = rtsp_open(pContext->mConfigPara.mMainRtspID, &rtsp_attr);
            if (result)
            {
                aloge("Do rtsp_open fail! ret:%d \n", result);
                return -1;
            }
        }
#endif
#ifdef SUPPORT_MOTION_SEARCH
        pStreamContext->bMotionSearchEnable = pContext->mConfigPara.motionAlarm_on;
        if (pStreamContext->bMotionSearchEnable)
        {
            // motion search must be enabled when VE is running.
            memset(&pStreamContext->mMotionParam, 0, sizeof(VencMotionSearchParam));
            if (pContext->mConfigPara.motionAlarm_useDefaultCfgEnable)
            {
                pStreamContext->mMotionParam.en_motion_search = 1;
                pStreamContext->mMotionParam.update_interval = pContext->mConfigPara.motionAlarm_update_interval;
                pStreamContext->mMotionParam.dis_default_para = 0;
            }
            else
            {
                pStreamContext->mMotionParam.en_motion_search = 1;
                pStreamContext->mMotionParam.update_interval = pContext->mConfigPara.motionAlarm_update_interval;
                pStreamContext->mMotionParam.dis_default_para = 1;
                pStreamContext->mMotionParam.hor_region_num = pContext->mConfigPara.motionAlarm_HorizontalRegionNum;
                pStreamContext->mMotionParam.ver_region_num = pContext->mConfigPara.motionAlarm_VerticalRegionNum;
                pStreamContext->mMotionParam.en_check_mv = 0;
                pStreamContext->mMotionParam.en_check_mad = 1;
                pStreamContext->mMotionParam.en_morpholog = 0;
                pStreamContext->mMotionParam.large_mv_th = 20;
                pStreamContext->mMotionParam.large_mad_th = 20;
                pStreamContext->mMotionParam.background_weight = 0;
                pStreamContext->mMotionParam.large_mv_ratio_th = 12.0f;
                pStreamContext->mMotionParam.non_zero_mv_ratio_th = 20.0f;
                pStreamContext->mMotionParam.large_mad_ratio_th = 10.0f;
            }
            AW_MPI_VENC_SetMotionSearchParam(pStreamContext->mVEncChn, &pStreamContext->mMotionParam);
            alogd("enable MotionSearch, update_interval:%d fps", pStreamContext->mMotionParam.update_interval);
            if (pContext->mConfigPara.motionAlarm_useDefaultCfgEnable)
            {
                // get the default hor_region_num and ver_region_num
                AW_MPI_VENC_GetMotionSearchParam(pStreamContext->mVEncChn, &pStreamContext->mMotionParam);
            }
            unsigned int size = pStreamContext->mMotionParam.hor_region_num * pStreamContext->mMotionParam.ver_region_num * sizeof(VencMotionSearchRegion);
            if (0 < size)
            {
                pStreamContext->mMotionResult.region = (VencMotionSearchRegion *)malloc(size);
                if (NULL == pStreamContext->mMotionResult.region)
                {
                    aloge("fatal error! malloc region failed! size=%d", size);
                }
                else
                {
                    memset(pStreamContext->mMotionResult.region, 0, size);
                }
            }
            else
            {
                aloge("fatal error! invalid size=%d, hor_region_num:%d, ver_region_num:%d", size, pStreamContext->mMotionParam.hor_region_num, pStreamContext->mMotionParam.ver_region_num);
            }
        }
#endif
        result = pthread_create(&pStreamContext->mStreamThreadId, NULL, getVencStreamThread, (void*)pStreamContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
    }
    if (pContext->mConfigPara.mMain2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mMain2ndStream;
        AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);

        getVencSpsPpsInfo(pStreamContext);
        AW_MPI_VENC_StartRecvPic(pStreamContext->mVEncChn);

        result = pthread_create(&pStreamContext->mStreamThreadId, NULL, getVencStreamThread, (void*)pStreamContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
#ifdef SUPPORT_TAKE_PICTURE
        if (pStreamContext->mTakePicture)
        {
#ifndef SUPPORT_RECREATE_JPEG
            AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, NULL);
            AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
#endif
            result = pthread_create(&pStreamContext->mCSIFrameThreadId, NULL, takePictureThread, (void*)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
        }
#endif
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            pStreamContext->mPdetRunInterval = pContext->mConfigPara.mMain2ndPdetRunIinterval;
            AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn, NULL);
            AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            result = pthread_create(&pStreamContext->mPdetRunThreadId, NULL, getPdetRunThread, (void *)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
            result = pthread_create(&pStreamContext->mGetPdetCSIThreadId, NULL, getPdetCSIThread, (void *)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
        }
#endif
    }

    if (pContext->mConfigPara.mSubEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mSubStream;
        AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);

        getVencSpsPpsInfo(pStreamContext);
        AW_MPI_VENC_StartRecvPic(pStreamContext->mVEncChn);

#ifdef SUPPORT_STREAM_OSD_TEST
        if (pContext->mConfigPara.mSubDrawOSDText)
        {
            CreateStreamOSD(&pContext->mStreamOsdConfig, pStreamContext->mVEncChn, pContext->mConfigPara.mSubDrawOSDText, 16, 32, 1);
        }
#endif

#ifdef SUPPORT_RTSP_TEST
        if (pContext->mConfigPara.mSubRtspID >= 0)
        {
            RtspServerAttr rtsp_attr;
            memset(&rtsp_attr, 0, sizeof(RtspServerAttr));
            rtsp_attr.net_type = pContext->mConfigPara.mRtspNetType;

            if (PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_H264;
            else if (PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_H265;
            else
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_LAST;

            rtsp_attr.frame_rate = pContext->mConfigPara.mSubEncodeFrameRate;

            result = rtsp_open(pContext->mConfigPara.mSubRtspID, &rtsp_attr);
            if (result)
            {
                aloge("Do rtsp_open fail! ret:%d \n", result);
                return -1;
            }
        }
#endif
        result = pthread_create(&pStreamContext->mStreamThreadId, NULL, getVencStreamThread, (void*)pStreamContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
    }
    if (pContext->mConfigPara.mSub2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mSub2ndStream;
        AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);

        getVencSpsPpsInfo(pStreamContext);
        AW_MPI_VENC_StartRecvPic(pStreamContext->mVEncChn);

        result = pthread_create(&pStreamContext->mStreamThreadId, NULL, getVencStreamThread, (void*)pStreamContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
#ifdef SUPPORT_TAKE_PICTURE
        if (pStreamContext->mTakePicture)
        {
#ifndef SUPPORT_RECREATE_JPEG
            AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, NULL);
            AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
#endif
            result = pthread_create(&pStreamContext->mCSIFrameThreadId, NULL, takePictureThread, (void*)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
        }
#endif
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            pStreamContext->mPdetRunInterval = pContext->mConfigPara.mSub2ndPdetRunIinterval;
            AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn, NULL);
            AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            result = pthread_create(&pStreamContext->mPdetRunThreadId, NULL, getPdetRunThread, (void *)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
            result = pthread_create(&pStreamContext->mGetPdetCSIThreadId, NULL, getPdetCSIThread, (void *)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
        }
#endif
    }

    if (pContext->mConfigPara.mThreeEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mThreeStream;
        AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);

        getVencSpsPpsInfo(pStreamContext);
        AW_MPI_VENC_StartRecvPic(pStreamContext->mVEncChn);

#ifdef SUPPORT_STREAM_OSD_TEST
        if (pContext->mConfigPara.mThreeDrawOSDText)
        {
            CreateStreamOSD(&pContext->mStreamOsdConfig, pStreamContext->mVEncChn, pContext->mConfigPara.mThreeDrawOSDText, 16, 32, 2);
        }
#endif

#ifdef SUPPORT_RTSP_TEST
        if (pContext->mConfigPara.mThreeRtspID >= 0)
        {
            RtspServerAttr rtsp_attr;
            memset(&rtsp_attr, 0, sizeof(RtspServerAttr));
            rtsp_attr.net_type = pContext->mConfigPara.mRtspNetType;

            if (PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_H264;
            else if (PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_H265;
            else
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_LAST;

            rtsp_attr.frame_rate = pContext->mConfigPara.mThreeEncodeFrameRate;

            result = rtsp_open(pContext->mConfigPara.mThreeRtspID, &rtsp_attr);
            if (result)
            {
                aloge("Do rtsp_open fail! ret:%d \n", result);
                return -1;
            }
        }
#endif
        result = pthread_create(&pStreamContext->mStreamThreadId, NULL, getVencStreamThread, (void*)pStreamContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
    }
    if (pContext->mConfigPara.mThree2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mThree2ndStream;
        AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);

        getVencSpsPpsInfo(pStreamContext);
        AW_MPI_VENC_StartRecvPic(pStreamContext->mVEncChn);

        result = pthread_create(&pStreamContext->mStreamThreadId, NULL, getVencStreamThread, (void*)pStreamContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
#ifdef SUPPORT_TAKE_PICTURE
        if (pStreamContext->mTakePicture)
        {
#ifndef SUPPORT_RECREATE_JPEG
            AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, NULL);
            AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
#endif
            result = pthread_create(&pStreamContext->mCSIFrameThreadId, NULL, takePictureThread, (void*)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
        }
#endif
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            pStreamContext->mPdetRunInterval = pContext->mConfigPara.mThree2ndPdetRunIinterval;
            AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn, NULL);
            AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            result = pthread_create(&pStreamContext->mPdetRunThreadId, NULL, getPdetRunThread, (void *)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
            result = pthread_create(&pStreamContext->mGetPdetCSIThreadId, NULL, getPdetCSIThread, (void *)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
        }
#endif
    }

    if (pContext->mConfigPara.mFourEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mFourStream;
        AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);

        getVencSpsPpsInfo(pStreamContext);
        AW_MPI_VENC_StartRecvPic(pStreamContext->mVEncChn);

#ifdef SUPPORT_STREAM_OSD_TEST
        if (pContext->mConfigPara.mFourDrawOSDText)
        {
            CreateStreamOSD(&pContext->mStreamOsdConfig, pStreamContext->mVEncChn, pContext->mConfigPara.mFourDrawOSDText, 16, 32, 3);
        }
#endif

#ifdef SUPPORT_RTSP_TEST
        if (pContext->mConfigPara.mFourRtspID >= 0)
        {
            RtspServerAttr rtsp_attr;
            memset(&rtsp_attr, 0, sizeof(RtspServerAttr));
            rtsp_attr.net_type = pContext->mConfigPara.mRtspNetType;

            if (PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_H264;
            else if (PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_H265;
            else
                rtsp_attr.video_type = RTSP_VIDEO_TYPE_LAST;

            rtsp_attr.frame_rate = pContext->mConfigPara.mFourEncodeFrameRate;

            result = rtsp_open(pContext->mConfigPara.mFourRtspID, &rtsp_attr);
            if (result)
            {
                aloge("Do rtsp_open fail! ret:%d \n", result);
                return -1;
            }
        }
#endif
        result = pthread_create(&pStreamContext->mStreamThreadId, NULL, getVencStreamThread, (void*)pStreamContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
    }
    if (pContext->mConfigPara.mFour2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mFour2ndStream;
        AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);

        getVencSpsPpsInfo(pStreamContext);
        AW_MPI_VENC_StartRecvPic(pStreamContext->mVEncChn);

        result = pthread_create(&pStreamContext->mStreamThreadId, NULL, getVencStreamThread, (void*)pStreamContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
#ifdef SUPPORT_TAKE_PICTURE
        if (pStreamContext->mTakePicture)
        {
#ifndef SUPPORT_RECREATE_JPEG
            AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn, NULL);
            AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
#endif
            result = pthread_create(&pStreamContext->mCSIFrameThreadId, NULL, takePictureThread, (void*)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
        }
#endif
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            pStreamContext->mPdetRunInterval = pContext->mConfigPara.mFour2ndPdetRunIinterval;
            AW_MPI_VI_CreateVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn, NULL);
            AW_MPI_VI_EnableVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            result = pthread_create(&pStreamContext->mPdetRunThreadId, NULL, getPdetRunThread, (void *)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
            result = pthread_create(&pStreamContext->mGetPdetCSIThreadId, NULL, getPdetCSIThread, (void *)pStreamContext);
            if (result != 0)
            {
                aloge("fatal error! pthread create fail[%d]", result);
            }
        }
#endif
    }

    return result;
}

static int stop(SampleSmartIPCDemoContext *pContext)
{
    int ret = 0;
    void *pRetVal = NULL;

    //v821 online encode must stop bk1 then stop bk0, because bk1 use frame counter of bk0.
    if (pContext->mConfigPara.mMain2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mMain2ndStream;
        pthread_join(pStreamContext->mStreamThreadId, &pRetVal);
        alogd("main2ndStream pRetVal=%p", pRetVal);
        if (pStreamContext->mTakePicture)
        {
            pthread_join(pStreamContext->mCSIFrameThreadId, &pRetVal);
            alogd("main2nd CSIFrame pRetVal=%p", pRetVal);
#ifndef SUPPORT_RECREATE_JPEG
            AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
            AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
#endif
        }
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            pthread_join(pStreamContext->mGetPdetCSIThreadId, &pRetVal);
            alogd("main2nd GetPdetCSI pRetVal=%p", pRetVal);
            cdx_sem_up(&pStreamContext->mSemPdetFrameReady);
            pthread_join(pStreamContext->mPdetRunThreadId, &pRetVal);
            alogd("main2nd PdetRun pRetVal=%p", pRetVal);
            AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            pthread_mutex_destroy(&pStreamContext->mMotionPdetParam.mPdetLock);
            cdx_sem_deinit(&pStreamContext->mMotionPdetParam.mSemPdetFrameReady);
        }
#endif
        AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        AW_MPI_VENC_StopRecvPic(pStreamContext->mVEncChn);
        AW_MPI_VENC_ResetChn(pStreamContext->mVEncChn);
        AW_MPI_VENC_DestroyChn(pStreamContext->mVEncChn);
        AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if(pStreamContext->mFile)
        {
            fclose(pStreamContext->mFile);
            pStreamContext->mFile = NULL;
        }
    }
    if (pContext->mConfigPara.mMainEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mMainStream;
        pthread_join(pStreamContext->mStreamThreadId, &pRetVal);
        alogd("mainStream pRetVal=%p", pRetVal);

#ifdef SUPPORT_OFFLINE_RAW_SIMULATE
        if (pContext->mConfigPara.mMainOfflineRawSimulateType >= 0)
        {
            pthread_join(pContext->SendRawThreadId, &pRetVal);
            alogd("SendRaw pRetVal=%p", pRetVal);
        }
#endif

        if (-1 != pContext->mConfigPara.mMainIspTdmRawProcessType)
        {
            tdm_raw_process_stop(pStreamContext->mIsp);
            tdm_raw_process_close(pStreamContext->mIsp);
        }

        AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if (pStreamContext->bMotionSearchEnable)
        {
            memset(&pStreamContext->mMotionParam, 0, sizeof(VencMotionSearchParam));
            pStreamContext->mMotionParam.en_motion_search = 0;
            AW_MPI_VENC_SetMotionSearchParam(pStreamContext->mVEncChn, &pStreamContext->mMotionParam);
            pStreamContext->bMotionSearchEnable = FALSE;
            alogd("disable MotionSearch");
        }
        AW_MPI_VENC_StopRecvPic(pStreamContext->mVEncChn);
        AW_MPI_VENC_ResetChn(pStreamContext->mVEncChn);
        AW_MPI_VENC_DestroyChn(pStreamContext->mVEncChn);
        AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if(pStreamContext->mFile)
        {
            fclose(pStreamContext->mFile);
            pStreamContext->mFile = NULL;
        }
#ifdef SUPPORT_STREAM_OSD_TEST
        if (pContext->mConfigPara.mMainDrawOSDText)
        {
            DestroyStreamOSD(&pContext->mStreamOsdConfig, pStreamContext->mVEncChn, 0);
        }
#endif
        if (pStreamContext->mMotionResult.region)
        {
            aloge("free pContext->mMotionResult.region");
            free(pStreamContext->mMotionResult.region);
            pStreamContext->mMotionResult.region = NULL;
        }
    }
    if (pContext->mConfigPara.mSub2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mSub2ndStream;
        pthread_join(pStreamContext->mStreamThreadId, &pRetVal);
        alogd("sub2ndStream pRetVal=%p", pRetVal);
        if (1 == pStreamContext->mTakePicture)
        {
            pthread_join(pStreamContext->mCSIFrameThreadId, &pRetVal);
            alogd("sub2nd CSIFrame pRetVal=%p", pRetVal);
#ifndef SUPPORT_RECREATE_JPEG
            AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
            AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
#endif
        }
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            pthread_join(pStreamContext->mGetPdetCSIThreadId, &pRetVal);
            alogd("sub2nd GetPdetCSI pRetVal=%p", pRetVal);
            cdx_sem_up(&pStreamContext->mSemPdetFrameReady);
            pthread_join(pStreamContext->mPdetRunThreadId, &pRetVal);
            alogd("sub2nd PdetRun pRetVal=%p", pRetVal);
            AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            pthread_mutex_destroy(&pStreamContext->mMotionPdetParam.mPdetLock);
            cdx_sem_deinit(&pStreamContext->mMotionPdetParam.mSemPdetFrameReady);
        }
#endif
        AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        AW_MPI_VENC_StopRecvPic(pStreamContext->mVEncChn);
        AW_MPI_VENC_ResetChn(pStreamContext->mVEncChn);
        AW_MPI_VENC_DestroyChn(pStreamContext->mVEncChn);
        AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if(pStreamContext->mFile)
        {
            fclose(pStreamContext->mFile);
            pStreamContext->mFile = NULL;
        }
    }
    if (pContext->mConfigPara.mSubEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mSubStream;
        pthread_join(pStreamContext->mStreamThreadId, &pRetVal);
        alogd("subStream pRetVal=%p", pRetVal);

        if (-1 != pContext->mConfigPara.mSubIspTdmRawProcessType)
        {
            tdm_raw_process_stop(pStreamContext->mIsp);
            tdm_raw_process_close(pStreamContext->mIsp);
        }

        AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        AW_MPI_VENC_StopRecvPic(pStreamContext->mVEncChn);
        AW_MPI_VENC_ResetChn(pStreamContext->mVEncChn);
        AW_MPI_VENC_DestroyChn(pStreamContext->mVEncChn);
        AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if(pStreamContext->mFile)
        {
            fclose(pStreamContext->mFile);
            pStreamContext->mFile = NULL;
        }
#ifdef SUPPORT_STREAM_OSD_TEST
        if (pContext->mConfigPara.mSubDrawOSDText)
        {
            DestroyStreamOSD(&pContext->mStreamOsdConfig, pStreamContext->mVEncChn, 1);
        }
#endif
    }
    if (pContext->mConfigPara.mThree2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mThree2ndStream;
        pthread_join(pStreamContext->mStreamThreadId, &pRetVal);
        alogd("three2ndStream pRetVal=%p", pRetVal);
        if (1 == pStreamContext->mTakePicture)
        {
            pthread_join(pStreamContext->mCSIFrameThreadId, &pRetVal);
            alogd("three2nd CSIFrame pRetVal=%p", pRetVal);
#ifndef SUPPORT_RECREATE_JPEG
            AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
            AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
#endif
        }
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            pthread_join(pStreamContext->mGetPdetCSIThreadId, &pRetVal);
            alogd("three2nd GetPdetCSI pRetVal=%p", pRetVal);
            cdx_sem_up(&pStreamContext->mSemPdetFrameReady);
            pthread_join(pStreamContext->mPdetRunThreadId, &pRetVal);
            alogd("three2nd PdetRun pRetVal=%p", pRetVal);
            AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            pthread_mutex_destroy(&pStreamContext->mMotionPdetParam.mPdetLock);
            cdx_sem_deinit(&pStreamContext->mMotionPdetParam.mSemPdetFrameReady);
        }
#endif
        AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        AW_MPI_VENC_StopRecvPic(pStreamContext->mVEncChn);
        AW_MPI_VENC_ResetChn(pStreamContext->mVEncChn);
        AW_MPI_VENC_DestroyChn(pStreamContext->mVEncChn);
        AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if(pStreamContext->mFile)
        {
            fclose(pStreamContext->mFile);
            pStreamContext->mFile = NULL;
        }
    }
    if (pContext->mConfigPara.mThreeEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mThreeStream;
        pthread_join(pStreamContext->mStreamThreadId, &pRetVal);
        alogd("threeStream pRetVal=%p", pRetVal);

        if (-1 != pContext->mConfigPara.mThreeIspTdmRawProcessType)
        {
            tdm_raw_process_stop(pStreamContext->mIsp);
            tdm_raw_process_close(pStreamContext->mIsp);
        }

        AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        AW_MPI_VENC_StopRecvPic(pStreamContext->mVEncChn);
        AW_MPI_VENC_ResetChn(pStreamContext->mVEncChn);
        AW_MPI_VENC_DestroyChn(pStreamContext->mVEncChn);
        AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if(pStreamContext->mFile)
        {
            fclose(pStreamContext->mFile);
            pStreamContext->mFile = NULL;
        }
#ifdef SUPPORT_STREAM_OSD_TEST
        if (pContext->mConfigPara.mThreeDrawOSDText)
        {
            DestroyStreamOSD(&pContext->mStreamOsdConfig, pStreamContext->mVEncChn, 2);
        }
#endif
    }
    if (pContext->mConfigPara.mFour2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mFour2ndStream;
        pthread_join(pStreamContext->mStreamThreadId, &pRetVal);
        alogd("four2ndStream pRetVal=%p", pRetVal);
        if (1 == pStreamContext->mTakePicture)
        {
            pthread_join(pStreamContext->mCSIFrameThreadId, &pRetVal);
            alogd("four2nd CSIFrame pRetVal=%p", pRetVal);
#ifndef SUPPORT_RECREATE_JPEG
            AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
            AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mTakePictureViChn);
#endif
        }
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            pthread_join(pStreamContext->mGetPdetCSIThreadId, &pRetVal);
            alogd("four2nd GetPdettCSI pRetVal=%p", pRetVal);
            cdx_sem_up(&pStreamContext->mSemPdetFrameReady);
            pthread_join(pStreamContext->mPdetRunThreadId, &pRetVal);
            alogd("four2nd PdetRun pRetVal=%p", pRetVal);
            AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mPdetViChn);
            pthread_mutex_destroy(&pStreamContext->mMotionPdetParam.mPdetLock);
            cdx_sem_deinit(&pStreamContext->mMotionPdetParam.mSemPdetFrameReady);
        }
#endif
        AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        AW_MPI_VENC_StopRecvPic(pStreamContext->mVEncChn);
        AW_MPI_VENC_ResetChn(pStreamContext->mVEncChn);
        AW_MPI_VENC_DestroyChn(pStreamContext->mVEncChn);
        AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if(pStreamContext->mFile)
        {
            fclose(pStreamContext->mFile);
            pStreamContext->mFile = NULL;
        }
    }
    if (pContext->mConfigPara.mFourEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mFourStream;
        pthread_join(pStreamContext->mStreamThreadId, &pRetVal);
        alogd("fourStream pRetVal=%p", pRetVal);

        if (-1 != pContext->mConfigPara.mFourIspTdmRawProcessType)
        {
            tdm_raw_process_stop(pStreamContext->mIsp);
            tdm_raw_process_close(pStreamContext->mIsp);
        }

        AW_MPI_VI_DisableVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        AW_MPI_VENC_StopRecvPic(pStreamContext->mVEncChn);
        AW_MPI_VENC_ResetChn(pStreamContext->mVEncChn);
        AW_MPI_VENC_DestroyChn(pStreamContext->mVEncChn);
        AW_MPI_VI_DestroyVirChn(pStreamContext->mVipp, pStreamContext->mViChn);
        if(pStreamContext->mFile)
        {
            fclose(pStreamContext->mFile);
            pStreamContext->mFile = NULL;
        }
#ifdef SUPPORT_STREAM_OSD_TEST
        if (pContext->mConfigPara.mFourDrawOSDText)
        {
            DestroyStreamOSD(&pContext->mStreamOsdConfig, pStreamContext->mVEncChn, 3);
        }
#endif
    }

    return ret;
}

#ifdef SUPPORT_PERSON_DETECT
static int pdetdestroy(VencStreamContext *pStreamContext)
{
    int ret = 0;
    SamplePdetInfo *pPdetInfo = &pStreamContext->mPdetInfo;
    SampleDewbInfo *pDewbInfo = &pStreamContext->mDewbInfo;

    pdet_deinit(pPdetInfo);
    De_Deinit(pDewbInfo);

    return ret;
}
#endif

static int destroy(SampleSmartIPCDemoContext *pContext)
{
    int ret = 0;

    if (pContext->mConfigPara.mMainEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mMainStream;

        if (pContext->mConfigPara.mMainIspTestEnable) {
            ispApiTestExit(&pStreamContext->mIspTestCfg);
        }

        if (pContext->mConfigPara.mMainDetectMipiDeskEnable) {
            detectMipiDeskTestExit(&pStreamContext->mDetMipiDeskCtrlCfg);
        }

        if (pStreamContext->mViAttr.large_dma_merge_en >= DMA_STITCH_HORIZONTAL) {
            alogd("It will close video%d when stitch_mode >= DMA_STITCH_HORIZONTAL\n", pStreamContext->mVipp - 1);
            AW_MPI_VI_DisableVipp(pStreamContext->mVipp - 1);
        }

        AW_MPI_VI_DisableVipp(pStreamContext->mVipp);
        AW_MPI_ISP_Stop(pStreamContext->mIsp);
        if (pStreamContext->mViAttr.large_dma_merge_en > DMA_STITCH_NONE)
            AW_MPI_ISP_Stop(pContext->mConfigPara.mMainViStitchIspChannelId);
        AW_MPI_VI_DestroyVipp(pStreamContext->mVipp);
#ifdef SUPPORT_RTSP_TEST
        if (pContext->mConfigPara.mMainRtspID >= 0)
        {
            rtsp_stop(pContext->mConfigPara.mMainRtspID);
            rtsp_close(pContext->mConfigPara.mMainRtspID);
        }
#endif
#ifdef SUPPORT_OFFLINE_RAW_SIMULATE
        if (pContext->mConfigPara.mMainOfflineRawSimulateType >= 0)
        {
            close_video_ptn(&pContext->mMainStream.mViAttr.mPtnCfg);
        }
#endif
    }
    if (pContext->mConfigPara.mMain2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mMain2ndStream;
        if (pStreamContext->mViAttr.large_dma_merge_en >= DMA_STITCH_HORIZONTAL) {
            alogd("It will close video%d when stitch_mode >= DMA_STITCH_HORIZONTAL\n", pStreamContext->mVipp - 1);
            AW_MPI_VI_DisableVipp(pStreamContext->mVipp - 1);
        }
        AW_MPI_VI_DisableVipp(pStreamContext->mVipp);
        AW_MPI_ISP_Stop(pStreamContext->mIsp);
        AW_MPI_VI_DestroyVipp(pStreamContext->mVipp);
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            // pdetdestroy(pStreamContext);
            motion_pdet_deinit(&pStreamContext->mMotionPdetParam);
        }
#endif
    }
    if (pContext->mConfigPara.mSubEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mSubStream;
        AW_MPI_VI_DisableVipp(pStreamContext->mVipp);
        AW_MPI_ISP_Stop(pStreamContext->mIsp);
        AW_MPI_VI_DestroyVipp(pStreamContext->mVipp);
#ifdef SUPPORT_RTSP_TEST
        if (pContext->mConfigPara.mSubRtspID >= 0)
        {
            rtsp_stop(pContext->mConfigPara.mSubRtspID);
            rtsp_close(pContext->mConfigPara.mSubRtspID);
        }
#endif
    }
    if (pContext->mConfigPara.mSub2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mSub2ndStream;
        AW_MPI_VI_DisableVipp(pStreamContext->mVipp);
        AW_MPI_ISP_Stop(pStreamContext->mIsp);
        AW_MPI_VI_DestroyVipp(pStreamContext->mVipp);
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            // pdetdestroy(pStreamContext);
            motion_pdet_deinit(&pStreamContext->mMotionPdetParam);
        }
#endif
    }
    if (pContext->mConfigPara.mThreeEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mThreeStream;
        AW_MPI_VI_DisableVipp(pStreamContext->mVipp);
        AW_MPI_ISP_Stop(pStreamContext->mIsp);
        AW_MPI_VI_DestroyVipp(pStreamContext->mVipp);
#ifdef SUPPORT_RTSP_TEST
        if (pContext->mConfigPara.mThreeRtspID >= 0)
        {
            rtsp_stop(pContext->mConfigPara.mThreeRtspID);
            rtsp_close(pContext->mConfigPara.mThreeRtspID);
        }
#endif
    }
    if (pContext->mConfigPara.mThree2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mThree2ndStream;
        AW_MPI_VI_DisableVipp(pStreamContext->mVipp);
        AW_MPI_ISP_Stop(pStreamContext->mIsp);
        AW_MPI_VI_DestroyVipp(pStreamContext->mVipp);
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            // pdetdestroy(pStreamContext);
            motion_pdet_deinit(&pStreamContext->mMotionPdetParam);
        }
#endif
    }
    if (pContext->mConfigPara.mFourEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mFourStream;
        AW_MPI_VI_DisableVipp(pStreamContext->mVipp);
        AW_MPI_ISP_Stop(pStreamContext->mIsp);
        AW_MPI_VI_DestroyVipp(pStreamContext->mVipp);
#ifdef SUPPORT_RTSP_TEST
        if (pContext->mConfigPara.mFourRtspID >= 0)
        {
            rtsp_stop(pContext->mConfigPara.mFourRtspID);
            rtsp_close(pContext->mConfigPara.mFourRtspID);
        }
#endif
    }
    if (pContext->mConfigPara.mFour2ndEnable)
    {
        VencStreamContext *pStreamContext = &pContext->mFour2ndStream;
        AW_MPI_VI_DisableVipp(pStreamContext->mVipp);
        AW_MPI_ISP_Stop(pStreamContext->mIsp);
        AW_MPI_VI_DestroyVipp(pStreamContext->mVipp);
#ifdef SUPPORT_PERSON_DETECT
        if (pStreamContext->mPdetEnable)
        {
            // pdetdestroy(pStreamContext);
            motion_pdet_deinit(&pStreamContext->mMotionPdetParam);
        }
#endif
    }

    return ret;
}

int main(int argc, char *argv[])
{
    int result = 0;
    ERRORTYPE ret;

    SampleSmartIPCDemoContext *pContext = (SampleSmartIPCDemoContext*)malloc(sizeof(SampleSmartIPCDemoContext));
    if (NULL == pContext)
    {
        aloge("fatal error! malloc pContext failed! size=%d", sizeof(SampleSmartIPCDemoContext));
        return -1;
    }
    gpSampleSmartIPCDemoContext = pContext;
    memset(pContext, 0, sizeof(SampleSmartIPCDemoContext));
    cdx_sem_init(&pContext->mSemExit, 0);

    /* register process function for SIGINT, to exit program. */
    if (signal(SIGINT, handle_exit) == SIG_ERR)
    {
        aloge("can't catch SIGSEGV");
    }

    if (message_create(&pContext->mMsgQueue) < 0)
    {
        aloge("message create fail!");
        goto _exit;
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
        aloge("fatal error! sys Init failed! ret=%d", ret);
        goto _exit;
    }

#ifdef SUPPORT_SAVE_STREAM
    record_init();
    pContext->mMainStream.mRecordHandler = -1;
    pContext->mMain2ndStream.mRecordHandler = -1;
    pContext->mSubStream.mRecordHandler = -1;
    pContext->mSub2ndStream.mRecordHandler = -1;
    pContext->mThreeStream.mRecordHandler = -1;
    pContext->mThree2ndStream.mRecordHandler = -1;
    pContext->mFourStream.mRecordHandler = -1;
    pContext->mFour2ndStream.mRecordHandler = -1;
#endif

    pContext->mStreamOsdConfig.mOverlayDrawStreamOSDBase = 100;

    prepare(pContext);

    //create msg queue thread
    result = pthread_create(&pContext->mMsgQueueThreadId, NULL, MsgQueueThread, pContext);
    if (result != 0)
    {
        aloge("fatal error! pthread create fail[%d]", result);
    }

    // start
    start(pContext);

#ifdef SUPPORT_IR_CUT_SWITCH_TEST
    if (pContext->mConfigPara.mIrCutSwitchTestEnable)
    {
        result = pthread_create(&pContext->mCGIrcutId, NULL, CGIRCUTThread, pContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
    }
#endif
#ifdef SUPPORT_SENSOR_FLIP_MIRROR
        result = pthread_create(&pContext->mSensorFlipThreadId, NULL, FlipSwitchThread, pContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
#endif

    // wb yuv
    if (pContext->mConfigPara.mWbYuvEnable)
    {
        result = pthread_create(&pContext->mWbYuvThreadId, NULL, getWbYuvThread, (void*)pContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
    }

    // test trigger code
    if (pContext->mConfigPara.mTestTriggerViTimeout)
    {
        result = pthread_create(&pContext->mTestTriggerThreadId, NULL, testTriggerThread, (void*)pContext);
        if (result != 0)
        {
            aloge("fatal error! pthread create fail[%d]", result);
        }
    }

#ifdef SUPPORT_AUIDO_TEST
    if (pContext->mConfigPara.audio_test_enable)
        audio_test_init(pContext);
#endif

    // wait for test end
    if (pContext->mConfigPara.mTestDuration > 0)
    {
        cdx_sem_down_timedwait(&pContext->mSemExit, pContext->mConfigPara.mTestDuration*1000);
    }
    else
    {
        cdx_sem_down(&pContext->mSemExit);
    }

#ifdef SUPPORT_AUIDO_TEST
    if (pContext->mConfigPara.audio_test_enable)
        audio_test_exit(pContext);
#endif

    pContext->mbExitFlag = 1;

    message_t stMsgCmd;
    stMsgCmd.command = MsgQueue_Stop;
    put_message(&pContext->mMsgQueue, &stMsgCmd);
    pthread_join(pContext->mMsgQueueThreadId, NULL);

    void *pRetVal = NULL;

    if (pContext->mConfigPara.mTestTriggerViTimeout)
    {
        pthread_join(pContext->mTestTriggerThreadId, &pRetVal);
        alogd("test pRetVal=%p", pRetVal);
    }

    if (pContext->mConfigPara.mWbYuvEnable)
    {
        pthread_join(pContext->mWbYuvThreadId, &pRetVal);
        alogd("WbYuv pRetVal=%p", pRetVal);
    }

#ifdef SUPPORT_IR_CUT_SWITCH_TEST
    if (pContext->mConfigPara.mIrCutSwitchTestEnable)
    {
        pthread_join(pContext->mCGIrcutId, &pRetVal);
        alogd("Ircut pRetVal=%p", pRetVal);
    }
#endif
#ifdef SUPPORT_SENSOR_FLIP_MIRROR
        pthread_join(pContext->mSensorFlipThreadId, &pRetVal);
        alogd("Flip pRetVal=%p", pRetVal);
#endif
    // stop
    stop(pContext);

    // deinit
    destroy(pContext);

#ifdef SUPPORT_SAVE_STREAM
    record_exit();
#endif

    ret = AW_MPI_SYS_Exit();
    if (ret != SUCCESS)
    {
        aloge("fatal error! sys exit failed!");
    }

_exit:
    cdx_sem_deinit(&pContext->mSemExit);
    message_destroy(&pContext->mMsgQueue);
    if(pContext!=NULL)
    {
        free(pContext);
        pContext = NULL;
    }
    gpSampleSmartIPCDemoContext = NULL;
    alogd("%s test result: %s", argv[0], ((0 == result) ? "success" : "fail"));
    return result;
}
