/*
 * =============================================================================
 *
 *       Filename:  media_muxer.c
 *
 *    Description:  音视频封装接口
 *
 *        Version:  1.0
 *        Created:  2019-09-12 10:15:23
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "avilib/avilib.h"
#include "mp4_muxer/mp4_muxer.h"
#include "media_muxer.h"
#include "externfunc.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define FOURCC uint32_t

#pragma pack (1)
typedef struct tagMYBITMAPINFOHEADER {
uint64_t biSize;
uint32_t  biWidth;
uint32_t  biHeight;
uint32_t  biPlanes;
uint32_t  biBitCount;
uint64_t biCompression;
uint64_t biSizeImage;
uint32_t  biXPelsPerMeter;
uint32_t  biYPelsPerMeter;
uint64_t biClrUsed;
uint64_t biClrImportant;
} MYBITMAPINFOHEADER;


typedef struct WAVEFORMATEX {
    uint32_t wFormatTag;
    uint32_t nChannels;
    uint64_t nSamplesPerSec;
    uint64_t nAvgBytesPerSec;
    uint32_t nBlockAlign;
    uint32_t wBitsPerSample;
    uint32_t cbSize;
} WAVEFORMATEX;
#pragma pack ()

typedef struct _listareastruct {
	FOURCC fcc;					// 标识
	uint64_t  cb;					// 大小，不包括最初的8个字节（fcc和cb两个域）
	FOURCC type;				// LIST类型
} LISTAREASTRUCT;

typedef struct _ChunkStruct
{
	FOURCC fcc;					// 标识
	uint64_t  cb;					// 块大小，不包括本结构
} CHUNKSTRUCT;

// prefix AMHF denoting Avi Main Header Flag
#define    AMHF_HASINDEX        0x00000010		//是否包含索引
#define    AMHF_MUSTUSEINDEX    0x00000020
#define    AMHF_ISINTERLEAVED    0x00000100		//是否包含保留区域
#define    AMHF_TRUSTCKTYPE    0x00000800 /* Use CKType to find key frames? */
#define    AMHF_WASCAPTUREFILE    0x00010000
#define    AMHF_COPYRIGHTED    0x00020000

typedef struct _avimainheader {
	FOURCC fcc;					// 必须为‘avih’
	uint64_t  cb;					// 本数据结构的大小，不包括最初的8个字节（fcc和cb两个域）
	uint64_t  dwMicroSecPerFrame;   // 视频帧间隔时间（以微秒为单位）
	uint64_t  dwMaxBytesPerSec;     // 这个AVI文件的最大数据率
	uint64_t  dwPaddingGranularity; // 数据填充的粒度
	uint64_t  dwFlags;         // AVI文件的全局标记，比如是否含有索引块等  (0x810   HASINDEX   |   TRUSTCKTYPE)
	uint64_t  dwTotalFrames;   // 总帧数
	uint64_t  dwInitialFrames; // 为交互格式指定初始帧数（非交互格式应该指定为0）
	uint64_t  dwStreams;       // 本文件包含的流的个数
	uint64_t  dwSuggestedBufferSize; // 建议读取本文件的缓存大小（应能容纳最大的块）
	uint64_t  dwWidth;         // 视频图像的宽（以像素为单位）
	uint64_t  dwHeight;        // 视频图像的高（以像素为单位）
	uint64_t  dwReserved[4];   // 保留
} AVIMAINHEADER;

typedef struct _avistreamheader {
	FOURCC fcc;  // '必须为‘strh’
	uint64_t  cb;   // 本数据结构的大小，不包括最初的8个字节（fcc和cb两个域）
	FOURCC fccType;    // 流的类型：‘auds’（音频流）、‘vids’（视频流）、‘mids’（MIDI流）、‘txts’（文字流）
	FOURCC fccHandler; // '指定流的处理者，对于音视频来说就是解码器
	uint64_t  dwFlags;    // '标记：是否允许这个流输出？调色板是否变化？
	uint32_t   wPriority;  // '流的优先级（当有多个相同类型的流时优先级最高的为默认流）
	uint32_t   wLanguage;
	uint64_t  dwInitialFrames; // 为交互格式指定初始帧数
	uint64_t  dwScale;   // '这个流使用的时间尺度
	// uint64_t  dwRate;	  // 4055
	float  dwRate;	  // 4055  使用浮点计算播放时间较精确
	uint64_t  dwStart;   // '流的开始时间
	uint64_t  dwLength;  // '流的帧数 (158)
	uint64_t  dwSuggestedBufferSize; // '读取这个流数据建议使用的缓存大小1536
	uint64_t  dwQuality;    // 流数据的质量指标（0 ~ 10,000）
	uint64_t  dwSampleSize; // 'Sample的大小
	struct {
		short int left;
		short int top;
		short int right;
		short int bottom;
	}  rcFrame;  // '指定这个流（视频流或文字流）在视频主窗口中的显示位置'
	// 视频主窗口由AVIMAINHEADER结构中的dwWidth和dwHeight决定
} AVISTREAMHEADER;


