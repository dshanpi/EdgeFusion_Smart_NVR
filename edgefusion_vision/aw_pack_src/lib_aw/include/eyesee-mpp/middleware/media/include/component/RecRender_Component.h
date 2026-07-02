/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : RecRender_Component.h
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2016/05/25
  Last Modified :
  Description   : mpp component implement
  Function List :
  History       :
******************************************************************************/
#ifndef __IPCLINUX_RECRENDER_COMPONENT_H__
#define __IPCLINUX_RECRENDER_COMPONENT_H__

#include <errno.h>
#include <tmessage.h>
#include <tsemaphore.h>

#include <mm_component.h>
#include <ComponentCommon.h>
//#include <vencoder.h>
//#include <mp4_mux_lib.h>
//#include <type_camera.h>
//#include <record_writer.h>
//#include <include_system/cedarx_avs_counter.h>
//#include <FsWriter.h>
#include <mm_comm_mux.h>
//#include "RecRenderSink.h"
//#include "RecRender_cache.h"

//#include <CDX_SystemBase.h>
//#include <aenc_sw_lib.h>

#include <RecorderMode.h>

//#define SINKINFO_SIZE (2)

#define MUX_FIFO_LEVEL  (256)

//must match with WRITE_BLOCK_SIZE inFsCache.c
#define PAYLOAD_CACHE_SIZE_VGA (128 * 1024 * 4)                     //640*480
#define PAYLOAD_CACHE_SIZE_HD (128 * 1024 * 8 * 2)                  //(1024*1024*3)   //1280*720
#define PAYLOAD_CACHE_SIZE_COMPRESSED_JPEG_HD (128 * 1024 * 8 * 2)  //(1024*1024*3)   //1280*720
#define PAYLOAD_CACHE_SIZE_FHD (128 * 1024 * 8 * 4)                 //1920*1080

//typedef enum REC_RENDER_PORT_INDEX_DEFINITION {
//    RECR_PORT_INDEX_VIDEO = 0,
//    RECR_PORT_INDEX_AUDIO,
//    RECR_PORT_INDEX_TEXT,
//
//    // counts of recorder render ports
//    MAX_REC_RENDER_PORTS
//} REC_RENDER_PORT_INDEX_DEFINITION;

#define MAX_REC_RENDER_PORTS MAX_TRACK_COUNT

typedef enum REC_FILE_STATE {
	FILE_NORMAL = 0,
	FILE_NEED_SWITCH_TO_IMPACT, //not used now.
	FILE_IMPACT_RECDRDING,  //not used now.
	FILE_NEED_SWITCH_TO_NORMAL, //normal switch file. switch to single muxer file, then set this file as normal.
} REC_FILE_STATE;

typedef struct RecSinkPacket {
    int     mId;    //in common, it is set to venc/aenc/tenc frame id; but in cacheManager, cacheManager set it uniquely discarding different streams!
    int     mStreamType;   //CodecType
    int     mFlags;  //AVPACKET_FLAGS
    int64_t     mPts;   //unit:us
    char*     mpData0;
    int     mSize0;
    char*     mpData1;
    int     mSize1;

    //for video frame.
    int mCurrQp;         //qp of current frame
    int mavQp;
	int mnGopIndex;      //index of gop
	int mnFrameIndex;    //index of current frame in gop.
	int mnTotalIndex;    //index of current frame in whole encoded frames

    unsigned int   nPackNum;
    VencPackInfo   stPackInfos[MAX_OUTPUT_PACK_NUM];

    //int  mSourceType;    //SourceType of [RecRenderSink.h]. indicate where RSPacket's data come from.
    int mRefCnt;    //used for cacheManager.
    /**
      streamId = portIndex = suffix of RecSink's stream array, mChnAttr.mVideoAttr[], media_inf.mMediaVideoInfo[].
      so portIndex can match to veChn by recRenderDataType->mVeChnBindStreamIdList.
    */
    int mStreamId;
    struct list_head mList;
} RecSinkPacket;

typedef struct
{
    int mStreamId;
    VENC_CHN mVeChn;
    VencHeaderData mH264SpsPpsInfo;
    struct list_head mList;
}VencHeaderDataNode;

