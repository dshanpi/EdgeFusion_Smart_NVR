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
#include <utils/plat_log.h>
#include <mpi_region.h>
#include "sample_common_osd.h"
#include "plat_math.h"

static FONT_SIZE_TYPE g_font_size = FONT_SIZE_BUTT;

static int CreateRgbPic(RGB_PIC_S *pRgbPic, char *text)
{
    int ret = 0;
    FONT_RGBPIC_S font_pic;
    memset(&font_pic, 0, sizeof(FONT_RGBPIC_S));
    font_pic.font_type     = FONT_SIZE_32;
    font_pic.rgb_type      = OSD_RGB_32;
    font_pic.enable_bg     = 0;
    // font_pic.foreground[0] = 0x6;
    // font_pic.foreground[1] = 0x1;
    // font_pic.foreground[2] = 0xFF;
    // font_pic.foreground[3] = 0xFF;
    // font_pic.background[0] = 0x21;
    // font_pic.background[1] = 0x21;
    // font_pic.background[2] = 0x21;
    // font_pic.background[3] = 0x11;
    font_pic.foreground[0] = 0xFF;
    font_pic.foreground[1] = 0xFF;
    font_pic.foreground[2] = 0xFF;
    font_pic.foreground[3] = 0xFF;
    font_pic.background[0] = 0x0;
    font_pic.background[1] = 0x0;
    font_pic.background[2] = 0x0;
    font_pic.background[3] = 0x0;

    memset(pRgbPic, 0, sizeof(RGB_PIC_S));
    pRgbPic->enable_mosaic = 0;
    pRgbPic->rgb_type      = OSD_RGB_32;
    ret = create_font_rectangle(text, &font_pic, pRgbPic);

    return ret;
}

static int ReleaseRgbPic(RGB_PIC_S *pRgbPic)
{
    return release_rgb_picture(pRgbPic);
}

static void SetStreamRGN(StreamOsdConfig *pStreamOsdConfig, int idx)
{
    BITMAP_S stBitmap;

    memset(&stBitmap, 0, sizeof(BITMAP_S));
    stBitmap.mPixelFormat = MM_PIXEL_FORMAT_RGB_8888;
    stBitmap.mWidth = pStreamOsdConfig->mRgbPic[idx].wide;
    stBitmap.mHeight = pStreamOsdConfig->mRgbPic[idx].high;
    stBitmap.mpData  = pStreamOsdConfig->mRgbPic[idx].pic_addr;
    AW_MPI_RGN_SetBitMap(pStreamOsdConfig->mOverlayDrawStreamOSDBase + idx, &stBitmap);
}

static void PrepareStremaRGN(StreamOsdConfig *pStreamOsdConfig, int mVeChn, int x, int y, int idx)
{
    RGN_ATTR_S stRegion;
    RGN_CHN_ATTR_S stRgnChnAttr;
    int overlay_x = 0;
    int overlay_y = 0;
    int ret = 0;

    overlay_x = x;
    overlay_y = y;
    overlay_x = AWALIGN(overlay_x, 16);
    overlay_y = AWALIGN(overlay_y, 16);
    alogv("StreamOSD[%d] coordinate(%d,%d)", idx, overlay_x, overlay_y);

    memset(&stRegion, 0, sizeof(RGN_ATTR_S));
    stRegion.enType = OVERLAY_RGN;
    stRegion.unAttr.stOverlay.mPixelFmt = MM_PIXEL_FORMAT_RGB_8888;
    stRegion.unAttr.stOverlay.mSize.Width = pStreamOsdConfig->mRgbPic[idx].wide;
    stRegion.unAttr.stOverlay.mSize.Height = pStreamOsdConfig->mRgbPic[idx].high;
    AW_MPI_RGN_Create(pStreamOsdConfig->mOverlayDrawStreamOSDBase + idx, &stRegion);

    MPP_CHN_S VeChn = {MOD_ID_VENC, 0, mVeChn};
    memset(&stRgnChnAttr, 0, sizeof(RGN_CHN_ATTR_S));
    stRgnChnAttr.bShow = TRUE;
    stRgnChnAttr.enType = stRegion.enType;
    stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.X = overlay_x;
    stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.Y = overlay_y;
    stRgnChnAttr.unChnAttr.stOverlayChn.mLayer = 0;
    stRgnChnAttr.unChnAttr.stOverlayChn.mFgAlpha = 0x40; // global alpha mode value for ARGB1555
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.Width = 16;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.Height = 16;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.mLumThresh = 60;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUMDIFF_THRESH;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = TRUE; // OSD反色
    AW_MPI_RGN_AttachToChn(pStreamOsdConfig->mOverlayDrawStreamOSDBase + idx, &VeChn, &stRgnChnAttr);
}

