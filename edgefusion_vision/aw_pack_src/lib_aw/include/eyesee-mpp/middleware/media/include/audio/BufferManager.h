/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : BufferManager.h
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2016/06/28
  Last Modified :
  Description   : mpi functions implement
  Function List :
  History       :
******************************************************************************/
#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <mm_comm_aio.h>

typedef struct PCMDataNode
{
    char *pData;
    int nDataLen; //unit:bytes
    char *pDataExtra;
    int nDataExtraLen;
    int64_t nPts; //unit:us
    struct list_head mList;
}PCMDataNode;

typedef struct PCMBufferManager
{
    char *mpStart;
    char *mpRead;
    char *mpWrite;
    char *mpPrefetch;
    int mTotalSize;
    int mDataSize; //all valid data size
    int mPrefetchSize;   //size of data which is got.
    int mFreeSize;
    pthread_mutex_t mLock;

    int mAlsaFrameBytes;
    int mSampleRate;
    int mFrameBytes;

    /**
      PCMDataNode is used to store input frame pts and store_position in buffer, help to caculate pts of pcm data in
      arbitrary position in buffer.
      output frame is independent of input frame.
    */
    struct list_head mIdlePCMDataList; //PCMDataNode
    struct list_head mValidPCMDataList;
    int mNodeNum;

    /**
      @return
        0: success
        -1: fail
    */
    int (*writeData)(struct PCMBufferManager *pMgr, char *pInBuf, int inSize, int64_t nPts, bool bMute);
    /**
      @return
        0: success
        -1: fail
    */
    int (*getData)(struct PCMBufferManager *pMgr, int reqSize, char **ppOutBuf, int *pSize, char **ppOutBufExtra, int *pSizeExtra, int64_t *pPts);
    /**
      @return
        0: success
        -1: fail
    */
    int (*releaseData)(struct PCMBufferManager *pMgr, char *pOutBuf, int relSize);
    /**
      @return
        free size, unit: bytes
    */
    int (*getFreeSize)(struct PCMBufferManager *pMgr);
    /**
      @return
        prefetch size, unit: bytes
    */
    int (*getPrefetchSize)(struct PCMBufferManager *pMgr);
} PCMBufferManager;

PCMBufferManager *PCMBufferMgr_Create(int nFrmNum, int nFrameBytes, int nAlsaFrameBytes, int nSampleRate);
void PCMBufferMgr_Destroy(PCMBufferManager *pMgr);

#endif /* __BUFFER_MANAGER_H__ */