typedef struct
{
    //use portId as streamId, see RecRenderComponentTunnelRequest(). port index is same to suffix of array
    //pRecRenderData->sInPortTunnelInfo[].
    int mStreamId;
    MPP_CHN_S mVeChn;
    struct list_head mList;
}VeChnBindStreamIdNode;

typedef struct
{
    VENC_CHN mVeChn;
    VencHeaderData mH264SpsPpsInfo;
}VencHeaderDataParam;

typedef struct RECRENDERDATATYPE {
    volatile COMP_STATETYPE state;
    pthread_mutex_t mStateMutex;
    COMP_CALLBACKTYPE *pCallbacks;
    void *pAppData;
    COMP_HANDLETYPE hSelf;
    char mThreadName[32];

    COMP_PORT_PARAM_TYPE sPortParam;
    COMP_PARAM_PORTDEFINITIONTYPE sInPortDef[MAX_REC_RENDER_PORTS];        // port definition
    COMP_INTERNAL_TUNNELINFOTYPE sInPortTunnelInfo[MAX_REC_RENDER_PORTS];  // tunnel information
    COMP_PARAM_BUFFERSUPPLIERTYPE sPortBufSupplier[MAX_REC_RENDER_PORTS];  // buffer surpplier type?
    BOOL mInputPortTunnelFlag[MAX_REC_RENDER_PORTS];                       //TRUE: tunnel mode; FALSE: non-tunnel mode.

    MPP_CHN_S mMppChnInfo;
    MUX_CHN_ATTR_S mChnAttr;      //match media_inf.

    pthread_t thread_id;
    CompInternalMsgType eTCmd;
    message_queue_t cmd_queue;
    volatile int mNoInputFrameFlag;  //1: no input frame to be muxed.

    CdxRecorderWriterCallbackInfo callback_writer;    //CdxRecorderWriterCallbackInfo
    //int track_exist;                    //decided by recorder_mode. tracks_count is removed.
    int64_t duration, duration_audio, duration_text;  //statistics for recRender. keep increase.
    /**
      about mnBasePts, mnAudioBasePts, mnTextBasePts, we have some to explain:
      we suppose video, audio capture must start at the same time, but their first pts may not be same. 
      That is because timing method is different, so use video,audio first pts as basePts, and subsequent pts minus its basePts,
      then the pts sent to RecRenderSink must all start from zero. That is our purpose!

      Now we give up above method, take pts as standard, it can discard recAVSync, because video pts and audio pts all
      come from system time, no error value. mnBasePts are only used to record first video pts now, other base pts do
      the same.
    */
    int64_t mnBasePts;                                //us.
    int64_t mnAudioBasePts;                           //us.
    int64_t mnTextBasePts;                            //us.
    int64_t file_size_bytes;                          //statistics for recRender. keep increase.
    _media_file_inf_t media_inf;                      //match to mChnAttr

    //RecSink start.
    //int                 mMuxerId;
    //MUXERMODES          nMuxerMode;
    //char                *mPath;
    int                 nOutputFd;
    unsigned int        nFallocateLen;
    BOOL                nCallbackOutFlag;

    CDX_RecordWriter    *pWriter;
    void                *pMuxerCtx; //AVFormatContext
    volatile BOOL   mbMuxerInit;
    BOOL            mbTrackInit[MAX_TRACK_COUNT];   //for write fd, clear when switch fd. streamId is array suffix.
    /**
      if many video stream exist, select which one as main video stream and calculate video duration. if only has audio,
      then used as indicate duration[0] for audio stream.
    */
    int             mRefVideoStreamIndex;
    /**
      mValidStreamIds maybe contain 1,2, not contain 0. video0 is mainStream. video1 is substream. It links with
      mChnAttr and mpMediaInf which contain all streams. But this muxChn maybe only wrap some streams, not all.
      so mValidStreamCount and mValidStreamIds[] are based on mChnAttr's whole streams.
    */
    int             mValidStreamCount;  //include audio. -1: all streams are valid. 0: all streams are invalid.
    int             mValidStreamIds[MAX_TRACK_COUNT];
    int             mDuration[MAX_VIDEO_TRACK_COUNT]; //for write fd, clear when switch fd. unit:ms. if only audio, then duration[0] is used to store audio stream duration, and durationAudio.
    int             mDurationAudio;                 //for write fd, clear when switch fd. unit:ms
    int             mDurationText;                  //for write fd, clear when switch fd.
    int             mPrevDuration[MAX_VIDEO_TRACK_COUNT];                  //duration sum before current pkt. unit:ms. mPrevDuration + CurPktDuration = mDuration
    int             mPrevDurationAudio;
    int             mPrevDurationText;
    int64_t             mLoopDuration;                  //total duration from start point when loop recording. unit:ms. when only has audio, it is same to mLoopDurationAudio
    int64_t             mLoopDurationAudio;                  //total duration from start point when loop recording. unit:ms
    int64_t             mFileSizeBytes;                 //for write fd, clear when switch fd.
    /**
      flag used to indicate that the condition for file switch has been checked and need to switch,no need to check any
      more. video duration exceed first, cache video.
    */
    BOOL                bNeedSw;
    /**
      flag used to indicate that the condition for file switch has been checked and need to switch,no need to check any
      more. audio duration exceed first, cache audio.
    */
    BOOL                bNeedSwAudio;
    int             mVideoFrameCounter;             //count video0 frame number during one file, clear when switch fd.
    int             mAudioFrmCnt;                   //count audio frame number during one file, clear when switch fd.
    /**
      we use frame pts which are all based on system time, then we don't need to do avsync.
    */
    int64_t             mPrevPts[MAX_TRACK_COUNT];  //calculated pts, based on mBasePts. clear when switch fd, unit:us
    int64_t             mBasePts[MAX_TRACK_COUNT];  //first source pts for every file. clear when switch fd, unit:us
    /**
      because packets sent to RecSink are based on same system time, so for playing avsync, relative pts is based on one
      stream BasePts. ref to RECRENDERDATATYPE->mnBasePts, mnAudioBasePts.
    */
    int                 mBasePtsRefStreamIndex;
    int64_t             mInputPrevPts[MAX_TRACK_COUNT];  //source pts. record input previous frame source pts, unit:us, clear when switch fd.
    int64_t             mOrigBasePts[MAX_TRACK_COUNT];  //first source pts. Keep valid during recSink Record, not clear when switch fd. unit:us
    int64_t             mDebugInputPts[MAX_TRACK_COUNT];    //for debug, source pts, unit:us, not clear.

    pthread_mutex_t     mutex_reset_writer_lock;    //lock nSwitchFd(mSwitchFilePath), reset_fd_flag, mbMuxerInit, rec_file, nOutputFd(mPath), pWriter, pMuxerCtx
    pthread_mutex_t     mJudgeSwitchlock; //judge if need switch.
    pthread_mutex_t     mMuxerInitlock; // for RecSinkMuxerInit() only.
    int                 nSwitchFd;  //need dup fd.
    //char                *mSwitchFilePath;
    unsigned int                 nSwitchFdFallocateSize;
    volatile BOOL            reset_fd_flag;  //true:new fd is ready and wait to be used; false: new fd is used, no new fd has come, there is not waiting fd.
    volatile BOOL            need_set_next_fd;   //true:need next fd, but not notify app; false: has already notify app need next fd, or not need next fd.
    volatile REC_FILE_STATE      rec_file;
    int64_t             mCurMaxFileDuration;  //unit:ms
    int64_t             mCurFileEndTm;  //unit:ms, match to mLoopDuration, based on duration, so it starts from zero.
    //int64_t             mMaxFileDuration;  //unit:ms, 0:means infinite.
    //int64_t             mMaxFileSizeBytes;  //unit:byte

    RECORDER_MODE       mRecordMode;
    RecordFileDurationPolicy mFileDurationPolicy;
    //FSWRITEMODE         mFsWriteMode;
    //int             mFsSimpleCacheSize;
    //_media_file_inf_t   *mpMediaInf;    //RECRENDERDATATYPE->media_inf
    //__extra_data_t      mAudioExtraDataForMp4;
    //CdxRecorderWriterCallbackInfo *mpCallbackWriter;    //RECRENDERDATATYPE->callback_writer
    volatile BOOL   mbSdCardState; //true:exist; false:disappear

    //RecSinkCallbackType*    mpCallbacks;
    //void*                 mpAppData; //RECRENDERDATATYPE

    //message_queue_t mMsgQueue;
    BOOL                mPrefetchFlag[MAX_VIDEO_TRACK_COUNT];  //true: non-audio data, validList->prefetchList, when true, it means key frame is done.
    BOOL                mPrefetchFlagAudio;
    /**
      for case bNeedSwAudio: audio duration reaches requirements first.
      Flag video pts has reached autio pts(i.d., time meet), video is enough to switch file.
    */
    BOOL                bTimeMeetAudio;
    int                 mVideoFrmCntWriteMore;  //for switchFileAudio. the frame number which video is written more. for video0.
    /**
      framePts to start to write more. used in switchFileAudio. for video0. unit:us
      after videoPts reaches audio, but not meet keyFrame, then video need continue to write. So we record these video
      frame pts from start to end.
    */
    int64_t             mVideoPtsWriteMoreSt; //for switchFileAudio. framePts which frame is start_more_frame to write to file.
    int64_t             mVideoPtsWriteMoreEnd;  //for switchFileAudio. framePts which frame is end_more_frame to write to file. for video0. unit:us
    struct list_head    mPrefetchRSPacketList;  //RecSinkPacket, valid->prefetch, at next file begin, move back to valid list.
    struct list_head    mValidRSPacketList; //RecSinkPacket
    struct list_head    mIdleRSPacketList;
    pthread_mutex_t     mRSPacketListMutex;

    BOOL mbShutDownNowFlag;

    THM_PIC rc_thm_pic;   // store thm pic info set by user
    int rc_thm_pic_ready; // flag used to indicate the thm pic is ready or not 
    pthread_mutex_t mThmPicLock;

    //struct list_head *mpVencHeaderDataList; //VencHeaderDataNode, point to recRender component's mVencHeaderDataList

    //int need_to_force_i_frm[MAX_VIDEO_TRACK_COUNT];//true: need to force i frame.
    int forced_i_frm[MAX_VIDEO_TRACK_COUNT];    //true: already force i frame. 

    int video_frm_cnt;  //count video0 input frame number, not clear.
    int audio_frm_cnt;  //count audio input frame number, not clear.

    int key_frm_sent[MAX_VIDEO_TRACK_COUNT];   // flag indicating the first video key frm is encounted or not, when new file is written.
    int v_frm_drp_cnt[MAX_VIDEO_TRACK_COUNT];  // to counter video packets that dropped until the first key frame is encounted;
    //int b_add_repair_info_flag;
    //int max_frms_tag_interval;  //unit:us, for mp4 repair.
    //RecSink done.
    

    //BOOL mbSdCardState;  //true:exist; false:disappear
    //int is_compress_source; /* gushiming compressed source */

    //int64_t             max_file_duration;  //unit:ms

    //RecordFileDurationPolicy mRecordFileDurationPolicy;

    //FSWRITEMODE         mFsWriteMode;
    //int             mSimpleCacheSize;

    int mVideoInputFrameNum; //video input node total count.
    //BOOL bWaitVideoInputFrameFlag;
    //pthread_cond_t mWaitVideoInputFrameCond;
    struct list_head mVideoInputFrameIdleList;  //ENCODER_NODE_T
    struct list_head mVideoInputFrameReadyList;
    struct list_head mVideoInputFrameUsedList;
    pthread_mutex_t mVideoInputFrameListMutex;
    //int mContinuousDiscardVideoNum; //used in non-tunnel mode

    int mAudioInputFrameNum;
    //BOOL bWaitAudioInputFrameFlag;
    //pthread_cond_t mWaitAudioInputFrameCond;
    struct list_head mAudioInputFrameIdleList;  //ENCODER_NODE_T
    struct list_head mAudioInputFrameReadyList;
    struct list_head mAudioInputFrameUsedList;
    pthread_mutex_t mAudioInputFrameListMutex;
    //int mContinuousDiscardAudioNum; //used in non-tunnel mode

    int mTextInputFrameNum;
    struct list_head mTextInputFrameIdleList;  //ENCODER_NODE_T
    struct list_head mTextInputFrameReadyList;
    struct list_head mTextInputFrameUsedList;
    pthread_mutex_t mTextInputFrameListMutex;

    //long long last_video_pts;
    //long long last_audio_pts; //use mDebugInputPts[] instead.

    struct list_head mVencHeaderDataList; //VencHeaderDataNode
    struct list_head mVeChnBindStreamIdList; //VeChnBindStreamIdNode

} RECRENDERDATATYPE;

