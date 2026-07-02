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
#ifndef _SAMPLE_SMART_IPC_DEMO_H_
#define _SAMPLE_SMART_IPC_DEMO_H_

#include <plat_type.h>
#include <tsemaphore.h>
#include <mpi_region.h>
#include "rgb_ctrl.h"
#include "tmessage.h"

#include "sample_common_venc.h"
#include "aiservice_pdet.h"
#include "de_hw_scale.h"
#include "sample_common_isp.h"
#include "motion_detect_common.h"
#include "motion_pdet.h"
#include "motion_detect.h"

#define MAX_FILE_PATH_SIZE  (256)
#define RECREATE_RECORD_HANDLER_BASE    (0xff)

#define INVALID_OVERLAY_HANDLE  (-1)

typedef enum SmartIPCMsgType
{
    Vi_Timeout,
    MsgQueue_Stop,
}SmartIPCMsgType;

typedef struct SampleSmartIPCDemoCmdLineParam
{
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
}SampleSmartIPCDemoCmdLineParam;

typedef struct MotionDetectZoneInfo
{
    int left_up_x;
    int left_up_y;
    int left_bottom_x;
    int left_bottom_y;
    int right_bottom_x;
    int right_bottom_y;
    int right_up_x;
    int right_up_y;
}MotionDetectZoneInfo;

typedef enum VencRecreateState
{
    VENC_RECREATE_IDLE,
    VENC_RECREATE_ACTIVE,
    VENC_RECREATE_DONE,
} VencRecreateState;

typedef struct VencRecreateDynamicConfig
{
    int mVencRecreateEnable;
    int mVencRecreateLoopCnt;
    enum VencRecreateState mVencRecreateFlag;
    int mVencRecreateInterval;
    int mVideoEncoderFmt;
    int mVideoFrameRate;
    int mVideoBitRate;
    int dstWidth;
    int dstHeight;
    int mKeyFrameInterval;
} VencRecreateDynamicConfig;

