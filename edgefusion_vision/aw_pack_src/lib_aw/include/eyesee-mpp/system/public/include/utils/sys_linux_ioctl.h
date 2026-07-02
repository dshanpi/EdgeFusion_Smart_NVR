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
/*
 * @description This file is used to compress compile conflicts between sys/ioctl.h and linux/ioctl.h, 
                I want you to use sys/ioctl.h priorly, but if source file must contain linux/ioctl.h, 
                then include this file to avoid compile warning of redefined macro.
 */
#ifndef _SYS_LINUX_IOCTL_H_
#define _SYS_LINUX_IOCTL_H_

#include <sys/ioctl.h>

//out/v853-perf1/compile_dir/target/linux-v853-perf1/linux-4.9.191/user_headers/include/asm-generic/ioctl.h
//                                                   linux-4.9.191 -> lichee/linux-4.9/
//glibc headers will include "user_headers/include/asm-generic/ioctl.h", 
//but musl don't, musl will define _IOC/_IOWR self, and will lead to redefinition conflicts.
#ifndef _ASM_GENERIC_IOCTL_H

#ifdef _IOC
#undef _IOC
#endif
#ifdef _IO
#undef _IO
#endif
#ifdef _IOR
#undef _IOR
#endif
#ifdef _IOW
#undef _IOW
#endif
#ifdef _IOWR
#undef _IOWR
#endif

#endif //!_ASM_GENERIC_IOCTL_H

#include <linux/ioctl.h>

#endif  /* _SYS_LINUX_IOCTL_H_ */
    
