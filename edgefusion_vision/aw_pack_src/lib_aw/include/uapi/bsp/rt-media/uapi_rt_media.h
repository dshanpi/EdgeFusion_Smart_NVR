/* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/** @file
 * Copyright (C) 2022 Allwinner, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _RT_MEDIA_H_
#define _RT_MEDIA_H_

//#include <stdio.h>
//#include <sys/types.h>
#include <stdbool.h>

#include <linux/ioctl.h>
#include <media/sunxi_camera_v2.h>
#include <media/vcodec_base.h>
#include <media/vencoder_base.h>

#define SENSOR_0_SIGN 0xAA66AA66
#define SENSOR_1_SIGN 0xBB66BB66
#define SENSOR_2_SIGN 0xCC66CC66

#if 1//config 0
#define CSI_SENSOR_0_VIPP_0 0
#define CSI_SENSOR_0_VIPP_1 4
#define CSI_SENSOR_0_VIPP_2 8
#define CSI_SENSOR_0_VIPP_3 12

#define CSI_SENSOR_1_VIPP_0 1
#define CSI_SENSOR_1_VIPP_1 5
#define CSI_SENSOR_1_VIPP_2 9
#define CSI_SENSOR_1_VIPP_3 13

#define CSI_SENSOR_2_VIPP_0 2
#define CSI_SENSOR_2_VIPP_1 6
#define CSI_SENSOR_2_VIPP_2 10
#define CSI_SENSOR_2_VIPP_3 14

#else//config 1
#define CSI_SENSOR_0_VIPP_0 0
#define CSI_SENSOR_0_VIPP_1 4
#define CSI_SENSOR_0_VIPP_2 8
#define CSI_SENSOR_0_VIPP_3 12

#define CSI_SENSOR_1_VIPP_0 2
#define CSI_SENSOR_1_VIPP_1 6
#define CSI_SENSOR_1_VIPP_2 10
#define CSI_SENSOR_1_VIPP_3 14

#define CSI_SENSOR_2_VIPP_0 1
#define CSI_SENSOR_2_VIPP_1 5
#define CSI_SENSOR_2_VIPP_2 9
#define CSI_SENSOR_2_VIPP_3 13
#endif

#define LBC_2X_COM_RATIO_EVEN 600
#define LBC_2X_COM_RATIO_ODD 450

#define LBC_2_5X_COM_RATIO_EVEN 440
#define LBC_2_5X_COM_RATIO_ODD 380

#define LBC_BIT_DEPTH 8

#define LBC_EXT_SIZE (2 * 1024)

#define NEI_TEST_CODE 0

typedef struct KERNEL_VBV_BUFFER_INFO{
	int share_fd;
	int size;
} KERNEL_VBV_BUFFER_INFO;

typedef enum {
	RT_VENC_CODEC_H264 = 0,
	RT_VENC_CODEC_JPEG = 1,
	RT_VENC_CODEC_H265 = 2,
} RT_VENC_CODEC_TYPE;

typedef struct video_stream_s {
	int id;
	uint64_t pts;
	unsigned int flag;
	unsigned int size0;
	unsigned int size1;
	unsigned int size2;

	unsigned int offset0;
	unsigned int offset1;
	unsigned int offset2;

	int keyframe_flag;
	//struct timeval tv;

	unsigned char *data0;
	unsigned char *data1;
	unsigned char *data2;

	unsigned int nDataNum;
	VencPackInfo stPackInfo[MAX_OUTPUT_PACK_NUM];

	unsigned int exposureTime; //every frame exp time, unit:us
} video_stream_s;
typedef video_stream_s STREAM_DATA_INFO;

/* overlay related */
#define MAX_OVERLAY_ITEM_SIZE  (64)

typedef struct OverlayItemInfo_t {
	unsigned short		start_x;
	unsigned short		start_y;
	unsigned int		widht;	// must 16_align: 'width%16 == 0'
	unsigned int		height; // must 16_align: 'height%16 == 0'
	VencOverlayCoverYuvS		cover_yuv;		   //when use RT_COVER_OSD should set the cover yuv
	VENC_OVERLAY_TYPE	   osd_type;	   //reference definition of VENC_OVERLAY_TYPE
	unsigned char       *data_buf;	//the vir addr of overlay block
	unsigned int		data_size;	//the size of bitmap
} OverlayItemInfo;