typedef struct SampleSmartIPCDemoConfig
{
    unsigned int mMultiSensorOnlineEnable;
    // rtsp
    int mRtspNetType;
    // common params
    int mVeRecRefBufReduceEnable;
    int mStreamBufSize;
    int mProductMode;
    int mVbrOptEnable;
    int mVbrOptRcPriority;
    int mVbrOptRcQualityLevel;
    int mRcMode;
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
    int mIspColorSpace;
    // ir cut switch test
    int mIrCutSwitchTestEnable;
    int mIrCutSwitchMode;
    int mIrCutSwitchInterval;
    int mIrCutSwitchIsp;
    char mIrCutSwitchIrBinPath[MAX_FILE_PATH_SIZE];
    char mIrCutSwitchRgbBinPath[MAX_FILE_PATH_SIZE];
    // sensor flip test
    int mSensorFlipMirror;
    int mFlipSwitchInterval;
    int mFlipMainVippEnable;
    int mFlipSubVippEnable;
    // audio test
    int audio_test_enable;
    int capture_en;
    int capture_sample_rate;
    int capture_bit_witdh;
    int capture_channel_cnt;
    int capture_ans_en;
    int capture_agc_en;
    int capture_aec_en;
    char capture_save_file[MAX_FILE_PATH_SIZE];
    int capture_save_file_duration;
    int capture_save_file_max_cnt;
    int playback_en;
    int playback_volume;
    char playback_file[MAX_FILE_PATH_SIZE];
    // motion search
    int motionAlarm_on;
    int motionAlarm_result_print_interval;
    int motionAlarm_sensitivity;
    int motionAlarm_support_zone;
    int motionAlarm_left_up_x;
    int motionAlarm_left_up_y;
    int motionAlarm_left_bottom_x;
    int motionAlarm_left_bottom_y;
    int motionAlarm_right_bottom_x;
    int motionAlarm_right_bottom_y;
    int motionAlarm_right_up_x;
    int motionAlarm_right_up_y;
    int motionAlarm_update_interval;
    int motionAlarm_useDefaultCfgEnable;
    int motionAlarm_HorizontalRegionNum;
    int motionAlarm_VerticalRegionNum;
    int motionAlarm_Threshold_High;
    int motionAlarm_Threshold_MediumHigh;
    int motionAlarm_Threshold_Default;
    int motionAlarm_Threshold_MediumLow;
    int motionAlarm_Threshold_Low;
    // main stream
    int mMainEnable;
    int mMainRtspID;
    ISP_DEV mMainIsp;
    int mMainIspAlgoFreq;
    int mMainIspD3dLbcRatio;
    VI_DEV mMainVipp;
    VI_CHN mMainViChn;
    int mMainSrcWidth;
    int mMainSrcHeight;
    PIXEL_FORMAT_E mMainPixelFormat;
    int mMainWdrEnable;
    int mMainViBufNum;
    int mMainSrcFrameRate;
    VENC_CHN mMainVEncChn;
    VencRecreateDynamicConfig mMainEncodeDynamicConfig;
    PAYLOAD_TYPE_E mMainEncodeType;
    int mMainEncodeWidth;
    int mMainEncodeHeight;
    int mMainEncodeFrameRate;
    int mMainEncodeBitrate;
    int mMainOnlineEnable;
    int mMainOnlineShareBufNum;
    unsigned int mMainViStitchMode;
    unsigned int mMainViStitchScalerEn;
    unsigned int mMainViStitchSensorAScalerWidth;
    unsigned int mMainViStitchSensorAScalerheight;
    unsigned int mMainViStitchSensorBScalerWidth;
    unsigned int mMainViStitchSensorBScalerheight;
    unsigned int mMainViStitchIspChannelId;
    unsigned char mMainIspTestEnable;
    unsigned int mMainIspTestIntervalMs;
    unsigned char mMainDetectMipiDeskEnable;
    unsigned int  mMainDetectIntervalMs;
    unsigned char mMainMipiChannel;
    BOOL mMainEncppEnable;
    int mMainVeRefFrameLbcMode;
    int mMainKeyFrameInterval;
    char mMainFilePath[MAX_FILE_PATH_SIZE];
    int mMainSaveOneFileDuration;
    int mMainSaveMaxFileCnt;
    char mMainDrawOSDText[MAX_FILE_PATH_SIZE];
    int mMainViSyncCtrlEnable;
    int mMainVeRxInputBufmultiplexEnable;
    int mMainViMirror;
    int mMainViFlip;
    // main tdm raw
    int mMainIspTdmRawProcessType;
    int mMainIspTdmRawWidth;
    int mMainIspTdmRawHeight;
    int mMainIspTdmRawRxBufNum;
    int mMainIspTdmRawProcessFrameCntMin;
    int mMainIspTdmRawProcessFrameCntMax;
    char mMainIspTdmRawFilePath[MAX_FILE_PATH_SIZE];
    //main offline raw simulate
    int mMainOfflineRawSimulateType;
    int mMainOfflineRawType;
    int mMainOfflineRawSimulateCntStart;
    int mMainOfflineRawSimulateCntEnd;
    char mMainOfflineRawFilePath[MAX_FILE_PATH_SIZE];
    // main 2nd stream
    int mMain2ndEnable;
    VI_DEV mMain2ndVipp;
    VI_CHN mMain2ndViChn;
    int mMain2ndSrcWidth;
    int mMain2ndSrcHeight;
    PIXEL_FORMAT_E mMain2ndPixelFormat;
    int mMain2ndViBufNum;
    int mMain2ndSrcFrameRate;
    VENC_CHN mMain2ndVEncChn;
    PAYLOAD_TYPE_E mMain2ndEncodeType;
    int mMain2ndEncodeWidth;
    int mMain2ndEncodeHeight;
    int mMain2ndEncodeFrameRate;
    int mMain2ndEncodeBitrate;
    int mMain2ndEncppSharpAttenCoefPer;
    int mMain2ndOnlineEnable;
    int mMain2ndOnlineShareBufNum;
    unsigned int mMain2ndViStitchMode;
    unsigned int mMain2ndViStitchScalerEn;
    unsigned int mMain2ndViStitchSensorAScalerWidth;
    unsigned int mMain2ndViStitchSensorAScalerheight;
    unsigned int mMain2ndViStitchSensorBScalerWidth;
    unsigned int mMain2ndViStitchSensorBScalerheight;
    BOOL mMain2ndEncppEnable;
    int mMain2ndVeRefFrameLbcMode;
    int mMain2ndKeyFrameInterval;
    char mMain2ndFilePath[MAX_FILE_PATH_SIZE];
    int mMain2ndSaveOneFileDuration;
    int mMain2ndSaveMaxFileCnt;
    int mMain2ndVeRxInputBufmultiplexEnable;
    int mMain2ndTakePicture;
    int mMain2ndTakePictureViChn;
    int mMain2ndTakePictureVencChn;
    int mMain2ndTakePictureInterval;
    char mMain2ndTakePictureFile[MAX_FILE_PATH_SIZE];
    int mMain2ndTakePictureFileCnt;
    int mMain2ndTakePictureOnlyCaptureYuv;
    // main 2nd pdet
    int mMain2ndPdetEnable;
    int mMain2ndPdetViChn;
    int mMain2ndPdetInputWidth;
    int mMain2ndPdetInputHeight;
    int mMain2ndPdetInputChannel;
    float mMain2ndPdetConfThres;
    int mMain2ndPdetRunIinterval;
    // sub stream
    int mSubEnable;
    int mSubRtspID;
    ISP_DEV mSubIsp;
    int mSubIspAlgoFreq;
    int mSubIspD3dLbcRatio;
    VI_DEV mSubVipp;
    VI_CHN mSubViChn;
    int mSubSrcWidth;
    int mSubSrcHeight;
    PIXEL_FORMAT_E mSubPixelFormat;
    int mSubWdrEnable;
    int mSubViBufNum;
    int mSubSrcFrameRate;
    VENC_CHN mSubVEncChn;
    VencRecreateDynamicConfig mSubEncodeDynamicConfig;
    PAYLOAD_TYPE_E mSubEncodeType;
    int mSubEncodeWidth;
    int mSubEncodeHeight;
    int mSubEncodeFrameRate;
    int mSubEncodeBitrate;
    int mSubEncppSharpAttenCoefPer;
    int mSubOnlineEnable;
    int mSubOnlineShareBufNum;
    BOOL mSubEncppEnable;
    int mSubVeRefFrameLbcMode;
    int mSubKeyFrameInterval;
    char mSubFilePath[MAX_FILE_PATH_SIZE];
    int mSubSaveOneFileDuration;
    int mSubSaveMaxFileCnt;
    char mSubDrawOSDText[MAX_FILE_PATH_SIZE];
    int mSubViMirror;
    int mSubViFlip;
    // sub tdm raw
    int mSubIspTdmRawProcessType;
    int mSubIspTdmRawWidth;
    int mSubIspTdmRawHeight;
    int mSubIspTdmRawRxBufNum;
    int mSubIspTdmRawProcessFrameCntMin;
    int mSubIspTdmRawProcessFrameCntMax;
    char mSubIspTdmRawFilePath[MAX_FILE_PATH_SIZE];
    // sub 2nd stream
    int mSub2ndEnable;
    VI_DEV mSub2ndVipp;
    VI_CHN mSub2ndViChn;
    int mSub2ndSrcWidth;
    int mSub2ndSrcHeight;
    PIXEL_FORMAT_E mSub2ndPixelFormat;
    int mSub2ndViBufNum;
    int mSub2ndSrcFrameRate;
    VENC_CHN mSub2ndVEncChn;
    PAYLOAD_TYPE_E mSub2ndEncodeType;
    int mSub2ndEncodeWidth;
    int mSub2ndEncodeHeight;
    int mSub2ndEncodeFrameRate;
    int mSub2ndEncodeBitrate;
    int mSub2ndEncppSharpAttenCoefPer;
    int mSub2ndOnlineEnable;
    int mSub2ndOnlineShareBufNum;
    BOOL mSub2ndEncppEnable;
    int mSub2ndVeRefFrameLbcMode;
    int mSub2ndKeyFrameInterval;
    char mSub2ndFilePath[MAX_FILE_PATH_SIZE];
    int mSub2ndSaveOneFileDuration;
    int mSub2ndSaveMaxFileCnt;
    int mSub2ndTakePicture;
    int mSub2ndTakePictureViChn;
    int mSub2ndTakePictureVencChn;
    int mSub2ndTakePictureInterval;
    char mSub2ndTakePictureFile[MAX_FILE_PATH_SIZE];
    int mSub2ndTakePictureFileCnt;
    int mSub2ndTakePictureOnlyCaptureYuv;
    // sub 2nd pdet
    int mSub2ndPdetEnable;
    int mSub2ndPdetViChn;
    int mSub2ndPdetInputWidth;
    int mSub2ndPdetInputHeight;
    int mSub2ndPdetInputChannel;
    float mSub2ndPdetConfThres;
    int mSub2ndPdetRunIinterval;
    // three stream
    int mThreeEnable;
    int mThreeRtspID;
    ISP_DEV mThreeIsp;
    int mThreeIspAlgoFreq;
    int mThreeIspD3dLbcRatio;
    VI_DEV mThreeVipp;
    VI_CHN mThreeViChn;
    int mThreeSrcWidth;
    int mThreeSrcHeight;
    PIXEL_FORMAT_E mThreePixelFormat;
    int mThreeWdrEnable;
    int mThreeViBufNum;
    int mThreeSrcFrameRate;
    VENC_CHN mThreeVEncChn;
    VencRecreateDynamicConfig mThreeEncodeDynamicConfig;
    PAYLOAD_TYPE_E mThreeEncodeType;
    int mThreeEncodeWidth;
    int mThreeEncodeHeight;
    int mThreeEncodeFrameRate;
    int mThreeEncodeBitrate;
    int mThreeOnlineEnable;
    int mThreeOnlineShareBufNum;
    BOOL mThreeEncppEnable;
    int mThreeVeRefFrameLbcMode;
    int mThreeKeyFrameInterval;
    char mThreeFilePath[MAX_FILE_PATH_SIZE];
    int mThreeSaveOneFileDuration;
    int mThreeSaveMaxFileCnt;
    char mThreeDrawOSDText[MAX_FILE_PATH_SIZE];
    int mThreeViMirror;
    int mThreeViFlip;
    // three tdm raw
    int mThreeIspTdmRawProcessType;
    int mThreeIspTdmRawWidth;
    int mThreeIspTdmRawHeight;
    int mThreeIspTdmRawRxBufNum;
    int mThreeIspTdmRawProcessFrameCntMin;
    int mThreeIspTdmRawProcessFrameCntMax;
    char mThreeIspTdmRawFilePath[MAX_FILE_PATH_SIZE];
    // three 2nd stream
    int mThree2ndEnable;
    VI_DEV mThree2ndVipp;
    VI_CHN mThree2ndViChn;
    int mThree2ndSrcWidth;
    int mThree2ndSrcHeight;
    PIXEL_FORMAT_E mThree2ndPixelFormat;
    int mThree2ndViBufNum;
    int mThree2ndSrcFrameRate;
    VENC_CHN mThree2ndVEncChn;
    PAYLOAD_TYPE_E mThree2ndEncodeType;
    int mThree2ndEncodeWidth;
    int mThree2ndEncodeHeight;
    int mThree2ndEncodeFrameRate;
    int mThree2ndEncodeBitrate;
    int mThree2ndEncppSharpAttenCoefPer;
    int mThree2ndOnlineEnable;
    int mThree2ndOnlineShareBufNum;
    BOOL mThree2ndEncppEnable;
    int mThree2ndVeRefFrameLbcMode;
    int mThree2ndKeyFrameInterval;
    char mThree2ndFilePath[MAX_FILE_PATH_SIZE];
    int mThree2ndSaveOneFileDuration;
    int mThree2ndSaveMaxFileCnt;
    int mThree2ndTakePicture;
    int mThree2ndTakePictureViChn;
    int mThree2ndTakePictureVencChn;
    int mThree2ndTakePictureInterval;
    char mThree2ndTakePictureFile[MAX_FILE_PATH_SIZE];
    int mThree2ndTakePictureFileCnt;
    int mThree2ndTakePictureOnlyCaptureYuv;
    // three 2nd pdet
    int mThree2ndPdetEnable;
    int mThree2ndPdetViChn;
    int mThree2ndPdetInputWidth;
    int mThree2ndPdetInputHeight;
    int mThree2ndPdetInputChannel;
    float mThree2ndPdetConfThres;
    int mThree2ndPdetRunIinterval;
    // four stream
    int mFourEnable;
    int mFourRtspID;
    ISP_DEV mFourIsp;
    int mFourIspAlgoFreq;
    int mFourIspD3dLbcRatio;
    VI_DEV mFourVipp;
    VI_CHN mFourViChn;
    int mFourSrcWidth;
    int mFourSrcHeight;
    PIXEL_FORMAT_E mFourPixelFormat;
    int mFourWdrEnable;
    int mFourViBufNum;
    int mFourSrcFrameRate;
    VENC_CHN mFourVEncChn;
    VencRecreateDynamicConfig mFourEncodeDynamicConfig;
    PAYLOAD_TYPE_E mFourEncodeType;
    int mFourEncodeWidth;
    int mFourEncodeHeight;
    int mFourEncodeFrameRate;
    int mFourEncodeBitrate;
    int mFourOnlineEnable;
    int mFourOnlineShareBufNum;
    BOOL mFourEncppEnable;
    int mFourVeRefFrameLbcMode;
    int mFourKeyFrameInterval;
    char mFourFilePath[MAX_FILE_PATH_SIZE];
    int mFourSaveOneFileDuration;
    int mFourSaveMaxFileCnt;
    char mFourDrawOSDText[MAX_FILE_PATH_SIZE];
    int mFourViMirror;
    int mFourViFlip;
    // four tdm raw
    int mFourIspTdmRawProcessType;
    int mFourIspTdmRawWidth;
    int mFourIspTdmRawHeight;
    int mFourIspTdmRawRxBufNum;
    int mFourIspTdmRawProcessFrameCntMin;
    int mFourIspTdmRawProcessFrameCntMax;
    char mFourIspTdmRawFilePath[MAX_FILE_PATH_SIZE];
    // four 2nd stream
    int mFour2ndEnable;
    VI_DEV mFour2ndVipp;
    VI_CHN mFour2ndViChn;
    int mFour2ndSrcWidth;
    int mFour2ndSrcHeight;
    PIXEL_FORMAT_E mFour2ndPixelFormat;
    int mFour2ndViBufNum;
    int mFour2ndSrcFrameRate;
    VENC_CHN mFour2ndVEncChn;
    PAYLOAD_TYPE_E mFour2ndEncodeType;
    int mFour2ndEncodeWidth;
    int mFour2ndEncodeHeight;
    int mFour2ndEncodeFrameRate;
    int mFour2ndEncodeBitrate;
    int mFour2ndEncppSharpAttenCoefPer;
    int mFour2ndOnlineEnable;
    int mFour2ndOnlineShareBufNum;
    BOOL mFour2ndEncppEnable;
    int mFour2ndVeRefFrameLbcMode;
    int mFour2ndKeyFrameInterval;
    char mFour2ndFilePath[MAX_FILE_PATH_SIZE];
    int mFour2ndSaveOneFileDuration;
    int mFour2ndSaveMaxFileCnt;
    int mFour2ndTakePicture;
    int mFour2ndTakePictureViChn;
    int mFour2ndTakePictureVencChn;
    int mFour2ndTakePictureInterval;
    char mFour2ndTakePictureFile[MAX_FILE_PATH_SIZE];
    int mFour2ndTakePictureFileCnt;
    int mFour2ndTakePictureOnlyCaptureYuv;
    // four 2nd pdet
    int mFour2ndPdetEnable;
    int mFour2ndPdetViChn;
    int mFour2ndPdetInputWidth;
    int mFour2ndPdetInputHeight;
    int mFour2ndPdetInputChannel;
    float mFour2ndPdetConfThres;
    int mFour2ndPdetRunIinterval;
    // isp and ve linkage
    BOOL mIspAndVeLinkageEnable;
    BOOL mRegionLinkEnable;
    BOOL mRegionLinkTexDetectEnable;
    BOOL mRegionLinkMotionDetectEnable;
    BOOL mCameraAdaptiveMovingAndStaticEnable;
    int mRegionLinkMotionDetectInv;
    int mVencLensMovingMaxQp;
    // wb yuv
    int mWbYuvEnable;
    int mWbYuvBufNum;
    int mWbYuvScalerRatio;
    int mWbYuvStartIndex;
    int mWbYuvTotalCnt;
    int mWbYuvStreamChn;
    char mWbYuvFilePath[MAX_FILE_PATH_SIZE];
    // vi timeout
    int mViTimeoutResetDisable;
    int mTestTriggerViTimeout;
    // encpp sharp debug
    int mEncppSharpDebugDisable;
    // tmd speed down
    int mTdmSpeedDownEnable;
    unsigned char mTdmTxValidNum;
    unsigned char mTdmTxInvalidNum;
    unsigned char mTdmTxValidNumOffset;
    // 2dnr and 3dnr
    int mVe2DNRDisable;
    int mVe3DNRDisable;
    // others
    int mTestDuration;  //unit:s, 0 means infinite.

    // motion pdet
    int mMotionPdetSensitivity;
    int mMotionPdetVerRegionNum;
    int mMotionPdetHorRegionNum;
    int mMotionPdetEnable;
    int mJudgeMotionNum;
    float mMotionPdetThres;
    float mMotionDynamicThres;
}SampleSmartIPCDemoConfig;

