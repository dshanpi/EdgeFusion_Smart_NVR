#ifndef _VDECSOFT_STREAM_H_
#define _VDECSOFT_STREAM_H_

#include <mm_comm_vdecsoft.h>

#ifdef __cplusplus
extern "C" {
#endif /* End of #ifdef __cplusplus */

typedef struct VdecSoftInputStream {
    VDECSOFT_STREAM_S *pStream;
    int nMilliSec;
} VdecSoftInputStream;

typedef struct VdecSoftOutFrame {
    VDECSOFT_FRAME_INFO_S *pFrameInfo;
    int nMilliSec;
} VdecSoftOutFrame;

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif /* _VDECSOFT_STREAM_H_ */
