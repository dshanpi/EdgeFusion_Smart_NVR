#ifndef __MOTION_DETECT_LOG_LEVEL_H__
#define __MOTION_DETECT_LOG_LEVEL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AWMD_LOG_LEVEL_CLOSE = 0,
    AWMD_LOG_LEVEL_ERROR = 1,
    AWMD_LOG_LEVEL_WARN = 2,
    AWMD_LOG_LEVEL_DEBUG = 3,
    AWMD_LOG_LEVEL_VERBOSE = 4,
}AWMD_LOG_LEVEL_TYPE;

extern AWMD_LOG_LEVEL_TYPE gMDLogLevel;

#ifdef __cplusplus
}
#endif

#endif  /* __MOTION_DETECT_LOG_LEVEL_H__ */

