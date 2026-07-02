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
#ifndef __WIFI_LOG_H
#define __WIFI_LOG_H

#include <stdint.h>

#if __cplusplus
extern "C" {
#endif

typedef enum __wmg_log_level{
	WMG_MSG_NONE = 0,
	WMG_MSG_ERROR,
	WMG_MSG_WARNING,
	WMG_MSG_INFO,
	WMG_MSG_DEBUG,
	WMG_MSG_MSGDUMP,
	WMG_MSG_EXCESSIVE,
	WMG_MSG_EXCESSIVE_MID,
	WMG_MSG_EXCESSIVE_MAX
}wmg_log_level_t;

typedef enum __wmg_log_setting{
	WMG_LOG_SETTING_NONE = 0,
	WMG_LOG_SETTING_TIME = 1 << 0,
	WMG_LOG_SETTING_FILE = 1 << 1,
	WMG_LOG_SETTING_FUNC = 1 << 2,
	WMG_LOG_SETTING_LINE = 1 << 3,
}wmg_log_setting_t;

void wmg_print(int level, int para_enable, const char *tag, const char *file, const char *func, int line, const char *fmt, ...);
void wmg_set_debug_level(wmg_log_level_t level);
int wmg_get_debug_level();
void wmg_set_log_para(int log_para);
int wmg_get_log_para();
#if defined(OS_NET_LINUX_OS) || defined(OS_NET_XRLINK_OS)
/*
 * This interface is used to transfer standard output to other io devices
 * @fd(> 0): witch io devices fd
 * @fd(-1): printing switches back to standard output
 * */
void wmg_set_log_fd(int fd);
#endif

#ifdef OS_NET_FREERTOS_OS

#define WMG_DEBUG(fmt,arg...) \
	wmg_print(WMG_MSG_DEBUG, 1, "WDG: ", NULL, NULL, 0, fmt,##arg)

#define WMG_DEBUG_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_DEBUG, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_INFO(fmt,arg...) \
	wmg_print(WMG_MSG_INFO, 1, "WIF: ", NULL, NULL, 0, fmt,##arg)

#define WMG_INFO_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_INFO, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_WARNG(fmt,arg...) \
	wmg_print(WMG_MSG_WARNING, 1, "WAR: ", NULL, NULL, 0, fmt,##arg)

#define WMG_WARNG_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_WARNING, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_ERROR(fmt,arg...) \
	wmg_print(WMG_MSG_ERROR, 1, "WER: ", NULL, NULL, 0, fmt,##arg)

#define WMG_ERROR_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_ERROR, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_DUMP(fmt,arg...) \
	wmg_print(WMG_MSG_MSGDUMP, 1, "WDP: ", NULL, NULL, 0, fmt,##arg)

#define WMG_DUMP_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_MSGDUMP, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_EXCESSIVE(fmt,arg...) \
	wmg_print(WMG_MSG_EXCESSIVE, 1, "WEX: ", NULL, NULL, 0, fmt,##arg)

#define WMG_EXCESSIVE_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_EXCESSIVE, 0, NULL, NULL, NULL, 0, fmt,##arg)

#else

#define WMG_DEBUG(fmt,arg...) \
	wmg_print(WMG_MSG_DEBUG, 1, "WDUG: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt, ##arg)

#define WMG_DEBUG_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_DEBUG, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_INFO(fmt,arg...) \
	wmg_print(WMG_MSG_INFO, 1, "WINF: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt,##arg)

#define WMG_INFO_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_INFO, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_WARNG(fmt,arg...) \
	wmg_print(WMG_MSG_WARNING, 1, "WWAR: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt,##arg)

#define WMG_WARNG_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_WARNING, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_ERROR(fmt,arg...) \
	wmg_print(WMG_MSG_ERROR, 1, "WERR: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt,##arg)

#define WMG_ERROR_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_ERROR, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_DUMP(fmt,arg...) \
	wmg_print(WMG_MSG_MSGDUMP, 1, "WDUP: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt,##arg)

#define WMG_DUMP_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_MSGDUMP, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_EXCESSIVE(fmt,arg...) \
	wmg_print(WMG_MSG_EXCESSIVE, 1, "WEXC: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt,##arg)

#define WMG_EXCESSIVE_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_EXCESSIVE, 0, NULL, NULL, NULL, 0, fmt,##arg)
#endif

#if __cplusplus
};  // extern "C"
#endif

#endif //__WIFI_LOG_H
