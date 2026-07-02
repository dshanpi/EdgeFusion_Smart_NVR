/* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
  This file is used to export some structs of vin_video to user space, mainly for rt_media.

  @author eric_wang
  @date 20250226
*/

#ifndef _VIN_VIDEO_BASE_H_
#define _VIN_VIDEO_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DAY_LIGNT_SENSOR_SIZE   96
#define NIGHT_LIGNT_SENSOR_SIZE 40

#pragma pack(1)
typedef struct taglight_sensor_attr_s {
	unsigned short u16LightValue;	/*adc value*/
	unsigned short ae_table_idx;
	unsigned int   u32SnsExposure;
	unsigned int   u32SnsAgain;
} LIGHT_SENSOR_ATTR_S;

typedef struct tagsensor_isp_config_s {
	int sign;                    //id0: 0xAA66AA66, id1: 0xBB66BB66
	int crc;                     //checksum, crc32 check, if crc = 0, do check
	int ver;                     //version, 0x01

	int light_enable;            //boot0 adc en
	int adc_mode;                //ADC mode, 0:the brighter the u16LightValue more, 1:the brighter the u16LightValue smaller
	unsigned short light_def;    //adc threshold: 1,adc_mode=0:smaller than it enter night mode, greater than it or equal enter day mode;
				     //2,adc_mode=1:greater than it enter night mode, smaller than it or equal enter day mode;

	unsigned char ircut_state;   //1:hold, 0:use ir_mode
	unsigned short ir_mode;	     //IR mode 0:force day mode, 1:auto mode, 2:force night mode
	unsigned short lv_liner_def; //lv threshold: smaller than it enter night mode, greater than it or equal enter day mode
	unsigned short lv_hdr_def;   //lv threshold: smaller than it enter night mode, greater than it or equal enter day mode

	unsigned short width;        //init size, 0:mean maximum size
	unsigned short height;
	unsigned short mirror;
	unsigned short filp;
	unsigned short fps;
	unsigned short wdr_mode;     //hdr or normal mode, wdr_mode = 0 mean normal,  wdr_mode = 1 mean hdr
	unsigned char flicker_mode;  //0:disable,1:50HZ,2:60HZ, default 50HZ

	unsigned char venc_format;   //1:H264 2:H265
	/* unsigned char reserv 256 */
	unsigned char sensor_deinit; //sensor not init in melis
	unsigned char get_yuv_en; //get melis yuv en
	unsigned char light_sensor_en; //use LIGHT_SENSOR_ATTR_S en
	unsigned char hdr_light_sensor_ratio; //LIGHT_SENSOR_ATTR_S hdr exposure ratio, default is 16
	unsigned char lightadc_debug_en; //printf data about LIGHT_SENSOR_ATTR_S
	unsigned char pix_num; //two or one pix_num, default is one pix_num
	unsigned char tdm_speed_down_en; //tdm speed down en
	unsigned char reserv[256 - 7];

	LIGHT_SENSOR_ATTR_S linear[DAY_LIGNT_SENSOR_SIZE]; //day linear mode parameter, auto exp
	LIGHT_SENSOR_ATTR_S linear_night[NIGHT_LIGNT_SENSOR_SIZE]; //night linear mode parameter
	LIGHT_SENSOR_ATTR_S hdr[DAY_LIGNT_SENSOR_SIZE];    //day hdr mode paramete
	LIGHT_SENSOR_ATTR_S hdr_night[NIGHT_LIGNT_SENSOR_SIZE];    //night hdr mode parameter
} SENSOR_ISP_CONFIG_S; //size is 3563 < 3584(3.5k)
#pragma pack()

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _VIN_VIDEO_BASE_H_ */