typedef struct VencStreamContext
{
    pthread_t mStreamThreadId;
    pthread_t mCSIFrameThreadId;
    pthread_t mPdetRunThreadId;
    pthread_t mGetPdetCSIThreadId;
    pthread_t mVencRecreateThreadId;
    pthread_mutex_t mPdetLock;
    cdx_sem_t mSemPdetFrameReady;
    VIDEO_FRAME_INFO_S mPdetCurFrame;
    ISP_DEV mIsp;
    VI_DEV mVipp;
    VI_CHN mViChn;
    VI_ATTR_S mViAttr;
    VENC_CHN mVEncChn;
    VENC_CHN_ATTR_S mVEncChnAttr;
    VENC_RC_PARAM_S mVEncRcParam;
    VencDirMvParam mDirMvParam;
    VencDirMvInfoResult mDirMvInfoResult;
    VENC_FRAME_RATE_S mVEncFrameRateConfig;
    int mIspAlgoFreq;
    int mStreamDataCnt;
    FILE* mFile;
    void *priv;
    int mEncppSharpAttenCoefPer;
    int mRecordHandler;
    int mIspAndVeLinkageEnable;
    int mCameraAdaptiveMovingAndStaticEnable;
    VencHeaderData mSpsPpsInfo;

    int mPdetEnable;
    VI_CHN mPdetViChn;
    SamplePdetInfo mPdetInfo;
    SampleDewbInfo mDewbInfo;
    int mPdetRunInterval;

    int mTakePicture;
    VI_CHN mTakePictureViChn;
    VENC_CHN mTakePictureVeChn;
    int mTakePictureInterval;
    char mTakePictureFile[MAX_FILE_PATH_SIZE];
    int mTakePictureFileCnt;
    int mTakePictureFileCurCnt;
    int mTakePictureOnlyCaptureYuv;

    BOOL bMotionSearchEnable;
    VencMotionSearchResult mMotionResult;
    VencMotionSearchParam mMotionParam;
    IspApiTestCtrlConfig mIspTestCfg;
    DetMipiDeskCtrlConfig mDetMipiDeskCtrlCfg;

    float total_score[VENC_MAX_CHN_NUM];

    MotionPdetParam mMotionPdetParam;
    unsigned int mVbvBufSize;
}VencStreamContext;

