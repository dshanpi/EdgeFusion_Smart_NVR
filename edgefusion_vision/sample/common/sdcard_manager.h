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
#ifndef _STORAGE_MANAGER_H_
#define _STORAGE_MANAGER_H_

#define SDCARD_DEV "/dev/mmcblk0p1"
#define SDCARD_DEV_2 "/dev/mmcblk0"
#define SDCARD_MOUNT_PATH "/mnt/extsd"


int sd_card_format_func();
int storage_manager_init(void);
int storage_manager_exit(void);
int storage_manager_check_mount(const char *mount_path, const char *dev_path);
int storage_manager_mount(const char *mount_path, const char *dev_path);
int storage_manager_umount(const char *mount_path);
int storage_manager_format(const char *dev_path);
int storage_manager_get_dir_capacity(const char *dir,
				     unsigned int *avail,
				     unsigned int *total);
/*input : fix=1, check fat file sytem and fix when is wrong; fix=0, dont fix*/
/*return  >=0 :are support , <0:not support this systemfile*/
int storage_manger_check_and_fix_filesystem_support(int fix);

/*checing sd card was be checking??*/
/*input: fb is a callback func, it will be run before reboot*/
int enable_sdcard_staus_checking(void *fb);
int disable_sdcard_staus_checking(void);

int check_sdcard_insert(void);

#endif /*_STORAGE_MANAGER_H_*/
