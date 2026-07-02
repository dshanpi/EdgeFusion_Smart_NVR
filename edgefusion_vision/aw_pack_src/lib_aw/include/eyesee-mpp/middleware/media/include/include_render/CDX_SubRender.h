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
#ifndef _CDX_SUBRENDER_H__
#define _CDX_SUBRENDER_H__
#ifdef __cplusplus
extern "C" {
#endif

//#include "sub_total_inf.h"
#include "sdecoder.h"
#include "CDX_Charset.h"

#if 0
enum
{
	SUB_RENDER_ALIGN_NONE  		= 0,
	SUB_RENDER_HALIGN_LEFT 		= 1,
	SUB_RENDER_HALIGN_CENTER 	= 2,
	SUB_RENDER_HALIGN_RIGHT 	= 3,
	SUN_RENDER_HALIGN_MASK		= 0x0000000f,
	SUB_RENDER_VALIGN_TOP		= (1 << 4),
	SUB_RENDER_VALIGN_CENTER	= (2 << 4),
	SUB_RENDER_VALIGN_BOTTOM	= (3 << 4),
	SUN_RENDER_VALIGN_MASK		= 0x000000f0
};


 enum
 {
 	SUB_RENDER_STYLE_NONE		    = 0,
 	SUB_RENDER_STYLE_BOLD		    = 1 << 0,
    SUB_RENDER_STYLE_ITALIC         = 1 << 1,
    SUB_RENDER_STYLE_UNDERLINE      = 1 << 2,
    SUB_RENDER_STYLE_STRIKETHROUGH  = 1 << 3
 };
#endif

typedef void CDX_SubRenderHAL;
//int 	SubRenderCreate(int layerStack);
CDX_SubRenderHAL* SubRenderCreate(void *callback_info);
int 	SubRenderDestory(CDX_SubRenderHAL* pHandle);
int 	SubRenderDraw(CDX_SubRenderHAL* pHandle, SubtitleItem *sub_info);
int 	SubRenderShow(CDX_SubRenderHAL* pHandle);
int 	SubRenderHide(CDX_SubRenderHAL* pHandle, unsigned    int  systemTime, int* hasSubShowFlag);
//int 	SubRenderSetTextColor(int color);
//int 	SubRenderGetTextColor();
//int 	SubRenderSetBackColor(int color);
//int 	SubRenderGetBackColor();
//int 	SubRenderSetFontSize(int fontsize);
//int 	SubRenderGetFontSize();
//int 	SubRenderSetPosition(int index,int posx,int posy);
//int 	SubRenderSetYPositionPercent(int index,int percent);
//int 	SubRenderGetPositionX(int index);
//int 	SubRenderGetPositionY(int index);
//int 	SubRenderGetWidth(int index);
//int 	SubRenderGetHeight();
int 	SubRenderSetCharset(CDX_SubRenderHAL* pHandle, int charset);
int 	SubRenderGetCharset(CDX_SubRenderHAL* pHandle);
//int     SubRenderSetAlign(int align);
//int		SubRenderGetAlign();
//int		SubRenderSetZorderTop();
//int     SubRenderSetZorderBottom();
//int     SubRenderSetFontStyle(int style);
//int     SubRenderGetFontStyle();

//int 	SubRenderGetScreenWidth();
//int 	SubRenderGetScreenHeight();
//int 	SubRenderGetScreenDirection();

//only fill SubtitleItem->nPts, nDuration.
int SubRenderGetEarliestEndItem(CDX_SubRenderHAL* pHandle, SubtitleItem *sub_info);


#ifdef __cplusplus
}
#endif
#endif

