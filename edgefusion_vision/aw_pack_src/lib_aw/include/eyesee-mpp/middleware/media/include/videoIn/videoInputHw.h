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
#ifndef __VIDEOINPUT_HW__
#define __VIDEOINPUT_HW__

//ref platform headers
#include <plat_defines.h>
#include <plat_errno.h>
#include <plat_math.h>
#include <plat_type.h>

//media api headers to app
#include <mm_common.h>
#include <mm_component.h>
#include <tmessage.h>
#include <tsemaphore.h>

#include "mm_comm_vi.h"
#include <mm_comm_region.h>

//#include <OsdGroups.h>
#include <isp_comm.h>
#include <isp_manage.h>
//#include "mpi_isp.h"

#define VI_CSI_NUM_MAX 3
#define VI_ISP_NUM_MAX 5
#define VI_VIPP_NUM_MAX 16 //must == HW_VIDEO_DEVICE_NUM.
#define VI_VIRCHN_NUM_MAX 4

#define VI_HIGH_FRAMERATE_STANDARD (60)    //>60 is high frame rate.
#define MEDIA_DEVICE "/dev/media0"

//typedef enum VI_CHANNEL_PORT_DEFINE_E {
//	VI_CHN_PORT_INDEX_CAP_IN = 0,
//	VI_CHN_PORT_INDEX_FILE_IN, /* FILE_IN : Decode H26x,mpeg data, or yuv file input*/
//	VI_CHN_PORT_INDEX_OUT,
//	VI_CHN_MAX_PORTS,
//} VI_CHANNEL_PORT_DEFINE_E;

typedef struct VI_DMA_LBC_CMP_S {
    unsigned char is_lossy;
    unsigned char bit_depth;
    unsigned char glb_enable;
    unsigned char dts_enable;
    unsigned char ots_enable;
    unsigned char msq_enable;
    unsigned int cmp_ratio_even;
    unsigned int cmp_ratio_odd;
    unsigned int mb_mi_bits[2];
    unsigned char rc_adv[4];
    unsigned char lmtqp_en;
    unsigned char lmtqp_min;
    unsigned char updata_adv_en;
    unsigned char updata_adv_ratio;
    unsigned int line_tar_bits[2];
} VI_DMA_LBC_CMP_S;

typedef struct VI_DMA_LBC_PARAM_S {
    unsigned int frm_wth;
    unsigned int frm_hgt;
    unsigned int frm_x;
    unsigned int frm_y;
    unsigned int line_tar_bits[2];
    unsigned int line_bits;
    unsigned int frm_bits;
    unsigned int mb_wth;
} VI_DMA_LBC_PARAM_S;

typedef struct VI_DMA_LBC_BS_S {
    unsigned char *cur_buf_ptr;
    unsigned int left_bits;
    unsigned int cnt;
    unsigned int sum;
} VI_DMA_LBC_BS_S;

typedef struct VI_DMA_LBC_CFG_S {
    unsigned int frm_wth;
    unsigned int frm_hgt;
    unsigned int is_lossy;
    unsigned int bit_depth;
    unsigned int glb_enable;
    unsigned int dts_enable;
    unsigned int ots_enable;
    unsigned int msq_enable;
    unsigned int line_tar_bits[2];
    unsigned int mb_min_bits[2];
    unsigned int rc_adv[4];
    unsigned int lmtqp_en;
    unsigned int lmtqp_min;
    unsigned int updataadv_en;
    unsigned int updataadv_ratio;
} VI_DMA_LBC_CFG_S;

typedef struct VI_DMA_LBC_STM_S {
    unsigned char *bs;
} VI_DMA_LBC_STM_S;

typedef struct VI_CHN_MAP_S {
    int mThdRunning;
    // VI_DEV mViDev;             
	/* DevX = SensorX; // X = 0,1,2,3 */
	/* ChnX = ScaleX;  // X = 0,1,2...16 */
    VI_CHN mViChn;  //vipp<<16 | virChn
    MM_COMPONENTTYPE *mViComp; /* video vi component instance */
    cdx_sem_t mSemCompCmd;
    MPPCallbackInfo mCallbackInfo;
    struct list_head mList;
} VI_CHN_MAP_S;

