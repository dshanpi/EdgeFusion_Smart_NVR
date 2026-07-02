#ifndef __AWISPAPI_H__
#define __AWISPAPI_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct {
	int (*ispApiInit)(void);
	int (*ispSetAfRegions)(int isp_id, int x1, int y1, int x2, int y2);
	int (*ispSetSceneMode)(int isp_id, int mode);
	int (*ispSetAeRegions)(int mIspId, int x1, int y1, int x2, int y2);
	int (*ispGetIspId)(int video_id);
	int (*ispStart)(int isp_id);
	int (*ispSetIspLargeImage)(int isp_id);
	int (*ispStop)(int isp_id);
	int (*ispWaitToExit)(int isp_id);
	int (*ispApiUnInit)(void);
	int (*ispSetFpsRanage)(int isp_id, int fps);
	int (*ispGetIspGain)(int isp_id);
	int (*ispGetIspExp)(int isp_id, unsigned int* num, unsigned int* den);
	int (*ispGetInfoLength)(int isp_id, int *i3a_length, int *debug_length);
	int (*ispGet3AParameters)(int isp_id, void *params);
	int (*ispGetDebugMessage)(int isp_id, void *msg);
} AWIspApi;

AWIspApi *CreateAWIspApi(void);
void DestroyAWIspApi(AWIspApi *hdl);

#ifdef  __cplusplus
}
#endif

#endif  /* __AWISPAPI_H__ */
