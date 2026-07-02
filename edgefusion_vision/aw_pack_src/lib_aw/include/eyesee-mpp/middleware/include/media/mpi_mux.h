/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : mpi_mux.h
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2016/04/27
  Last Modified :
  Description   : mpi functions declaration for mux
  Function List :
  History       :
******************************************************************************/
#ifndef __IPCLINUX_MPI_MUX_H__
#define __IPCLINUX_MPI_MUX_H__

//ref platform headers
#include "plat_type.h"
#include "plat_errno.h"
#include "plat_defines.h"
#include "plat_math.h"

//media api headers to app
#include "mm_common.h"
#include "mm_comm_mux.h"
#include "mm_comm_venc.h"
#include <mm_comm_tenc.h>

//media internal common headers.
#include <vencoder.h>
#include <ComponentCommon.h>
#include <RecorderMode.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    VENC_STREAM_S mVencStream;
    int mStreamId; ///< streamId = portIndex = suffix of RecRenderData's media_inf's mMediaVideoInfo[] array. portIndex can match to veChn.
}MUX_VENC_STREAM_S;

typedef struct
{
    AUDIO_STREAM_S mAencStream;
    int mStreamId; ///< streamId = portIndex = suffix of RecRenderData's media_inf.
}MUX_AENC_STREAM_S;

typedef struct
{
    TEXT_STREAM_S mTencStream;
    int mStreamId; ///< streamId = portIndex = suffix of RecRenderData's media_inf.
}MUX_TENC_STREAM_S;

ERRORTYPE AW_MPI_MUX_CreateChn(MUX_CHN muxChn, MUX_CHN_ATTR_S *pChnAttr, int nFd, int nFallocateLen);
ERRORTYPE AW_MPI_MUX_DestroyChn(MUX_CHN muxChn);
ERRORTYPE AW_MPI_MUX_StartChn(MUX_CHN muxChn);
ERRORTYPE AW_MPI_MUX_StopChn(MUX_CHN muxChn, BOOL bShutDownNowFlag);
ERRORTYPE AW_MPI_MUX_SetFd(MUX_CHN muxChn, int nFd, int nFallocateLen);
ERRORTYPE AW_MPI_MUX_GetChnAttr(MUX_CHN muxChn, MUX_CHN_ATTR_S *pChnAttr);
ERRORTYPE AW_MPI_MUX_SetChnAttr(MUX_CHN muxChn, MUX_CHN_ATTR_S *pChnAttr);
ERRORTYPE AW_MPI_MUX_SetH264SpsPpsInfo(MUX_CHN muxChn, VENC_CHN VeChn, VencHeaderData *pH264SpsPpsInfo);
ERRORTYPE AW_MPI_MUX_SetH265SpsPpsInfo(MUX_CHN muxChn, VENC_CHN VeChn, VencHeaderData *pH265SpsPpsInfo);

ERRORTYPE AW_MPI_MUX_SwitchFd(MUX_CHN muxChn, int fd, int nFallocateLen);
ERRORTYPE AW_MPI_MUX_SwitchFileNormal(MUX_CHN muxChn, int fd, int nFallocateLen);
ERRORTYPE AW_MPI_MUX_RegisterCallback(MUX_CHN muxChn, MPPCallbackInfo *pCallback);
ERRORTYPE AW_MPI_MUX_SetStrmIds(MUX_CHN muxChn, MuxStreamIdsInfo *pStrmIdsInfo);
ERRORTYPE AW_MPI_MUX_SetSwitchFileDurationPolicy(MUX_CHN muxChn, RecordFileDurationPolicy ePolicy);
ERRORTYPE AW_MPI_MUX_GetSwitchFileDurationPolicy(MUX_CHN muxChn, RecordFileDurationPolicy *pPolicy);
ERRORTYPE AW_MPI_MUX_SetSdCardState(MUX_CHN muxChn, BOOL bExist);
ERRORTYPE AW_MPI_MUX_SetThmPic(MUX_CHN muxChn, char *p_thm_pic, int thm_pic_size);
ERRORTYPE AW_MPI_MUX_SetVeChnBindStreamId(MUX_CHN muxChn, VENC_CHN VeChn, int nStreamId);
/**
  Async mode and sync mode can't be mixed.
*/
ERRORTYPE AW_MPI_MUX_SendVideoStream(MUX_CHN muxChn, VENC_STREAM_S *pStream, int nStreamId);
ERRORTYPE AW_MPI_MUX_SendAudioStream(MUX_CHN muxChn, AUDIO_STREAM_S *pStream, int nStreamId);
ERRORTYPE AW_MPI_MUX_SendTextStream(MUX_CHN muxChn, TEXT_STREAM_S *pStream, int nStreamId);
ERRORTYPE AW_MPI_MUX_SendVideoStreamSync(MUX_CHN muxChn, VENC_STREAM_S *pStream, int nStreamId);
ERRORTYPE AW_MPI_MUX_SendAudioStreamSync(MUX_CHN muxChn, AUDIO_STREAM_S *pStream, int nStreamId);
ERRORTYPE AW_MPI_MUX_SendTextStreamSync(MUX_CHN muxChn, TEXT_STREAM_S *pStream, int nStreamId);

#ifdef __cplusplus
}
#endif

#endif /* __IPCLINUX_MPI_MUX_H__ */
