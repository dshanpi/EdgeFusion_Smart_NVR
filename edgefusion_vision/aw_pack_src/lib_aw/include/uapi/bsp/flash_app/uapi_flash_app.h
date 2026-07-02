/* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
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

 * SUNXI Flash App Driver
 *
 * 2024.12.1  lujianliang <lujianliang@allwinnertech.com>
 */

#ifndef __UAPI_FLASH_APP_H__
#define __UAPI_FLASH_APP_H__

#include <linux/types.h>

typedef enum {
	STORAGE_NAND = 0,
	STORAGE_SD,
	STORAGE_EMMC,
	STORAGE_NOR,
	STORAGE_EMMC3,
	STORAGE_SPI_NAND,
	STORAGE_SD1,
	STORAGE_EMMC0,
	STORAGE_UFS,
} SUNXI_BOOT_STORAGE;

struct rawpart_op_param {
	char name[16];
	loff_t offset;
	size_t len;
	__u64 user_data;
};

struct secstorage_op_param {
	int item;
	unsigned int *buf;
	unsigned int len;
};

#define RAWPART_READ		_IOR('M', 1, struct rawpart_op_param)
#define RAWPART_WRITE		_IOW('M', 2, struct rawpart_op_param)
#define SECURE_STORAGE_READ	_IO('M', 3)
#define SECURE_STORAGE_WRITE	_IO('M', 4)

#endif /* __UAPI_FLASH_APP_H__ */