typedef struct
{
    VI_DEV mVipp;
    unsigned int mFrameBufId;
    struct list_head mList;
}VippFrame;
typedef struct {
	//pthread_t threadid;
	//pthread_t threadid_gyro;
	//pthread_t threadid_videoPorcess;
    char mThreadName[32];
	VI_DEV  vipp_dev_id; /* vipp id num */
	int	vipp_enable; /* 1:enable; 0:disable. default=0. */
    struct list_head mChnList;  //element type: VI_CHN_MAP_S
    pthread_mutex_t mLock;

    MPPCallbackFuncType mMppCallback;
    void *pAppData;

    //int mVippTimeoutCnt;
    int iDropFrameNum; //left frames to drop.
    int64_t mLastGetFrameTm;    //unit:ms

	int 	refs[32];
    VIDEO_FRAME_INFO_S	VideoFrameInfo[32]; //store frame info from vipp.
    pthread_mutex_t mRefsLock;

    //for debug video frame occupy
    struct list_head mIdleFrameList;    //VippFrame
    struct list_head mReadyFrameList;
    pthread_mutex_t mFrameListLock;

    //osd manage    
    pthread_mutex_t mRegionLock;
    struct list_head mOverlayList;	//ChannelRegionInfo
    struct list_head mCoverList;    //ChannelRegionInfo
    struct list_head mOrlList;    	//ChannelRegionInfo, rectangle line.

    //for osd reconstruct
    //OsdGroups *mpOsdGroups;


    int bTakeLongExpPic;
    int iTakeLongExpRef;
    pthread_mutex_t mLongExpLock;

    long long last_v_frm_pts;

    VI_ATTR_S mstAttr;
    VI_DMA_LBC_CMP_S mLbcCmp;
    unsigned char *mLbcFillDataAddr;
    unsigned int mLbcFillDataLen;

    int yuv_expand_head_bytes;
} viChnManager;

typedef struct VIDevManager
{
    pthread_mutex_t             mManagerLock;
    viChnManager                *gpVippManager[VI_VIPP_NUM_MAX];
    struct hw_isp_media_dev     *media;
    BOOL                        mSetFrequency; //TRUE:had set .  FALSE: ready to set.
    unsigned int                mClockFrequency; //
    pthread_t mCapThreadId; //use select() to traverse all isp_video_device.
    message_queue_t mCmdQueue;
}VIDevManager;

//extern struct hw_isp_media_dev *media;

ERRORTYPE videoInputHw_Construct(int vipp_id);
ERRORTYPE videoInputHw_Destruct(int vipp_id);
ERRORTYPE videoInputHw_searchExistDev(VI_DEV vipp_id, viChnManager **ppViDev);
ERRORTYPE videoInputHw_RegisterCallback(int vipp_id, void *pAppData, MPPCallbackFuncType pMppCallBack);
ERRORTYPE videoInputHw_addChannel(int vipp_id, VI_CHN_MAP_S *pChn);
ERRORTYPE videoInputHw_removeChannel(int vipp_id, VI_CHN_MAP_S *pChn);
ERRORTYPE videoInputHw_searchExistDevVirChn(VI_DEV vipp_id,VI_CHN ViChn, VI_CHN_MAP_S **ppChn);
MM_COMPONENTTYPE *videoInputHw_GetChnComp(VI_DEV ViDev, VI_CHN ViChn);
VI_CHN_MAP_S *videoInputHw_CHN_MAP_S_Construct();
void videoInputHw_CHN_MAP_S_Destruct(VI_CHN_MAP_S *pChannel);

//ERRORTYPE videoInputHw_initVipp(VI_DEV Vipp_id);
ERRORTYPE videoInputHw_setVippEnable(VI_DEV Vipp_id);
ERRORTYPE videoInputHw_setVippDisable(VI_DEV Vipp_id);
ERRORTYPE videoInputHw_SetVippShutterTime(VI_DEV Vipp_id, VI_SHUTTIME_CFG_S *pShutTime);
int videoInputHw_IsLongShutterBusy(VI_DEV Vipp_id);
ERRORTYPE videoInputHw_IncreaseLongShutterRef(VI_DEV Vipp_id);
ERRORTYPE videoInputHw_DecreaseLongShutterRef(VI_DEV Vipp_id);
ERRORTYPE videoInputHw_searchVippStatus(VI_DEV Vipp_id, int *pStatus);

