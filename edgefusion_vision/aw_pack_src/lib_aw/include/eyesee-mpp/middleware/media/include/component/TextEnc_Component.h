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

#ifndef __TEXTENC_COMPONENT_H__
#define __TEXTENC_COMPONENT_H__

//ref platform headers
#include "plat_type.h"
#include "plat_errno.h"
#include "plat_defines.h"
#include "plat_math.h"

//media api headers to app
#include "mm_common.h"
#include "mm_comm_tenc.h"
#include "mpi_tenc.h"
#include <TextEncApi.h>
//media internal common headers.
#include "media_common.h"
#include "mm_component.h"
#include "ComponentCommon.h"
#include "tmessage.h"
#include "tsemaphore.h"


#define MAX_TENCODER_PORTS (2)
#define TFIFO_LEVEL         TEXTENC_PACKET_CNT
#define TENC_FRAME_COUNT    (TFIFO_LEVEL+1)



typedef struct TEXTENCDATATYPE {
    COMP_STATETYPE state;
    pthread_mutex_t mStateLock;
    COMP_CALLBACKTYPE *pCallbacks;
    void* pAppData;
    COMP_HANDLETYPE hSelf;
    char mThreadName[32];

    COMP_PORT_PARAM_TYPE sPortParam;
    COMP_PARAM_PORTDEFINITIONTYPE sInPortDef;
    COMP_PARAM_PORTDEFINITIONTYPE sOutPortDef;
    COMP_INTERNAL_TUNNELINFOTYPE sInPortTunnelInfo;
    COMP_INTERNAL_TUNNELINFOTYPE sOutPortTunnelInfo;

    COMP_PARAM_BUFFERSUPPLIERTYPE sPortBufSupplier[MAX_TENCODER_PORTS];
    BOOL mInputPortTunnelFlag;
    BOOL mOutputPortTunnelFlag;   //TRUE: tunnel mode; FALSE: non-tunnel mode.

    MPP_CHN_S mMppChnInfo;
    pthread_t thread_id;
    CompInternalMsgType eTCmd;
    message_queue_t  cmd_queue;
    volatile int mNoInputTextFlag; //1: aenclib:no input pcm data to be encoded.
    volatile int mNoOutFrameFlag; //1: aenclib:no out frame to store encoded audio data.

    __text_enc_info_t text_info;
    TEXTENCCONTENT_t *htext_enc; 


    int64_t mSendTextFailCnt;    // for update pts 

    
    int mOutputFrameNotifyPipeFds[2];

    struct list_head    mIdleOutFrameList;  //AEncCompOutputBuffer
    struct list_head    mReadyOutFrameList; //AEncCompOutputBuffer, for non-tunnel mode. when mOutputPortTunnelFlag == FALSE, use it to store encoded frames.
    struct list_head    mUsedOutFrameList; //AEncCompOutputBuffer, for non-tunnel mode. when mOutputPortTunnelFlag == FALSE, use it to store user occupied frames.
    int                 mFrameNodeNum;
    BOOL                mWaitOutFrameFullFlag;
    BOOL                mWaitOutFrameFlag;  //for non-tunnel mode, wait outFrame coming!
    pthread_mutex_t     mInputTextMutex;
    pthread_mutex_t     mOutFrameListMutex;
    pthread_cond_t      mOutFrameFullCondition;
    pthread_cond_t      mOutFrameCondition; //for non-tunnel mode, wait outFrame coming!
}TEXTENCDATATYPE;


ERRORTYPE TextEncComponentDeInit(PARAM_IN COMP_HANDLETYPE hComponent);
ERRORTYPE TextEncComponentInit(PARAM_IN COMP_HANDLETYPE hComponent);



#endif