typedef struct VideoInputOSD_t {
	unsigned char		   osd_num; //num of overlay region
	VENC_OVERLAY_ARGB_TYPE	   argb_type;//reference definition of VENC_OVERLAY_ARGB_TYPE
	OverlayItemInfo 	   item_info[MAX_OVERLAY_ITEM_SIZE];
	//* for v5v200 and newer ic
    unsigned int                invert_mode; //LESSTHAN_UNIT_LUMDIFF_THRESH, ref to INVERT_COLOR_MODE_E
    unsigned int                invert_threshold;
    //* end
} VideoInputOSD;

typedef struct catch_jpeg_config {
	int 		   channel_id;
	int 		   width; //encode dst width
	int 		   height;
	int 		   qp;
	int            b_enable_sharp;
	int 		   rotate_angle; //90, 180, 270. clock-wise.
	VideoInputOSD  overlay_info;
} catch_jpeg_config;

typedef struct catch_jpeg_buf_info {
	unsigned int   buf_size; // set the max_size, return valid_size
	unsigned char *buf;
} catch_jpeg_buf_info;

#if defined(CONFIG_FRAMEDONE_TWO_BUFFER)
#define CONFIG_YUV_BUF_NUM (2)
#else
#define CONFIG_YUV_BUF_NUM (3)
#endif

typedef struct config_yuv_buf_info {
	unsigned char *phy_addr[CONFIG_YUV_BUF_NUM];
	int buf_size;
	int buf_num;
} config_yuv_buf_info;

typedef struct rt_yuv_info {
	int bset; //indicate if fd is got and set
	int fd;
	unsigned char *phy_addr;
	int width;
	int height;
	uint64_t pts; //unit us
} rt_yuv_info;


#define VIDEO_INPUT_CHANNEL_NUM (16)

typedef enum VIDEO_OUTPUT_MODE{
	OUTPUT_MODE_STREAM = 0, //yuv -> venc -> stream, output stream
	OUTPUT_MODE_YUV    = 1, // only get yuv, output yuv. rt driver thread get yuv, always keep the latest frame and discard old frame when buffer busy
	OUTPUT_MODE_YUV_GET_DIRECT = 2, // direct get yuv by vin interface, keep the old frames and discard current frame when buffer busy
	OUTPUT_MODE_ENCODE_FILE_YUV	= 3, //for debug: not use isp,vin. app send yuv to rt_media's rt_venc_component to encode.
	OUTPUT_MODE_ENCODE_OUTSIDE_YUV = 4, //yuv is from outer. app send yuv to rt_media's rt_venc_component to encode.
} VIDEO_OUTPUT_MODE;

typedef enum rt_pixelformat_type {
	RT_PIXEL_YUV420SP, //nv12
	RT_PIXEL_YVU420SP, //nv21
	RT_PIXEL_YUV420P,
	RT_PIXEL_YVU420P,
	RT_PIXEL_YUV422SP,
	RT_PIXEL_YVU422SP,
	RT_PIXEL_YUV422P,
	RT_PIXEL_YVU422P,
	RT_PIXEL_YUYV422,
	RT_PIXEL_UYVY422,
	RT_PIXEL_YVYU422,
	RT_PIXEL_VYUY422,
	RT_PIXEL_LBC_25X,
	RT_PIXEL_LBC_2X,
	RT_PIXEL_NUM
} rt_pixelformat_type;
typedef rt_pixelformat_type RT_PIXELFORMAT_TYPE;

typedef struct {
    unsigned int        nThumbSize;
    unsigned char       *pThumbBuf;
    unsigned int        bWriteToFile;
    unsigned int        nEncodeCnt;
} RTVencThumbInfo; ///< compare to VencThumbInfo

#define MAX_ROI_AREA 8

