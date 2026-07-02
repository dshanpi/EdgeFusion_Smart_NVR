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
 * TinyServer.h
 *
 *  Created on: 2016年6月29日
 *      Author: A
 */

#ifndef LIVE_IPCPROGRAM_TINYSERVER_H_
#define LIVE_IPCPROGRAM_TINYSERVER_H_

#include <pthread.h>
#include <string>
#include "MediaStream.h"

class TinySource;
class RTSPServer;
class Groupsock;
class H264VideoStreamDiscreteFramer;
class TaskScheduler;
class UsageEnvironment;
class RTPSink;
class RTCPInstance;
//forward

class TinyServer {
  public:
    static TinyServer* createServer();
    static TinyServer* createServer(const std::string& ip, int port);
    virtual ~TinyServer();

    MediaStream* createMediaStream(std::string const& name, MediaStream::MediaStreamAttr attr);
    RTSPServer* getRTSPServer();
    int runWithNewThread();
    void run();
    void stop();

  protected:
    TinyServer(const std::string ip, int port);

  private:
    void postConstructor();
    static void* loopThreadFunc(void* server);

    TaskScheduler* _scheduler;
    UsageEnvironment* _env;
    RTSPServer* _rtspServer;
    std::string _serverIp;
    int _port;

    char _runFlag;
    bool _loopRunInThread;
    pthread_t _loopThreadId;
};

#endif /* LIVE_IPCPROGRAM_TINYSERVER_H_ */