ERRORTYPE videoInputHw_Open_Media(); /*Open Media+ISP+CSI Device*/
ERRORTYPE videoInputHw_Close_Media(); /*Close Media+ISP+CSI Device*/
ERRORTYPE videoInputHw_ChnInit(int vipp_id); /*Open /dev/video[0~3] node*/
ERRORTYPE videoInputHw_ChnExit(int vipp_id); /*Close /dev/video[0~3] node*/
ERRORTYPE videoInputHw_SetChnAttr(VI_DEV vipp_id, VI_ATTR_S *pstAttr); /*Set /dev/video[0~3] node attr*/
ERRORTYPE videoInputHw_GetChnAttr(VI_DEV vipp_id, VI_ATTR_S *pstAttr); /*Get /dev/video[0~3] node attr*/
ERRORTYPE videoInputHw_GetViBufInfo(VI_DEV ViCh, const unsigned int buf_id, VI_BUFFER_INFO_S *pstInfo);
ERRORTYPE videoInputHw_SetNextPtn(VI_DEV ViCh, VI_ATTR_S *pstAttr);
int videoInputHw_GetPtnFrameNum(VI_DEV ViCh);
ERRORTYPE videoInputHw_SetVIFreq(VI_DEV ViDev, int nFreq);
ERRORTYPE videoInputHw_ChnEnable(int vipp_id); /*Enable /dev/video[0~3] node*/
ERRORTYPE videoInputHw_ChnDisable(int vipp_id); /*Disable /dev/video[0~3] node*/
ERRORTYPE videoInputHw_GetIspDev(VI_DEV Vipp_id, ISP_DEV *pIspDev);
VI_DEV videoInputHw_GetVippByIspDev(ISP_DEV nIspDev);
ERRORTYPE videoInputHw_GetBkID(VI_DEV VippID, int *pBkID);
ERRORTYPE videoInputHw_SetCrop(VI_DEV Vipp_id, const VI_CROP_CFG_S *pCropCfg);
ERRORTYPE videoInputHw_GetCrop(VI_DEV Vipp_id, VI_CROP_CFG_S *pCropCfg);
ERRORTYPE videoInputHw_SetSyncCtrl(VI_DEV Vipp_id, const struct csi_sync_ctrl *sync);
ERRORTYPE videoInputHw_SetNextPtn(VI_DEV ViCh, VI_ATTR_S *pstAttr);
int videoInputHw_GetFrameNum(VI_DEV ViCh);
//ERRORTYPE videoInputHw_SetOsdMaskRegion(int *pvipp_id, VI_OsdMaskRegion *pstOsdMaskRegion);
//ERRORTYPE videoInputHw_UpdateOsdMaskRegion(int *pvipp_id, int onoff);
ERRORTYPE videoInputHw_SetRegions(VI_DEV vipp_id, RgnChnAttachDetailPack *pPack);
ERRORTYPE videoInputHw_DeleteRegions(VI_DEV vipp_id, RgnChnAttachDetailPack *pPack);
ERRORTYPE videoInputHw_UpdateOverlayBitmap(VI_DEV vipp_id, RGN_HANDLE RgnHandle, BITMAP_S *pBitmap);
ERRORTYPE videoInputHw_UpdateRegionChnAttr(VI_DEV ViDev, RGN_HANDLE RgnHandle, const RGN_CHN_ATTR_S *pRgnChnAttr);
ERRORTYPE videoInputHw_RefsReduceAndRleaseData(int vipp_id, VIDEO_FRAME_INFO_S *pstFrameInfo);
ERRORTYPE videoInputHw_GetFrameSyncStart(int *pvipp_id, int *pExpTime, unsigned int *pFrameid, int nMilliSec);
ERRORTYPE videoInputHw_SetBKFramedoneMerge(VI_DEV Vipp_id, struct mrg_int_ch_cfg *cfg);
ERRORTYPE videoInputHw_SetInputBitWidthStart(VI_DEV Vipp_id, const enum set_bit_width bitwidth);
ERRORTYPE videoInputHw_SetInputBitWidthStop(VI_DEV Vipp_id, const enum set_bit_width bitwidth);
ERRORTYPE videoInputHw_SetDropFrame(VI_DEV Vipp_id, unsigned int drop_num);
ERRORTYPE videoInputHw_SetMirror(VI_DEV vipp_id, int value);
ERRORTYPE videoInputHw_SetFlip(VI_DEV vipp_id, int value);
ERRORTYPE videoInputHw_GetMirror(VI_DEV vipp_id, int *pvalue);
ERRORTYPE videoInputHw_GetFlip(VI_DEV vipp_id, int *pvalue);

