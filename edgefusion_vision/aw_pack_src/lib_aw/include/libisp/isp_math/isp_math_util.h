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

#ifndef _ISP_MATH_UTIL_H_
#define _ISP_MATH_UTIL_H_

#include "../include/isp_type.h"
#include "matrix.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../isp_version.h"
#if (ISP_VERSION == 603)
#define USE_ARM_NEON 0
#else
#include <arm_neon.h>
#define USE_ARM_NEON 1
#endif

#define square(x)	((x) * (x))
#define clear(x)	memset (&(x), 0, sizeof (x))

#define array_size(array)	(sizeof(array) / sizeof((array)[0]))

#define min(a, b) ({				\
	typeof(a) __a = (a);			\
	typeof(b) __b = (b);			\
	__a < __b ? __a : __b;			\
})

#define min_t(type, a, b) ({			\
	type __a = (a);				\
	type __b = (b);				\
	__a < __b ? __a : __b;			\
})

#define max(a, b) ({				\
	typeof(a) __a = (a);			\
	typeof(b) __b = (b);			\
	__a > __b ? __a : __b;			\
})

#define max_t(type, a, b) ({			\
	type __a = (a);				\
	type __b = (b);				\
	__a > __b ? __a : __b;			\
})

#define clamp(val, min, max) ({			\
	typeof(val) __val = (val);		\
	typeof(min) __min = (min);		\
	typeof(max) __max = (max);		\
	__val = __val < __min ? __min : __val;	\
	__val > __max ? __max : __val;		\
})

#define clamp_t(type, val, min, max) ({		\
	type __val = (val);			\
	type __min = (min);			\
	type __max = (max);			\
	__val = __val < __min ? __min : __val;	\
	__val > __max ? __max : __val;		\
})

#define div_round_up(num, denom)	(((num) + (denom) - 1) / (denom))
#define div_round(num, denom)	(((num) + ((denom) >> 1)) / (denom))

#define Q4		16
#define Q8		256
#define Q10		1024

#define SHIFT_Q4	4
#define SHIFT_Q8	8
#define SHIFT_Q10	10

#define R_SHIFT_N(x,n)		(((x) + (1 << (n-1)))>>(n))
#define L_SHIFT_N(x,n)		((x)<<(n))
#define IS_BETWEEN(x,a,b)	(((x)>=(a)) && ((x)<=(b)))

#define N_ROUND(x,y)		(((x)/(y))*(y))

#define ABS(x) ({						\
		long ret;					\
		if (sizeof(x) == sizeof(long)) {		\
			long __x = (x);				\
			ret = (__x < 0) ? -__x : __x;		\
		} else {					\
			int __x = (x);				\
			ret = (__x < 0) ? -__x : __x;		\
		}						\
		ret;						\
	})

void HorizontalMirror(int arr[64]);
void VerticalMirror(int arr[64]);

int ValueInterp(int curr, int x_low,int x_high,int y_low,int y_high);
void Conv(int u[],int v[],int w[], int m, int n);

int ArrayFindMinIndex(int array[], int num);
int ArrayFindSecondMinIndex(int array[], int num, int exclude);
int ArrayFindMaxIndex(int array[],int num);
int ArrayFindSecondMaxIndex(int array[], int num, int exclude);

int ArraySum(int *array,int len);
int ArrayStdVar(int x[], int n);

void ShellSort(int *a,int *sub, int n);

int SqrtM(unsigned int a);

int RoundQ4(int x);

struct matrix {
	float value[3][3];
};

void matrix_zero(struct matrix *m);
void matrix_invert(struct matrix *m);
void matrix_multiply(struct matrix *a, const struct matrix *b);
void matrix_float_to_s2q8(HW_S8Q8 out [3][3], const struct matrix *in);
void matrix_float_to_s4q8(HW_S8Q8 out[3][3], const struct matrix *in);
void matrix_float_to_s8q8(HW_S8Q8 out[3][3], const struct matrix *in);

void spline(float x[], float y[], int n, float yp1, float ypn, float y2[]);
void splint(float xa[], float ya[], float y2a[], int n, float x, float *y);
void spline_interp_u16(unsigned short x[], unsigned short y[], int n,
				unsigned short x2[], unsigned short y2[], int n2);
void gaussianSmooth(float *srcTbl, int RowNum, int ColNum);
void cat32X24TableTo16X16Table(float *srcTbl, float *dstTbl);
void cat32X24TableToTwo16X16Table(float *srcTbl, float *dstTbl_l, float *dstTbl_r);

#endif /*_ISP_MATH_UTIL_H_*/

