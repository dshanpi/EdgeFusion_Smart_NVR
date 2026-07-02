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
#include "de_hw_scale.h"

static int de_device_open()
{
    int de_fd = -1;
    de_fd = open("/dev/disp", O_RDWR);
    if (de_fd < 0)
    {
        printf("Open DE device failed!\n");
        return -1;
    }

    return de_fd;
}

static void de_device_close(int de_fd)
{
    if (de_fd > 0)
        close(de_fd);
}

static int de_reset_device_config(SampleDewbInfo *pDewbInfo)
{
    if (!pDewbInfo)
    {
        aloge("pDewbInfo is null");
        return -1;
    }

    int ret = -1;
    unsigned long arg[4] = {0};
    pDewbInfo->mDeId = 0;

    struct disp_device_config conf;
    memset(&conf, 0, sizeof(conf));
    conf.type = DISP_OUTPUT_TYPE_NONE; // set DISP_OUTPUT_TYPE_NONE to reset DE

    arg[0] = 0; // pDewbInfo->mDeId;
    arg[1] = (unsigned long)&conf;
    ret = ioctl(pDewbInfo->mDeFd, DISP_DEVICE_SET_CONFIG, (void *)arg);
    if (ret != 0)
    {
        printf("de set config err %d\n", ret);
        return -1;
    }

  return 0;
}

static int de_set_device_config(SampleDewbInfo *pDewbInfo)
{
    if (!pDewbInfo)
    {
        aloge("pDewbInfo is null");
        return -1;
    }

    int ret = -1;
    unsigned long arg[4] = {0};
    pDewbInfo->mDeId = 0;

    struct disp_device_config conf;
    memset(&conf, 0, sizeof(conf));
    conf.type = DISP_OUTPUT_TYPE_RTWB;
    conf.mode = DISP_TV_MOD_480P;
    conf.timing.x_res = pDewbInfo->mOutputInfo.mWidth;
    conf.timing.y_res = pDewbInfo->mOutputInfo.mHeight;
    conf.timing.frame_period = 16666667; //60hz
    conf.format = DISP_CSC_TYPE_RGB; //pDewbInfo->mOutputInfo.ePixFmtTpye; // output format type: DISP_CSC_TYPE_RGB
    conf.cs = DISP_BT709;
    conf.bits = DISP_DATA_8BITS;
    conf.eotf = DISP_EOTF_GAMMA22;
    conf.range = DISP_COLOR_RANGE_16_235;
    conf.dvi_hdmi = DISP_HDMI;
    conf.scan = DISP_SCANINFO_NO_DATA;

    arg[0] = 0; //pDewbInfo->mDeId;
    arg[1] = (unsigned long)&conf;
    ret = ioctl(pDewbInfo->mDeFd, DISP_DEVICE_SET_CONFIG, (void*)arg);
    if (ret != 0)
    {
        printf("de set config err %d\n", ret);
    }

    return ret;
}

static int de_capture_start(SampleDewbInfo *pDewbInfo)
{
    if (!pDewbInfo)
    {
        aloge("pDewbInfo is null");
        return -1;
    }

    int ret = -1;
    unsigned long arg[4] = {0};
    struct disp_capture_init_info capture_info;
    memset(&capture_info, 0, sizeof(struct disp_capture_init_info));
    capture_info.port = DISP_CAPTURE_AFTER_DEP;

    arg[0] = 0; //pDewbInfo->mDeId;
    arg[1] = (unsigned long)&capture_info;
    ret = ioctl(pDewbInfo->mDeFd, DISP_CAPTURE_START, (void *)arg);
    if (ret != 0)
    {
        printf("de capture start err %d\n", ret);
    }

    return ret;
}

static int de_capture_stop(SampleDewbInfo *pDewbInfo)
{
    if (!pDewbInfo)
    {
        aloge("pDewbInfo is null");
        return -1;
    }

    int ret = -1;
    unsigned long arg[4] = {0};

    arg[0] = 0; //pDewbInfo->mDeId;
    ret = ioctl(pDewbInfo->mDeFd, DISP_CAPTURE_STOP, (void *)arg);
    if (ret != 0)
    {
        printf("de capture stop err %d\n", ret);
    }

    return ret;
}

