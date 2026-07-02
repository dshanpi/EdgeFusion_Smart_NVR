
/*
 ******************************************************************************
 *
 * isp.h
 *
 * Hawkview ISP - isp.h module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2016/05/27	VIDEO INPUT
 *
 *****************************************************************************
 */
#ifndef _ISP_H_
#define _ISP_H_
#include "include/isp_type.h"
#include "include/isp_manage.h"
#include "include/isp_tuning.h"
#include "isp_version.h"
#include "video.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct ldci_frame_config {
	HW_U8 *buf;
	int size;
	int width;
	int height;
} ldci_frame_config_t;

int media_dev_init(void);
void media_dev_exit(void);
int isp_ir_reset(int dev_id, int mode_flag);
int isp_read_cfg_bin(int dev_id, int mode_flag, char *isp_cfg_bin_path);
int isp_reset(int dev_id);
int isp_init(int dev_id);
int isp_update(int dev_id);
int isp_get_imageparams(int dev_id, isp_image_params_t *pParams);
int isp_stop(int dev_id);
int isp_stop_ldci(int dev_id);
int isp_exit(int dev_id);
int isp_run(int dev_id);
int isp_events_stop(int dev_id);
int isp_events_restar(int dev_id);
int isp_events_init(int dev_id);
int isp_set_sync(int mode);
int isp_set_ldci_source(int dev_id, int mode);
int isp_set_ldci_frame(int dev_id, ldci_frame_config_t *frame_params);
int isp_set_stitch_mode(int isp_id, enum stitch_mode_t stitch_mode);
HW_S32 isp_pthread_join(int dev_id);
HW_S32 isp_get_cfg(int dev_id, HW_U8 group_id, HW_U32 cfg_ids, void *cfg_data);
HW_S32 isp_set_cfg(int dev_id, HW_U8 group_id, HW_U32 cfg_ids, void *cfg_data);
HW_S32 isp_stats_req(int dev_id, struct isp_stats_context *stats_ctx);

int isp_change_software_tdm_id(int dev_id, int change_flags);
HW_S32 isp_set_fps(int dev_id, int sensor_fps);
int isp_get_fps(int dev_id, int *g_fps);
char *isp_get_sensor_name(int dev_id);
int isp_set_ae_roi(int dev_id, struct isp_h3a_coor_win *coor, HW_U16 force_ae_target, HW_U16 enable);
HW_S32 isp_set_ae_ev_idx(struct isp_lib_context *isp_gen, int curr_idx);
HW_S32 isp_set_attr_cfg(int dev_id, HW_U32 ctrl_id, void *value);
HW_S32 isp_get_attr_cfg(int dev_id, HW_U32 ctrl_id, void *value);

HW_S32 isp_get_encpp_cfg_ctrl(int dev_id, struct isp_video_device *video, HW_U32 ctrl_id, void *value);
HW_S32 isp_set_attr_cfg_ctrl(int dev_id, struct isp_video_device *video, HW_U32 ctrl_id, void *value);
HW_S32 isp_get_attr_cfg_ctrl(int dev_id, struct isp_video_device *video, HW_U32 ctrl_id, void *value);
HW_S32 isp_set_initial_cfg(int dev_id, HW_U32 ctrl_id, void *value);
HW_S32 isp_set_saved_ctx(int dev_id);
HW_S32 isp_get_sensor_info(int dev_id, struct sensor_config *cfg);
HW_S32 isp_set_ae_flicker_comp(int dev_id, HW_S16 enable);
HW_S32 isp_tdm_map(int dev_id, struct isp_tdm_map_cfg *reg);
HW_S32 isp_get_tdm_data(int dev_id, struct vin_isp_tdm_data *data);
HW_S32 isp_send_tdm_data(int dev_id, struct vin_isp_tdm_data *data);
void isp_register_tdmbuffer_done_callback(int dev_id, void *func);
int isp_return_tdmbuffer(int dev_id, struct vin_isp_tdm_event_status *status);

