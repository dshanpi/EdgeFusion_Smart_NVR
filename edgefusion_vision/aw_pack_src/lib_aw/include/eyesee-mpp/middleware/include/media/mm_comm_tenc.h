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
#ifndef __MM_COMM_TENC_H__
#define __MM_COMM_TENC_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

///////////////////////////

#include "plat_type.h"
#include "plat_errno.h" 
#include "mm_common.h"

/* invlalid channel ID */
#define ERR_TENC_INVALID_CHNID     DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* at lease one parameter is illagal ,eg, an illegal enumeration value  */
#define ERR_TENC_ILLEGAL_PARAM     DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* channel exists */
#define ERR_TENC_EXIST             DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/* channel exists */
#define ERR_TENC_UNEXIST           DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* using a NULL point */
#define ERR_TENC_NULL_PTR          DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* try to enable or initialize system,device or channel, before configing attribute */
#define ERR_TENC_NOT_CONFIG        DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* operation is not supported by NOW */
#define ERR_TENC_NOT_SUPPORT       DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/* operation is not permitted ,eg, try to change stati attribute */
#define ERR_TENC_NOT_PERM          DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* failure caused by malloc memory */
#define ERR_TENC_NOMEM             DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* failure caused by malloc buffer */
#define ERR_TENC_NOBUF             DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* no data in buffer */
#define ERR_TENC_BUF_EMPTY         DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* no buffer for new data */
#define ERR_TENC_BUF_FULL          DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* system is not ready,had not initialed or loaded*/
#define ERR_TENC_SYS_NOTREADY      DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/* system is busy*/
#define ERR_TENC_BUSY              DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
/* component state is same as user wanted */
#define ERR_TENC_SAMESTATE         DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_SAMESTATE)
/* component state is transit to invalid state */
#define ERR_TENC_INVALIDSTATE      DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALIDSTATE)
/* component current state can't transit to destination state */
#define ERR_TENC_INCORRECT_STATE_TRANSITION DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_INCORRECT_STATE_TRANSITION)
/* Attempting a command that is not allowed during the present state. */
#define ERR_TENC_INCORRECT_STATE_OPERATION DEF_ERR(MOD_ID_TENC, EN_ERR_LEVEL_ERROR, EN_ERR_INCORRECT_STATE_OPERATION)


typedef struct TEXT_STREAM_S
{ 
    unsigned char       *pStream;       /* the virtual address of stream */
    unsigned char       *pStreamExtra;
    unsigned int        mLen;           /* stream lenth, by bytes */
    unsigned int        mExtraLen;
    unsigned long long  mTimeStamp;     /* frame time stamp */
    int                 mId;            /* frame id */
} TEXT_STREAM_S;

typedef struct TEXT_FRAME_S
{
    char                mpAddr[200];
    unsigned long long  mTimeStamp; //unit:us
    unsigned int        mLen;
    unsigned int        mId;
} TEXT_FRAME_S;

typedef struct TEXTINFO_t
{
    int enc_enable_type;                // bit0-gps; bit1-gsensor; bit2-adas; bit3-driver_id
    int text_enc_type;                  // CODEC_ID_GGAD
    int video_frame_rate;
    int gsensor_rate;       // set its value according to video_frame_rate
    int adas_rate;
}TEXTINFO_t;

typedef struct TENC_CHN_ATTR_S
{
    TEXTINFO_t tInfo;                           /*the attribution of audio encoder*/
}TENC_CHN_ATTR_S;



//////////////////////////

#ifdef __cplusplus
}
#endif /* __cplusplus */ 

#endif
