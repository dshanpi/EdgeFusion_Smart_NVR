
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __OPUS_ENC_LIB_API__
#define __OPUS_ENC_LIB_API__

#include <aenc_sw_lib.h>

extern struct __AudioENC_AC320 *AudioOPUSENCEncInit();
extern int AudioOPUSENCEncExit(struct __AudioENC_AC320 *p);

#endif // __OPUS_ENC_LIB_API__

#ifdef __cplusplus
}
#endif
