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
#ifndef _SAMPLE_RTSP_H_
#define _SAMPLE_RTSP_H_

#include <plat_type.h>
#include <tsemaphore.h>

#define MAX_FILE_PATH_SIZE  (256)

#define INVALID_OVERLAY_HANDLE  (-1)

typedef enum ISP_DEV_TYPE_E
{
    ISP_DEV_MAIN = 0,
    ISP_DEV_SUB  = 1
} ISP_DEV_TYPE_E;

typedef enum VI_DEV_TYPE_E
{
    VI_DEV_MAIN = 0,
    VI_DEV_SUB  = 1,
    VI_DEV_JPEG = 2,
    VI_DEV_TYPE_MAX
} VI_DEV_TYPE_E;

typedef enum VENC_CHN_TYPE_E
{
    VENC_CHN_MAIN_STREAM      = 0,
    VENC_CHN_SUB_STREAM       = 1,
    VENC_CHN_SUB_LAPSE_STREAM = 2,
    VENC_CHN_TYPE_MAX
} VENC_CHN_TYPE_E;

typedef struct SampleRtspCmdLineParam
{
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
}SampleRtspCmdLineParam;

typedef struct SampleRtspConfig
{
    // main stream
    ISP_DEV mMainIsp;
    VI_DEV mMainVipp;
    VI_CHN mMainViChn;
    int mMainSrcWidth;
    int mMainSrcHeight;
    PIXEL_FORMAT_E mMainPixelFormat;
    int mMainWdrEnable;
    int mMainViBufNum;
    int mMainSrcFrameRate;
    VENC_CHN mMainVEncChn;
    PAYLOAD_TYPE_E mMainEncodeType;
    int mMainEncodeWidth;
    int mMainEncodeHeight;
    int mMainEncodeFrameRate;
    int mMainEncodeBitrate;
    char mMainFilePath[MAX_FILE_PATH_SIZE];
    int mMainOnlineEnable;
    int mMainOnlineShareBufNum;
    BOOL mMainEncppEnable;

    // sub stream
    ISP_DEV mSubIsp;
    VI_DEV mSubVipp;
    VI_CHN mSubViChn;
    int mSubSrcWidth;
    int mSubSrcHeight;
    PIXEL_FORMAT_E mSubPixelFormat;
    int mSubWdrEnable;
    int mSubViBufNum;
    int mSubSrcFrameRate;
    VENC_CHN mSubVEncChn;
    PAYLOAD_TYPE_E mSubEncodeType;
    int mSubEncodeWidth;
    int mSubEncodeHeight;
    int mSubEncodeFrameRate;
    int mSubEncodeBitrate;
    char mSubFilePath[MAX_FILE_PATH_SIZE];
    int mSubEncppSharpAttenCoefPer;
    BOOL mSubVippCropEnable;
    int mSubVippCropRectX;
    int mSubVippCropRectY;
    int mSubVippCropRectWidth;
    int mSubVippCropRectHeight;
    int mSubOnlineEnable;
    int mSubOnlineShareBufNum;
    BOOL mSubEncppEnable;

    // sub lapse stream
    VI_DEV mSubLapseVipp;
    VI_CHN mSubLapseViChn;
    int mSubLapseSrcWidth;
    int mSubLapseSrcHeight;
    PIXEL_FORMAT_E mSubLapsePixelFormat;
    int mSubLapseWdrEnable;
    int mSubLapseViBufNum;
    int mSubLapseSrcFrameRate;
    VENC_CHN mSubLapseVEncChn;
    PAYLOAD_TYPE_E mSubLapseEncodeType;
    int mSubLapseEncodeWidth;
    int mSubLapseEncodeHeight;
    int mSubLapseEncodeFrameRate;
    int mSubLapseEncodeBitrate;
    char mSubLapseFilePath[MAX_FILE_PATH_SIZE];
    int mSubLapseEncppSharpAttenCoefPer;
    int mSubLapseTime;
    BOOL mSubLapseEncppEnable;

    // isp and ve linkage
    BOOL mIspAndVeLinkageEnable;
    BOOL mCameraAdaptiveMovingAndStaticEnable;
    int mVencLensMovingMaxQp;
    // wb yuv
    int mWbYuvEnable;
    int mWbYuvBufNum;
    int mWbYuvScalerRatio;
    int mWbYuvStartIndex;
    int mWbYuvTotalCnt;
    int mWbYuvStreamChn;
    char mWbYuvFilePath[MAX_FILE_PATH_SIZE];
    // rtsp
    int mRtspNetType;
    int mStreamBufSize;
    // others
    int mTestDuration;  //unit:s, 0 means infinite.
}SampleRtspConfig;

typedef struct VencStreamContext
{
    pthread_t mStreamThreadId;
    ISP_DEV mIsp;
    VI_DEV mVipp;
    VI_CHN mViChn;
    VI_ATTR_S mViAttr;
    VENC_CHN mVEncChn;
    VENC_CHN_ATTR_S mVEncChnAttr;
    VENC_RC_PARAM_S mVEncRcParam;
    VENC_FRAME_RATE_S mVEncFrameRateConfig;
    BOOL mbSubLapseEnable;
    int mStreamDataCnt;
    FILE* mFile;
    void *priv;
    int mEncppSharpAttenCoefPer;
}VencStreamContext;

typedef struct VencOsdContext
{
    pthread_t mStreamThreadId;
    RGN_HANDLE mOverlayHandle[VENC_MAX_CHN_NUM];
    PIXEL_FORMAT_E mPixelFormat;
    void *priv;
}VencOsdContext;

typedef struct SampleRtspContext
{
    SampleRtspCmdLineParam mCmdLinePara;
    SampleRtspConfig mConfigPara;

    cdx_sem_t mSemExit;
    int mbExitFlag;

    BOOL mMainIspRunFlag;
    BOOL mSubIspRunFlag;

    VencStreamContext mMainStream;
    VencStreamContext mSubStream;
    VencStreamContext mSubLapseStream;

    pthread_t mWbYuvThreadId;
}SampleRtspContext;

#endif  /* _SAMPLE_RTSP_H_ */

