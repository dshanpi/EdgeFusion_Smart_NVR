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
#ifndef __SEMAPHORE_H_X6E3__
#define __SEMAPHORE_H_X6E3__

#include <pthread.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

typedef struct cdx_sem_t
{
	pthread_cond_t 	condition;
	pthread_mutex_t mutex;
	unsigned int 	semval;
}cdx_sem_t;

int cdx_sem_init(cdx_sem_t* tsem, unsigned int val);
void cdx_sem_deinit(cdx_sem_t* tsem);
void cdx_sem_down(cdx_sem_t* tsem);
int cdx_sem_down_timedwait(cdx_sem_t* tsem, unsigned int timeout);
void cdx_sem_up(cdx_sem_t* tsem);
void cdx_sem_wait_unique(cdx_sem_t* tsem);
int cdx_sem_timedwait_unique(cdx_sem_t* tsem, unsigned int timeout);
void cdx_sem_up_unique(cdx_sem_t* tsem);
void cdx_sem_reset(cdx_sem_t* tsem);
void cdx_sem_wait(cdx_sem_t* tsem);
int cdx_sem_timewait(cdx_sem_t* tsem, unsigned int msec);
void cdx_sem_signal(cdx_sem_t* tsem);
unsigned int cdx_sem_get_val(cdx_sem_t* tsem);


#define cdx_mutex_lock(x)	pthread_mutex_lock(x)
#define cdx_mutex_unlock(x)  pthread_mutex_unlock(x)

#if 0
#define cdx_cond_wait_while_exp(tsem, expression) \
	pthread_mutex_lock(&tsem.mutex); \
	while (expression) { \
		LOGV("sem_wait:%d",__LINE__); \
		pthread_cond_wait(&tsem.condition, &tsem.mutex); \
		LOGV("sem_wait end:%d",__LINE__); \
	} \
	pthread_mutex_unlock(&tsem.mutex);

#define cdx_cond_wait_if_exp(tsem, expression) \
	pthread_mutex_lock(&tsem.mutex); \
	if (expression) { \
		LOGV("sem_wait:%d",__LINE__); \
		pthread_cond_wait(&tsem.condition, &tsem.mutex); \
		LOGV("sem_wait end:%d",__LINE__); \
	} \
	pthread_mutex_unlock(&tsem.mutex);
#else

#define cdx_cond_wait_while_exp(tsem, expression) \
	pthread_mutex_lock(&tsem.mutex); \
	while (expression) { \
		pthread_cond_wait(&tsem.condition, &tsem.mutex); \
	} \
	pthread_mutex_unlock(&tsem.mutex);

#define cdx_cond_wait_if_exp(tsem, expression) \
	pthread_mutex_lock(&tsem.mutex); \
	if (expression) { \
		pthread_cond_wait(&tsem.condition, &tsem.mutex); \
	} \
	pthread_mutex_unlock(&tsem.mutex);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
