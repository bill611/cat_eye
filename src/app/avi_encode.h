#ifndef MPEG4HeadH
#define MPEG4HeadH

#ifdef __cplusplus
extern "C" {
#endif

#include "avilib/avilib.h"
#include <stdint.h>
#include <pthread.h>
#define AUDIOBLOCKSIZE 1024


#pragma once

#define WRITE_READ	0
#define READ_ONLY	1

//#define uint64_t unsigned int
#define FOURCC uint32_t
#define HANDLE int

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

typedef struct _CMPEG4Head
{
	FILE *hFile;
	avi_t *avi_lib;
	char filename[64];				//文件名，含路径
    pthread_mutex_t mutex;
	uint64_t dwFrameCnt;			//视频流帧数
	uint64_t dwAudioFrame;			//音频流帧数
	uint64_t dwStreamSize;			//视频流大小
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
	uint8_t (*WriteAviHead)(struct _CMPEG4Head *This);
	uint8_t m_ReadWrite; // 读写标志, =0写文件  =1读文件
	int  m_VideoType;  // 视频类型 =0H264 =1divx

	void (*InitAudio)(struct _CMPEG4Head *This, uint32_t Channels,uint64_t Sample,uint64_t dwBlockSize);		//通道数，位率，块大小
	uint8_t (*WriteVideo)(struct _CMPEG4Head *This, const void *pData,uint64_t dwSize);
	uint8_t (*WriteAudio)(struct _CMPEG4Head *This, const void *pData,uint64_t dwSize);
	int (*GetAviTotalTime)(struct _CMPEG4Head *This);
	int (*GetAviFileFrameRate)(struct _CMPEG4Head *This);
	int (*ReadAviData)(struct _CMPEG4Head *This, void *pData,uint64_t *dwSize,int *InCameraWidth,int *InCameraHeight);
	void (*DestoryMPEG4)(struct _CMPEG4Head **This);
}MPEG4Head;

MPEG4Head* Mpeg4_Create(int Width,int Height,const char *FileName, int ReadWrite, int VideoType);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