static int setup_input_config(SampleDewbInfo *pDewbInfo, struct disp_layer_config2 *config, struct disp_capture_info2 *info)
{
    if(!pDewbInfo || !config || !info)
    {
        aloge("param is null, please check it");
        return -1;
    }

    long long input_h = ((long long)pDewbInfo->mInputInfo.mHeight) << 32;
    long long input_w = ((long long)pDewbInfo->mInputInfo.mWidth) << 32;

    config->channel                 = 0;
    config->layer_id                = 0;
    config->enable                  = 1;
    config->info.mode               = LAYER_MODE_BUFFER;
    config->info.zorder             = 0;
    config->info.alpha_mode         = 2;    // global pixel alpha
    config->info.alpha_value        = 0xff; // global pixel alpha
    config->info.screen_win.x       = 0;
    config->info.screen_win.y       = 0;
    config->info.screen_win.width   = pDewbInfo->mOutputInfo.mWidth;
    config->info.screen_win.height  = pDewbInfo->mOutputInfo.mHeight;
    config->info.fb.crop.y          = 0;
    config->info.fb.crop.x          = 0;
    config->info.fb.crop.height     = input_h; // 定点小数。高32bit 为整数，低32bit 为小数
    config->info.fb.crop.width      = input_w; // 定点小数。高32bit 为整数，低32bit 为小数
    config->info.fb.size[0].width   = pDewbInfo->mInputInfo.mWidth;
    config->info.fb.size[0].height  = pDewbInfo->mInputInfo.mHeight;
    config->info.fb.fd              = pDewbInfo->mInputInfo.mIonFd; // layer->input.fb.fd;
    config->info.fb.eotf            = DISP_EOTF_UNDEF;
    config->info.fb.format          = DISP_FORMAT_YUV420_SP_UVUV; //pDewbInfo->mInputInfo.ePixFmt; //DISP_FORMAT_RGB_888; // DISP_FORMAT_YUV420_P
    config->info.fb.align[0]        = 4;

    if (config->info.fb.format == DISP_FORMAT_YUV420_P
            || config->info.fb.format == DISP_FORMAT_YUV420_SP_UVUV
            || config->info.fb.format == DISP_FORMAT_YUV420_SP_VUVU
            || config->info.fb.format == DISP_FORMAT_YUV420_SP_UVUV_10BIT
            || config->info.fb.format == DISP_FORMAT_YUV420_SP_VUVU_10BIT) {
        config->info.fb.size[1].width       = pDewbInfo->mInputInfo.mWidth / 2;
        config->info.fb.size[1].height      = pDewbInfo->mInputInfo.mHeight / 2;
        config->info.fb.size[2].width       = pDewbInfo->mInputInfo.mWidth / 2;
        config->info.fb.size[2].height      = pDewbInfo->mInputInfo.mHeight / 2;
    } else if (config->info.fb.format == DISP_FORMAT_YUV444_P) {
        config->info.fb.size[1].width       = pDewbInfo->mInputInfo.mWidth;
        config->info.fb.size[1].height      = pDewbInfo->mInputInfo.mHeight;
        config->info.fb.size[2].width       = pDewbInfo->mInputInfo.mWidth;
        config->info.fb.size[2].height      = pDewbInfo->mInputInfo.mHeight;
    } else if (config->info.fb.format == DISP_FORMAT_YUV422_P
            || config->info.fb.format == DISP_FORMAT_YUV422_SP_UVUV
            || config->info.fb.format == DISP_FORMAT_YUV422_SP_VUVU
            || config->info.fb.format == DISP_FORMAT_YUV422_SP_UVUV_10BIT
            || config->info.fb.format == DISP_FORMAT_YUV422_SP_VUVU_10BIT) {
        config->info.fb.size[1].width    = config->info.fb.size[0].width / 2;
        config->info.fb.size[1].height   = config->info.fb.size[0].height;
        config->info.fb.size[2].width    = config->info.fb.size[0].width / 2;
        config->info.fb.size[2].height   = config->info.fb.size[0].height;
    } else if (config->info.fb.format == DISP_FORMAT_YUV411_P
                    || config->info.fb.format == DISP_FORMAT_YUV411_SP_UVUV
                    || config->info.fb.format == DISP_FORMAT_YUV411_SP_VUVU
                    || config->info.fb.format == DISP_FORMAT_YUV411_SP_UVUV_10BIT
                    || config->info.fb.format == DISP_FORMAT_YUV411_SP_VUVU_10BIT) {
        config->info.fb.size[1].width    = config->info.fb.size[0].width / 4;
        config->info.fb.size[1].height   = config->info.fb.size[0].height;
        config->info.fb.size[2].width    = config->info.fb.size[0].width / 4;
        config->info.fb.size[2].height   = config->info.fb.size[0].height;
    } else {
        config->info.fb.size[1].width    = config->info.fb.size[0].width;
        config->info.fb.size[1].height   = config->info.fb.size[0].height;
        config->info.fb.size[2].width    = config->info.fb.size[0].width;
        config->info.fb.size[2].height   = config->info.fb.size[0].height;
    }

    info->window.x = 0;
    info->window.y = 0;
    info->window.width = pDewbInfo->mOutputInfo.mWidth;
    info->window.height = pDewbInfo->mOutputInfo.mHeight;
    info->out_frame.size[0].width = pDewbInfo->mOutputInfo.mWidth;
    info->out_frame.size[0].height = pDewbInfo->mOutputInfo.mHeight;
    info->out_frame.crop.x = 0;
    info->out_frame.crop.y = 0;
    info->out_frame.crop.width = pDewbInfo->mOutputInfo.mWidth;
    info->out_frame.crop.height = pDewbInfo->mOutputInfo.mHeight;
    info->out_frame.format = DISP_FORMAT_RGB_888; // pDewbInfo->mOutputInfo.ePixFmt; // DISP_FORMAT_RGB_888
    info->out_frame.fd = pDewbInfo->mOutputInfo.mIonFd;

    return 0;
}


