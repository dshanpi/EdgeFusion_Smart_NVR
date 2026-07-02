
/*
 ******************************************************************************
 *
 * isp_tone_mapping.h
 *
 * Hawkview ISP - isp_tone_mapping.h module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2016/03/25	VIDEO INPUT
 *
 *****************************************************************************
 */

#ifndef _ISP_TONE_MAPPING_H_
#define _ISP_TONE_MAPPING_H_

#define ISP_GTM_START_FRAME_NUM	3

/* TONE_MAPPING */

typedef enum isp_gtm_type {
	ISP_GTM_FIXED,
	ISP_GTM_DYNAMIC_DRC,
	ISP_GTM_DYNAMIC_GAMMA,
	ISP_GTM_KEEP_LUM_DRC,
	ISP_GTM_LDCI,
	ISP_GTM_ERMHE,
	ISP_GTM_TYPE_MAX,
} gtm_type_t;

typedef enum isp_gamma_type {
	ISP_GTM_GAMMA_FIXED,
	ISP_GTM_GAMMA_DYNAMIC,
	ISP_GTM_GAMMA_TYPE_MAX,
} gtm_gamma_type_t;

typedef enum isp_ldci_video_type {
	ISP_LDCI_VIDEO_INTERNAL = 0,
	ISP_LDCI_VIDEO_EXTERNAL = 1,
	ISP_LDCI_VIDEO_KERNEL = 2,
} ldci_video_type_t;

typedef struct isp_gtm_ini_cfg {
	HW_S8 hist_sel;
	gtm_type_t gtm_type;
	gtm_gamma_type_t gamma_type;
	HW_U32 AutoAlphaEn;//if enable, Need BrightPixellValue DarkPixelValue;
	HW_S32 hist_pix_cnt;
	HW_S32 bright_minval;
	HW_S32 dark_minval;
	HW_S16 plum_var[9][9];
	HW_S16 gtm_cfg[ISP_GTM_HEQ_MAX];
}gtm_ini_cfg_t;

typedef enum isp_gtm_param_type {
	ISP_GTM_INI_DATA,
	ISP_GTM_PARAM_TYPE_MAX,
} gtm_param_type_t;


typedef struct gtm_test_config {
	int isp_test_mode;
} gtm_test_config_t;

typedef struct isp_gtm_param {
	gtm_param_type_t type;
	int isp_platform_id;
	int gtm_frame_id;
	HW_BOOL gtm_enable;
	gtm_ini_cfg_t gtm_ini;

	int contrast;	//for interface
	int brightness; 	//for interface

	int BrightPixellValue;
	int DarkPixelValue;

	int bkgBrightness;
	int lastBkgBrightness;
	int Qi;
	int Qo;
	int QoTar;
	int tarClip;
	int curClip;
	int wdr_mode;

	unsigned short *gamma_tbl; // for hardware
	unsigned short *drc_table;
	unsigned short *drc_table_last;
	gtm_test_config_t test_cfg;

	HW_U8 ldci_entity_id;
	HW_U8 ldci_work_en;
	HW_U8 ldci_work_cnt;
	HW_U16 ldci_video_chn;
	HW_U8 ldci_video_init_en;
	HW_U16 current_colorspace;
	HW_U8 ldci_large_dma_merge;
	HW_U8 *ldci_external_buf;
	HW_U8 ldci_video_sel;
	pthread_mutex_t ldci_external_buf_lock;
} gtm_param_t;

typedef struct isp_gtm_stats {
	struct isp_gtm_stats_s *gtm_stats;
	unsigned char *gtm_ldci_buf;
} gtm_stats_t;

typedef struct isp_gtm_result {
	HW_U16 Hist_MaxVal;
	HW_U16 avg_lum;
	HW_U16 avg_var;
	HW_U16 hist_div;
	float hratio_last;
	HW_U8 ldci_merge_mode;
	HW_S32 hdr_req;
	HW_U16 drc_table_output[ISP_DRC_TBL_SIZE];
} gtm_result_t;

typedef struct isp_gtm_core_ops {
	int (*isp_gtm_set_params)(void *gtm_core_obj, gtm_param_t *param, gtm_result_t *result);
	int (*isp_gtm_get_params)(void *gtm_core_obj, gtm_param_t **param);
	int (*isp_gtm_run)(void *gtm_core_obj, gtm_stats_t *stats, gtm_result_t *result);
} isp_gtm_core_ops_t;

void* gtm_init(isp_gtm_core_ops_t **gtm_core_ops);
void  gtm_exit(void *gtm_core_obj);


#endif /*_ISP_TONE_MAPPING_H_*/


