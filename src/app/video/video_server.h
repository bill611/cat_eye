#ifndef _VIDEO_SERVICE_H
#define _VIDEO_SERVICE_H

typedef void (*EncCallbackFunc)(void *data,int size,int frame_type);
typedef void (*RecordStopCallbackFunc)(void);

int rkVideoInit(void);
int rkVideoDisplayLocal(void);
int rkVideoDisplayPeer(int w,int h,void* encCallback);
int rkVideoDisplayOff(void);
int rkVideoFaceOnOff(int type);
int rkVideoStop(void);
int rkH264EncOn(int w,int h,EncCallbackFunc encCallback);
int rkH264EncOff(void);
int rkVideoCapture(char *file_name);
int rkVideoRecordStart(int w,int h,EncCallbackFunc encCallback);
int rkVideoRecordSetStopFunc(RecordStopCallbackFunc recordCallback);
int rkVideoRecordStop(void);
int rkGetVideoRun(void);

#endif