typedef struct {
	VencMBModeCtrl mb_md_ctl;
	VencMbModeLambdaQpMap  low32;
	VencMbSplitMap         high32;
} RTVencMbMapParam;

#define RT_MAX_VENC_COMP_NUM (4)

typedef struct rt_dma_stitch_rotate {
	int rotate_en; //1: enable, 0:disable
	int sensor_a_rotate; //support to rotate sensorA frame. 0, 90, 180, 270
} rt_dma_stitch_rotate_t;

typedef struct rt_media_config_s {
	int channelId;				// channel ID. equivalent to vipp number, scope[0,15]. vi param
	int venc_freq;				// venc working frequence, venc param
	RT_VENC_CODEC_TYPE encodeType; 			//encode type:0)h264 1)mjpeg 2)h265, venc param
	int width;	// resolution width, vi param
	int height;// resolution height, vi param
	int dst_width; ///< operate venc. encode output size. 0 means same to width. venc param
	int dst_height; //venc param
	int fps;					///< operate isp. fps:1~30, vi param
	int dst_fps;					///< vi_comp software output fps. fps:1~30, venc dst frame rate. venc param
	int bitrate;				///< operate venc. h264 bitrate (bps, not kbps), venc param
	int gop;					///< operate venc. h264. I-Frame interval. venc param
	VENC_RC_MODE mRcMode; //the type of rc, venc param
	int vbr_opt_enable; ///< operate venc. 0: use old vbr bitrate control, 1: use new vbr bitrate control. venc param
	VencVbrParam vbr_param; //venc param. old vbr param.
	int profile; //VENC_H264PROFILETYPE, VENC_H265PROFILETYPE, venc param
	int level; //VENC_H264LEVELTYPE or VENC_H265LEVELTYPE, venc param
	/**
	  user config drop frame number in rt_vi_comp, only valid in offline mode. drop_frame_num = 5 means first five frames
	  are dropped, begin to encode sixth frame.  vi param
	*/
	int drop_frame_num;
	VencQPRange qp_range; //venc param
	VIDEO_OUTPUT_MODE output_mode;

	//int enable_bin_image;	// 0: disable, 1: enable
	//int bin_image_moving_th;//range[1,31], 1:all frames are moving,
							//			  31:have no moving frame, default: 20

	//int enable_mv_info; 	// 0: disable, 1: enable

	//* about isp
	int enable_wdr; ///< config isp: set wdr, [0,1], vi param

	RT_PIXELFORMAT_TYPE pixelformat;//* just set it to: RT_PIXELFORMAT_UNKNOWN, vi param, venc param
	VENC_OUTPUT_FMT outputformat; //venc param
	//RTVencMotionSearchParam motion_search_param;// should setup the param if use motion search by encoder
	int enable_overlay; //venc param
	int jpg_quality; //venc param for jpeg
	int bonline_channel; //vi param, venc param
	int share_buf_num; //vi param, venc param
	int jpg_mode;//0: jpg 1: mjpg, venc param for jpeg
	VencBitRateRange bit_rate_range; //venc param, for mjpeg
	VencH264VideoSignal venc_video_signal; //venc param
	eVencProductMode product_mode; //venc param
	//RTVencROIConfig   sRoiConfig[MAX_ROI_AREA];
	eVeLbcMode rec_lbc_mode; //venc param

	int demo_start;
	int enable_sharp; // venc param. 1: config venc to enable sharp, then follow isp config if enable_isp2ve_linkage. 0:disable venc encpp sharp.
	int en_16_align_fill_data; //vi param. when encppSharp enable, rt_vi need fill 16 align height, rt_venc will know it too.
	int vin_buf_num; //vi param
	int breduce_refrecmem;//Can reduce memory usage and optimize memory efficiency. default set enable, venc param
	int venc_rxinput_buf_multiplex_en;
	sWbYuvParam  s_wbyuv_param;
	int vbv_buf_size;			//all bitstream buf size. venc param
	int vbv_thresh_size;		//one bitstream max size. venc param

	int enable_isp2ve_linkage; //venc param
	int enable_ve2isp_linkage; //venc param, when multi ve link to one isp, we must make only one vencoder to transport info to isp.
	int skip_sharp_param_frame_num; //venc param
	//int ve_encpp_sharp_atten_coef_per;      //decrease encpp sharpness percent [0, 100]. 100 means no decrease, venc param
	bool limit_venc_recv_frame; //venc param, control venc encode frame number. init to 0: don't receive frame to encode.

	int enable_tdm_raw; //vi param
	unsigned int tdm_rxbuf_cnt; //vi param
	VencMBModeCtrl mb_mode_ctl; //venc param
	VencMbModeLambdaQpMap mMapLow; //venc param
	VencMbSplitMap mMapHigh; //venc param
	enum vi_dma_stitch_mode_t dma_merge_mode; //vi param
	struct dma_overlay_para dma_overlay_ctx; //vi param
	struct dma_merge_scaler_cfg dma_merge_scaler_cfg; //vi param
	rt_dma_stitch_rotate_t dma_stitch_rotate_cfg; //vi param
	struct tdm_speeddn_cfg tdm_spdn_cfg;
	unsigned int bEnableMultiOnlineSensor; // only for online. multi sensor online scene. venc param
	unsigned int bEnableImageStitching; // only for multi sensor online scene. image stitching. venc param
} rt_media_config_s;
typedef rt_media_config_s VideoInputConfig;

