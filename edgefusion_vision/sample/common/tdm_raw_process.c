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
#include <sys/time.h>

#include "media/mpi_isp.h"

#include "tdm_raw_process.h"

#define TDM_RAW_PROCESS_NUM_MAX       (4)

typedef struct tdm_raw_process_context {
    ISP_DEV mIsp;
    tdm_raw_process_config_param config_param;
    int process_enable;
    int process_frame_cnt;
    FILE *tdm_raw_fp;
    unsigned char *tdm_buf;
    unsigned int tdm_buf_size;
} tdm_raw_process_context;

static tdm_raw_process_context gTdmRawProcessContext[TDM_RAW_PROCESS_NUM_MAX];

static struct tdm_raw_process_context *tdm_raw_process_get_context(int isp)
{
    if (isp >= TDM_RAW_PROCESS_NUM_MAX)
    {
        aloge("fatal error! invalid isp id %d >= %d\n", isp, TDM_RAW_PROCESS_NUM_MAX);
        return NULL;
    }
    return &gTdmRawProcessContext[isp];
}

static void tdm_raw_data_process_callback(void *param)
{
    int ret = 0;
    struct vin_isp_tdm_event_status *status = (struct vin_isp_tdm_event_status *)param;
    if (NULL == status)
    {
        aloge("fatal error! status is NULL!");
        return;
    }
    int isp = status->dev_id;
    if (isp >= TDM_RAW_PROCESS_NUM_MAX)
    {
        aloge("fatal error! invalid isp id %d >= %d\n", isp, TDM_RAW_PROCESS_NUM_MAX);
        return;
    }
    struct tdm_raw_process_context *pContext = tdm_raw_process_get_context(isp);

    if (pContext->process_enable)
    {
        if (!(0 == pContext->config_param.frame_cnt_min && 0 == pContext->config_param.frame_cnt_max))
        {
            if (pContext->config_param.frame_cnt_min > pContext->process_frame_cnt || pContext->process_frame_cnt >= pContext->config_param.frame_cnt_max)
            {
                goto exit;
            }
        }

        unsigned int tdm_buf_id = status->buf_id;
        unsigned int tdm_buf_size = status->buf_size;
        unsigned int tdm_frm_fill_len = status->fill_len;
        unsigned int tdm_frm_head_len = status->head_len;
        unsigned int width = pContext->config_param.width;
        unsigned int height = pContext->config_param.height;

        switch (pContext->config_param.type)
        {
            case TDM_RAW_DUMP_8BIT:
            case TDM_RAW_DUMP_10BIT:
            {
                if (NULL == pContext->tdm_raw_fp)
                {
                    char fdstr[128];
                    snprintf(fdstr, 128, "%s", pContext->config_param.tdm_raw_file_path);
                    pContext->tdm_raw_fp = fopen(fdstr, "w");
                    if (NULL == pContext->tdm_raw_fp)
                    {
                        aloge("fatal error! open %s failed! errno is %d", fdstr, errno);
                        break;
                    }
                }
                if (NULL == pContext->tdm_buf)
                {
                    if (0 == tdm_buf_size)
                    {
                        aloge("fatal error! isp%d tdm_buf_size is 0 !", isp);
                        fclose(pContext->tdm_raw_fp);
                        break;
                    }
                    pContext->tdm_buf = malloc(tdm_buf_size);
                    if (pContext->tdm_buf == NULL)
                    {
                        aloge("fatal error! isp%d malloc size is %d error!", isp, tdm_buf_size);
                        fclose(pContext->tdm_raw_fp);
                        break;
                    }
                    memset(pContext->tdm_buf, 0, tdm_buf_size);
                    pContext->tdm_buf_size = tdm_buf_size;
                }

                struct vin_isp_tdm_data data;
                memset(&data, 0, sizeof(struct vin_isp_tdm_data));
                data.buf = pContext->tdm_buf;
                memset(data.buf, 0, tdm_buf_size);
                data.buf_size = tdm_buf_size;
                data.req_buf_id = tdm_buf_id;
                AW_MPI_ISP_GetTdmData(isp, &data);

                char *tdm_data = data.buf;
                int tdm_data_len = data.buf_size;

                fwrite(tdm_data, tdm_data_len, 1, pContext->tdm_raw_fp);

                alogv("isp%d save tdm raw file success, tdm_buf id:%d size:%d", isp, tdm_buf_id, tdm_buf_size);

                break;
            }
            case TDM_RAW_DUMP_8BIT_FOR_TOOLS:
            {
                char fdstr[128];
                FILE *fp = NULL;
                snprintf(fdstr, 128, "%s_%dx%d_%d_%d.bin", pContext->config_param.tdm_raw_file_path, width, height, pContext->process_frame_cnt, tdm_buf_id);
                fp = fopen(fdstr, "w");
                if (NULL == fp)
                {
                    aloge("fatal error! open %s failed! errno is %d", fdstr, errno);
                    break;
                }

                struct vin_isp_tdm_data data;
                memset(&data, 0, sizeof(struct vin_isp_tdm_data));
                data.buf = malloc(tdm_buf_size);
                if (data.buf == NULL)
                {
                    aloge("fatal error! isp%d malloc size is %d error!", isp, tdm_buf_size);
                    fclose(fp);
                    break;
                }
                memset(data.buf, 0, tdm_buf_size);
                data.buf_size = tdm_buf_size;
                data.req_buf_id = tdm_buf_id;
                AW_MPI_ISP_GetTdmData(isp, &data);

                char *tdm_data = data.buf + tdm_frm_fill_len + tdm_frm_head_len;
                int tdm_data_len = data.buf_size - tdm_frm_fill_len - tdm_frm_head_len;

                fwrite(tdm_data, tdm_data_len, 1, fp);

                alogv("isp%d save tdm raw file success, tdm_buf id:%d size:%d, tdm_frm fill_len:%d head_len:%d",
                    isp, tdm_buf_id, tdm_buf_size, tdm_frm_fill_len, tdm_frm_head_len);

                if (data.buf)
                {
                    free(data.buf);
                    data.buf = NULL;
                    data.buf_size = 0;
                }
                if (fp)
                {
                    fclose(fp);
                    fp = NULL;
                }

                break;
            }
            case TDM_RAW_DUMP_10BIT_FOR_TOOLS:
            {
                char fdstr[128];
                FILE *fp = NULL;
                snprintf(fdstr, 128, "%s_%dx%d_%d_%d.bin", pContext->config_param.tdm_raw_file_path, width, height, pContext->process_frame_cnt, tdm_buf_id);
                fp = fopen(fdstr, "w");
                if (NULL == fp)
                {
                    aloge("fatal error! open %s failed! errno is %d", fdstr, errno);
                    break;
                }

                struct vin_isp_tdm_data data;
                memset(&data, 0, sizeof(struct vin_isp_tdm_data));
                data.buf = malloc(tdm_buf_size);
                if (data.buf == NULL)
                {
                    aloge("fatal error! isp%d malloc size is %d error!", isp, tdm_buf_size);
                    fclose(fp);
                    break;
                }
                memset(data.buf, 0, tdm_buf_size);
                data.buf_size = tdm_buf_size;
                data.req_buf_id = tdm_buf_id;
                AW_MPI_ISP_GetTdmData(isp, &data);

                char *tdm_data = data.buf + tdm_frm_fill_len + tdm_frm_head_len;
                int tdm_data_len = data.buf_size - tdm_frm_fill_len - tdm_frm_head_len;

                int buf_size = width*height*2;
                unsigned short *buf = (unsigned short *)malloc(buf_size);
                if (buf == NULL)
                {
                    aloge("fatal error! isp%d malloc size is %d error!", isp, buf_size);
                    free(data.buf);
                    fclose(fp);
                    break;
                }
                memset(buf, 0, buf_size);

                int stride = (((width * 10) + 511) & ~511)/8;
                for (int h = 0; h < height; h++)
                {
                    for (int w = 0; w < stride/5; w++)
                    {
                        unsigned short p0 = tdm_data[stride*h + w*5 + 0];
                        unsigned short p1 = tdm_data[stride*h + w*5 + 1];
                        unsigned short p2 = tdm_data[stride*h + w*5 + 2];
                        unsigned short p3 = tdm_data[stride*h + w*5 + 3];
                        unsigned short p4 = tdm_data[stride*h + w*5 + 4];
                        buf[width*h + w*4 + 0] = (p0 << 2) + (p4 >> 6);
                        buf[width*h + w*4 + 1] = (p1 << 2) + ((p4 & 0x3f) >> 4);
                        buf[width*h + w*4 + 2] = (p2 << 2) + ((p4 & 0xf) >> 2);
                        buf[width*h + w*4 + 3] = (p3 << 2) + (p4 & 0x3);
                    }
                }

                fwrite(buf, buf_size, 1, fp);

                alogv("isp%d save tdm raw file success, tdm_buf id:%d size:%d, tdm_frm fill_len:%d head_len:%d",
                    isp, tdm_buf_id, tdm_buf_size, tdm_frm_fill_len, tdm_frm_head_len);

                if (buf)
                {
                    free(buf);
                    buf = NULL;
                    buf_size = 0;
                }
                if (data.buf)
                {
                    free(data.buf);
                    data.buf = NULL;
                    data.buf_size = 0;
                }
                if (fp)
                {
                    fclose(fp);
                    fp = NULL;
                }

                break;
            }
            case TDM_RAW_SEND_8BIT:
            case TDM_RAW_SEND_10BIT:
            {
                if (NULL == pContext->tdm_raw_fp)
                {
                    char fdstr[128];
                    snprintf(fdstr, 128, "%s", pContext->config_param.tdm_raw_file_path);
                    pContext->tdm_raw_fp = fopen(fdstr, "r");
                    if (NULL == pContext->tdm_raw_fp)
                    {
                        aloge("fatal error! open %s failed! errno is %d", fdstr, errno);
                        break;
                    }
                }
                if (NULL == pContext->tdm_buf)
                {
                    if (0 == tdm_buf_size)
                    {
                        aloge("fatal error! isp%d tdm_buf_size is 0 !", isp);
                        fclose(pContext->tdm_raw_fp);
                        break;
                    }
                    pContext->tdm_buf = malloc(tdm_buf_size);
                    if (pContext->tdm_buf == NULL)
                    {
                        aloge("fatal error! isp%d malloc size is %d error!", isp, tdm_buf_size);
                        fclose(pContext->tdm_raw_fp);
                        break;
                    }
                    memset(pContext->tdm_buf, 0, tdm_buf_size);
                    pContext->tdm_buf_size = tdm_buf_size;
                }

                struct vin_isp_tdm_data data;
                memset(&data, 0, sizeof(struct vin_isp_tdm_data));
                data.buf = pContext->tdm_buf;
                memset(data.buf, 0, tdm_buf_size);
                data.buf_size = tdm_buf_size;
                data.req_buf_id = tdm_buf_id;

                ret = fread(data.buf, 1, data.buf_size, pContext->tdm_raw_fp);
                if (ret != data.buf_size)
                {
                    aloge("fatal error! isp%d read file failed! ret %d != size %d", isp, ret, data.buf_size);
                }

                ret = AW_MPI_ISP_SendTdmData(isp, &data);
                if (0 == ret)
                    alogv("isp%d send tdm raw file success, tdm_buf id:%d size:%d", isp, tdm_buf_id, tdm_buf_size);
                else
                    aloge("fatal error! isp%d send tdm raw file failed! ret=%d", isp, ret);

                break;
            }
            default:
                aloge("fatal error! isp%d tdm raw process type %d is invalid!", isp, pContext->config_param.type);
                break;
        }
    }

exit:
    pContext->process_frame_cnt++;

    AW_MPI_ISP_ReturnTdmBuf(isp, status);

    return;
}

