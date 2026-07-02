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
#ifndef ALIB_TYPEDEF_H
#define ALIB_TYPEDEF_H

/*
* Register Ur Platform
* __OS_LINUX
* __OS_ANDROID
* __OS_DS5
* __OS_UCOS
* __OS_WIN
* Coming here...
*/

typedef signed char     alib_int8;
typedef unsigned char   alib_uint8;
typedef short           alib_int16;
typedef unsigned short  alib_uint16;
typedef int             alib_int32;
typedef unsigned int    alib_uint32;

typedef void*           alib_handle;
typedef float           alib_f32;
typedef double          alib_lf64;

typedef long int          alib_intptr;
typedef unsigned long int alib_uintptr;

typedef void            alib_void;

typedef unsigned int    alib_size;

#ifndef false
#define false 0
#endif

#ifndef true
#define true  1
#endif


#if (defined __OS_LINUX) || (defined __OS_ANDROID) || (defined __OS_DS5)

#include <stdint.h>
typedef int64_t         alib_int64;
typedef uint64_t        alib_uint64;

#ifndef	TCHAR
typedef char TCHAR;
#endif

#elif (defined __OS_UCOS)

typedef signed long long    alib_int64;
typedef unsigned long long  alib_uint64;

#ifndef	TCHAR
typedef char TCHAR;
#endif

#elif (defined __OS_WIN)

typedef __int64           alib_int64;
typedef unsigned __int64  alib_uint64;
#include <tchar.h>

#else
#error "No specific platform..."
#endif

#endif
