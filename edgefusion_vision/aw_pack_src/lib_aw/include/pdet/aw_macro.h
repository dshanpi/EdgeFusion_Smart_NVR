#ifndef _AW_MACRO_H
#define _AW_MACRO_H

//#define WIN64
#define LINUX
//#define ANDROID
//#define NEON

#ifdef WIN64
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS
#endif // WIN64

#ifdef ANDROID
#include <android/log.h>
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, "", fmt, ##args)
#endif // ANDROID

#ifdef NEON
#include <sys/time.h>
#include <arm_neon.h>
#endif //NEON

#define FIXEDPT

#endif // !_AW_MACRO_H