ERRORTYPE videoInputHw_IspAe_SetMode(int *pIspId, int value);
ERRORTYPE videoInputHw_IspAe_SetExposureBias(int *pIspId, int value);
ERRORTYPE videoInputHw_IspAe_SetExposure(int *pIspId, int value);
ERRORTYPE videoInputHw_IspAe_SetISOSensitiveMode(int *pIspId, int mode);
ERRORTYPE videoInputHw_IspAe_SetISOSensitive(int *pIspId, int value);
ERRORTYPE videoInputHw_IspAe_SetMetering(int *pIspId, int value);
ERRORTYPE videoInputHw_IspAe_SetGain(int *pIspId, int value);
ERRORTYPE videoInputHw_IspAe_SetEvIdx(int *pIspId, int value);
ERRORTYPE videoInputHw_IspAe_SetLock(int *pIspId, int value);
ERRORTYPE videoInputHw_IspAe_SetTable(int *pIspId, struct ae_table_info *ae_table);
ERRORTYPE videoInputHw_IspAe_RoiArea(int *pIspId, SIZE_S Res, RECT_S RoiRgn, AW_U16 ForceAeTarget, AW_U16 Enable);
ERRORTYPE videoInputHw_IspAe_RoiMeteringArea(int *pIspId, SIZE_S Res, RECT_S RoiRgn);
ERRORTYPE videoInputHw_IspAe_SetFaceAeCfg(int *pIspId, struct isp_face_ae_attr_info face_ae_info, SIZE_S Res);
ERRORTYPE videoInputHw_IspAe_GetFaceAeCfg(int *pIspId, struct isp_face_ae_attr_info *face_ae_info);
ERRORTYPE videoInputHw_Isp_ReadIspBin(int *pIspId, ISP_CFG_BIN_MODE ModeFlag, char *IspCfgBinPath);
ERRORTYPE videoInputHw_Isp_SetAiIsp(int *pIspId, isp_ai_isp_info *ai_isp_info_entity);

