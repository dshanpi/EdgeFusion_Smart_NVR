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
#ifndef __FS_WRITER_H__
#define __FS_WRITER_H__

#include <errno.h>
#include <cedarx_stream.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum tag_FSWRITEMODE {
    FSWRITEMODE_CACHETHREAD = 0,
    FSWRITEMODE_SIMPLECACHE,
    FSWRITEMODE_DIRECT,
}FSWRITEMODE;

typedef struct FsCacheMemInfo
{
    char              *mpCache;
    unsigned int             mCacheSize;
}FsCacheMemInfo;

typedef struct tag_FsWriter FsWriter;
typedef struct tag_FsWriter
{
    FSWRITEMODE mMode;
    ssize_t (*fsWrite)(FsWriter *thiz, const char *buf, size_t size);
    int (*fsSeek)(FsWriter *thiz, int64_t nOffset, int fromWhere);
    int64_t (*fsTell)(FsWriter *thiz);
    int (*fsTruncate)(FsWriter *thiz, int64_t nLength);
    int (*fsFlush)(FsWriter *thiz);
    void *mPriv;
}FsWriter;
FsWriter* createFsWriter(FSWRITEMODE mode, struct cdx_stream_info *pStream, char *pCache, unsigned int nCacheSize, unsigned int vCodec);
int destroyFsWriter(FsWriter *thiz);

extern ssize_t fileWriter(struct cdx_stream_info *pStream, const char *buffer, size_t size);

#if defined(__cplusplus)
}
#endif

#endif
