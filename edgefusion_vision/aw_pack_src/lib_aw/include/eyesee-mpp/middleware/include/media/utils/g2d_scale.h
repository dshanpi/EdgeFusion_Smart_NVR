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
/** @file

  File Name     : g2d_scale.h
  Version       : Initial Draft
  Author        : eric_wang
  Created       : 2023/02/17
  Last Modified :
  Description   : use g2d to scale/convert picture size and format
  Function List :
  History       :
*/

#ifndef _G2D_SCALE_H_
#define _G2D_SCALE_H_

#include <mm_common.h>
#include <mm_comm_video.h>
#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct ScalePictureParam
{
    PIXEL_FORMAT_E eSrcPixFormat;
    enum v4l2_colorspace eSrcColorSpace; //V4L2_COLORSPACE_REC709, V4L2_COLORSPACE_REC709_PART_RANGE, enum user_v4l2_colorspace
    unsigned int mSrcPhyAddrs[3];
    SIZE_S mSrcPicSize;
    RECT_S mSrcValidRect;

    PIXEL_FORMAT_E eDstPixFormat;
    enum v4l2_colorspace eDstColorSpace; //V4L2_COLORSPACE_REC709, V4L2_COLORSPACE_REC709_PART_RANGE, enum user_v4l2_colorspace
    unsigned int mDstPhyAddrs[3];
    SIZE_S mDstPicSize;
    RECT_S mDstValidRect;

    int nRotate; //0, 90, 180, 270. clock-wise. can't scale!
    BOOL bHFlip; //true:horizontal flip
    BOOL bVFlip; //false:vertical flip
}ScalePictureParam;

int ScalePictureByG2d(ScalePictureParam *pScaleInfo, int nG2dFd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _G2D_SCALE_H_ */