typedef struct VencOsdContext
{
    pthread_t mStreamThreadId;
    RGN_HANDLE mOverlayHandle[VENC_MAX_CHN_NUM];
    PIXEL_FORMAT_E mPixelFormat;
    void *priv;
}VencOsdContext;

struct audio_test_context {
    int ai_dev;
    AI_CHN ai_chn;
    AIO_ATTR_S ai_chn_attr;
    int capture_task_exit;
    int capture_task_running;
    pthread_t capture_task_trd;

    int ao_dev;
    AO_CHN ao_chn;
    int playback_task_exit;
    int playback_task_running;
    pthread_t playback_task_trd;
};

typedef struct SampleSmartIPCDemoContext
{
    SampleSmartIPCDemoCmdLineParam mCmdLinePara;
    SampleSmartIPCDemoConfig mConfigPara;

    cdx_sem_t mSemExit;
    int mbExitFlag;
    int mViresetFlag;

    VencStreamContext mMainStream;
    VencStreamContext mMain2ndStream;
    VencStreamContext mSubStream;
    VencStreamContext mSub2ndStream;
    VencStreamContext mThreeStream;
    VencStreamContext mThree2ndStream;
    VencStreamContext mFourStream;
    VencStreamContext mFour2ndStream;

    StreamOsdConfig mStreamOsdConfig;

    pthread_t mWbYuvThreadId;
	pthread_t mCGIrcutId;
    pthread_t mSensorFlipThreadId;
	pthread_t mMsgQueueThreadId;
	message_queue_t mMsgQueue;

    pthread_t mTestTriggerThreadId;
    pthread_t SendRawThreadId;

    struct audio_test_context audio_test_context;
}SampleSmartIPCDemoContext;

#endif  /* _SAMPLE_SMART_IPC_DEMO_H_ */