static void DestroyStreamRGN(StreamOsdConfig *pStreamOsdConfig, int mVeChn, int idx)
{
    if (idx >= 16)
    {
        aloge("fatal error! invalid idx %d", idx);
        return;
    }
    pStreamOsdConfig->mRgbPicLastSize[idx] = 0;
    MPP_CHN_S VeChn = {MOD_ID_VENC, 0, mVeChn};
    AW_MPI_RGN_DetachFromChn(pStreamOsdConfig->mOverlayDrawStreamOSDBase + idx, &VeChn);
    AW_MPI_RGN_Destroy(pStreamOsdConfig->mOverlayDrawStreamOSDBase + idx);
}

int UpdateStreamOSDRgb(StreamOsdConfig *pStreamOsdConfig, int mVeChn, char *text,  int x, int y, int idx)
{
    unsigned int curSize = 0;
    int ret = 0;

    if (FONT_SIZE_BUTT == g_font_size)
    {
        g_font_size = FONT_SIZE_32;
        ret = load_font_file(g_font_size);
        if (ret < 0)
        {
            aloge("load_font_file %d fail! ret:%d\n", ret, g_font_size);
            return ret;
        }
    }

    if (0 != CreateRgbPic(&pStreamOsdConfig->mRgbPic[idx], text))
    {
        aloge("fatal error!!! CreateFrontRectangle fail");
        return -1;
    }

    curSize = pStreamOsdConfig->mRgbPic[idx].wide * pStreamOsdConfig->mRgbPic[idx].high * 4;

    if (pStreamOsdConfig->mRgbPicLastSize[idx] != curSize)
    {
        DestroyStreamRGN(pStreamOsdConfig, mVeChn, idx);
        PrepareStremaRGN(pStreamOsdConfig, mVeChn, x, y, idx);
    }
    pStreamOsdConfig->mRgbPicLastSize[idx] = curSize;

    SetStreamRGN(pStreamOsdConfig, idx);
    ReleaseRgbPic(&pStreamOsdConfig->mRgbPic[idx]);

    return ret;
}

int CreateStreamOSD(StreamOsdConfig *pStreamOsdConfig, int mVeChn, char *text,  int x, int y, int idx)
{
    int ret = 0;

    if (FONT_SIZE_BUTT == g_font_size)
    {
        g_font_size = FONT_SIZE_32;
        ret = load_font_file(g_font_size);
        if (ret < 0)
        {
            aloge("load_font_file %d fail! ret:%d\n", ret, g_font_size);
            return ret;
        }
    }

    if (0 != CreateRgbPic(&pStreamOsdConfig->mRgbPic[idx], text))
    {
        aloge("fatal error!!! CreateFrontRectangle fail");
        return -1;
    }

    PrepareStremaRGN(pStreamOsdConfig, mVeChn, x, y, idx);

    pStreamOsdConfig->mRgbPicLastSize[idx] = pStreamOsdConfig->mRgbPic[idx].wide * pStreamOsdConfig->mRgbPic[idx].high * 4;

    SetStreamRGN(pStreamOsdConfig, idx);
    ReleaseRgbPic(&pStreamOsdConfig->mRgbPic[idx]);

    return ret;
}

void DestroyStreamOSD(StreamOsdConfig *pStreamOsdConfig, int mVeChn, int idx)
{
    int ret = 0;

    if (FONT_SIZE_BUTT != g_font_size)
    {
        ret = unload_font_file(g_font_size);
        if (ret < 0)
        {
            aloge("unload_font_file %d fail! ret:%d\n", ret, g_font_size);
        }
        g_font_size = FONT_SIZE_BUTT;
    }

    DestroyStreamRGN(pStreamOsdConfig, mVeChn, idx);
}