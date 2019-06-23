#ifndef _VIDEO_SERVICE_H
#define _VIDEO_SERVICE_H

int rkVideoInit(void);
int rkVideoDisplayOnOff(int type);
int rkVideoFaceOnOff(int type);
int rkVideoStop(void);
int rkVideoStopCapture(void);
int rkVideoStartRecord(void);
int rkVideoStopRecord(void);

#endif

