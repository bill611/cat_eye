#ifndef MPEG4HeadH
#define MPEG4HeadH

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#define AUDIOBLOCKSIZE 1024


#pragma once

#define WRITE_READ	0
#define READ_ONLY	1

//#define uint64_t unsigned int
#define FOURCC uint64_t
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
	FOURCC fcc;					// ��ʶ
	uint64_t  cb;					// ��С�������������8���ֽڣ�fcc��cb������
	FOURCC type;				// LIST����
} LISTAREASTRUCT;

typedef struct _ChunkStruct
{
	FOURCC fcc;					// ��ʶ
	uint64_t  cb;					// ���С�����������ṹ
} CHUNKSTRUCT;

// prefix AMHF denoting Avi Main Header Flag
#define    AMHF_HASINDEX        0x00000010		//�Ƿ��������
#define    AMHF_MUSTUSEINDEX    0x00000020
#define    AMHF_ISINTERLEAVED    0x00000100		//�Ƿ������������
#define    AMHF_TRUSTCKTYPE    0x00000800 /* Use CKType to find key frames? */
#define    AMHF_WASCAPTUREFILE    0x00010000
#define    AMHF_COPYRIGHTED    0x00020000

typedef struct _avimainheader {
	FOURCC fcc;					// ����Ϊ��avih��
	uint64_t  cb;					// �����ݽṹ�Ĵ�С�������������8���ֽڣ�fcc��cb������
	uint64_t  dwMicroSecPerFrame;   // ��Ƶ֡���ʱ�䣨��΢��Ϊ��λ��
	uint64_t  dwMaxBytesPerSec;     // ���AVI�ļ������������
	uint64_t  dwPaddingGranularity; // ������������
	uint64_t  dwFlags;         // AVI�ļ���ȫ�ֱ�ǣ������Ƿ����������  (0x810   HASINDEX   |   TRUSTCKTYPE)
	uint64_t  dwTotalFrames;   // ��֡��
	uint64_t  dwInitialFrames; // Ϊ������ʽָ����ʼ֡�����ǽ�����ʽӦ��ָ��Ϊ0��
	uint64_t  dwStreams;       // ���ļ����������ĸ���
	uint64_t  dwSuggestedBufferSize; // �����ȡ���ļ��Ļ����С��Ӧ���������Ŀ飩
	uint64_t  dwWidth;         // ��Ƶͼ��Ŀ�������Ϊ��λ��
	uint64_t  dwHeight;        // ��Ƶͼ��ĸߣ�������Ϊ��λ��
	uint64_t  dwReserved[4];   // ����
} AVIMAINHEADER;

typedef struct _avistreamheader {
	FOURCC fcc;  // '����Ϊ��strh��
	uint64_t  cb;   // �����ݽṹ�Ĵ�С�������������8���ֽڣ�fcc��cb������
	FOURCC fccType;    // �������ͣ���auds������Ƶ��������vids������Ƶ��������mids����MIDI��������txts������������
	FOURCC fccHandler; // 'ָ�����Ĵ����ߣ���������Ƶ��˵���ǽ�����
	uint64_t  dwFlags;    // '��ǣ��Ƿ�����������������ɫ���Ƿ�仯��
	uint32_t   wPriority;  // '�������ȼ������ж����ͬ���͵���ʱ���ȼ���ߵ�ΪĬ������
	uint32_t   wLanguage;
	uint64_t  dwInitialFrames; // Ϊ������ʽָ����ʼ֡��
	uint64_t  dwScale;   // '�����ʹ�õ�ʱ��߶�
	// uint64_t  dwRate;	  // 4055
	float  dwRate;	  // 4055  ʹ�ø�����㲥��ʱ��Ͼ�ȷ
	uint64_t  dwStart;   // '���Ŀ�ʼʱ��
	uint64_t  dwLength;  // '����֡�� (158)
	uint64_t  dwSuggestedBufferSize; // '��ȡ��������ݽ���ʹ�õĻ����С1536
	uint64_t  dwQuality;    // �����ݵ�����ָ�꣨0 ~ 10,000��
	uint64_t  dwSampleSize; // 'Sample�Ĵ�С
	struct {
		short int left;
		short int top;
		short int right;
		short int bottom;
	}  rcFrame;  // 'ָ�����������Ƶ����������������Ƶ�������е���ʾλ��'
	// ��Ƶ��������AVIMAINHEADER�ṹ�е�dwWidth��dwHeight����
} AVISTREAMHEADER;


// prefix AIEF denoting Avi Index Entry Flag
#define    AIEF_LIST            0x00000001L        // indexed chunk is a list
#define    AIEF_KEYFRAME        0x00000010L        // indexed chunk is a key frame
#define    AIEF_NOTIME            0x00000100L        // indexed chunk frame do not consume any time
#define    AIEF_COMPUSE        0x0FFF0000L        // these bits are used by compressor
#define    AIEF_FIXKEYFRAME    0x00001000L        // XXX: borrowed from VLC, avoid using

typedef struct _avioldindex {
	FOURCC  fcc;  // ����Ϊ��idx1��
	uint64_t   cb;   // �����ݽṹ�Ĵ�С�������������8���ֽڣ�fcc��cb������
	struct _avioldindex_entry {
		uint64_t   dwChunkId;   // ���������ݿ�����ַ���
		uint64_t   dwFlags;     // ˵�������ݿ��ǲ��ǹؼ�֡���ǲ��ǡ�rec ���б����Ϣ
		uint64_t   dwOffset;    // �����ݿ����ļ��е�ƫ����
		uint64_t   dwSize;      // �����ݿ�Ĵ�С
	} aIndex[1]; // ����һ�����飡Ϊÿ��ý�����ݿ鶼����һ��������Ϣ
} AVIOLDINDEX;

typedef struct _CMPEG4Head
{
	FILE *hFile;
	char filename[64];				//�ļ�������·��
    pthread_mutex_t mutex;
	uint64_t dwFrameCnt;			//��Ƶ��֡��
	uint64_t dwAudioFrame;			//��Ƶ��֡��
	uint64_t dwStreamSize;			//��Ƶ����С
	uint64_t dwStartTick;
	uint64_t dwEndTick;
	int FirstFrame;		//�Ƿ��ǵ�һ֡����
	int ReadFirstFrame;	//�Ƿ��Ѿ���ȡ��һ֡
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
	uint8_t m_ReadWrite; // ��д��־, =0д�ļ�  =1���ļ�
	int  m_VideoType;  // ��Ƶ���� =0H264 =1divx

	void (*InitAudio)(struct _CMPEG4Head *This, uint32_t Channels,uint64_t Sample,uint64_t dwBlockSize);		//ͨ������λ�ʣ����С
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