HW_S32 isp_get_info_length(HW_S32* i3a_length, HW_S32* debug_length);
HW_S32 isp_get_version(char* version);
HW_S32 isp_get_awb_gain_ir(int dev_id, HW_S32 *rgain_ir, HW_S32 *bgain_ir);
HW_S32 isp_get_awb_ir_win_cnt(int dev_id, HW_S32 *irlight_win_cnt, HW_S32 *normal_win_cnt);
HW_S32 isp_get_ae_info(int dev_id, isp_3a_info_ae *ae_info);
HW_S32 isp_get_awb_info(int dev_id, isp_3a_info_awb *awb_info);
HW_S32 isp_set_ae_flicker_comp(int dev_id, HW_S16 enable);
HW_S32 isp_set_d3d_lbc_ratio(struct isp_video_device *video, HW_U32 d3d_lbc_ratio);
HW_S32 isp_set_bk_buffer_align(struct isp_video_device *video, struct bk_buffer_align *bk_align);
HW_S32 isp_set_bk_width_stride(struct isp_video_device *video, HW_U8 enable);
HW_S32 isp_sensor_mipi_switch_comp_process(int dev_id, struct isp_lib_context *ctx, struct sensor_mipi_switch_entity *sensor_mipi_switch_info);
HW_S32 isp_sensor_mipi_switch_ctrl(int dev_id, struct isp_video_device *switch_video, struct sensor_mipi_switch_entity *sensor_mipi_switch_info);
int isp_set_stitch_mode(int isp_id, enum stitch_mode_t stitch_mode);

/*******************isp for video buffer*********************/
int isp_set_algo_freq_div(int dev_id, HW_U16 div_ratio, HW_U16 div_flag);
int isp_get_algo_freq_div(int dev_id, HW_U16 *div_ratio);
int isp_get_temp(int dev_id);
HW_S32 isp_get_lv(int dev_id);
HW_S32 isp_get_ev_lv_adj(int dev_id);
HW_S32 isp_get_encpp_cfg(int dev_id, HW_U32 ctrl_id, void *value);
HW_S32 isp_get_debug_msg(int dev_id, void* msg);
HW_S32 isp_get_3a_parameters(int dev_id, void* params);
void* isp_get_ctx_addr(int dev_id);
HW_S32 isp_get_yuv_ystat(struct isp_size pic_size, struct isp_h3a_coor_win target_area, void* VirAddr);
HW_S32 isp_get_awb_stats_avg(int dev_id, HW_U32 *awb_stats_ravg, HW_U32 *awb_stats_gavg, HW_U32 *awb_stats_bavg);
HW_S32 isp_get_ae_weight_lum(int dev_id);
HW_S32 isp_save_debug_info(int dev_id, int is_save_buf,
    const char *file_name, char *isp_info_param);
HW_S32 isp_print_out_af(int dev_id);
HW_S32 isp_get_flash_delay_frame(int dev_id, int* flash_delay_frame);
HW_S32 isp_get_iso_value(int dev_id);
HW_S32 isp_get_exp_time(int dev_id, unsigned int* num, unsigned int* den);
#if ISP_AI_SCENE_CONF
HW_S32 isp_set_scene(int dev_id, scene_mode_t scene_mode);
#endif

typedef enum
{
	ISPInfoType_Level1 = 1 << 0,
	ISPInfoType_Level2 = 1 << 1,
	ISPInfoType_Level3 = 1 << 2,
	ISPInfoType_Level4 = 1 << 3,
} ISPInfoType;

#define SEI_LEVEL1_LEN (40)
#define SEI_LEVEL2_LEN (48)
#define SEI_LEVEL3_LEN (7008)
#define SEI_LEVEL4_LEN (176)
typedef struct
{
	int nInfoBitFlags; // ISPInfoType_exp | ISPInfoType_colortemp | ISPInfoType_awb | ISPInfoType_version
	char mInfoLevel1[SEI_LEVEL1_LEN];
	char mInfoLevel2[SEI_LEVEL2_LEN];
	char mInfoLevel3[SEI_LEVEL3_LEN];
	char mInfoLevel4[SEI_LEVEL4_LEN];
} ISPSeiInfo;

int isp_GetSEIInfo(int dev_id, ISPSeiInfo *pInfo);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /*_ISP_H_*/