// prefix AIEF denoting Avi Index Entry Flag
#define    AIEF_LIST            0x00000001L        // indexed chunk is a list
#define    AIEF_KEYFRAME        0x00000010L        // indexed chunk is a key frame
#define    AIEF_NOTIME            0x00000100L        // indexed chunk frame do not consume any time
#define    AIEF_COMPUSE        0x0FFF0000L        // these bits are used by compressor
#define    AIEF_FIXKEYFRAME    0x00001000L        // XXX: borrowed from VLC, avoid using

typedef struct _avioldindex {
	FOURCC  fcc;  // 必须为‘idx1’
	uint64_t   cb;   // 本数据结构的大小，不包括最初的8个字节（fcc和cb两个域）
	struct _avioldindex_entry {
		uint64_t   dwChunkId;   // 表征本数据块的四字符码
		uint64_t   dwFlags;     // 说明本数据块是不是关键帧、是不是‘rec ’列表等信息
		uint64_t   dwOffset;    // 本数据块在文件中的偏移量
		uint64_t   dwSize;      // 本数据块的大小
	} aIndex[1]; // 这是一个数组！为每个媒体数据块都定义一个索引信息
} AVIOLDINDEX;


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/


//---------------------------------------------------------------------------------------


static int SetFilePointer(FILE *stream,long offset,int *para2,int fromwhere)
{
	fseek(stream,offset,fromwhere);
}

static uint8_t ReadFile(FILE *stream,void *buffer,size_t size,uint64_t *lpNumberOfBytesRead,int *lpOverlapped)
{
	int Ret = fread(buffer,size,1,stream);
	*lpNumberOfBytesRead = Ret * size;
	if (*lpNumberOfBytesRead != size) {
		return 0;
	} else {
		return 1;
	}
}


