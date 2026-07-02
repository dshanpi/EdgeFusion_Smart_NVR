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

#ifndef __UAPI_LCD_FB_H__
#define __UAPI_LCD_FB_H__

#include <linux/types.h>

struct lcd_fb_dmabuf_export {
	int fd;
	unsigned int flags;
};

/* IOCTL command definitions */
#define LCD_FB_FILE_MAGICNUM 'L'
#define LCDFB_IO_GET_DMABUF_FD _IOW(LCD_FB_FILE_MAGICNUM, 1, struct lcd_fb_dmabuf_export)

#endif /* __UAPI_LCD_FB_H__ */