/**
  encode_config is used to store preset configurations of rt_media from board.dts.
*/
typedef struct encode_config {
	int b_get_from_dts;//1:node status is "okay", 0: node status is "disabled" or node is not exist.
	char *name;
	int ch_id; //vipp_num
	int bonline_channel; //vi param, venc param
	int share_buf_num; //vi param, venc param
	int vin_buf_num; //vi param
	int venc_freq; //venc param
	RT_VENC_CODEC_TYPE codec_type;
	int res_w;
	int res_h;
	int dst_w;
	int dst_h;
	int fps;
	int bit_rate;//kb
	int gop;
	int enable_sharp;
	int vbr;
	int vbr_opt_enable;
	int drop_frame_num; //vi param.
	eVencProductMode product_mode;
	VencQPRange qp_range;
	VencVbrParam vbr_param;
	VIDEO_OUTPUT_MODE out_mode;
	RT_PIXELFORMAT_TYPE pix_fmt;//* just set it to: RT_PIXELFORMAT_UNKNOWN
	int reduce_refrec_mem;//Can reduce memory usage and optimize memory efficiency. default set enable
	int isp2ve_en;
	int ve2isp_en;
	int region_detect_link_en;
	int enable_tdm_raw;
	unsigned int tdm_rxbuf_cnt;
	enum vi_dma_stitch_mode_t dma_merge_mode;
	struct dma_merge_scaler_cfg dma_merge_scaler_cfg;
	struct tdm_speeddn_cfg tdm_spdn_cfg;
	unsigned int catchjpg_en;
	unsigned int jpg_w;
	unsigned int jpg_h;
	unsigned int jpg_qp;
	int jpg_enable_sharp; //1:enable, 0:disable
	int venc_rxinput_buf_multiplex_en;
	int rec_lbc_mode;
} encode_config;
typedef encode_config PresetRtMediaChnConfig;

typedef enum RT_POWER_LINE_FREQUENCY{
	RT_FREQUENCY_DISABLED  = 0,
	RT_FREQUENCY_50HZ	   = 1,
	RT_FREQUENCY_60HZ	   = 2,
	RT_FREQUENCY_AUTO	   = 3,
} RT_POWER_LINE_FREQUENCY; ///< same to enum power_line_frequency

typedef enum RT_AE_METERING_MODE{
	RT_AE_METERING_MODE_AVERAGE = 0,
	RT_AE_METERING_MODE_CENTER = 1,
	RT_AE_METERING_MODE_SPOT = 2,
	RT_AE_METERING_MODE_MATRIX = 3,
} RT_AE_METERING_MODE; ///< same to enum ae_metering_mode