int De_Init(SampleDewbInfo *pDewbInfo)
{
    int ret = -1;

    if (!pDewbInfo)
    {
        aloge("pDewbInfo is null");
        return -1;
    }

    /* open disp drv */
    pDewbInfo->mDeFd =  de_device_open();
    if (pDewbInfo->mDeFd < 0)
    {
        printf("de_device_open err\n");
        return -1;
    }

    /* set rtwb device mode */
    ret = de_set_device_config(pDewbInfo);
    if (ret != 0)
    {
        printf("de_set_device_config err, ret=%d\n", ret);
        de_device_close(pDewbInfo->mDeFd);
        return -1;
    }

    /* capture start */
    ret = de_capture_start(pDewbInfo);
    if ( ret != 0)
    {
        printf("de capture start failed, ret=%d\n", ret);
        if (pDewbInfo->mDeFd > 0)
            de_device_close(pDewbInfo->mDeFd);

        return -1;
    }

    return 0;
}

int De_Wb_Scale(SampleDewbInfo *pDewbInfo)
{
    if (!pDewbInfo)
    {
        aloge("pDewbInfo is null");
        return -1;
    }

    int ret = -1;
    unsigned long arg[4] = {0};

    /* setup input config */
    struct disp_layer_config2 config;
    struct disp_capture_info2 info;
    memset(&config, 0, sizeof(struct disp_layer_config2));
    memset(&info, 0, sizeof(struct disp_capture_info2));
    setup_input_config(pDewbInfo, &config, &info);

    /* capture commit */
    arg[0] = 0; // pDewbInfo->mDeId;    // screen 0
    arg[1] = (unsigned long)&config;
    arg[2] = 1;
    arg[3] = (unsigned long)&info;
    ret = ioctl(pDewbInfo->mDeFd, DISP_RTWB_COMMIT, (void *)(arg));
    if (ret != 0)
    {
        printf("de capture rtwb commit err %d\n", ret);
        return -1;
    }

    return 0;
}

int De_reset(SampleDewbInfo *pDewbInfo)
{
    int ret = -1;

    if(!pDewbInfo)
    {
        aloge("pDewbInfo is null");
        return -1;
    }

    ret = de_capture_stop(pDewbInfo);
    if ( ret != 0 )
    {
        printf("de capture stop failed, ret=%d\n", ret);
        return ret;
    }

    /*reset DE config*/
    de_reset_device_config(pDewbInfo);
    printf("De_reset, de_reset_device_config\n");

    /* set rtwb device mode */
    de_set_device_config(pDewbInfo);
    printf("De_reset, de_set_device_config\n");

    /* capture start */
    ret = de_capture_start(pDewbInfo);
    if ( ret != 0 )
    {
        printf("de capture start failed, ret=%d\n", ret);
        return ret;
    }

    return 0;
}

int De_Deinit(SampleDewbInfo *pDewbInfo)
{
    int ret = -1;

    if (!pDewbInfo)
    {
        aloge("pDewbInfo is null");
        return -1;
    }

    ret = de_capture_stop(pDewbInfo);
    if ( ret != 0)
    {
        printf("de capture stop failed, ret=%d\n", ret);
        de_device_close(pDewbInfo->mDeFd);
        return -1;
    }

    if (pDewbInfo->mDeFd > 0)
        de_device_close(pDewbInfo->mDeFd);

    return 0;
}
