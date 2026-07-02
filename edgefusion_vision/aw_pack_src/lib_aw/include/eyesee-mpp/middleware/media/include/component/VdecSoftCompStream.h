
#ifndef _VDECSOFTCOMPSTREAM_H_
#define _VDECSOFTCOMPSTREAM_H_

#include <mm_comm_vdecsoft.h>
#include <plat_type.h>

#ifdef __cplusplus
extern "C" {
#endif /* End of #ifdef __cplusplus */

typedef struct VDecSoftCompOutputFrame {
    VDECSOFT_FRAME_INFO_S *pFrame;
    struct list_head mList;
} VDecSoftCompOutputFrame;

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif /* _VDECSOFTCOMPSTREAM_H_ */
