#ifndef _VIDEO_SERVICE_H
#define _VIDEO_SERVICE_H

typedef void (*EncCallbackFunc)(void *data,int size);

int rkVideoInit(void);
int rkVideoDisplayLocal(void);
int rkVideoDisplayPeer(int w,int h,void* encCallback);
int rkVideoDisplayOff(void);
int rkVideoFaceOnOff(int type);
int rkVideoStop(void);
int rkH264EncOn(int w,int h,EncCallbackFunc encCallback);
int rkH264EncOff(void);

#endif

