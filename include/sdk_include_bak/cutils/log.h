#ifdef HAVE_ANDROID_LOG
#include <log/log.h>
#else
#ifndef ALOGV
#define ALOGV printf
#endif
#ifndef ALOGI
#define ALOGI printf
#endif
#ifndef ALOGE
#define ALOGE printf
#endif
#endif
