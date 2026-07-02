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

#ifndef __VIDEO_H_
#define __VIDEO_H_

#include <stdbool.h>

#include "../V4l2Camera/sunxi_camera_v2.h"

#if (ISP_VERSION == 603)
#define HW_VIDEO_DEVICE_NUM 6
#else
#define HW_VIDEO_DEVICE_NUM 16
#endif

struct video_plane {
	unsigned int size;
	int dma_fd;
	void *mem;
	unsigned int  mem_phy;
};

struct video_buffer {
	unsigned int index;
	unsigned int bytesused;
	unsigned int frame_cnt;
	unsigned int exp_time;
	struct timeval timestamp;
	bool error;
	bool allocated;
	unsigned int nplanes;
	struct video_plane *planes;
};

struct buffers_pool {
	unsigned int nbufs;
	struct video_buffer *buffers;
};

struct video_selection_rect {
	int   left;
	int   top;
	int   width;
	int   height;
};

struct video_fmt {
	enum v4l2_buf_type type;
	enum v4l2_memory memtype;
	struct v4l2_pix_format_mplane format;
	unsigned int nbufs;
	unsigned int nplanes;
	unsigned int fps;
	unsigned int capturemode;
	unsigned int use_current_win;
	unsigned int wdr_mode;
	unsigned int drop_frame_num;
	int index;
	unsigned char ve_online_en;
	enum dma_buffer_num dma_buf_num;
	enum mipi_pix_num pixel_num;
	unsigned char tdm_speed_down_en;
	unsigned char tdm_tx_valid_num;
	unsigned char tdm_tx_invalid_num;
	unsigned char tdm_tx_valid_num_offset;
	unsigned char large_dma_merge_en;
	unsigned char video_selection_en;
	struct video_selection_rect rect;
	unsigned int tdm_rxbuf_cnt;
	FILE *ptn_file;
	unsigned char ptn_en;
	unsigned int ptn_count_total;
	unsigned int *ptn_count;
	unsigned int ptn_frame_temp;
	unsigned char *ptn_read_dir;
	unsigned char ptn_type;
	unsigned int start_frame;
	unsigned int end_frame;
	struct vin_pattern_config ptn_cfg;
	struct dma_merge_scaler_cfg scaler_cfg;
};

struct osd_fmt {
	int clipcount;		/* number of clips */
	int chromakey;
	int global_alpha;
	int reverse_close[64];	/*osd reverse close, 1:close, 0:open*/
	int rgb_cover[8];
	int glb_alpha[64];
	int inv_w_rgn[8];
	int inv_h_rgn[8];
	int inv_th;
	int width;
	void *bitmap[64];
	struct v4l2_rect region[64];	/* overlay or cover win */
};

//Object Rectangle Label..
struct orl_fmt {
	int clipcount;		/* number of clips */
	int mThick;     //unit: pixel. [0, 7]
	int mRgbColor[64]; // color of line.
	struct v4l2_rect region[64];	/* overlay or cover win */
};

struct video_event {
	unsigned int event_id;
	unsigned int event_type;
	unsigned int frame_cnt;
	struct timespec vsync_ts;
};

struct isp_video_device {
	unsigned int id;
	unsigned int isp_id;
	struct media_entity *entity;

	enum v4l2_buf_type type;
	enum v4l2_memory memtype;

	struct v4l2_pix_format_mplane format;

	unsigned int nbufs;
	unsigned int nplanes;
	unsigned int capturemode;
	unsigned int use_current_win;
	unsigned int wdr_mode;
	struct buffers_pool *pool;
	unsigned int fps;
	unsigned int drop_frame_num;
	unsigned char ve_online_en;
	enum dma_buffer_num dma_buf_num;
	FILE *ptn_file;
	unsigned char ptn_en;
	unsigned int ptn_count_total;
	unsigned int ptn_count;
	unsigned char ptn_read_dir;
	unsigned char ptn_type;
	unsigned int start_frame;
	unsigned int end_frame;
	struct vin_pattern_config ptn_cfg;

