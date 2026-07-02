
/*
 ******************************************************************************
 *
 * isp_rolloff.h
 *
 * Hawkview ISP - isp_rolloff.h module
 *
 * Copyright (c) 2021 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *   4.0		guoqiuwang	   2021/09/30	    VIDEO INPUT
 *
 *****************************************************************************
 */

#include "isp_manage.h"

#ifndef _ISP_3A_ROLLOFF_H_
#define _ISP_3A_ROLLOFF_H_

#define ISP_LIB_USE_ROLLOFF         1

#define CORRECTFREQ                 6
#define ROLLOFFSTART                (5 * CORRECTFREQ)

#define TH_FROSTEDGLASS             0.022
#define TH_LOWLIGHT                 35
#define TH_COMPLEX                  0.55
#define TH_HIGHSAT                  0.63
#define TH_SEMIFLAT                 0.045
#define TH_MAXPERCENTAGE            0.90
#define TH_THRATIO                  0.20
#define TH_DARKPIXEL                20
#define TH_BRIGHTPIXEL              240
#define TH_AREAOPENNUMBER           45
#define TH_FLATGRADRG               0.026
#define TH_RGDIFFPEAK               0.18
#define TH_AREAOPENNUMBER2          30
#define TH_HIGHPERCENTAGE           0.85
#define TH_LOWPERCENTAGE            0.15
#define TH_ERRBASEDIFF              0.1
#define TH_FLATSTDY                 70
#define TH_FLATMEANY                210
#define TH_FLATRANGERG              0.24
#define TH_DIFFA                    15
#define TH_DIFFB                    10
#define TH_SATURATION               0.53
#define TH_LOWSATURATION            0.25
#define TH_ERRGAINDIFF              0.03

#define Y_WEIGHT_R                  0.299
#define Y_WEIGHT_G                  0.587
#define Y_WEIGHT_B                  0.114

#define TWICE_CORRECT_TABLE_NUM     11

#define RATIOEDGE                   2

#if (ISP_VERSION >= 600)
#define ROLLOFF_ROW_SIZE            24
#define ROLLOFF_COL_SIZE            32
#define CENTERX                     15
#define CENTERY                     12
#else
#define ROLLOFF_ROW_SIZE            32
#define ROLLOFF_COL_SIZE            32
#define CENTERX                     15
#define CENTERY                     16
#endif
#define ROLLOFF_WIN_SIZE            (ROLLOFF_ROW_SIZE * ROLLOFF_COL_SIZE)

#define LSCGAINPOOLSIZE             64


typedef struct Point {
	HW_S32 row;
	HW_S32 col;
} Point_t;

typedef struct Lab {
	float Lab_L;
	float Lab_a;
	float Lab_b;
} Lab_t;
typedef struct notFlatPointInfo {
	Lab_t Lab;
	HW_S32 position;
} notFlatPointInfo_t;

typedef struct flatPointInfo {
	HW_U16 position;
	float _RG;
	float Lab_L;
	float Lab_a;
	float Lab_b;
} flatPointInfo_t;

typedef struct flatPointDistTOGain {
	HW_U8 dist;
	float gain;
} flatPointDistToGain_t;

typedef struct flatDistToGain {
	float *gain;
} flatDistToGain_t;

typedef struct LscGainPool {
	float *LscRGain;
	float *LscGGain;
	float *LscBGain;
	float *LscRGain_l;
	float *LscGGain_l;
	float *LscBGain_l;
} LscGainPool_t;

typedef enum rolloff_correct_mode {
	RGBChCorrect = 0,
	PreCorrect,
	RChCorrect,
	TwiceCorrect,
} rolloff_correct_mode_t;

typedef struct FlatPreCorrectInfo {
	float flatPreIndex;
	float flatPreDiff;
} flatPreCorrectInfo_t;


typedef enum isp_rolloff_param_type {
	ISP_ROLLOFF_INI_DATA,
	ISP_ROLLOFF_PARAM_TYPE_MAX,
} rolloff_param_type_t;

typedef struct isp_rolloff_ini_cfg {
	HW_U16 Rgain[ROLLOFF_WIN_SIZE * 11];
} rolloff_ini_cfg_t;

typedef enum rolloff_triger_type {
	ROLLOFF_TRIGER_BY_LUM_IDX = 0,
	ROLLOFF_TRIGER_BY_GAIN_IDX,
	ROLLOFF_TRIGER_MAX,
} rolloff_triger_type_t;

typedef struct isp_rolloff_iso_config {
	rolloff_triger_type_t	triger_type;
	HW_S32	shading_comp_dynamic_cfg[14];
} isp_rolloff_iso_config_t;

typedef struct isp_rolloff_setting {
	HW_U16	gain_pool_size;
	HW_U16  table_switch;
	HW_U16	correct_freqs;
	HW_U16	correct_start;
	HW_U16	pre_lowBright;
	HW_U16	pre_complex;
	HW_U16	pre_highSatu;
	HW_U16	rch_semiflat;
	HW_U16	rgb_semiflat;
} isp_rolloff_setting_t;

typedef struct isp_rolloff_param {
	HW_U8 stitch_mode;
	rolloff_param_type_t type;
	HW_S32 isp_platform_id;
	HW_S32 rolloff_frame_id;
	isp_sensor_info_t rolloff_sensor_info;
	rolloff_ini_cfg_t rolloff_ini;
	isp_rolloff_setting_t rolloff_ctrl;
} rolloff_param_t;

typedef struct isp_rolloff_stats {
	struct isp_stats_s *rolloff_stats;
} rolloff_stats_t;

typedef struct isp_rolloff_result {
	float RGainTbl[ISP_LENS_TBL_SIZE];
	float GGainTbl[ISP_LENS_TBL_SIZE];
	float BGainTbl[ISP_LENS_TBL_SIZE];
	float RGainTbl_l[ISP_LENS_TBL_SIZE];
	float GGainTbl_l[ISP_LENS_TBL_SIZE];
	float BGainTbl_l[ISP_LENS_TBL_SIZE];
	rolloff_correct_mode_t correctMode;
} rolloff_result_t;

typedef struct isp_rolloff_core_ops {
	HW_S32 (*isp_rolloff_set_params)(void *rolloff_core_obj, rolloff_param_t *param, rolloff_result_t *result);
	HW_S32 (*isp_rolloff_get_params)(void *rolloff_core_obj, rolloff_param_t **param);
	HW_S32 (*isp_rolloff_run)(void *rolloff_core_obj, rolloff_stats_t *stats, rolloff_result_t *result);
} isp_rolloff_core_ops_t;

void *rolloff_init(isp_rolloff_core_ops_t **rolloff_core_ops);
void  rolloff_exit(void *rolloff_core_obj);

#endif /*_ISP_3A_ROLLOFF_H_*/