int tdm_raw_process_open(int isp, tdm_raw_process_config_param *param)
{
    int ret = 0;

    if (isp >= TDM_RAW_PROCESS_NUM_MAX)
    {
        aloge("fatal error! invalid isp id %d >= %d\n", isp, TDM_RAW_PROCESS_NUM_MAX);
        return -1;
    }
    struct tdm_raw_process_context *pContext = tdm_raw_process_get_context(isp);
    memset(pContext, 0, sizeof(tdm_raw_process_context));

    pContext->mIsp = isp;
    memcpy(&pContext->config_param, param, sizeof(tdm_raw_process_config_param));

    alogd("isp%d open process tdm raw data, type:%d, path:%s, w:%d, h:%d, frame_cnt min:%d max:%d", isp,
        pContext->config_param.type, pContext->config_param.tdm_raw_file_path,
        pContext->config_param.width, pContext->config_param.height,
        pContext->config_param.frame_cnt_min, pContext->config_param.frame_cnt_max);

    ret |= AW_MPI_ISP_RegisterTdmBufDoneCallback(isp, &tdm_raw_data_process_callback);

    return ret;
}

int tdm_raw_process_start(int isp)
{
    if (isp >= TDM_RAW_PROCESS_NUM_MAX)
    {
        aloge("fatal error! invalid isp id %d >= %d\n", isp, TDM_RAW_PROCESS_NUM_MAX);
        return -1;
    }
    struct tdm_raw_process_context *pContext = tdm_raw_process_get_context(isp);

    pContext->process_frame_cnt = 0;
    pContext->process_enable = 1;

    alogd("isp%d start process tdm raw data", isp);

    return 0;
}

