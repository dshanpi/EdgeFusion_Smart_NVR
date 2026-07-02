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
#ifndef LOG_PRINT_H_
#define LOG_PRINT_H_

#define _GLOG_INFO  0
#define _GLOG_WARN  1
#define _GLOG_ERROR 2
#define _GLOG_FATAL 3

#ifdef __cplusplus
extern "C" {
#endif
typedef struct GLogConfig
{
    //"log messages go to stderr instead of logfiles"
    int FLAGS_logtostderr;  // = false;  //--logtostderr=1, GLOG_logtostderr=1
    //"color messages logged to stderr (if supported by terminal)"
    int FLAGS_colorlogtostderr; // = true;
    //"log messages at or above this level are copied to stderr in addition to logfiles.  This flag obsoletes --alsologtostderr."
    int FLAGS_stderrthreshold; // = google::GLOG_WARNING;
    //"Messages logged at a lower level than this don't actually get logged anywhere"
    int FLAGS_minloglevel; // = google::GLOG_INFO;
    //"Buffer log messages logged at this level or lower (-1 means don't buffer; 0 means buffer INFO only;...)"
    int FLAGS_logbuflevel; // = -1;
    //"Buffer log messages for at most this many seconds"
    int FLAGS_logbufsecs; // = 0;
    //"approx. maximum log file size (in MB). A value of 0 will be silently overridden to 1."
    int FLAGS_max_log_size; // = 25;
    //"Stop attempting to log to disk if the disk is full."
    int FLAGS_stop_logging_if_full_disk; // = true;

    //e.g., "/tmp/log/LOG-"
    char LogDir[128];   //e.g., "/tmp/log"
    char InfoLogFileNameBase[128];  //e.g., "LOG-"
    char LogFileNameExtension[128];    //e.g., "SDV-"
}GLogConfig;
void log_init(const char *program, GLogConfig *pConfig);
void log_quit();
int log_printf(const char *file, const char *func, int line, const int level, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