typedef struct {
	int grey; // 0 color 1 gray
	int ir_on;
	int ir_flash_on;
} RTIrParam;

typedef struct {
	struct isp_cfg_attr_data isp_attr_cfg;
} RTIspCtrlAttr;

#define MAX_ISP_ORL_NUM (16)
struct orl_win {
	unsigned int left;
	unsigned int top;
	unsigned int width;
	unsigned int height;
	unsigned int rgb_orl;
};

typedef struct {
	unsigned char on;
	unsigned char orl_cnt;
	unsigned int orl_width;
	struct orl_win orl_win[MAX_ISP_ORL_NUM];
	int vipp_id;	/* This vipp_id is only valid in the two sensor merge mode. In other ways, this parameter is invalid */
} RTIspOrl;

typedef struct {
	int en_weak_text_th;
	float weak_text_th;
} RTVenWeakTextTh;

#define RT_MAX_INSERT_FRAME_LEN (196569)

// notify callback event to AWVideoInput_cxt, using netlink.
typedef enum {
    RTCB_EVENT_VENC_DROP_FRAME = 0,
} RTCbEventType;

typedef struct {
    RTCbEventType eEventType;
    int nData1;
    int nData2;
} RTCbEvent;

/**
  generic netlink rt_media family definition.
*/
enum {
	RT_MEDIA_CMD_UNSPEC = 0,	/* Reserved */
	RT_MEDIA_CMD_SEND,		/* user->kernel, send some infos such as channelId */
	RT_MEDIA_CMD_CB_EVENT,	/* kernel->user, send rt_media callback event to AWVideoInput */
	__RT_MEDIA_CMD_MAX,
};

#define RT_MEDIA_CMD_MAX (__RT_MEDIA_CMD_MAX - 1)

/**
  rt_media family netlink attribute definition.
*/
enum {
	RT_MEDIA_CMD_ATTR_UNSPEC = 0,
	RT_MEDIA_CMD_ATTR_CHNID, //record channel id, type = int.
	RT_MEDIA_CMD_ATTR_RTCBEVENT, //type = RTCbEvent
	__RT_MEDIA_CMD_ATTR_MAX,
};

#define RT_MEDIA_CMD_ATTR_MAX (__RT_MEDIA_CMD_ATTR_MAX - 1)

typedef struct {
	int flag;			//0: print; 1: save to file; others: reserve for future use
	char *path;			//path string
	unsigned int len;	//path string length
} VIN_ISP_REG_GET_CFG;

//typedef enum {
//	RT_BX_TO_CLOSE = 0,
//	RT_B10_TO_B8 = 1,
//	RT_B8_TO_B10 = 2,
//} RT_VIN_SET_BIT_WIDTH;  //same to enum set_bit_width

typedef enum {
    RTSEIDataType_ISP = 1<<0,
    RTSEIDataType_VIPP = 1<<1,
    RTSEIDataType_VENC = 1<<2,
} RTSEIDataType;

typedef struct {
	bool bEnable;
	int nSeiDataTypeFlags; //RTSEIDataType_ISP, RTSEIDataType_VIPP, RTSEIDataType_VENC
	int nFrameIntervalForISPLevel1; //Exp
	int nFrameIntervalForISPLevel2; //Colortmp
	int nFrameIntervalForISPLevel3; //Awb
	int nFrameIntervalForVIPP;
	int nFrameIntervalForVencLevel1; //nSceneStatus, nMoveStatus
	int nFrameIntervalForVencLevel2; //nMadTh,bOnlineEn
} RT_VENC_SEI_ATTR;

enum RT_MEDIA_IOCTL_CMD {
	IOCTL_RT_UNKOWN                         = 0x100,

	IOCTL_CONFIG                            = _IOW('M', 1, rt_media_config_s),  //+1
	IOCTL_START                             = _IO('M', 2),   //+2
	IOCTL_PAUSE                             = _IO('M', 3),   //+3
	IOCTL_DESTROY                           = _IO('M', 4), //+4

	IOCTL_GET_VBV_BUFFER_INFO               = _IOR('M', 5, KERNEL_VBV_BUFFER_INFO), //+5

