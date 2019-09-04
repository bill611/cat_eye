#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "avi_encode.h"
#include "externfunc.h"

#if DBG_MPEG4 > 0
	#define DBG_P( x... ) printf( x )
#else
	#define DBG_P( x... )
#endif

#define FILE_CURRENT SEEK_CUR
#define INVALID_HANDLE_VALUE NULL
//---------------------------------------------------------------------------------------


static int SetFilePointer(FILE *stream,long offset,int *para2,int fromwhere)
{
	fseek(stream,offset,fromwhere);
}

static int WriteFile(FILE* stream,const void* buffer,size_t size,uint64_t *lpNumberOfBytesWritten,int *lpOverlapped)
{
	int Ret = fwrite(buffer,size,1,stream);
	*lpNumberOfBytesWritten = Ret * size;
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
	AVI_set_audio(This->avi_lib,Channels, Sample, 16, WAVE_FORMAT_PCM, Sample);
	/*if(This->dwFrameCnt==0)
	{
		This->mChannels = Channels;
		This->mSample = Sample;
		This->mBlockSize = dwBlockSize;
		This->bHaveAudio = 1;
	}*/
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
static uint8_t WriteAviHead(struct _CMPEG4Head *This)
{
	uint8_t bRet = 0;
	if(This->hFile!=INVALID_HANDLE_VALUE && This->dwFrameCnt>0)
	{
        pthread_mutex_lock(&This->mutex);
		StopTimer(This);

		uint64_t WriteSize;
		LISTAREASTRUCT ListHead;
		fflush(This->hFile);
		uint64_t dwFileSize = GetFileSize(This->filename);
		uint64_t diff_time = This->dwEndTick - This->dwStartTick;
		float dwFrameRate = This->dwFrameCnt * 1000.0 / (float)diff_time;
		// printf("[%s] cnt:%ld, time:%lld, FrameRate :%f,size:%lld\n",
				// __func__,
				// This->dwFrameCnt,
				// diff_time,
				// dwFrameRate,
				// dwFileSize);
		SetFilePointer(This->hFile,0,NULL,0);
		ListHead.fcc = 0x46464952;		//文件头RIFF
		ListHead.cb = dwFileSize-8;		//文件大小
		ListHead.type = 0x20495641;		//'AVI '
		WriteFile(This->hFile,&ListHead,sizeof(LISTAREASTRUCT),&WriteSize,NULL);

		//必须的第一个hdrl列表,用于描述AVI文件中各个流的格式信息，
		ListHead.fcc = 0x5453494C;		//'LIST'
		ListHead.cb = 2048*2+256;				// 4(type)+64(AVIMAINHEADER)+138(AVISTREAMHEADER)
		ListHead.type = 0x6C726468;		//'hdrl'
		WriteFile(This->hFile,&ListHead,sizeof(LISTAREASTRUCT),&WriteSize,NULL);

		//hdrl列表嵌套了一系列块和子列表——首先是一个‘avih’块，用于记录AVI文件的全局信息，比如流的数量、视频图像的宽和高等
		AVIMAINHEADER AviMainHead;
		AviMainHead.fcc = 0x68697661;			//'avih'
		AviMainHead.cb = 56;
		AviMainHead.dwMicroSecPerFrame = (diff_time * 1000) / This->dwFrameCnt;
		AviMainHead.dwMaxBytesPerSec = 0;
		AviMainHead.dwPaddingGranularity = 0;
		AviMainHead.dwFlags = 0;
		AviMainHead.dwTotalFrames = This->dwFrameCnt;
		AviMainHead.dwInitialFrames = 0;		//为交互格式指定初始帧数
		if(This->bWriteAudio)
			AviMainHead.dwStreams = 2;
		else
			AviMainHead.dwStreams = 1;
		AviMainHead.dwSuggestedBufferSize = 0;
		AviMainHead.dwWidth = This->m_Width;
		AviMainHead.dwHeight = This->m_Height;
		AviMainHead.dwReserved[0] = 0;
		AviMainHead.dwReserved[1] = 0;
		AviMainHead.dwReserved[2] = 0;
		AviMainHead.dwReserved[3] = 0;
		WriteFile(This->hFile,&AviMainHead,sizeof(AVIMAINHEADER),&WriteSize,NULL);

		//然后，就是一个或多个‘strl’子列表。（文件中有多少个流，这里就对应有多少个‘strl’子列表。）每个‘strl’子列表至少包含一个‘strh’块和一个‘strf’块
		ListHead.fcc = 0x5453494C;		//'LIST'
		ListHead.cb = 2040;				//4(strl) + 64(strh) + 48(strf) + 14(strn)
		ListHead.type = 0x6C727473;		//'strl'
		WriteFile(This->hFile,&ListHead,sizeof(LISTAREASTRUCT),&WriteSize,NULL);

		AVISTREAMHEADER AviStreamHead;
		AviStreamHead.fcc = 0x68727473;		//'strh'
		AviStreamHead.cb = 56;				//// 本数据结构的大小，不包括最初的8个字节（fcc和cb两个域）
		AviStreamHead.fccType = 0x73646976;	//'vids'  // 流的类型：‘auds’（音频流）、‘vids’（视频流）、‘mids’（MIDI流）、‘txts’（文字流）
		if(This->m_VideoType==0)
			AviStreamHead.fccHandler = 0x34363248;	//'h264' 指定流的处理者，对于音视频来说就是解码器
		else
			AviStreamHead.fccHandler = 0x78766964;	//'divx' 指定流的处理者，对于音视频来说就是解码器
		AviStreamHead.dwFlags = 0;				//标记：是否允许这个流输出？调色板是否变化
		AviStreamHead.wPriority = 0;			//流的优先级（当有多个相同类型的流时优先级最高的为默认流）
		AviStreamHead.wLanguage = 0;
		AviStreamHead.dwInitialFrames = 0;		//为交互格式指定初始帧数(非交互格式应指定为0)
		AviStreamHead.dwScale = 1;				// 这个流使用的时间尺度
		AviStreamHead.dwRate = dwFrameRate;
		AviStreamHead.dwStart = 0;				// 流的开始时间
		AviStreamHead.dwLength = This->dwFrameCnt;		//帧数 流的长度（单位与dwScale和dwRate的定义有关）
		AviStreamHead.dwSuggestedBufferSize = 40000;	// 读取这个流数据建议使用的缓存大小,应能容纳最大帧
		AviStreamHead.dwQuality = 0;				// 流数据的质量指标（0 ~ 10,000）
		AviStreamHead.dwSampleSize = 0;				// Sample的大小
		AviStreamHead.rcFrame.left = 0;				// 指定这个流（视频流或文字流）在视频主窗口中的显示位置
		AviStreamHead.rcFrame.top = 0;
		AviStreamHead.rcFrame.right = This->m_Width;
		AviStreamHead.rcFrame.bottom = This->m_Height;
		WriteFile(This->hFile,&AviStreamHead,sizeof(AVISTREAMHEADER),&WriteSize,NULL);

		//‘strf’块，用于说明流的具体格式。如果是视频流，则使用一个BITMAPINFO数据结构来描述；如果是音频流，则使用一个WAVEFORMATEX数据结构来描述
		CHUNKSTRUCT Chunk;
		Chunk.fcc = 0x66727473;		//'strf'
		Chunk.cb = 0x28;
		WriteFile(This->hFile,&Chunk,sizeof(CHUNKSTRUCT),&WriteSize,NULL);

		MYBITMAPINFOHEADER BitmapInfo;
		BitmapInfo.biSize = sizeof(MYBITMAPINFOHEADER);
		BitmapInfo.biWidth = This->m_Width;
		BitmapInfo.biHeight = This->m_Height;
		BitmapInfo.biPlanes = 1;
		BitmapInfo.biBitCount = 24;
		if(This->m_VideoType==0)
			BitmapInfo.biCompression = 0x34363248;		//h264
		else
			BitmapInfo.biCompression = 0x78766964;		//divx
		BitmapInfo.biSizeImage = This->m_Width * This->m_Height * 3;
		BitmapInfo.biXPelsPerMeter = 0;
		BitmapInfo.biYPelsPerMeter = 0;
		BitmapInfo.biClrUsed = 0;
		BitmapInfo.biClrImportant = 0;
		WriteFile(This->hFile,&BitmapInfo,sizeof(MYBITMAPINFOHEADER),&WriteSize,NULL);

		//‘JUNK’块，填充
		Chunk.fcc = 0x4B4E554A;		//'JUNK'
		Chunk.cb = 2048-12-64-48-8;		//LIST+strl+strf+Junk_head
		WriteFile(This->hFile,&Chunk,sizeof(CHUNKSTRUCT),&WriteSize,NULL);
		SetFilePointer(This->hFile,Chunk.cb,NULL,FILE_CURRENT);


		if(This->bWriteAudio)
		{
			ListHead.fcc = 0x5453494C;		//'LIST'
			ListHead.cb = 2040;				//4(strl) + 64(strh) + 26+0(strf) + 14(strn)
			ListHead.type = 0x6C727473;		//'strl'
			WriteFile(This->hFile,&ListHead,sizeof(LISTAREASTRUCT),&WriteSize,NULL);

			AVISTREAMHEADER AviStreamHead;
			AviStreamHead.fcc = 0x68727473;		//'strh'
			AviStreamHead.cb = 56;				//// 本数据结构的大小，不包括最初的8个字节（fcc和cb两个域）
			AviStreamHead.fccType = 0x73647561;	//'auds'  // 流的类型：‘auds’（音频流）、‘vids’（视频流）、‘mids’（MIDI流）、‘txts’（文字流）
			AviStreamHead.fccHandler = 0x01;	//指定流的处理者，对于音视频来说就是解码器,这里是PCM
			AviStreamHead.dwFlags = 0;				//标记：是否允许这个流输出？调色板是否变化
			AviStreamHead.wPriority = 0;			//流的优先级（当有多个相同类型的流时优先级最高的为默认流）
			AviStreamHead.wLanguage = 0;
			AviStreamHead.dwInitialFrames = 0;//InitAudioFrame;		//为交互格式指定初始帧数
			AviStreamHead.dwScale = 1;				// 这个流使用的时间尺度,这里为两个字节(16bit PCM音频)
			AviStreamHead.dwRate = This->mChannels*This->mSample;		//音频速度，每秒多少字节
			AviStreamHead.dwStart = 0;				//流的开始时间
			AviStreamHead.dwLength = This->dwAudioFrame*This->mBlockSize/2;		//流的长度（单位与dwScale和dwRate的定义有关），音频总长除时间尺度
			AviStreamHead.dwSuggestedBufferSize = 8000;	// 读取这个流数据建议使用的缓存大小8000
			AviStreamHead.dwQuality = 0;				// 流数据的质量指标（0 ~ 10,000）
			AviStreamHead.dwSampleSize = 2*This->mChannels;				// Sample的大小,PCM用2  ADPCM使用256
			AviStreamHead.rcFrame.left = 0;				// 指定这个流（视频流或文字流）在视频主窗口中的显示位置
			AviStreamHead.rcFrame.top = 0;
			AviStreamHead.rcFrame.right = 0;
			AviStreamHead.rcFrame.bottom = 0;
			WriteFile(This->hFile,&AviStreamHead,sizeof(AVISTREAMHEADER),&WriteSize,NULL);

			//‘strf’块，用于说明流的具体格式。如果是视频流，则使用一个BITMAPINFO数据结构来描述；如果是音频流，则使用一个WAVEFORMATEX数据结构来描述
			Chunk.fcc = 0x66727473;		//'strf'
			Chunk.cb = 18+0;				//sizeof(WAVEFORMATEX)+0(扩展数据)
			WriteFile(This->hFile,&Chunk,sizeof(CHUNKSTRUCT),&WriteSize,NULL);

			//音频数据
			WAVEFORMATEX WaveEx;
			WaveEx.wFormatTag = 0x01;			//1.PCM 2.ADPCM
			WaveEx.nChannels = This->mChannels;		//通道数
			WaveEx.nSamplesPerSec = This->mSample;	//采样率
			WaveEx.nAvgBytesPerSec = This->mSample*2*This->mChannels;
			WaveEx.nBlockAlign = 2;
			WaveEx.wBitsPerSample = 16;
			WaveEx.cbSize = 0;
			WriteFile(This->hFile,&WaveEx,sizeof(WAVEFORMATEX),&WriteSize,NULL);
			//扩展数据
			//unsigned char usTmp[] = {0xF4,0x01,0x07,0x00,0x00,0x01,0x00,0x00,0x00,0x02,0x00,0xFF,0x00,0x00,
			//	0x00,0x00,0xC0,0x00,0x40,0x00,0xF0,0x00,0x00,0x00,0xCC,0x01,0x30,0xFF,0x88,0x01,0x18,0xFF};
			//WriteFile(hFile,&usTmp,sizeof(usTmp),&WriteSize,NULL);

			//填充‘JUNK’块
			Chunk.fcc = 0x4B4E554A;		//'JUNK'
			Chunk.cb = 2048-12-64-(Chunk.cb+8)-8;		//LIST(12)+strh(64)+strf+Junk_head
			WriteFile(This->hFile,&Chunk,sizeof(CHUNKSTRUCT),&WriteSize,NULL);
			SetFilePointer(This->hFile,Chunk.cb,NULL,FILE_CURRENT);
		}
		else
		{
			//‘JUNK’块，填充
			Chunk.fcc = 0x4B4E554A;		//'JUNK'
			Chunk.cb = 2048-8;		//LIST+strl+strf+Junk_head
			WriteFile(This->hFile,&Chunk,sizeof(CHUNKSTRUCT),&WriteSize,NULL);
			SetFilePointer(This->hFile,Chunk.cb,NULL,FILE_CURRENT);
		}

		Chunk.fcc = 0x4B4E554A;		//'JUNK'
		Chunk.cb = 256-8-64-4;		//LIST+strl+strf+Junk_head
		WriteFile(This->hFile,&Chunk,sizeof(CHUNKSTRUCT),&WriteSize,NULL);
		SetFilePointer(This->hFile,Chunk.cb,NULL,FILE_CURRENT);

		Chunk.fcc = 0x4B4E554A;		//'JUNK'
		Chunk.cb = 1024-8;		//LIST+strl+strf+Junk_head
		WriteFile(This->hFile,&Chunk,sizeof(CHUNKSTRUCT),&WriteSize,NULL);
		SetFilePointer(This->hFile,Chunk.cb,NULL,FILE_CURRENT);

		//AVI文件必需的第二个列表——‘movi’列表，用于保存真正的媒体流数据（视频图像帧数据或音频采样数据等）。
		ListHead.fcc = 0x5453494C;		//'LIST'
		ListHead.cb = This->dwStreamSize+4;	//dwStreamSize + type size
		ListHead.type = 0x69766F6D;		//'movi'
		WriteFile(This->hFile,&ListHead,sizeof(LISTAREASTRUCT),&WriteSize,NULL);

		bRet = 1;
        pthread_mutex_unlock(&This->mutex);
		//LeaveCriticalSection (&mutex);		//解锁
	}
	return bRet;
}
//---------------------------------------------------------------------------------------
static uint8_t WriteVideo(struct _CMPEG4Head *This, const void *pData,uint64_t dwSize)
{
        pthread_mutex_lock(&This->mutex);
		StartTimer(This);
		AVI_write_frame(This->avi_lib,pData,dwSize,1);
		This->dwFrameCnt++;
        pthread_mutex_unlock(&This->mutex);
		/*
	uint8_t bRet = 0;
	unsigned char *p = (unsigned char *)pData;
	if(This->hFile != INVALID_HANDLE_VALUE)
	{
        pthread_mutex_lock(&This->mutex);
		//EnterCriticalSection (&mutex);		//加锁

		StartTimer(This);
		uint64_t WriteSize;
		CHUNKSTRUCT Chunk;
		Chunk.fcc = 0x63643030;		//'00dc'
		Chunk.cb = dwSize;
		WriteFile(This->hFile,&Chunk,sizeof(CHUNKSTRUCT),&WriteSize,NULL);
//		printf("[%s] WriteSize:%d,cnt:%d\n",__FUNCTION__,WriteSize,sizeof(CHUNKSTRUCT));
		if(WriteSize == sizeof(CHUNKSTRUCT))
		{
			WriteFile(This->hFile,p,dwSize,&WriteSize,NULL);
			if(WriteSize == dwSize) {
				This->dwStreamSize += (dwSize + sizeof(CHUNKSTRUCT));
				if(This->dwFrameCnt==0) {
					This->InitAudioFrame = This->dwAudioFrame;
				}
				This->dwFrameCnt++;

				bRet = 1;
				if(dwSize % 2) { //对齐
					WriteFile(This->hFile,"\0",1,&WriteSize,NULL);
					This->dwStreamSize += 1;
				}
			}
		}
        pthread_mutex_unlock(&This->mutex);
		//LeaveCriticalSection (&mutex);		//解锁
	}
	return bRet;
	*/
}
//---------------------------------------------------------------------------------------
static uint8_t WriteAudio(struct _CMPEG4Head *This, const void *pData,uint64_t dwSize)
{
        pthread_mutex_lock(&This->mutex);
		AVI_write_audio(This->avi_lib,pData,dwSize);
        pthread_mutex_unlock(&This->mutex);
		/*
	int i;
	uint8_t bRet = 0;
	if(This->hFile!=INVALID_HANDLE_VALUE && This->bHaveAudio)
	{
		uint64_t Pos = 0;
		int nBlock = (dwSize+This->mBlockSize-1) / This->mBlockSize;
		const char *p = (const char *)pData;
        pthread_mutex_lock(&This->mutex);
		//EnterCriticalSection (&mutex);		//加锁
		for(i=0;i<nBlock;i++)
		{
			uint64_t WriteSize;
			CHUNKSTRUCT Chunk;
			Chunk.fcc = 0x62773130;		//'01wb'
			Chunk.cb = dwSize - Pos >= This->mBlockSize ? This->mBlockSize : dwSize - Pos;
			WriteFile(This->hFile,&Chunk,sizeof(CHUNKSTRUCT),&WriteSize,NULL);
			if(WriteSize==sizeof(CHUNKSTRUCT))
			{
				WriteFile(This->hFile,p,Chunk.cb,&WriteSize,NULL);
				p += Chunk.cb;
				if(This->dwAudioFrame==0)
				{
					This->InitVideoFrame = This->dwFrameCnt;
				}
				This->dwAudioFrame++;		//音频流帧数

				if(WriteSize==Chunk.cb)
				{
					This->dwStreamSize += (Chunk.cb + sizeof(CHUNKSTRUCT));

					bRet = 1;
					if(Chunk.cb % 2)
					{
						//对齐
						WriteFile(This->hFile,"\0",1,&WriteSize,NULL);
						This->dwStreamSize += 1;
					}
				}
			}
		}
		This->bWriteAudio = 1;
        pthread_mutex_unlock(&This->mutex);
		//LeaveCriticalSection (&mutex);		//解锁
	}
	return bRet;
	*/
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
	DBG_P("[%s]\n",__FUNCTION__);
	if(This->hFile != INVALID_HANDLE_VALUE)
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
						SetFilePointer(This->hFile,0-sizeof(CHUNKSTRUCT),NULL,FILE_CURRENT);
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
	/*
	if((*This)->hFile != INVALID_HANDLE_VALUE)
	{
		if((*This)->m_ReadWrite == WRITE_READ)
			WriteAviHead(*This);
		fflush((*This)->hFile);
		fclose((*This)->hFile);
	}
	*/
	StopTimer(*This);
	uint64_t diff_time = (*This)->dwEndTick - (*This)->dwStartTick;
	float dwFrameRate = (*This)->dwFrameCnt * 1000.0 / (float)diff_time;
	AVI_set_video((*This)->avi_lib,(*This)->m_Width,(*This)->m_Height,dwFrameRate,"H264");
	AVI_close((*This)->avi_lib);
	free(*This);
	(*This) = NULL;
}
//---------------------------------------------------------------

//bReadWrite 读写标志, =0写文件  =1读文件
MPEG4Head* Mpeg4_Create(int Width,int Height,const char *FileName, int ReadWrite, int VideoType)
{
	FILE *fd;
	struct _CMPEG4Head *This = (MPEG4Head*)calloc(1,sizeof(MPEG4Head));
	DBG_P("[%s] enter!\n",__FUNCTION__);
	if(ReadWrite == READ_ONLY) {
	} else {
		This->avi_lib = AVI_open_output_file(FileName);
		This->dwStartTick = 0;
		This->dwEndTick = 0;
		This->dwFrameCnt = 0;
		This->m_Width = Width;
		This->m_Height = Height;
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
		This->FirstFrame = 1;
		This->ReadFirstFrame = 0;
		This->m_ReadWrite = ReadWrite;
		This->m_VideoType = VideoType;
        pthread_mutexattr_t mutexattr;
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&This->mutex, &mutexattr);
		pthread_mutexattr_destroy(&mutexattr);
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
