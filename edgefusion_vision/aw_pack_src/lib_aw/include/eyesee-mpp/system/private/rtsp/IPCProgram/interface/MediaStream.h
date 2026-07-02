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
 * MediaStream.h
 *
 *  Created on: 2016年8月31日
 *      Author: liu
 */

#ifndef IPCPROGRAM_INTERFACE_MEDIASTREAM_H_
#define IPCPROGRAM_INTERFACE_MEDIASTREAM_H_

#include <stdint.h>
#include <time.h>
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <sys/time.h>
#include <condition_variable>

//#define SAVE_FILE
#define FILE_SUFFIX ".h264"

class TinyServer;
class Groupsock;
class H264or5VideoNaluFramer;
class UnicastVideoMediaSubsession;
class RTPSink;
class TinySource;
class RTCPInstance;

namespace Byl {
class FrameNaluParser;
}

class MediaStream {
  public:
    struct MediaStreamAttr {
        enum StreamType { STREAM_TYPE_UNICAST, STREAM_TYPE_MULTICAST, STREAM_TYPE_INVALID };

        enum VideoType { VIDEO_TYPE_H265, VIDEO_TYPE_H264, VIDEO_TYPE_INVALID };

        enum AudioType {
            AUDIO_TYPE_AAC, AUDIO_TYPE_INVALID
        };

        VideoType videoType;
        AudioType audioType;
        StreamType streamType;
    };

    enum FrameDataType {
        FRAME_DATA_TYPE_I,
        FRAME_DATA_TYPE_B,
        FRAME_DATA_TYPE_P,
        FRAME_DATA_TYPE_HEADER,
        FRAME_DATA_TYPE_UNKNOW
    };

    MediaStream(std::string const &name, TinyServer &server, MediaStreamAttr attr);
    ~MediaStream();

    std::string streamURL();

    void appendVideoData(unsigned char *data, unsigned int size);
    void appendVideoData(unsigned char *data, unsigned int size, FrameDataType dataType);
    void appendVideoData(unsigned char *data, unsigned int size, uint64_t pts, FrameDataType dataType);
    void appendAudioData(unsigned char *data, unsigned int size, uint64_t pts);

    void setVideoFrameRate(int frameRate);
    int getVideoFrameRate();

    typedef void(OnNewClientConnectFunc)(void *context);
    void setNewClientCallback(OnNewClientConnectFunc *function, void *context);

  private:
    TinySource *createVideoSource();
    TinySource *createAudioSource();
    RTPSink *createVideoSink();
    RTPSink *createAudioSink();
    void unicastDestructor();
    void multicastDestructor();

    static void unicastConstructor(void *context);
    static void multicastConstructor(void *context);
    static void disconnectAllClient(void *context);
    static void afterPlaying(MediaStream *stream);
    void play();
    void saveHeaderInfo(uint8_t *data, uint32_t size, uint64_t pts);
    void sendDataToQueue(uint8_t *data, uint32_t size, uint64_t pts);
    void checkDurationTime();

    TinyServer &_server;
    std::string _streamName;
    const MediaStreamAttr _mediaStreamAttr;
    struct timeval _lastGetFrameTime;
    int _frameRate;

    Groupsock *_rtpGroupsock;
    Groupsock *_rtcpGroupsock;

    TinySource *_videoSource;
    TinySource *_audioSource;
    UnicastVideoMediaSubsession *_videoSubsession;
    H264or5VideoNaluFramer *_naluFramer;
    RTPSink *_videoSink;
    RTCPInstance *_rtcp;
    Byl::FrameNaluParser *_nal_parser;

    uint32_t _disconn_token;
    uint32_t _construct_token;
    std::mutex _task_mutex;
    std::condition_variable _task_complete;

    std::vector<uint8_t> _headerInfo;

    OnNewClientConnectFunc *_newRecvCallback;

#ifdef SAVE_FILE
    std::ofstream *_of;
#endif
};
#endif /* IPCPROGRAM_INTERFACE_MEDIASTREAM_H_ */
