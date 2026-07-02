/* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note */
/* Copyright(c) 2020 - 2026 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2021-2028 Allwinnertech Co., Ltd.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __UAPI_ISP_IQ_FILE_H__
#define __UAPI_ISP_IQ_FILE_H__

#include <linux/types.h>

struct isp_param_iq_file_write_info {
	char *file_path;
	loff_t offset;
};

/* IOCTL command definitions */
#define ISP_PARAM_IQ_FILE_MAGICNUM 'I'
#define ISP_PARAM_WRITE_IQ_FILE _IOW(ISP_PARAM_IQ_FILE_MAGICNUM, 1, struct isp_param_iq_file_write_info)

#endif /* __UAPI_ISP_IQ_FILE_H__ */
