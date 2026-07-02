/******************************************************************************
  Copyright (C), 2001-2023, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : FillVencSeiData.h
  Version       : Initial Draft
  Author        : Allwinner
  Created       : 2023/04/10
  Last Modified :
  Description   : fill vencoder SEI data function
  Function List :
  History       :
******************************************************************************/

#ifndef __FILLVENCSEIDATA_H__
#define __FILLVENCSEIDATA_H__

#include "VideoEnc_Component.h"

void FreeSeiDataBuffer(VIDEOENCDATATYPE* pVideoEncData);
int FillSeiDataToVencLib(VIDEOENCDATATYPE *pVideoEncData);

#endif
