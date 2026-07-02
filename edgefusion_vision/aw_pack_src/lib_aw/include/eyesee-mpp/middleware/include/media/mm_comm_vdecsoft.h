#ifndef __IPCLINUX_COMM_VDECSOFT_H__
#define __IPCLINUX_COMM_VDECSOFT_H__

#if (MPPCFG_VDECSOFT == OPTION_VDECSOFT_ENABLE)
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#endif

#include "mm_comm_video.h"
#include "mm_common.h"
#include "plat_errno.h"

typedef struct VDECSOFT_CHN_ATTR_S {
    PAYLOAD_TYPE_E mType; /* 待解码的视频类型 ，等效AVCodecID*/
    PIXEL_FORMAT_E mOutputPixelFormat;
    BOOL bEnableExtraFrameNum;  // TRUE:use user defined value, FALSE:use default value written in
                                // config file.
    int mExtraFrameNum; /* let vdeclib malloc more frame buffer base on initial frame number.*/
    BOOL useBufferPoll;
} VDECSOFT_CHN_ATTR_S;

typedef struct VDECSOFT_STREAM_S {
    unsigned char* pAddr;
    int mLen;
    int64_t mPTS;
    BOOL mbEndOfFrame;  /* is the end of a frame */
    BOOL mbEndOfStream; /* is the end of all stream */
} VDECSOFT_STREAM_S;

typedef struct VDEVSOFT_FRAME_S {
    unsigned int mWidth;  // unit:pixel, whole picture width and height, not equal to buffer width
                          // and height because picture width <= y-stride.
    unsigned int mHeight;

    VIDEO_FIELD_E mField;
    PIXEL_FORMAT_E mPixelFormat;

    unsigned int mPhyAddr[3];
    unsigned int* mpVirAddr[3]; /* Y, U, V; Y, UV; Y, VU */
    unsigned int mStride[3];
    int dma_fd;

    size_t crop_top;
    size_t crop_bottom;
    size_t crop_left;
    size_t crop_right;
    int64_t pts;
} VDEVSOFT_FRAME_S;

typedef struct VDECSOFT_FRAME_INFO_S {
    void* pVdecSoftAVFrame;  // AVFrame
    VDEVSOFT_FRAME_S VFrame;
    int mId;  // id identify frame uniquely
} VDECSOFT_FRAME_INFO_S;

#define ERR_VDECSOFT_INVALID_CHNID \
    DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define ERR_VDECSOFT_ILLEGAL_PARAM \
    DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define ERR_VDECSOFT_EXIST DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
#define ERR_VDECSOFT_NULL_PTR DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define ERR_VDECSOFT_NOT_CONFIG DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
#define ERR_VDECSOFT_NOT_SUPPORT DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
#define ERR_VDECSOFT_NOT_PERM DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define ERR_VDECSOFT_UNEXIST DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
#define ERR_VDECSOFT_NOMEM DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
#define ERR_VDECSOFT_NOBUF DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
#define ERR_VDECSOFT_BUF_EMPTY DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
#define ERR_VDECSOFT_BUF_FULL DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
#define ERR_VDECSOFT_SYS_NOTREADY DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define ERR_VDECSOFT_BUSY DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define ERR_VDECSOFT_SAMESTATE DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_SAMESTATE)
#define ERR_VDECSOFT_INVALIDSTATE DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_INVALIDSTATE)
#define ERR_VDECSOFT_INCORRECT_STATE_TRANSITION \
    DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_INCORRECT_STATE_TRANSITION)
#define ERR_VDECSOFT_INCORRECT_STATE_OPERATION \
    DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_INCORRECT_STATE_OPERATION)
#define ERR_VDECSOFT_BADADDR DEF_ERR(MOD_ID_VDECSOFT, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
#endif /* __IPCLINUX_COMM_VDECSOFT_H__ */
