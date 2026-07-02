/******************************************************************************
  Copyright (C), 2001-2023, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : CapsEncppYUVData.h
  Version       : Initial Draft
  Author        : Allwinner
  Created       : 2023/01/30
  Last Modified :
  Description   : capture encpp yuv data function
  Function List :
  History       :
******************************************************************************/

#ifndef __CAPSENCPPYUVDATA_H__
#define __CAPSENCPPYUVDATA_H__

#include "VideoEnc_Component.h"
#include "sunxi_camera_v2.h"

int SendCropYuvToApp(VIDEOENCDATATYPE *pVideoEncData);

#endif
