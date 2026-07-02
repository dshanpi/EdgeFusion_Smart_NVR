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
#ifndef AUDIODEC_DECODE_H
#define AUDIODEC_DECODE_H
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#define cdx_mutex_lock(x)	pthread_mutex_lock(x)
#define cdx_mutex_unlock(x)  pthread_mutex_unlock(x)

typedef void AudioDecoderLib;

typedef struct CedarAudioCodec 
{
    const char *name;
    struct __AudioDEC_AC320 *(*init)(void);
    int	 (*exit)(struct __AudioDEC_AC320 *p);
    int   flag;         // indicating whether need to build bs frm header.
} CedarAudioCodec;


typedef struct InitCodecInfo 
{
	const char *handle;
    const char *name;
    const char *init;
    const char *exit;
    int   flag;
} InitCodecInfo;

typedef struct RaFormatInofStruct
{
    unsigned int    ulSampleRate;
    unsigned int    ulActualRate;
    unsigned short  usBitsPerSample;
    unsigned short  usNumChannels;
    unsigned short  usAudioQuality;
    unsigned short  usFlavorIndex;
    unsigned int    ulBitsPerFrame;
    unsigned int    ulGranularity;
    unsigned int    ulOpaqueDataSize;
    unsigned char*  pOpaqueData;
} RaFormatInfoT;
/*********************************************************************************/

typedef enum INPUTBSFILLMODE
{
    BSFILL_MODE_UNKONE = 0,
    BSFILL_MODE_NORMAL,
    BSFILL_MODE_HDRWRAP,
    BSFILL_MODE_BSWRAP,
}INPUTBSFILLMODE;

typedef struct CedarInputBsManage
{
    INPUTBSFILLMODE mode;
    unsigned char   *pHoloStartAddr0;
    int             nHoloLen0;
    unsigned char   *pHoloStartAddr1;
    int             nHoloLen1;
}CedarInputBsManage;

typedef struct InputStreamBuf
{
    CedarInputBsManage InputBsManage;
    int                CedarAbsPackHdrLen;
    unsigned char      CedarAbsPackHdr[16];
}InputStreamBufT;

typedef struct AudioDecoderContextStructLib
{
    pthread_mutex_t   mutex_audiodec_thread;
    int               nAudioInfoFlags;  // flag indicating current bs contains frm header or not.
    InputStreamBufT   Streambuffer;     // for adding pkt header 
    Ac320FileRead     DecFileInfo;
    com_internal      pInternal;
    
    CedarAudioCodec   *pCedarCodec;     // structure above specific library
	void              *libhandle;
    AudioDEC_AC320    *pCedarAudioDec;  // handle to specific library
	const char        *handlename;
}AudioDecoderContextLib;


int ParseRequestAudioBitstreamBuffer(AudioDecoderLib* pDecoder,
               int              nRequireSize,
               unsigned char**  ppBuf,
               int*             pBufSize,
               unsigned char**  ppRingBuf,
               int*             pRingBufSize,
               int*             nOffset);
int ParseUpdateAudioBitstreamData(AudioDecoderLib* pDecoder,
               int     nFilledLen,
               int64_t nTimeStamp,
               int     nOffset);
int ParseAudioStreamDataSize(AudioDecoderLib* pDecoder);
void BitstreamQueryQuality(AudioDecoderLib* pDecoder, int* pValidPercent, int* vbv);
void ParseBitstreamSeekSync(AudioDecoderLib* pDecoder, int64_t nSeekTime, int nGetAudioInfoFlag);

int InitializeAudioDecodeLib(AudioDecoderLib*    pDecoder,
               AudioStreamInfo* pAudioStreamInfo,
               BsInFor *pBsInFor,
               AudioDecBufInfo *pAudioDecBufInfo);
int DecodeAudioFrame(AudioDecoderLib* pDecoder,
               char*        ppBuf,
               int*          pBufSize);
int DestroyAudioDecodeLib(AudioDecoderLib* pDecoder);

void SetAudiolibRawParam(AudioDecoderLib* pDecoder, int commond);

void SetAudioDecEOS(AudioDecoderLib* pDecoder);

AudioDecoderLib* CreateAudioDecodeLib(void);

typedef enum AudioInternCtlCmd{
    AUDIO_INTERNAL_CMD_GET_APE_FILE_VERSION,
    AUDIO_INTERNAL_CMD_NUM
}AudioInternCtlCmd;

void AudioInternalCtl(int cmd, void* parm1, void* parm2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif//AUDIODEC_DECODE_H
