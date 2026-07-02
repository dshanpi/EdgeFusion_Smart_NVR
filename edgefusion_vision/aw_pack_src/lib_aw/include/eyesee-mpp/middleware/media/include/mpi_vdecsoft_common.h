#ifndef __MPI_VDECSOFT_COMMON_H__
#define __MPI_VDECSOFT_COMMON_H__

#include "mm_common.h"
#include "mm_component.h"
#include "plat_type.h"

ERRORTYPE VDECSOFT_Construct(void);
ERRORTYPE VDECSOFT_Destruct(void);
MM_COMPONENTTYPE* VDECSOFT_GetChnComp(MPP_CHN_S* pMppChn);

#endif /* __MPI_VDECSOFT_COMMON_H__ */