//private interface
ERRORTYPE RecRenderGetPortDefinition(PARAM_IN COMP_HANDLETYPE hComponent,
                                     PARAM_INOUT COMP_PARAM_PORTDEFINITIONTYPE *pPortDef);
ERRORTYPE RecRenderSetPortDefinition(PARAM_IN COMP_HANDLETYPE hComponent,
                                     PARAM_IN COMP_PARAM_PORTDEFINITIONTYPE *pPortDef);
ERRORTYPE RecRenderGetCompBufferSupplier(PARAM_IN COMP_HANDLETYPE hComponent,
                                         PARAM_INOUT COMP_PARAM_BUFFERSUPPLIERTYPE *pPortBufSupplier);
ERRORTYPE RecRenderSetCompBufferSupplier(PARAM_IN COMP_HANDLETYPE hComponent,
                                         PARAM_IN COMP_PARAM_BUFFERSUPPLIERTYPE *pPortBufSupplier);
ERRORTYPE RecRenderGetPortParam(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_OUT COMP_PORT_PARAM_TYPE *pPortParam);
ERRORTYPE RecRenderGetMPPChannelInfo(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_OUT MPP_CHN_S *pChn);
ERRORTYPE RecRenderGetTunnelInfo(PARAM_IN COMP_HANDLETYPE hComponent,
                                 PARAM_INOUT COMP_INTERNAL_TUNNELINFOTYPE *pTunnelInfo);
