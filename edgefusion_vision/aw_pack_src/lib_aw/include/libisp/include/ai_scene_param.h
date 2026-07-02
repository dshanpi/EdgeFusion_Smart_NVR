/*
 ******************************************************************************
 *
 * ai_scene_param.h
 *
 * Hawkview ISP - ai_scene_param.h module
 *
 * Copyright (c) 2023 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Lin jiandao   2023/11/07	    ai scene conf.
 *
 *****************************************************************************
 */

#ifndef _AI_SCENE_PARAM_H_
#define _AI_SCENE_PARAM_H_

#include "isp_manage.h"

struct isp_ai_scene_tune_setting blue_sky_tune_settings = {
    .isp_ai_smooth_frames = 10,
    .contrast_level = 256,
    .saturation_level = 768,
    .sharpness_level = 256,
    .brightness_level = 366,
    .denoise_level = 256,
    .hue_level = 8,
    .tdf_level = 256,
    .r_gain = 256,
    .b_gain = 256,
    .ae_table_night_length = 6,
    .ae_ai_night_table = {
		18000,    30,   256,   256,   266,   266,
		   30,    30,   256,   512,   266,   266,
		   30,    20,  	512,   512,   266,   266,
		   20,    20,  	512,  1024,   266,   266,
		   20,    15,  1024,  1024,   266,   266,
		   15,    15,  1024,  4000,   266,   266
    }
};

struct isp_ai_scene_tune_setting green_plants_tune_settings = {
    .isp_ai_smooth_frames = 10,
    .contrast_level = 256,
    .saturation_level = 768,
    .sharpness_level = 256,
    .brightness_level = 512,
    .denoise_level = 256,
    .hue_level = 0,
    .tdf_level = 256,
    .r_gain = 256,
    .b_gain = 256,
    .ae_table_night_length = 6,
    .ae_ai_night_table = {
		18000,    30,   256,   256,   266,   266,
		   30,    30,   256,   512,   266,   266,
		   30,    20,  	512,   512,   266,   266,
		   20,    20,  	512,  1024,   266,   266,
		   20,    15,  1024,  1024,   266,   266,
		   15,    15,  1024,  4000,   266,   266
    }
};

struct isp_ai_scene_tune_setting night_view_tune_settings = {
    .isp_ai_smooth_frames = 10,
    .contrast_level = 256,
    .saturation_level = 256,
    .sharpness_level = 64,
    .brightness_level = 256,
    .denoise_level = 1024,
    .hue_level = 0,
    .tdf_level = 400,
    .r_gain = 256,
    .b_gain = 256,
    .ae_table_night_length = 6,
    .ae_ai_night_table = {
        18000,    30,   256,   256,   266,   266,
		   30,    30,   256,   512,   266,   266,
		   30,    20,  	512,   512,   266,   266,
		   20,    20,  	512,  1024,   266,   266,
		   20,    10,  1024,  1024,   266,   266,
		   10,    10,  1024,  8000,   266,   266
    }
};

struct isp_ai_scene_tune_setting portrait_tune_settings = {
    .isp_ai_smooth_frames = 10,
    .contrast_level = 256,
    .saturation_level = 512,
    .sharpness_level = 128,
    .brightness_level = 512,
    .denoise_level = 256,
    .hue_level = 8,
    .tdf_level = 256,
    .r_gain = 256,
    .b_gain = 256,
    .ae_table_night_length = 6,
    .ae_ai_night_table = {
		18000,    30,   256,   256,   266,   266,
		   30,    30,   256,   512,   266,   266,
		   30,    20,  	512,   512,   266,   266,
		   20,    20,  	512,  1024,   266,   266,
		   20,    15,  1024,  1024,   266,   266,
		   15,    15,  1024,  4000,   266,   266
    }
};

struct isp_ai_scene_tune_setting last_tune_settings = {
    .isp_ai_smooth_frames = 10,
    .contrast_level = 256,
    .saturation_level = 256,
    .sharpness_level = 256,
    .brightness_level = 256,
    .denoise_level = 256,
    .hue_level = 0,
    .tdf_level = 256,
    .r_gain = 256,
    .b_gain = 256,
    .ae_table_night_length = 6,
    .ae_ai_night_table = {
		18000,    30,   256,   256,   266,   266,
		   30,    30,   256,   512,   266,   266,
		   30,    20,  	512,   512,   266,   266,
		   20,    20,  	512,  1024,   266,   266,
		   20,    15,  1024,  1024,   266,   266,
		   15,    15,  1024,  4000,   266,   266
    }
};

struct isp_ai_scene_tune_setting normal_tune_settings = {
    .isp_ai_smooth_frames = 10,
    .contrast_level = 256,
    .saturation_level = 256,
    .sharpness_level = 256,
    .brightness_level = 256,
    .denoise_level = 256,
    .hue_level = 0,
    .tdf_level = 256,
    .r_gain = 256,
    .b_gain = 256,
    .ae_table_night_length = 6,
    .ae_ai_night_table = {
		18000,    30,   256,   256,   266,   266,
		   30,    30,   256,   512,   266,   266,
		   30,    20,  	512,   512,   266,   266,
		   20,    20,  	512,  1024,   266,   266,
		   20,    15,  1024,  1024,   266,   266,
		   15,    15,  1024,  4000,   266,   266
    }
};

#endif /*_AI_SCENE_PARAM_H_*/