	void *priv;
};

int video_init(struct isp_video_device *video);
void video_cleanup(struct isp_video_device *video);
int video_to_isp_id(struct isp_video_device *video);
int video_set_fmt(struct isp_video_device *video, struct video_fmt *vfmt);
int video_get_fmt(struct isp_video_device *video, struct video_fmt *vfmt);
void video_set_next_ptn(struct isp_video_device *video, struct video_fmt *vfmt);
void video_set_ldci_mode(struct isp_video_device *video, unsigned int ldci_select);
int video_req_buffers(struct isp_video_device *video, struct buffers_pool *pool);
int video_free_buffers(struct isp_video_device *video);
int video_wait_buffer(struct isp_video_device *video, int timeout);
int video_dequeue_buffer(struct isp_video_device *video,	struct video_buffer *buffer);
int video_queue_buffer(struct isp_video_device *video, unsigned int buf_id);
int video_s_selection(struct isp_video_device *video, struct v4l2_selection *s);
int video_stream_on(struct isp_video_device *video);
int video_stream_off(struct isp_video_device *video);

struct buffers_pool *buffers_pool_new(struct isp_video_device *video);
void buffers_pool_delete(struct isp_video_device *video);
int video_save_frames(struct isp_video_device *video, unsigned int buf_id, char *path);

int overlay_set_fmt(struct isp_video_device *video, struct osd_fmt *ofmt);
int overlay_update(struct isp_video_device *video, int on_off);
//draw osd rectangle line.
int orl_set_fmt(struct isp_video_device *video, struct orl_fmt *pOrlFmt);

int video_set_control(struct isp_video_device *video, int cmd, int value);
int video_get_control(struct isp_video_device *video, int cmd, int *value);
int video_query_control(struct isp_video_device *video, struct v4l2_queryctrl *ctrl);
int video_query_menu(struct isp_video_device *video, struct v4l2_querymenu *menu);
int video_bk_fd_merge(struct isp_video_device *video, struct mrg_int_ch_cfg *cfg);
int video_get_controls(struct isp_video_device *video, unsigned int count,
		      struct v4l2_ext_control *ctrls);
int video_set_controls(struct isp_video_device *video, unsigned int count,
		      struct v4l2_ext_control *ctrls);

int video_event_subscribe(struct isp_video_device *video, unsigned int type);
int video_event_unsubscribe(struct isp_video_device *video, unsigned int type);
int video_wait_event(struct isp_video_device *video);
int video_dequeue_event(struct isp_video_device *video, struct video_event *vi_event);
int video_set_sync_ctrl(struct isp_video_device *video, const struct csi_sync_ctrl *sync);
int video_set_top_clk(struct isp_video_device *video, unsigned int rate);
int video_set_selection(struct isp_video_device *video, struct video_selection_rect *rect);

int video_set_isp_d3d_lbc_ratio(struct isp_video_device *video, unsigned int d3d_lbc_ratio);
int video_set_isp_bk_buffer_align(struct isp_video_device *video, struct bk_buffer_align *bk_align);
int video_set_isp_bk_width_stride(struct isp_video_device *video, unsigned char enable);
int video_set_tdm_drop_frame_num(struct isp_video_device *video, unsigned int drop_num);
int video_tdm_raw_npu_mode_disable(struct isp_video_device *video, unsigned char not_npu_mode);
int video_set_input_bit_width_start(struct isp_video_device *video, enum set_bit_width bitwidth);
int video_set_input_bit_width_stop(struct isp_video_device *video, enum set_bit_width bitwidth);
int video_set_dma_overlay(struct isp_video_device *video, struct dma_overlay_para *dma_overlay_ctx);
int video_set_vbv_share_yuv(struct isp_video_device *video, unsigned int enable);

#endif /* __VIDEO_H_ */