//---------------------------------------------------------------------------------------
static void InitAudioMpeg4(struct _CMPEG4Head *This, uint32_t Channels,uint64_t Sample,uint64_t dwBlockSize)
{
	// AVI_set_audio(This->avi_lib,Channels, Sample, 16, WAVE_FORMAT_PCM, Sample);
	mp4MuxerAudioInit(Channels, Sample, 16);
}
//---------------------------------------------------------------------------------------
static void StartTimer(struct _CMPEG4Head *This)
{
	if(This->dwStartTick == 0) {
		This->dwStartTick = MyGetTickCount();
		// printf("[%s] StartTick :%ld\n",__FUNCTION__,This->dwStartTick);
	}
}
//---------------------------------------------------------------------------------------
static void StopTimer(struct _CMPEG4Head *This)
{
	if(This->dwEndTick == 0) {
		This->dwEndTick = MyGetTickCount();
		// printf("[%s] EndTick :%ld\n",__FUNCTION__,This->dwEndTick);
	}
}
//---------------------------------------------------------------------------------------
static uint8_t WriteVideo(struct _CMPEG4Head *This, void *pData,uint32_t dwSize)
{
	if (dwSize == 0)
		return 0;
	pthread_mutex_lock(&This->mutex);
	// StartTimer(This);
	// AVI_write_frame(This->avi_lib,pData,dwSize,1);
	mp4MuxerAppendVideo(pData,dwSize);
	This->dwFrameCnt++;
	pthread_mutex_unlock(&This->mutex);
}
//---------------------------------------------------------------------------------------
static uint8_t WriteAudio(struct _CMPEG4Head *This, void *pData,uint32_t dwSize)
{
	if (dwSize == 0)
		return 0;
	pthread_mutex_lock(&This->mutex);
	// AVI_write_audio(This->avi_lib,pData,dwSize);
	mp4MuxerAppendAudio(pData,dwSize);
	pthread_mutex_unlock(&This->mutex);
}
//---------------------------------------------------------------------------------------
// 获取AVI文件播放总时间
static int GetAviTotalTime(struct _CMPEG4Head *This)
{
	int Ret = 0;
	int FileOffset;
	uint64_t ReadSize;
	AVISTREAMHEADER AviStreamHead;

	FileOffset = sizeof(LISTAREASTRUCT)*3 + sizeof(AVIMAINHEADER);
	SetFilePointer(This->hFile, FileOffset, NULL, 0);

	if(ReadFile(This->hFile,&AviStreamHead,sizeof(AVISTREAMHEADER),&ReadSize,NULL)) {
		Ret = (AviStreamHead.dwLength/AviStreamHead.dwRate)-1;
		// printf("ret:%d,len:%d,rate:%f\n", Ret,AviStreamHead.dwLength,AviStreamHead.dwRate);
	}
	//printf("Read Avi File Fail!\n");
	SetFilePointer(This->hFile,20+2048*2+256+1024+12,NULL,0);
//	printf("time :%d\n",Ret);
	return Ret;
}
//--------------------------------------------------------------------------------------
// 获取AVI文件帧率
static int GetAviFileFrameRate(struct _CMPEG4Head *This)
{
	int Ret = 0;
	int FileOffset;
	uint64_t ReadSize;
	AVISTREAMHEADER AviStreamHead;

	FileOffset = sizeof(LISTAREASTRUCT)*3 + sizeof(AVIMAINHEADER);
	SetFilePointer(This->hFile, FileOffset, NULL, 0);

	if(ReadFile(This->hFile,&AviStreamHead,sizeof(AVISTREAMHEADER),&ReadSize,NULL)) {
		Ret = AviStreamHead.dwRate;
	}
	//printf("Read Avi File Fail!\n");
	SetFilePointer(This->hFile,20+2048*2+256+1024+12,NULL,0);
	return Ret;
}

//--------------------------------------------------------------------------------------
// 读视频数据或音频数据
// 返回值 1:视频数据  2:音频数据 0:无效
static int ReadAviData(struct _CMPEG4Head *This, void *pData,uint64_t *dwSize,int *InCameraWidth,int *InCameraHeight)
{
	int Ret = 0;
	unsigned char tmp;
	AVIMAINHEADER AviMainHead;
	if(This->hFile != NULL)
	{
		uint64_t ReadSize;
		CHUNKSTRUCT Chunk;
		*dwSize = 0;
		if (This->ReadFirstFrame == 0) {
			This->ReadFirstFrame = 1;
			SetFilePointer(This->hFile,sizeof(LISTAREASTRUCT) * 2,NULL,0);

			ReadFile(This->hFile,&AviMainHead,sizeof(AVIMAINHEADER),&ReadSize,NULL);
			if (ReadSize==sizeof(AVIMAINHEADER)) {
				*InCameraWidth = AviMainHead.dwWidth;
				*InCameraHeight = AviMainHead.dwHeight;
			} else {
				return Ret;
			}
			SetFilePointer(This->hFile,20+2048*2+256+1024+12,NULL,0);
		}
		ReadFile(This->hFile,&Chunk,sizeof(CHUNKSTRUCT),&ReadSize,NULL);
		if(ReadSize==sizeof(CHUNKSTRUCT)) {
			if(Chunk.fcc == 0x63643030) {// 视频数据
				ReadFile(This->hFile,pData,Chunk.cb,&ReadSize,NULL);
				if(Chunk.cb == ReadSize) {
					*dwSize = ReadSize;
					This->dwStreamSize += (*dwSize + sizeof(CHUNKSTRUCT));
					if(This->dwFrameCnt==0) {
						This->InitAudioFrame = This->dwAudioFrame;
					}
					This->dwFrameCnt++;
					Ret = 1;
					if(*dwSize % 2) {//对齐
						ReadFile(This->hFile,&tmp,1,&ReadSize,NULL);
						This->dwStreamSize += 1;
					}
				}
			} else if(Chunk.fcc == 0x62773130) {// 音频数据
				char *p = (char *)pData;
				do{
					ReadFile(This->hFile,p,Chunk.cb,&ReadSize,NULL);
					p += Chunk.cb;
					if(This->dwAudioFrame==0) {
						This->InitVideoFrame = This->dwFrameCnt;
					}
					This->dwAudioFrame++;		//音频流帧数

					if(ReadSize==Chunk.cb) {
						*dwSize += ReadSize;
						This->dwStreamSize += (Chunk.cb + sizeof(CHUNKSTRUCT));
						Ret = 2;
						if(Chunk.cb % 2) { //对齐
							ReadFile(This->hFile, &tmp, 1, &ReadSize, NULL);
							This->dwStreamSize += 1;
						}
					}
					// 重读一下，看是否还有音频数据
					memset(&Chunk, 0, sizeof(CHUNKSTRUCT));
					ReadFile(This->hFile,&Chunk,sizeof(CHUNKSTRUCT),&ReadSize,NULL);
					if(Chunk.fcc != 0x62773130) {
						SetFilePointer(This->hFile,0-sizeof(CHUNKSTRUCT),NULL,SEEK_CUR);
						break;
					}
				}while(1);
			}
		}
	}
	return Ret;
}
//---------------------------------------------------------------
static void DestoryMPEG4Head(struct _CMPEG4Head **This)
{
	// StopTimer(*This);
	// uint64_t diff_time = (*This)->dwEndTick - (*This)->dwStartTick;
	// float dwFrameRate = (*This)->dwFrameCnt * 1000.0 / (float)diff_time;
	mp4MuxerStop();
	// AVI_set_video((*This)->avi_lib,(*This)->m_Width,(*This)->m_Height,dwFrameRate,"H264");
	// AVI_close((*This)->avi_lib);
	if (*This)
		free(*This);
	(*This) = NULL;
}
//---------------------------------------------------------------

