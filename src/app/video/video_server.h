#ifndef _VIDEO_SERVICE_H
#define _VIDEO_SERVICE_H

typedef void (*EncCallbackFunc)(void *data,int size);

int rkVideoInit(void);
int rkVideoDisplayOnOff(int type);
int rkVideoFaceOnOff(int type);
int rkVideoStop(void);
int rkVideoStopCapture(void);
int rkVideoStartRecord(int w,int h,EncCallbackFunc encCallback);
int rkVideoStopRecord(void);

#endif