ERRORTYPE RecRenderGetChnAttr(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_INOUT MUX_CHN_ATTR_S *pChnAttr);
ERRORTYPE RecRenderSetChnAttr(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_IN MUX_CHN_ATTR_S *pChnAttr);
//public interface
ERRORTYPE RecRenderSendCommand(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_IN COMP_COMMANDTYPE Cmd,
                               PARAM_IN unsigned int nParam1, PARAM_IN void *pCmdData);
ERRORTYPE RecRenderGetState(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_OUT COMP_STATETYPE *pState);
ERRORTYPE RecRenderSetCallbacks(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_IN COMP_CALLBACKTYPE *pCallbacks,
                                PARAM_IN void *pAppData);
ERRORTYPE RecRenderGetConfig(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_IN COMP_INDEXTYPE nIndex,
                             PARAM_INOUT void *pComponentConfigStructure);
ERRORTYPE RecRenderSetConfig(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_IN COMP_INDEXTYPE nIndex,
                             PARAM_IN void *pComponentConfigStructure);
ERRORTYPE RecRenderComponentTunnelRequest(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_IN unsigned int nPort,
                                          PARAM_IN COMP_HANDLETYPE hTunneledComp, PARAM_IN unsigned int nTunneledPort,
                                          PARAM_INOUT COMP_TUNNELSETUPTYPE *pTunnelSetup);
ERRORTYPE RecRenderEmptyThisBuffer(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_IN COMP_BUFFERHEADERTYPE *pBuffer);
ERRORTYPE RecRenderComponentDeInit(PARAM_IN COMP_HANDLETYPE hComponent);
ERRORTYPE RecRenderComponentInit(PARAM_IN COMP_HANDLETYPE hComponent);
#endif /* __IPCLINUX_RECRENDER_COMPONENT_H__ */