	IOCTL_GET_STREAM_HEADER_SIZE            = _IOR('M', 6, int),	 //+6
	IOCTL_GET_STREAM_HEADER_DATA            = _IOR('M', 7, unsigned char),	 //+7

	IOCTL_GET_STREAM_DATA                   = _IOR('M', 8, video_stream_s),	   //+8
	IOCTL_RETURN_STREAM_DATA                = _IOW('M', 9, video_stream_s),  //+9

	IOCTL_CATCH_JPEG_START                  = _IOW('M', 10, catch_jpeg_config),
	IOCTL_CATCH_JPEG_GET_DATA               = _IOR('M', 11, catch_jpeg_buf_info),
	IOCTL_CATCH_JPEG_STOP                   = _IO('M', 12),

	IOCTL_SET_OSD                           = _IOW('M', 13, VideoInputOSD),

	//IOCTL_GET_BIN_IMAGE_DATA,
	//IOCTL_GET_MV_INFO_DATA,

	IOCTL_CONFIG_YUV_BUF_INFO               = 14,
	IOCTL_REQUEST_YUV_FRAME                 = _IOR('M', 15, rt_yuv_info),
	IOCTL_RETURN_YUV_FRAME                  = _IOW('M', 16, unsigned char *),

	//csi-tdm-isp-vipp operate
	IOCTL_GET_ISP_LV                        = _IOR('M', 17, int),
	IOCTL_GET_ISP_HIST                      = _IOR('M', 18, unsigned int),
	IOCTL_GET_ISP_EXP_GAIN                  = _IOR('M', 19, struct sensor_exp_gain),
	IOCTL_SET_ISP_AE_METERING_MODE          = _IOW('M', 20, RT_AE_METERING_MODE),
	IOCTL_SET_IR_PARAM                      = _IOW('M', 21, RTIrParam),
	IOCTL_SET_VERTICAL_FLIP                 = _IOW('M', 22, int),
	IOCTL_SET_HORIZONTAL_FLIP               = _IOW('M', 23, int),
	IOCTL_SET_ISP_ATTR_CFG                  = _IOW('M', 24, RTIspCtrlAttr),
	IOCTL_GET_ISP_ATTR_CFG                  = _IOR('M', 25, RTIspCtrlAttr),
	IOCTL_SET_ISP_ORL                       = _IOW('M', 26, RTIspOrl),
	IOCTL_SET_ISP_AE_MODE                   = _IOW('M', 27, int),
	IOCTL_SET_POWER_LINE_FREQ               = _IOW('M', 28, RT_POWER_LINE_FREQUENCY),
	IOCTL_GET_TDM_BUF                       = _IOR('M', 29, struct vin_isp_tdm_event_status),
	IOCTL_RETURN_TDM_BUF                    = _IOW('M', 30, struct vin_isp_tdm_event_status),
	IOCTL_GET_TDM_DATA                      = _IOR('M', 31, struct vin_isp_tdm_data),
	IOCTL_SET_BRIGHTNESS                    = _IOW('M', 32, int),
	IOCTL_SET_CONTRAST                      = _IOW('M', 33, int),
	IOCTL_SET_SATURATION                    = _IOW('M', 34, int),
	IOCTL_SET_HUE                           = _IOW('M', 35, int),
	IOCTL_SET_SHARPNESS                     = _IOW('M', 36, int),
	IOCTL_SET_SENSOR_EXP                    = _IOW('M', 37, int),
	IOCTL_SET_SENSOR_GAIN                   = _IOW('M', 38, int),
	IOCTL_GET_SENSOR_NAME                   = _IOR('M', 39, char *),
	IOCTL_GET_SENSOR_RESOLUTION             = _IOR('M', 40, struct sensor_resolution),
	IOCTL_GET_TUNNING_CTL_DATA              = _IOR('M', 41, struct tunning_ctl_data),
	IOCTL_GET_SENSOR_RESERVE_ADDR           = 42,
	IOCTL_GET_ISP_REG                       = _IOR('M', 43, VIN_ISP_REG_GET_CFG),
	IOCTL_SET_TDM_DROP_FRAME                = _IOW('M', 44, unsigned int),
	IOCTL_SET_INPUT_BIT_WIDTH_START         = _IOW('M', 45, enum set_bit_width),
	IOCTL_SET_INPUT_BIT_WIDTH_STOP          = _IOW('M', 46, enum set_bit_width),
	IOCTL_GET_SENSOR_INFO                   = _IOW('M', 47, struct sensor_config),
	IOCTL_SET_CAMERA_LOW_POWER_MODE         = _IOR('M', 48, struct sensor_lowpw_cfg),
	IOCTL_SET_SENSOR_STANDBY                = _IOR('M', 49, struct sensor_standby_status),
	IOCTL_GET_ISP_SHARP_PARAM               = _IOR('M', 50, sEncppSharpParam),
	IOCTL_SET_ISP_SAVE_AE                   = _IOR('M', 51, int),
	IOCTL_GET_CSI_STATUS                    = _IOR('M', 52, int),

