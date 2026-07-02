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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include <plat_log.h>

#include "record.h"
#include "sdcard_manager.h"

//#define INFO_DEBUG

#ifdef INFO_DEBUG
#define DB_PRT(fmt, args...)                                          \
    do {                                                                  \
        printf("[FUN]%s [LINE]%d  " fmt, __FUNCTION__, __LINE__, ##args); \
    } while (0)
#else
#define DB_PRT(fmt, args...)                                          \
    do {} while (0)
#endif

#define ERR_PRT(fmt, args...)                                                                        \
    do {                                                                                                 \
        printf("\033[0;32;31m ERROR! [FUN]%s [LINE]%d  " fmt "\033[0m", __FUNCTION__, __LINE__, ##args); \
    } while (0)

typedef struct RECORD_CONFIG_PRIV_S
{
	int mValid;
	int mVeChn;
	int mRecordHandler;
	int mMaxFileCnt;
	int mCurFileCnt;
	int mFileStart;
	uint64_t mMaxFileDuration;  //unit: us
	FILE *mFp;
	char mFilePath[MAX_RECORD_FILE_PATH_LEN];
	char mCurFilePath[MAX_RECORD_FILE_PATH_LEN];

	uint64_t mBasePts;
	uint64_t mPrePts;
	uint64_t mCurPts;

	void (*requestKeyFrame)(int nVeChn, int *result);
	int requestKeyFrameDoneFlag;
}RECORD_CONFIG_PRIV_S;

typedef struct RECORD_CONTEXT_S
{
	RECORD_CONFIG_PRIV_S mRecordConfig[MAX_RECORD_NUM];
}RECORD_CONTEXT_S;


static RECORD_CONTEXT_S gRecordContext;

struct sd_insert_check_s
{
	int sd_init;
	pthread_t thId;
	int thread_status;
	/*callback functions , run it when reboot*/
	void (*callback_func)();
} sd_insert_check;

static void *sdcard_insert_mount_check(void *param)
{
	struct sd_insert_check_s *sd = param;
	int sd_last_status = 0;
	int sd_cur_status = 0;
	int ret;

	sd_last_status = check_sdcard_insert();

	while(sd->thread_status) {
		sd_cur_status = check_sdcard_insert();
		DB_PRT("sd_cur_status :%d \n", sd_cur_status);

		if (sd_cur_status) {
			/*checking sd was be mount, and umount it*/
			ret = storage_manager_check_mount(SDCARD_MOUNT_PATH, SDCARD_DEV);
			if(ret == 1) //was mount
			{
				DB_PRT("sd was mount \n");
				sd->sd_init = 1;
			}
			else if (ret == 0) //not mount
			{
				DB_PRT("sd %s was not mount \n", SDCARD_DEV);
			    /*check sd file system is VFAT ?*/
				ret = storage_manger_check_and_fix_filesystem_support(0); //input 1 will fix vfat erro when vfat has some wrong.
				if (ret == 0)
				{
					/* mount sd */
					DB_PRT("now mount sd card %s\n", SDCARD_DEV);
					ret = storage_manager_mount(SDCARD_MOUNT_PATH, SDCARD_DEV);
					if(ret < 0) {
						printf("mount sd card erro \n");
						return (void*)-4;
					}
					sd->sd_init = 1;
				}
				else
				{
				    DB_PRT("mount sd card %s fail\n", SDCARD_DEV);
                    ret = storage_manager_check_mount(SDCARD_MOUNT_PATH, SDCARD_DEV_2);
        			if(ret == 1) //was mount
        			{
        				DB_PRT("sd %s was mount \n", SDCARD_DEV);
        				sd->sd_init = 1;
        			}
        			else
        			{
        			    DB_PRT("sd %s was not mount \n", SDCARD_DEV_2);
        			    /*check sd file system is VFAT ?*/
        				ret = storage_manger_check_and_fix_filesystem_support(0); //input 1 will fix vfat erro when vfat has some wrong.
        				if (ret == 0)
        				{
        					/* mount sd */
        					DB_PRT("now mount sd card %s\n", SDCARD_DEV_2);
        					ret = storage_manager_mount(SDCARD_MOUNT_PATH, SDCARD_DEV_2);
        					if(ret < 0) {
        						printf("mount sd card erro \n");
        						return (void*)-4;
        					}
        					sd->sd_init = 1;
        				}
        				else
        				{
                            ERR_PRT("mount sd card %s fail\n", SDCARD_DEV_2);
        				}
        			}
				}
			}
			else //erro
			{
				printf("check sd card erro \n");
			}
		} else {
			ret = storage_manager_check_mount(SDCARD_MOUNT_PATH, NULL);
			if(ret == 1) //was mount, need umount it
			{
				DB_PRT("Now mount sd card");
				storage_manager_umount(SDCARD_MOUNT_PATH);
			}
			sd->sd_init = 0;
		}

		usleep(5*1000*1000);
	}
    return (void*)0;
}

static int getFileName(char *p, char *pFileName)
{
	int cnt = 0;
	char *pTmp = NULL;

	if (NULL == p || NULL == pFileName)
	{
		aloge("file name is null!");
		return -1;
	}

	pTmp = pFileName;
	while (1)
	{
		if ('.' == *pTmp || '\0' == *pTmp)
			break;
		pTmp++;
		cnt++;
	}
	p[cnt+1] = '\0';
	snprintf(p, cnt+1, "%s", pFileName);

	return 0;
}

static int openFile(RECORD_CONFIG_PRIV_S *pConfig)
{
	pConfig->mFp = fopen(pConfig->mCurFilePath, "wb");
	if (NULL == pConfig->mFp)
	{
		aloge("fatal error! record[%d] open file[%s] fail! errno is %d", \
		      pConfig->mRecordHandler, pConfig->mCurFilePath, errno);
		return -1;
	}
	alogd("record[%d] open file[%s] success!", \
		pConfig->mRecordHandler, pConfig->mCurFilePath);
	return 0;
}

static int closeFile(RECORD_CONFIG_PRIV_S *pConfig)
{
	if (NULL == pConfig->mFp)
	{
		aloge("fatal error! record[%d] close file[%s] fail!", \
		      pConfig->mRecordHandler, pConfig->mCurFilePath);
		return -1;
	}
	alogd("record[%d] close file[%s] success!", pConfig->mRecordHandler, \
	      pConfig->mCurFilePath);
	fclose(pConfig->mFp);
	pConfig->mFp = NULL;

	return 0;
}

static int wirteFile(RECORD_CONFIG_PRIV_S *pConfig, RECORD_FRAME_S *pFrm)
{
	int writeLen = 0;
	if (pFrm->mpFrm && pConfig->mFp)
	{
		writeLen = fwrite(pFrm->mpFrm, 1, pFrm->mFrmSize, pConfig->mFp);
		if (pFrm->mFrmSize != writeLen)
			alogw("write frame fail! frame len[%d] wirte len[%d]", \
			      pFrm->mFrmSize, writeLen);
	}
	else
		aloge("fatal error! frame is null!");

	return 0;
}

int record_init(void)
{
	int ret;
	RECORD_CONFIG_PRIV_S *pConfig = NULL;

	memset(&gRecordContext, 0, sizeof(RECORD_CONTEXT_S));
	for (int i = 0; i < MAX_RECORD_NUM; i++)
	{
		pConfig = &gRecordContext.mRecordConfig[i];
		pConfig->mValid = 0;
		pConfig->mRecordHandler = -1;
		pConfig->mMaxFileCnt = 0;
		pConfig->mMaxFileDuration = 0;
		pConfig->mFileStart = 0;
		pConfig->mBasePts = 0;
		pConfig->mPrePts = 0;
		pConfig->mCurPts = 0;
		pConfig->requestKeyFrame = NULL;
		pConfig->requestKeyFrameDoneFlag = 0;
		memset(&pConfig->mFilePath, 0, sizeof(pConfig->mFilePath));
		memset(&pConfig->mCurFilePath, 0, sizeof(pConfig->mCurFilePath));
	}

	/*init storage_manager*/
	storage_manager_init();

	sd_insert_check.thread_status= 1;

	ret = pthread_create(&sd_insert_check.thId , NULL, sdcard_insert_mount_check, &sd_insert_check);
	if (0 != ret) {
		ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
	}

	return 0;
}

int record_exit(void)
{
	RECORD_CONFIG_PRIV_S *pConfig = NULL;

	/*stop thread and wait it stop*/
	sd_insert_check.thread_status = 0 ;
	pthread_join(sd_insert_check.thId, NULL);
	sd_insert_check.sd_init = 0;

	/* exit storage_manager */
	storage_manager_exit();

	for (int i = 0; i < MAX_RECORD_NUM; i++)
	{
		pConfig = &gRecordContext.mRecordConfig[i];
		if (pConfig->mFp)
			closeFile(pConfig);
		memset(&pConfig->mFilePath, 0, sizeof(pConfig->mFilePath));
		pConfig->mValid = 0;
		pConfig->mRecordHandler = -1;
		pConfig->mMaxFileCnt = 0;
		pConfig->mMaxFileDuration = 0;
		pConfig->mBasePts = 0;
		pConfig->mPrePts = 0;
		pConfig->mCurPts = 0;
	}

	return 0;
}


int record_set_config(RECORD_CONFIG_S *pSetConfig)
{
	int nSetSuceessFlag = 0;
	RECORD_CONFIG_PRIV_S *pConfig = NULL;

	for (int i = 0; i < MAX_RECORD_NUM; i++)
	{
		pConfig = &gRecordContext.mRecordConfig[i];
		if (!pConfig->mValid)
		{
			pConfig->mValid = 1;
			pConfig->mVeChn = pSetConfig->mVeChn;
			pConfig->mRecordHandler = pSetConfig->mRecordHandler;
			pConfig->mMaxFileCnt = pSetConfig->mMaxFileCnt;
			pConfig->mMaxFileDuration = pSetConfig->mMaxFileDuration;
			sprintf(pConfig->mFilePath, "%s", pSetConfig->mFilePath);
			pConfig->requestKeyFrame = pSetConfig->requestKeyFrame;
			alogd("record[%d] requestKeyFrame[%p]", i, pConfig->requestKeyFrame);
			nSetSuceessFlag = 1;
			alogd("record[%d] valid[%d] VeChn[%d] handler[%d] max file cnt[%d], max file duration[%lld] file path[%s]\n", \
			      i, pConfig->mValid, pConfig->mVeChn, pConfig->mRecordHandler, \
			      pConfig->mMaxFileCnt, pConfig->mMaxFileDuration, \
			      pConfig->mFilePath);
			break;
		}
		else
		{
			alogd("record[%d] arleady used, find next!", i);
			continue;
		}
	}

	if (!nSetSuceessFlag)
	{
		aloge("fatal error! record set config fail!");
		return -1;
	}

	return 0;
}

int record_send_data(int nRecordHandler, RECORD_FRAME_S *pFrm)
{
	int ret = 0;
	int nFindFlag = 0;
	uint64_t nFileDuration = 0;
	RECORD_CONFIG_PRIV_S *pConfig = NULL;

	for (int i = 0; i < MAX_RECORD_NUM; i++)
	{
		pConfig = &gRecordContext.mRecordConfig[i];
		if (!pConfig->mValid)
			continue;
		if (nRecordHandler == pConfig->mRecordHandler)
		{
			nFindFlag = 1;
			break;
		}
	}

	if (!nFindFlag)
	{
		aloge("fatal error! not find record handler[%d]", nRecordHandler);
		return -1;
	}

	// check sd card not ready, delete it for save /tmp when no sd card.
	/*if(sd_insert_check.sd_init == 0)
	{
		//card was be exit, close file fp
		if(pConfig->mFp != NULL) {
			closeFile(pConfig);
			pConfig->mCurFileCnt+=1;
			pConfig->mBasePts = 0;
		}
			return -1;
	}*/

	if (0 != pConfig->mBasePts)
		nFileDuration = pFrm->mPts - pConfig->mBasePts;
	if ((nFileDuration >= pConfig->mMaxFileDuration) || (1 == pFrm->mChangeFd))
	{
		pConfig->mBasePts = pFrm->mPts;
		pConfig->mPrePts = pFrm->mPts;
		pConfig->mCurPts = pFrm->mPts;
		alogd("record[%d] file[%s] wirte complete duration[%lld] MaxFileDuration[%lld], so close it", \
		      pConfig->mRecordHandler, pConfig->mFilePath, nFileDuration, pConfig->mMaxFileDuration);
		ret = closeFile(pConfig);
		if (ret < 0)
			return -1;
		pConfig->mCurFileCnt+=1;
		if (pConfig->mCurFileCnt > pConfig->mMaxFileCnt)
		{
			alogd("record[%d] max file cnt[%d] current file cnt[%d], set current file cnt to 0", \
			      pConfig->mRecordHandler, pConfig->mMaxFileCnt, pConfig->mCurFileCnt);
			pConfig->mCurFileCnt = 0;
		}
	}

	if (NULL == pConfig->mFp)
	{
		pConfig->mBasePts = pFrm->mPts;
		pConfig->mPrePts = pFrm->mPts;
		pConfig->mCurPts = pFrm->mPts;
		char tmpStr[MAX_RECORD_FILE_PATH_LEN] = {0};
		ret = getFileName(tmpStr, pConfig->mFilePath);
		//alogd("get file name[%s]", tmpStr);
		sprintf(pConfig->mCurFilePath, "%s_%d.raw", tmpStr, pConfig->mCurFileCnt);
		alogd("record[%d] output file path[%s]", pConfig->mRecordHandler, pConfig->mCurFilePath);
		ret = openFile(pConfig);
		if (ret < 0)
		{
			return -1;
		}
		pConfig->mFileStart = 1;
		alogd("record[%d] first frame", pConfig->mRecordHandler);
	}

	if (pConfig->mFileStart)
	{
		//alogd("record[%d] FrameType=%d", pConfig->mRecordHandler, pFrm->mFrameType);
		if (RECORD_FRAME_TYPE_I != pFrm->mFrameType)
		{
			if (pConfig->requestKeyFrame)
			{
				if (pConfig->requestKeyFrameDoneFlag == 0)
				{
					int result = 0;
					pConfig->requestKeyFrame(pConfig->mVeChn, &result);
					if (result != 0)
					{
						pConfig->requestKeyFrameDoneFlag = 0;
						aloge("record[%d] why requestKeyFrame function return failed! ret=%d", pConfig->mRecordHandler, result);
					}
					else
					{
						pConfig->requestKeyFrameDoneFlag = 1;
						alogd("record[%d] requestKeyFrame done and success", pConfig->mRecordHandler);
					}
				}
			}
			else
			{
				pConfig->requestKeyFrameDoneFlag = 0;
				aloge("record[%d] why requestKeyFrame function is null!", pConfig->mRecordHandler);
			}
			alogd("record[%d] need key frame, skip this frame!", pConfig->mRecordHandler);
			return 0;
		}
		else
		{
			pConfig->requestKeyFrameDoneFlag = 0;
			alogd("record[%d] got I frame", pConfig->mRecordHandler);
		}
		pConfig->mFileStart = 0;
	}
	wirteFile(pConfig, pFrm);
	pConfig->mPrePts = pFrm->mPts;
	/*alogd("record[%d] write current pts[%lld] pre pts[%lld] base pts[%lld] duration[%lld]\n", \
	      pConfig->mRecordHandler, pFrm->mPts, pConfig->mPrePts, pConfig->mBasePts, pFrm->mPts-pConfig->mBasePts);*/
    return 0;
}