//bReadWrite 读写标志, =0写文件  =1读文件
MPEG4Head* Mpeg4_Create(int Width,int Height,const char *FileName, int ReadWrite)
{
	FILE *fd;
	struct _CMPEG4Head *This = (MPEG4Head*)calloc(1,sizeof(MPEG4Head));
	if(ReadWrite == READ_ONLY) {
	} else {
		mp4MuxerInit(Width,Height,FileName);
		// This->avi_lib = AVI_open_output_file(FileName);
		This->dwStartTick = 0;
		This->dwEndTick = 0;
		This->dwFrameCnt = 0;
		This->m_Width = Width;
		This->m_Height = Height;
		This->FirstFrame = 1;
        pthread_mutexattr_t mutexattr;
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&This->mutex, &mutexattr);
		pthread_mutexattr_destroy(&mutexattr);
	}
	/*
	if(access(FileName,F_OK) == -1) { //判断文件是否存在存在与否
		fd = fopen(FileName,"wb+");	//创建文件
		fclose(fd);
	}
	if(ReadWrite == READ_ONLY)
		This->hFile = fopen(FileName,"rb");
	else
		This->hFile = fopen(FileName,"rb+");
	if(This->hFile == INVALID_HANDLE_VALUE) {
		//'文件创建失败
		printf("[%s] fopen file %s fail!\n",__FUNCTION__,FileName);
		printf("error %s\n",strerror(errno));
		return NULL;
	} else {
		strcpy(This->filename,FileName);
		This->dwStreamSize = 0;
		This->dwFrameCnt = 0;
		SetFilePointer(This->hFile,20+2048*2+256+1024+12,NULL,0);
		This->dwStartTick = 0;
		This->dwEndTick = 0;
		This->bHaveAudio = 0;
		This->bWriteAudio = 0;
		This->InitVideoFrame = 0;
		This->InitAudioFrame = 0;
		This->ReadFirstFrame = 0;
		This->m_ReadWrite = ReadWrite;
	}
	*/

	This->InitAudio = InitAudioMpeg4;
	This->WriteVideo = WriteVideo;
	This->WriteAudio = WriteAudio;
	This->GetAviTotalTime = GetAviTotalTime;
	This->GetAviFileFrameRate = GetAviFileFrameRate;
	This->ReadAviData = ReadAviData;
	This->DestoryMPEG4 = DestoryMPEG4Head;
	return This;
}