int tdm_raw_process_stop(int isp)
{
    if (isp >= TDM_RAW_PROCESS_NUM_MAX)
    {
        aloge("fatal error! invalid isp id %d >= %d\n", isp, TDM_RAW_PROCESS_NUM_MAX);
        return -1;
    }
    struct tdm_raw_process_context *pContext = tdm_raw_process_get_context(isp);

    pContext->process_enable = 0;

    alogd("isp%d stop process tdm raw data", isp);

    return 0;
}

int tdm_raw_process_close(int isp)
{
    int ret = 0;

    if (isp >= TDM_RAW_PROCESS_NUM_MAX)
    {
        aloge("fatal error! invalid isp id %d >= %d\n", isp, TDM_RAW_PROCESS_NUM_MAX);
        return -1;
    }
    struct tdm_raw_process_context *pContext = tdm_raw_process_get_context(isp);

    alogd("isp%d close process tdm raw data", isp);

    ret |= AW_MPI_ISP_RegisterTdmBufDoneCallback(isp, NULL);

    if (pContext->tdm_buf)
    {
        free(pContext->tdm_buf);
        pContext->tdm_buf = NULL;
        pContext->tdm_buf_size = 0;
    }

    if (pContext->tdm_raw_fp)
    {
        fclose(pContext->tdm_raw_fp);
        pContext->tdm_raw_fp = NULL;
    }

    return ret;
}
