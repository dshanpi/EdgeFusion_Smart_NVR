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
#ifndef __API_ACTION_H_
#define __API_ACTION_H_

#include <os_net_utils.h>
#include <os_net_queue.h>
#include <os_net_sem.h>
#include <os_net_mutex.h>
#include <os_net_thread.h>

typedef struct {
    int (*action)(void **call_argv, void **cb_argv);
    const char *name;
} act_func_t;

typedef uint32_t act_post_timeout_t;

typedef struct {
    os_net_queue_t queue;
    os_net_thread_t thread;
    os_net_thread_pid_t pid;
    os_net_sem_t sem;
    os_net_recursive_mutex_t mutex;
    bool is_sync;
    bool enable;
} act_handle_t;

/*
 * 1.ACT_FUNC_NUM is currently set to 20
 * 2.bt   use 0 ~ 9 table number
 * 3.wifi use 10 ~ 19 table number
 * 4.If other modules want to use the api action, needs to define the table number
 */
#define ACT_FUNC_NUM 20

os_net_status_t act_register_handler(act_handle_t *handle, uint8_t id, act_func_t *action);

os_net_status_t act_init(act_handle_t *handle, const char *identifier, bool is_sync);

os_net_status_t act_deinit(act_handle_t *handle);

os_net_status_t act_transfer(act_handle_t *handle, uint8_t id, uint16_t opcode, int call_arg_len,
                             int cb_arg_len, ...);

#define OS_NET_TASK_QUEUE_LEN (24)
#define OS_NET_TASK_STACK_SIZE (4096)
#define OS_NET_TASK_PRIO (0)

#endif /*__ACTION_H_*/
