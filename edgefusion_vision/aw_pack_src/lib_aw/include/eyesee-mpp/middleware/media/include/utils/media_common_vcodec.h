/******************************************************************************
  Copyright (C), 2001-2021, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : media_common_vcodec.h
  Version       : Initial Draft
  Author        : Allwinner PDC-PD5 Team
  Created       : 2021/05/15
  Last Modified :
  Description   : divide media_common to several parts, to decrease correlation.
  Function List :
  History       :
******************************************************************************/
#ifndef _MEDIA_COMMON_VCODEC_H_
#define _MEDIA_COMMON_VCODEC_H_

#include <mm_comm_venc.h>
#include "vdecoder.h"
#include "vencoder.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

VENC_CODEC_TYPE map_PAYLOAD_TYPE_E_to_VENC_CODEC_TYPE(PAYLOAD_TYPE_E nPayLoadType);
enum EVIDEOCODECFORMAT map_PAYLOAD_TYPE_E_to_EVIDEOCODECFORMAT(PAYLOAD_TYPE_E nPayLoadType);
PAYLOAD_TYPE_E map_VENC_CODEC_TYPE_to_PAYLOAD_TYPE_E(VENC_CODEC_TYPE eVencCodecType);
PAYLOAD_TYPE_E map_EVIDEOCODECFORMAT_to_PAYLOAD_TYPE_E(enum EVIDEOCODECFORMAT eCodecFormat);
VENC_OUTPUT_PACK_TYPE map_VENC_DATA_TYPE_U_to_VENC_OUTPUT_PACK_TYPE(VENC_DATA_TYPE_U UMppPackType, PAYLOAD_TYPE_E ePayloadType);
VENC_DATA_TYPE_U map_VENC_OUTPUT_PACK_TYPE_to_VENC_DATA_TYPE_U(VENC_OUTPUT_PACK_TYPE UVencPackType, PAYLOAD_TYPE_E ePayloadType);

ERRORTYPE setVideoEncodingBitRateToVENC_CHN_ATTR_S(VENC_CHN_ATTR_S *pChnAttr, int bitRate);
unsigned int GetBitRateFromVENC_CHN_ATTR_S(VENC_CHN_ATTR_S *pAttr);
ERRORTYPE SetFrameRateToVENC_CHN_ATTR_S(const VENC_FRAME_RATE_S *pFrameRate, VENC_CHN_ATTR_S *pAttr);
ERRORTYPE GetFrameRateFromVENC_CHN_ATTR_S(const VENC_CHN_ATTR_S *pAttr, VENC_FRAME_RATE_S *pFrameRate);
ERRORTYPE GetEncodeDstSizeFromVENC_CHN_ATTR_S(VENC_CHN_ATTR_S *pAttr, SIZE_S *pDstSize);
ERRORTYPE SetEncodeDstSizeToVENC_CHN_ATTR_S(VENC_CHN_ATTR_S *pChnAttr, SIZE_S *pDstSize);
int GetIQpOffsetFromVENC_CHN_ATTR_S(VENC_CHN_ATTR_S *pChnAttr);
BOOL GetFastEncFlagFromVENC_CHN_ATTR_S(VENC_CHN_ATTR_S *pChnAttr);
BOOL GetPIntraEnableFromVENC_CHN_ATTR_S(VENC_CHN_ATTR_S *pChnAttr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _MEDIA_COMMON_VCODEC_H_ */

