#ifndef _VIDEO_SERVICE_H
#define _VIDEO_SERVICE_H

typedef void (*EncCallbackFunc)(void *data,int size);

int rkVideoInit(void);
int rkVideoDisplayOnOff(int type);
int rkVideoFaceOnOff(int type);
int rkVideoStop(void);
int rkH264EncOn(int w,int h,EncCallbackFunc encCallback);
int rkH264EncOff(void);

#endif

