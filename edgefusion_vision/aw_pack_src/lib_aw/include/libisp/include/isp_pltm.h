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

#ifndef _ISP_PLTM_H_
#define _ISP_PLTM_H_

typedef struct isp_pltm_ini_cfg {
	HW_S32 pltm_cfg[ISP_PLTM_MAX];
	HW_S16 pltm_dynamic_cfg[ISP_PLTM_DYNAMIC_MAX];
}pltm_ini_cfg_t;

typedef enum isp_pltm_param_type {
	ISP_PLTM_PARAM_TYPE_MAX,
} pltm_param_type_t;

typedef struct isp_pltm_param {
	pltm_param_type_t type;
	int isp_platform_id;
	int pltm_frame_id;
	HW_BOOL pltm_enable;
	pltm_ini_cfg_t pltm_ini;

	HW_U8  pltm_max_stage;
	HW_U8  hist0_sel;
	HW_U8  hist1_sel;
	HW_U16 ae_lum_idx;
	HW_U16 *gamma_tbl;
	HW_U32 *hist0_buf;
	HW_U32 *hist1_buf;

	HW_S8 wdr_stitch_en;
	HW_S32 wdr_ratio_real;
	HW_U16 pltm_min_stren;
	int ThreshValue;
	isp_sensor_info_t sensor_info;
} pltm_param_t;

typedef struct isp_pltm_stats {
	struct isp_pltm_stats_s *pltm_stats;
} pltm_stats_t;

typedef struct isp_pltm_result {
	//int pltm_weight_lum[4];
	//int pltm_normal_lum[4];
	//int pltm_hdr_weight[4];
	int pltm_hdr_calc_ratio[4];
	int pltm_hdr_ratio;
	HW_U16 pltm_auto_stren;
	HW_U16 pltm_sharp_ss_compensation; //to provide the probability for sharpness module by adjusting its' ss_ns_lw & ls_ns_lw to parameter which can control the dark noise.
	HW_U16 pltm_sharp_ls_compensation; //to provide the probability for sharpness module by adjusting its' ss_ns_lw & ls_ns_lw to parameter which can control the dark noise.
	HW_U16 pltm_d2d_compensation;
	HW_U16 pltm_d3d_compensation;
	HW_U16 pltm_dark_block_num;
	HW_S32 cur_pic_lum;
	HW_S32 tar_pic_lum;
} pltm_result_t;

typedef struct isp_pltm_core_ops {
	int (*isp_pltm_set_params)(void *pltm_core_obj, pltm_param_t *param, pltm_result_t *result);
	int (*isp_pltm_get_params)(void *pltm_core_obj, pltm_param_t **param);
	int (*isp_pltm_run)(void *pltm_core_obj, pltm_stats_t *stats, pltm_result_t *result);
} isp_pltm_core_ops_t;

void* pltm_init(isp_pltm_core_ops_t **pltm_core_ops);
void  pltm_exit(void *pltm_core_obj);


#endif /*_ISP_TONE_MAPPING_H_*/


