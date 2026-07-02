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
#include "aiservice_pdet.h"

static int nv21_draw_point(unsigned char* yBuffer, unsigned char* uvBuffer, int w, int h, int x, int y, int yColor, int uColor, int vColor) {
    if (x < 0 || x >= w) return -1;
    if (y < 0 || y >= h) return -1;
    yBuffer[y*w+x] = yColor;
    uvBuffer[(y/2)*w+x/2*2] = uColor;
    uvBuffer[(y/2)*w+x/2*2+1] = vColor;
    return 0;
}

static int nv21_draw_rect(unsigned char* yBuffer, unsigned char* uvBuffer, int w, int h, int left, int top, int right, int bottom, int yColor, int uColor, int vColor) {
    int i;
    for (i = left; i <= right; i++) {
        nv21_draw_point(yBuffer, uvBuffer, w, h, i, top, yColor, uColor, vColor);
        nv21_draw_point(yBuffer, uvBuffer, w, h, i, bottom, yColor, uColor, vColor);
    }
    for (i = top; i <= bottom; i++) {
        nv21_draw_point(yBuffer, uvBuffer, w, h, left, i, yColor, uColor, vColor);
        nv21_draw_point(yBuffer, uvBuffer, w, h, right, i, yColor, uColor, vColor);
    }
    return 0;
}


int pdet_init(SamplePdetInfo *pPdetInfo)
{
    if (!pPdetInfo)
    {
        aloge("pPdetInfo is null");
        return -1;
    }

    pPdetInfo->pdet_container = (AW_Det_Container *)calloc(1, sizeof(AW_Det_Container));
    if (!pPdetInfo->pdet_container)
    {
        aloge("fail to allocate memory for [pdet_container]\n");
        goto _err_calloc_container;
    }

    pPdetInfo->pdet_outputs = (AW_Det_Outputs *)calloc(1, sizeof(AW_Det_Outputs));
    if(!pPdetInfo->pdet_outputs)
    {
        aloge("fail to allocate memory for [pdet_outputs]\n");
        goto _err_calloc_pdet_output;
    }

    int flag = aw_init_person_det(pPdetInfo->pdet_container, pPdetInfo->pdet_outputs, \
                                    pPdetInfo->pdet_input_w, pPdetInfo->pdet_input_h, \
                                    pPdetInfo->pdet_input_c, pPdetInfo->pdet_conf_thres);
    if (flag == -1)
    {
        aloge("fail to make smile detction pdet_container\n");
        goto _err_init_pdet;
    }
    alogd("aw_init_person_det ok!\n");


    return 0;

_err_init_pdet:
    if(pPdetInfo->pdet_container != NULL)
    {
        aw_free_person_det(pPdetInfo->pdet_container, pPdetInfo->pdet_outputs);
        pPdetInfo->pdet_container = NULL;
    }

_err_calloc_pdet_output:
    if(pPdetInfo->pdet_outputs != NULL)
    {
        free(pPdetInfo->pdet_outputs);
        pPdetInfo->pdet_outputs = NULL;
    }

_err_calloc_container:
    if(pPdetInfo->pdet_container != NULL)
    {
        free(pPdetInfo->pdet_container);
        pPdetInfo->pdet_container = NULL;
    }

    return -1;
}


void pdet_deinit(SamplePdetInfo *pPdetInfo)
{
    if (!pPdetInfo)
    {
        aloge("pPdetInfo is null");
        return;
    }

    if(pPdetInfo->pdet_container != NULL)
    {
        aw_free_person_det(pPdetInfo->pdet_container, pPdetInfo->pdet_outputs);
        pPdetInfo->pdet_container = NULL;
        pPdetInfo->pdet_outputs = NULL;
    }

    if(pPdetInfo->pdet_container != NULL)
    {
        free(pPdetInfo->pdet_container);
        pPdetInfo->pdet_container = NULL;
    }

    if(pPdetInfo->pdet_outputs != NULL)
    {
        free(pPdetInfo->pdet_outputs);
        pPdetInfo->pdet_outputs = NULL;
    }
}

int pdet_run(SamplePdetInfo *pPdetInfo, AW_Img *input_img)
{
    if (!pPdetInfo || !input_img)
    {
        aloge("pPdetInfo or input_img is null, please check it");
        return -1;
    }

    int i = 0, j = 0;
    pPdetInfo->pdet_outputs->num = 0;
    memset(&pPdetInfo->mCoordinate, 0, sizeof(pPdetInfo->mCoordinate));

    aw_run_person_det(pPdetInfo->pdet_container, input_img, pPdetInfo->pdet_outputs, pPdetInfo->pdet_conf_thres);

    for (i = 0; i < pPdetInfo->pdet_outputs->num; i++)
    {
        int label = pPdetInfo->pdet_outputs->persons[i].label;
        float prob = (float)(pPdetInfo->pdet_outputs->persons[i].prob);

        if (label == 1)
            continue;

        int x1 =pPdetInfo->pdet_outputs->persons[i].bbox.tl_x;
        int y1 =pPdetInfo->pdet_outputs->persons[i].bbox.tl_y;
        int x2 =pPdetInfo->pdet_outputs->persons[i].bbox.br_x;
        int y2 =pPdetInfo->pdet_outputs->persons[i].bbox.br_y;
        alogd("%d, label %d, prob %f, x1 %d, y1 %d, x2 %d, y2 %d\n", i, label, prob, x1, y1, x2, y2);

        if (i < MAX_VIPP_ORL_NUM)
        {
            pPdetInfo->mCoordinate[j].x = x1;
            pPdetInfo->mCoordinate[j].y = y1;
            pPdetInfo->mCoordinate[j].width = x2 - x1;
            pPdetInfo->mCoordinate[j].height = y2 - y1;
            j++;
        }
    }
    pPdetInfo->pdet_num = j;

    if (pPdetInfo->pdet_num > 0)
    {
        alogd("pdet_outputs_num: %d\n", pPdetInfo->pdet_num);
    }

    return 0;
}


