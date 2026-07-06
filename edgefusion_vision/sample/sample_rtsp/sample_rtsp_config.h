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

#ifndef _SAMPLE_RTSP_CONFIG_H_
#define _SAMPLE_RTSP_CONFIG_H_

#define CFG_MainIsp                     "main_isp"
#define CFG_MainVipp                    "main_vipp"
#define CFG_MainSrcWidth                "main_src_width"
#define CFG_MainSrcHeight               "main_src_height"
#define CFG_MainPixelFormat             "main_pixel_format"
#define CFG_MainWdrEnable               "main_wdr_enable"
#define CFG_MainViBufNum                "main_vi_buf_num"
#define CFG_MainSrcFrameRate            "main_src_frame_rate"
#define CFG_MainViChn                   "main_viChn"
#define CFG_MainVEncChn                 "main_venc_chn"
#define CFG_MainEncodeType              "main_encode_type"
#define CFG_MainEncodeWidth             "main_encode_width"
#define CFG_MainEncodeHeight            "main_encode_height"
#define CFG_MainEncodeFrameRate         "main_encode_frame_rate"
#define CFG_MainEncodeBitrate           "main_encode_bitrate"
#define CFG_MainFilePathh               "main_file_path"
#define CFG_MainOnlineEnable            "main_online_en"
#define CFG_MainOnlineShareBufNum       "main_online_share_buf_num"
#define CFG_MainEncppEnable             "main_encpp_enable"

#define CFG_SubIsp                      "sub_isp"
#define CFG_SubVipp                     "sub_vipp"
#define CFG_SubSrcWidth                 "sub_src_width"
#define CFG_SubSrcHeight                "sub_src_height"
#define CFG_SubPixelFormat              "sub_pixel_format"
#define CFG_SubWdrEnable                "sub_wdr_enable"
#define CFG_SubViBufNum                 "sub_vi_buf_num"
#define CFG_SubSrcFrameRate             "sub_src_frame_rate"

#define CFG_SubVippCropEnable           "sub_vipp_crop_en"
#define CFG_SubVippCropRectX            "sub_vipp_crop_rect_x"
#define CFG_SubVippCropRectY            "sub_vipp_crop_rect_y"
#define CFG_SubVippCropRectWidth        "sub_vipp_crop_rect_w"
#define CFG_SubVippCropRectHeight       "sub_vipp_crop_rect_h"

#define CFG_SubViChn                    "sub_viChn"
#define CFG_SubVEncChn                  "sub_venc_chn"
#define CFG_SubEncodeType               "sub_encode_type"
#define CFG_SubEncodeWidth              "sub_encode_width"
#define CFG_SubEncodeHeight             "sub_encode_height"
#define CFG_SubEncodeFrameRate          "sub_encode_frame_rate"
#define CFG_SubEncodeBitrate            "sub_encode_bitrate"
#define CFG_SubFilePath                 "sub_file_path"
#define CFG_SubOnlineEnable             "sub_online_en"
#define CFG_SubOnlineShareBufNum        "sub_online_share_buf_num"
#define CFG_SubEncppEnable              "sub_encpp_enable"

#define CFG_SubLapseViChn               "sub_lapse_viChn"
#define CFG_SubLapseVEncChn             "sub_lapse_venc_chn"
#define CFG_SubLapseEncodeType          "sub_lapse_encode_type"
#define CFG_SubLapseEncodeWidth         "sub_lapse_encode_width"
#define CFG_SubLapseEncodeHeight        "sub_lapse_encode_height"
#define CFG_SubLapseEncodeFrameRate     "sub_lapse_encode_frame_rate"
#define CFG_SubLapseEncodeBitrate       "sub_lapse_encode_bitrate"
#define CFG_SubLapseFilePath            "sub_lapse_file_path"
#define CFG_SubLapseTime                "sub_lapse_time"
#define CFG_SubLapseEncppEnable         "sub_lapse_encpp_enable"

#define CFG_IspAndVeLinkageEnable       "isp_ve_linkage_enable"
#define CFG_CameraAdaptiveMovingAndStaticEnable "camera_adaptive_moving_and_static_enable"
#define CFG_VencLensMovingMaxQp         "ve_lens_moving_max_qp"

#define CFG_WbYuvEnable                 "wb_yuv_enable"
#define CFG_WbYuvBufNum                 "wb_yuv_buf_num"
#define CFG_WbYuvScalerRatio            "wb_yuv_scaler_ratio"
#define CFG_WbYuvStartIndex             "wb_yuv_start_index"
#define CFG_WbYuvTotalCnt               "wb_yuv_total_cnt"
#define CFG_WbYuvStreamChannel          "wb_yuv_stream_channel"
#define CFG_WbYuvFilePath               "wb_yuv_file_path"

#define CFG_RtspNetType                 "rtsp_net_type"
#define CFG_StreamBufSize               "stream_buf_size"

#define CFG_TestDuration                "test_duration"

#endif  /* _SAMPLE_RTSP_CONFIG_H_ */
