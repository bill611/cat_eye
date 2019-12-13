/*
 * =============================================================================
 *
 *       Filename:  media_muxer.h
 *
 *    Description:  音视频封装接口
 *
 *        Version:  1.0
 *        Created:  2019-09-12 10:18:22 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MEDIA_MUXER_H
#define _MEDIA_MUXER_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "avilib/avilib.h"

#define WRITE_READ	0
#define READ_ONLY	1

	typedef struct _CMPEG4Head {
		FILE *hFile;
		avi_t *avi_lib;
		char filename[64];				//文件名，含路径
		pthread_mutex_t mutex;
		uint32_t dwFrameCnt;			//视频流帧数
		uint32_t dwAudioFrame;			//音频流帧数
		uint32_t dwStreamSize;			//视频流大小
		uint64_t dwStartTick;
		uint64_t dwEndTick;
		int FirstFrame;		//是否是第一帧数据
		int ReadFirstFrame;	//是否已经读取第一帧
		int m_Width;
		int m_Height;
		int InitVideoFrame;
		int InitAudioFrame;
		uint8_t bHaveAudio;
		uint8_t bWriteAudio;
		uint64_t mBlockSize;
		uint32_t mChannels;
		uint64_t mSample;
		uint8_t m_ReadWrite; // 读写标志, =0写文件  =1读文件
		int  m_VideoType;  // 视频类型 =0H264 =1divx

		void (*InitAudio)(struct _CMPEG4Head *This, uint32_t Channels,uint64_t Sample,uint64_t dwBlockSize);		//通道数，位率，块大小
		uint8_t (*WriteVideo)(struct _CMPEG4Head *This, void *pData,uint32_t dwSize);
		uint8_t (*WriteAudio)(struct _CMPEG4Head *This, void *pData,uint32_t dwSize);
		int (*GetAviTotalTime)(struct _CMPEG4Head *This);
		int (*GetAviFileFrameRate)(struct _CMPEG4Head *This);
		int (*ReadAviData)(struct _CMPEG4Head *This, void *pData,uint64_t *dwSize,int *InCameraWidth,int *InCameraHeight);
		void (*DestoryMPEG4)(struct _CMPEG4Head **This);
	}MPEG4Head;

MPEG4Head* Mpeg4_Create(int Width,int Height,char *FileName, int ReadWrite);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
