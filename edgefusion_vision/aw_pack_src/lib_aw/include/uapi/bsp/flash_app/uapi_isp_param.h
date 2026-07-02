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

#ifndef __UAPI_ISP_PARAM_H__
#define __UAPI_ISP_PARAM_H__

#include <linux/types.h>

struct isp_param_write_info {
	int sensor_id;
	loff_t offset;
	size_t len;
	unsigned char *isp_config;
};

struct isp_param_read_info {
	int sensor_id;
	loff_t offset;
	size_t len;
	unsigned char *isp_config;
};

/* IOCTL command definitions */
#define ISP_PARAM_MAGICNUM 'I'
#define SENSOR_INFO_PARAM_WRITE                                                \
	_IOW(ISP_PARAM_MAGICNUM, 1, struct isp_param_write_info)
#define SENSOR_INFO_PARAM_READ                                                 \
	_IOR(ISP_PARAM_MAGICNUM, 2, struct isp_param_read_info)
#endif /* __UAPI_ISP_PARAM_H__ */
