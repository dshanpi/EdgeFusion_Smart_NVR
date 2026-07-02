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
#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum __AUDIO_ENC_RESULT
{
    ERR_AUDIO_ENC_ABSEND        = -2,
    ERR_AUDIO_ENC_UNKNOWN       = -1,
    ERR_AUDIO_ENC_NONE          = 0,   //decode successed, no error
    ERR_AUDIO_ENC_PCMUNDERFLOW  = 1,   //pcm data is underflow
    ERR_AUDIO_ENC_OUTFRAME_UNDERFLOW = 2,  //out frame buffer is underflow.
    ERR_AUDIO_ENC_
} __audio_enc_result_t;


enum AUDIO_ENCODER_TYPE
{
       AUDIO_ENCODER_AAC_TYPE,
       AUDIO_ENCODER_LPCM_TYPE,    //only used by mpeg2ts
       AUDIO_ENCODER_PCM_TYPE,
       AUDIO_ENCODER_MP3_TYPE,
       AUDIO_ENCODER_G711A_TYPE,
       AUDIO_ENCODER_G711U_TYPE,
       AUDIO_ENCODER_G726A_TYPE,
       AUDIO_ENCODER_G726U_TYPE,
       AUDIO_ENCODER_OPUS_TYPE,
};

typedef enum AUDIO_ENCODER_TYPE AUDIO_ENCODER_TYPE;

typedef struct AudioEncConfig
{
    AUDIO_ENCODER_TYPE nType;
    int     nInSamplerate;   //输入fs
    int     nInChan;         //输入pcm chan 1:mon 2:stereo
    int     nBitrate;        //bs
    int     nSamplerBits;    //only for 16bits
    int     nOutSamplerate;  //输出fs,now OutSamplerate must equal InSamplerate
    int     nOutChan;        //编码输出 chan
    int     nFrameStyle;    //for aac: 0:add head,1:raw data; for pcm: 2:mpegTs pcm(big endian), other: common pcm(little endian)

    int     mInBufSize; //input abs buffer size.
    int     mOutBufCnt;

    //int g726_enc_law;   // added for g726 encoding, 1:a-law;0:u-law
}AudioEncConfig;

typedef void* AudioEncoder;

AudioEncoder* CreateAudioEncoder();
void DestroyAudioEncoder(AudioEncoder* pEncoder);
int InitializeAudioEncoder(AudioEncoder *pEncoder, AudioEncConfig *pConfig);
int ResetAudioEncoder(AudioEncoder* pEncoder);
int EncodeAudioStream(AudioEncoder *pEncoder);
int WriteAudioStreamBuffer(AudioEncoder *pEncoder, char* pBuf, int len);
int RequestAudioFrameBuffer(AudioEncoder *pEncoder, char **pOutBuf, unsigned int *size, long long *pts, int *bufId);
int ReturnAudioFrameBuffer(AudioEncoder *pEncoder, char *pOutBuf, unsigned int size, long long pts, int bufId);
int AudioEncoder_GetValidPcmDataSize(AudioEncoder *pEncoder);
int AudioEncoder_GetTotalPcmBufSize(AudioEncoder *pEncoder);
int AudioEncoder_GetEmptyFrameNum(AudioEncoder *pEncoder);
int AudioEncoder_GetTotalFrameNum(AudioEncoder *pEncoder);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_ENC_API_H
