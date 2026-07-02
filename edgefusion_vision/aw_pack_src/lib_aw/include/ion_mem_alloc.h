/* Copyright (c) 2019-2035 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.


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
#ifndef MEM_INTERFACE_H
#define MEM_INTERFACE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    IonHeapType_CARVEOUT = 0,   //carveout or cma
    //IonHeapType_CMA,
    IonHeapType_IOMMU = 3,
    IonHeapType_POOL = 4,
    IonHeapType_UNKNOWN = -1,
}IonHeapType;
typedef struct IonAllocAttr
{
    unsigned int nLen;
    IonHeapType eIonHeapType;
    bool bSupportCache; //if ion memory attach cache attribute. 1:support; 0:disable
}IonAllocAttr;

struct SunxiMemOpsS
{
    int (*open)(void);
    void (*close)(void);
    int (*total_size)(void);
    void *(*palloc)(int /*size*/);
    void *(*pallocExtend)(IonAllocAttr *);
    void  (*pfree)(void* /*mem*/);
    void (*flush_cache)(void * /*mem*/, int /*size*/);
    void *(*cpu_get_phyaddr)(void * /*viraddr*/);
    void *(*cpu_get_viraddr)(void * /*phyaddr*/);
    int (*mem_set)(void * /*s*/, int /*c*/, size_t /*n*/);
    int (*mem_cpy)(void * /*dest*/, void * /*src*/, size_t /*n*/);
    int (*mem_read)(void * /*dest */, void * /*src*/, size_t /*n*/);
    int (*mem_write)(void * /*dest*/, void * /*src*/, size_t /*n*/);
    int (*setup)(void);
    int (*shutdown)(void);
    void *(*palloc_secure)(int /*size*/);
    int (*get_bufferFd)(void * /*viraddr*/);
    void *(*get_viraddr_byFd)(int /*nShareFd*/);
    int (*set_iommu_dev_path)(char *);
    void  (*pclose_fd_byViraddr)(void* /*mem*/);
    void *(*merge)(int /*num*/, void *[] /*viraddr[]*/);
    int (*unmerge)(void * /*phyaddr*/);
//    int mem_actual_width;
//    int mem_actual_height;
};

static inline int SunxiMemOpen(struct SunxiMemOpsS *memops)
{
    return memops->open();
}

static inline void SunxiMemClose(struct SunxiMemOpsS *memops)
{
    memops->close();
}

static inline int SunxiMemTotalSize(struct SunxiMemOpsS *memops)
{
    return memops->total_size();
}

//static inline void SunxiMemSetActualSize(struct SunxiMemOpsS *memops,int width,int height)
//{
//    memops->mem_actual_width = width;
//    memops->mem_actual_height = height;
//}

//static inline void SunxiMemGetActualSize(struct SunxiMemOpsS *memops,int *width,int *height)
//{
//    *width = memops->mem_actual_width;
//    *height = memops->mem_actual_height;
//}

static inline void *SunxiMemPalloc(struct SunxiMemOpsS *memops, int nSize)
{
    return memops->palloc(nSize);
}
static inline void *SunxiMemPallocExtend(struct SunxiMemOpsS *memops, IonAllocAttr *pAttr)
{
    return memops->pallocExtend(pAttr);
}

static inline void SunxiMemPfree(struct SunxiMemOpsS *memops, void *pMem)
{
    memops->pfree(pMem);
}

static inline void SunxiMemFlushCache(struct SunxiMemOpsS *memops, void *pMem, int nSize)
{
    memops->flush_cache(pMem, nSize);
}

static inline void SunxiMemSet(struct SunxiMemOpsS *memops, void *pMem, int nValue, int nSize)
{
   memops->mem_set(pMem, nValue, nSize);
}

static inline void SunxiMemCopy(struct SunxiMemOpsS *memops, void *pMemDst, void *pMemSrc, int nSize)
{
    memops->mem_cpy(pMemDst, pMemSrc, nSize);
}

static inline int SunxiMemRead(struct SunxiMemOpsS *memops, void *pMemDst, void *pMemSrc, int nSize)
{
    memops->mem_read(pMemDst, pMemSrc, nSize);
    return 0;
}

static inline int SunxiMemWrite(struct SunxiMemOpsS *memops, void *pMemDst, void *pMemSrc, int nSize)
{
    (void)memops; /*not use memops */
    memops->mem_write(pMemDst, pMemSrc, nSize);
    return 0;
}

static inline void *SunxiMemGetPhysicAddressCpu(struct SunxiMemOpsS *memops, void *virt)
{
    return memops->cpu_get_phyaddr(virt);
}

static inline void *SunxiMemGetVirtualAddressCpu(struct SunxiMemOpsS *memops, void *phy)
{
    return memops->cpu_get_viraddr(phy);
}

static inline int SunxiMemSetup(struct SunxiMemOpsS *memops)
{
    return memops->setup();
}

static inline int SunxiMemShutdown(struct SunxiMemOpsS *memops)
{
    return memops->shutdown();
}

static inline void *SunxiMemPallocSecure(struct SunxiMemOpsS *memops, int nSize)
{
    return memops->palloc_secure(nSize);
}

static inline int SunxiMemGetBufferFd(struct SunxiMemOpsS *memops,
        void *virt)
{
    return memops->get_bufferFd(virt);
}

static inline void *SunxiMemGetViraddrByFd(struct SunxiMemOpsS *memops,
    int share_fd)
{
    return memops->get_viraddr_byFd(share_fd);
}

static inline int SunxiMemSetIommuDevPath(struct SunxiMemOpsS *memops, char *iommu_dev_path)
{
    return memops->set_iommu_dev_path(iommu_dev_path);
}

static inline void SunxiMemPcloseFdByViraddr(struct SunxiMemOpsS *memops,
        void *pMem)
{
    memops->pclose_fd_byViraddr(pMem);
}

static inline void* SunxiMemMerge(struct SunxiMemOpsS *memops, int num, void *virt[])
{
    return memops->merge(num, virt);
}

static inline int SunxiMemUnmerge(struct SunxiMemOpsS *memops, void *phy)
{
    return memops->unmerge(phy);
}

struct SunxiMemOpsS* GetMemAdapterOpsS(void);

#ifdef __cplusplus
}
#endif

#endif