ERRORTYPE videoInputHw_IspAwb_SetMode(int *pIspId, int value);
ERRORTYPE videoInputHw_IspAwb_SetColorTemp(int *pIspId, int value);
ERRORTYPE videoInputHw_IspAwb_SetStatsSyncMode(int *pIspId, ISP_AWB_STATS_MODE IspAwbStatsMode);
ERRORTYPE videoInputHw_Isp_SetFlicker(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_SetMirror(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_SetFlip(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_SetBrightness(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_SetContrast(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_SetSaturation(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_SetSharpness(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_SetHue(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_SetColorEffect(int *pIspId, enum colorfx ColorStyle);
ERRORTYPE videoInputHw_Isp_SetScene(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_SetIrStatus(int *pIspId, ISP_CFG_MODE ModeFlag);
ERRORTYPE videoInputHw_Isp_SetD3dLbcRatio(int *pIspId, unsigned int value);
ERRORTYPE videoInputHw_Isp_SetStitchMode(int *pIspId, enum stitch_mode_t stitch_mode);
ERRORTYPE videoInputHw_Isp_SetSensorFps(int *pIspId, int value);
//add by jaosn
ERRORTYPE videoInputHw_IspAe_GetExposureLine(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_IspAe_GetEvIdx(int *pIspId, int *value);
ERRORTYPE videoInputHw_IspAe_GetMaxEvIdx(int *pIspId, int *value);
ERRORTYPE videoInputHw_IspAe_GetAeLock(int *pIspId, int *value);
ERRORTYPE videoInputHw_IspAe_GetISOLumIdx(int *pIspId, int *value);
ERRORTYPE videoInputHw_IspAe_GetEvLv(int *pIspId, int *value);
ERRORTYPE videoInputHw_IspAe_GetEvLvAdj(int *pIspId, int *value);
ERRORTYPE videoInputHw_Isp_GetIrStatus(int *pIspId, int *value);
ERRORTYPE videoInputHw_Isp_GetIrAwbGain(int *pIspId, int *rgain_ir, int *bgain_ir);
ERRORTYPE videoInputHw_Isp_GetIrAwbWinCnt(int *pIspId, int *irlight_win_cnt, int *normal_win_cnt);
ERRORTYPE videoInputHw_IspAwb_GetCurColorT(int *pIspId, int *value);
ERRORTYPE videoInputHw_IspAe_GetWeightLum(int *pIspId, int *value);

ERRORTYPE videoInputHw_IspAe_GetMode(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_IspAe_GetExposureBias(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_IspAe_GetExposure(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_IspAe_GetISOSensitiveMode(int *pIspId, int *mode);
ERRORTYPE videoInputHw_IspAe_GetISOSensitive(int *pIspId, int *value);
ERRORTYPE videoInputHw_IspAe_GetMetering(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_IspAe_GetGain(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_IspAwb_GetMode(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_IspAwb_GetColorTemp(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_Isp_GetFlicker(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_Isp_GetMirror(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_Isp_GetFlip(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_Isp_GetBrightness(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_Isp_GetContrast(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_Isp_GetSaturation(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_Isp_GetSharpness(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_Isp_GetHue(int *pIspId, int *pvalue);
ERRORTYPE videoInputHw_Isp_GetScene(int *pIspId, int *value);
ERRORTYPE videoInputHw_IspAwb_SetRGain(int *pIspId, int value);  /*value: [256, 256 * 64]*/
ERRORTYPE videoInputHw_IspAwb_GetRGain(int *pIspId, int *value);
ERRORTYPE videoInputHw_IspAwb_SetBGain(int *pIspId, int value);  /*value: [256, 256 * 64]*/
ERRORTYPE videoInputHw_IspAwb_GetBGain(int *pIspId, int *value);
ERRORTYPE videoInputHw_IspAwb_SetGrGain(int *pIspId, int value);  /*value: [256, 256 * 64]*/
ERRORTYPE videoInputHw_IspAwb_GetGrGain(int *pIspId, int *value);
ERRORTYPE videoInputHw_IspAwb_SetGbGain(int *pIspId, int value);  /*value: [256, 256 * 64]*/
ERRORTYPE videoInputHw_IspAwb_GetGbGain(int *pIspId, int *value);

ERRORTYPE videoInputHw_Isp_SetWDR(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_GetWDR(int *pIspId, int *value);
ERRORTYPE videoInputHw_Isp_SetNR(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_GetNR(int *pIspId, int *value);
ERRORTYPE videoInputHw_Isp_Set3DNR(int *pIspId, int value);
ERRORTYPE videoInputHw_Isp_Get3DNR(int *pIspId, int *value);
ERRORTYPE videoInputHw_Isp_GetPltmNextStren(int *pIspId, int *value);
ERRORTYPE videoInputHw_Isp_SetVe2IspParam(int *pIspId, struct enc_VencVe2IspParam *pVe2IspParam);
//ERRORTYPE videoInputHw_Isp_GetIsp2VeParam(int *pIspId, struct enc_VencIsp2VeParam *pIsp2VeParam);
ERRORTYPE videoInputHw_Isp_GetIsp2VeParam_AeStats(VI_DEV nVippId, struct isp_ae_stats_s *pAeStatsInfo);
ERRORTYPE videoInputHw_Isp_GetIsp2VeParam_EncppSharpEn(ISP_DEV isp_id, int *pEncppSharpEnable);
ERRORTYPE videoInputHw_Isp_GetIsp2VeParam_EncppSharpCfg(ISP_DEV isp_id, IspEncppSharpConfig *pEncppSharpCfg);
ERRORTYPE videoInputHw_Isp_SetSensorMipiSwitch(int *pIspId, struct sensor_mipi_switch_entity *switch_entity);
ERRORTYPE videoInputHw_Isp_GetSensorMipiSwitch(int *pIspId, struct sensor_mipi_switch_entity *switch_entity);
ERRORTYPE videoInputHw_Isp_SetSpecialScene(int *pIspId, scene_mode_t target_scene);
ERRORTYPE videoInputHw_Isp_SetIspAlgoFreq(int *pIspId, unsigned short value, unsigned short flag);
ERRORTYPE videoInputHw_Isp_GetIspAlgoFreq(int *pIspId, unsigned short *value);
#endif