	//venc operate
	IOCTL_SET_FORCE_KEYFRAME                = _IO('M', 70),
	IOCTL_RESET_ENCODER_TYPE                = _IOW('M', 71, RT_VENC_CODEC_TYPE),
	IOCTL_REQUEST_ENC_YUV_FRAME             = _IOR('M', 72, unsigned char *),
	IOCTL_SUBMIT_ENC_YUV_FRAME              = _IOW('M', 73, rt_yuv_info),
	IOCTL_SET_QP_RANGE                      = _IOW('M', 74, VencQPRange),
	IOCTL_SET_BITRATE                       = _IOW('M', 75, int),
	IOCTL_SET_VBR_PARAM                     = _IOW('M', 76, VencVbrParam),
	IOCTL_GET_SUM_MB_INFO                   = _IOR('M', 77, VencMBSumInfo),
	IOCTL_SET_ENC_MOTION_SEARCH_PARAM       = _IOW('M', 78, VencMotionSearchParam),
	IOCTL_GET_ENC_MOTION_SEARCH_RESULT      = _IOR('M', 79, VencMotionSearchResult),
	IOCTL_SET_VENC_SUPER_FRAME_PARAM        = _IOW('M', 80, VencSuperFrameConfig),
	IOCTL_SET_FPS                           = _IOW('M', 81, int),
	IOCTL_SET_SHARP                         = _IOW('M', 82, int),
	IOCTL_SET_ROI                           = _IOW('M', 84, VencROIConfig),
	IOCTL_SET_GDC                           = _IOW('M', 85, sGdcParam),
	IOCTL_SET_ROTATE                        = _IOW('M', 86, int),
	IOCTL_SET_REC_REF_LBC_MODE              = _IOW('M', 87, eVeLbcMode),
	IOCTL_SET_WEAK_TEXT_TH                  = _IOW('M', 88, RTVenWeakTextTh),
	//IOCTL_SET_REGION_D3D_PARAM              = _IOW('M', 89, RTVencRegionD3DParam),
	//IOCTL_GET_REGION_D3D_RESULT             = _IOR('M', 90, RTVencRegionD3DResult),
	IOCTL_SET_CHROMA_QP_OFFSET              = _IOW('M', 91, int),
	IOCTL_SET_H264_CONSTRAINT_FLAG          = _IOW('M', 92, VencH264ConstraintFlag),
	IOCTL_SET_VE2ISP_D2D_LIMIT              = _IOW('M', 93, VencVe2IspD2DLimit),
	IOCTL_SET_EN_SMALL_SEARCH_RANGE         = _IOW('M', 94, int),
	IOCTL_SET_FORCE_CONF_WIN                = _IOW('M', 95, VencForceConfWin),
	IOCTL_SET_ROT_VE2ISP                    = _IOW('M', 96, VencRotVe2Isp),
	IOCTL_SET_INSERT_DATA                   = _IOW('M', 97, VencInsertData),
	IOCTL_GET_INSERT_DATA_BUF_STATUS        = _IOR('M', 98, VENC_BUF_STATUS),
	IOCTL_SET_GRAY                          = _IOW('M', 99, int),
	IOCTL_SET_WB_YUV                        = _IOW('M', 100, sWbYuvParam),
	IOCTL_GET_WB_YUV                        = _IOWR('M', 101, RTVencThumbInfo),
	IOCTL_SET_2DNR                          = _IOW('M', 102, s2DfilterParam),
	IOCTL_SET_3DNR                          = _IOW('M', 103, int),
	IOCTL_SET_CYCLIC_INTRA_REFRESH          = _IOW('M', 104, VencCyclicIntraRefresh),
	IOCTL_SET_P_FRAME_INTRA                 = _IOW('M', 105, int),
	IOCTL_RESET_IN_OUT_BUFFER               = _IO('M', 106),
	IOCTL_DROP_FRAME                        = _IOW('M', 107, int), ///< notify rt_media to drop <n> video frames in rt_venc.
	IOCTL_SET_CAMERA_MOVE_STATUS            = _IOW('M', 108, eCameraStatus),
	IOCTL_SET_CROP                          = _IOW('M', 109, VencCropCfg),
	IOCTL_SET_VENC_TARTGET_BITS_CLIP_PARAM  = _IOW('M', 110, VencTargetBitsClipParam),
	IOCTL_SET_JPEG_QUALITY                  = _IOW('M', 111, int),
	IOCTL_SET_FIX_QP                        = _IOW('M', 112, VencFixQP),
	IOCTL_SET_MB_RC_MOVE_STATUS             = _IOW('M', 113, int),
	IOCTL_SET_H264_VIDEO_TIMING             = _IOW('M', 114, VencH264VideoTiming),
	IOCTL_SET_H265_VIDEO_TIMING             = _IOW('M', 115, VencH265TimingS),
	IOCTL_SET_ENC_AND_DEC_CASE              = _IOW('M', 116, int),
	IOCTL_SET_MB_MAP_PARAM                  = _IOW('M', 117, RTVencMbMapParam),
	IOCTL_SET_VENC_SEI_ATTR                 = _IOW('M', 118, RT_VENC_SEI_ATTR),
	IOCTL_SET_P_SKIP                        = _IOW('M', 119, int),
	IOCTL_SET_NULL_FRAME                    = _IOW('M', 120, int),
	IOCTL_SET_REGION_DETECT_LINK            = _IOW('M', 121, sRegionLinkParam),
	IOCTL_GET_REGION_DETECT_LINK            = _IOW('M', 122, sRegionLinkParam),
	IOCTL_SET_VBR_OPT_PARAM                 = _IOW('M', 123, VencVbrOptParam),
	IOCTL_GET_VBR_OPT_PARAM                 = _IOR('M', 124, VencVbrOptParam),
	IOCTL_GET_ENC_MOTION_SEARCH_PARAM       = _IOR('M', 125, VencMotionSearchParam),
	IOCTL_SET_VENC_SHARP_PARAM              = _IOW('M', 126, sEncppSharpParam),
	IOCTL_CATCH_JPEG_SET_CONFIG             = _IOW('M', 127, catch_jpeg_config),
	IOCTL_VENC_INCREASE_RECV_FRAME          = _IOW('M', 129, int),
	IOCTL_SET_VENC_FREQ                     = _IOW('M', 130, int),

	//other operate
	IOCTL_GET_SETUP_RECORDER_IN_KERNEL_CONFIG = _IOR('M', 180, int),
	IOCTL_GET_PRESET_CONFIG                 = _IOR('M', 181, PresetRtMediaChnConfig),
};

/**
  NETLINK_GENERIC related info
  RT_MEDIA_GENL_NAME: rt_media generic netlink family name, string_length + 1 <= GENL_NAMSIZ.
  RT_MEDIA_GENL_VERSION: rt_media generic netlink family version
*/
#define RT_MEDIA_GENL_NAME	"genl-rt_media"
#define RT_MEDIA_GENL_VERSION	0x1

#endif
