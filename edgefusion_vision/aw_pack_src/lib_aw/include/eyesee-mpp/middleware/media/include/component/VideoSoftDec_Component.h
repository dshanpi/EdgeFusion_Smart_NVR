#ifndef __IPCLINUX_SOFT_VIDEODEC_COMPONENT_H__
#define __IPCLINUX_SOFT_VIDEODEC_COMPONENT_H__

#include <mm_comm_vdecsoft.h>

#include "ComponentCommon.h"
#include "VdecSoftStream.h"
#include "media_common.h"
#include "mm_comm_video.h"
#include "mm_common.h"
#include "mm_component.h"
#include "plat_defines.h"
#include "plat_errno.h"
#include "plat_math.h"
#include "plat_type.h"
#include "tmessage.h"
#include "tsemaphore.h"

typedef enum VIDEOSOFTDECODE_PORT_INDEX_DEFINITION {
    VDECSOFT_PORT_INDEX_DEMUX = 0x0,
    VDECSOFT_OUT_PORT_INDEX_VRENDER = 0x1,
    VDECSOFT_PORT_INDEX_CLOCK = 0x2,
} VIDEOSOFTDECODE_PORT_INDEX_DEFINITION;

ERRORTYPE VideoSoftDecSendCommand(COMP_HANDLETYPE hComponent, COMP_COMMANDTYPE Cmd,
                                  unsigned int nParam1, void* pCmdData);
ERRORTYPE VideoSoftDecGetState(COMP_HANDLETYPE hComponent, COMP_STATETYPE* pState);
ERRORTYPE VideoSoftDecSetCallbacks(COMP_HANDLETYPE hComponent, COMP_CALLBACKTYPE* pCallbacks,
                                   void* pAppData);
ERRORTYPE VideoSoftDecGetConfig(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_IN COMP_INDEXTYPE nIndex,
                                PARAM_INOUT void* pComponentConfigStructure);
ERRORTYPE VideoSoftDecSetConfig(PARAM_IN COMP_HANDLETYPE hComponent, PARAM_IN COMP_INDEXTYPE nIndex,
                                PARAM_IN void* pComponentConfigStructure);
ERRORTYPE VideoSoftDecComponentTunnelRequest(PARAM_IN COMP_HANDLETYPE hComponent,
                                             PARAM_IN unsigned int nPort,
                                             PARAM_IN COMP_HANDLETYPE hTunneledComp,
                                             PARAM_IN unsigned int nTunneledPort,
                                             PARAM_INOUT COMP_TUNNELSETUPTYPE* pTunnelSetup);

ERRORTYPE VideoSoftDecEmptyThisBuffer(PARAM_IN COMP_HANDLETYPE hComponent,
                                      PARAM_IN COMP_BUFFERHEADERTYPE* pBuffer);
ERRORTYPE VideoSoftDecFillThisBuffer(PARAM_IN COMP_HANDLETYPE hComponent,
                                     PARAM_IN COMP_BUFFERHEADERTYPE* pBuffer);
ERRORTYPE VideoSoftDecComponentDeInit(COMP_HANDLETYPE hComponent);
ERRORTYPE VideoSoftDecComponentInit(PARAM_IN COMP_HANDLETYPE hComponent);

#endif /* __IPCLINUX_SOFT_VIDEODEC_COMPONENT_H__ */
