/******************************************************************************
  Copyright (C), 2001-2025, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : motion_detect.h
  Version       : Initial Draft
  Author        : Allwinner
  Created       : 2025/02/27
  Last Modified :
  Description   : motion detect function header file
  Function List :
  History       :
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __MOTION_DETECT_H__
#define __MOTION_DETECT_H__

#define MOTION_DETECT_ALIGN  (16)

typedef struct {
    int nPixXBgn;
    int nPixXEnd;
    int nPixYBgn;
    int nPixYEnd;
    int nTotalNum;
    int nLargeMadNum;
    int bIsMotion;
} MOTION_DETECT_REGION_S;

typedef struct {
    int nHorRegionNum;
    int nVerRegionNum;
    int nMotionRegionNum;
    MOTION_DETECT_REGION_S *mRegion;
} MOTION_DETECT_RESULT_S;

typedef struct {
    int nPicStride;
    int nPicWidth;
    int nPicHeight;
    int bCropEn;
    int nCropLeft;
    int nCropTop;
    int nCropWidth;
    int nCropHeight;
} MOTION_DETECT_STATIC_CFG_S;

typedef struct {
    int bDefaultCfgDis;
    int nHorRegionNum;
    int nVerRegionNum;
    int bMorphologEn;           // range[0, 1]
    int nLargeMadTh;            // range[0, 255]
    int nBackgroundWeight;      // range[0, 255]
    float fLargeMadRatioTh;     // range[0, 100.0f]
} MOTION_DETECT_DYNAMIC_CFG_S;

void *MotionDetectCreat(MOTION_DETECT_STATIC_CFG_S *static_cfg);
void MotionDetectRelease(void *handle);
int MotionDetectInit(void *handle, MOTION_DETECT_DYNAMIC_CFG_S *dynamic_cfg);
void MotionDetectDestroy(void *handle);
int MotionDetectProcessOneFrame(void *handle, unsigned char *pic);
int MotionDetectSetParam(void *hanle, MOTION_DETECT_DYNAMIC_CFG_S *cfg);
int MotionDetectGetResult(void *handle, MOTION_DETECT_RESULT_S *result);

#endif

#ifdef __cplusplus
}
#endif
