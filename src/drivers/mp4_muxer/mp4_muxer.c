#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mp4v2/mp4v2.h"
#include "faac.h"
#include "externfunc.h"

typedef struct _MP4ENC_NaluUnit
{
	int type;
	int size;
	unsigned char *data;

}MP4ENC_NaluUnit;

typedef struct _MP4ENC_INFO {
	unsigned int u32FrameRate;
	unsigned int u32Width;
	unsigned int u32Height;
	unsigned int u32Profile;
}MP4ENC_INFO;

typedef struct _MuxerQueue {
	uint8_t  *buffer;	// 队列内容
	uint32_t cur_leng; 		// 当前队列长度
	uint32_t total_leng; 			// 队列长度
	uint32_t index_read; 	// 读下标
	uint32_t index_write; 	// 写下标
}MuxerQueue;

static MP4ENC_INFO enc_info;
static MP4FileHandle mp4_fd = NULL;
static MP4TrackId video = MP4_INVALID_TRACK_ID;
static MP4TrackId audio = MP4_INVALID_TRACK_ID;
static faacEncHandle faac_fd = NULL;
static unsigned long inputSample = 0;        //输入样本大小，在打开编码器时会得到此值
static unsigned long maxOutputBytes = 0;  //最大输出，编码后的输出数据大小不会高于这个值，也是打开编码器时获得
static unsigned int    mPCMBitSize = 16;    //pcm位深，用于计算一帧pcm大小
static MuxerQueue muxer_queue;

static int mPCMBufferSize = 0;    //一帧PCM缓存大小
static int mCountSize = 0;           //计算缓存大小

static unsigned char* aacData ;
static unsigned char* pcm_buffer ;
static unsigned int pcm_data_buffer_leng ;
static uint64_t video_start_tick = 0;
static int video_frame_cnt = 0;
static int audio_sample = 0;

static int muxerQueueInit(int size)
{
	memset(&muxer_queue,0,sizeof(muxer_queue));
	muxer_queue.buffer = (uint8_t *)calloc(1,size);
	muxer_queue.total_leng = size;
}

static int muxerQueueUninit(void)
{
	if (muxer_queue.buffer)
		free(muxer_queue.buffer);
}

static int muxerQueueWrite(uint8_t * data,uint32_t size)
{
	if (size == 0)
		return 0;
	if (muxer_queue.cur_leng + size > muxer_queue.total_leng) {
		printf("[%s]full!\n", __func__);
		return 0;
	}
	muxer_queue.cur_leng += size;
	if (muxer_queue.index_write + size > muxer_queue.total_leng) {
		// 若写指针已经位于接近末尾位置，则分段拷贝数据，循环队列
		int tail_leng = muxer_queue.total_leng - muxer_queue.index_write;
		memcpy(&muxer_queue.buffer[muxer_queue.index_write],data,tail_leng) ;
		int head_leng = size - tail_leng;
		memcpy(&muxer_queue.buffer[0],&data[tail_leng],head_leng) ;
		muxer_queue.index_write = head_leng;
	} else {
		memcpy(&muxer_queue.buffer[muxer_queue.index_write],data,size) ;
		muxer_queue.index_write += size;
	}
	return 1;
}

static int muxerQueueRead(uint8_t * data,uint32_t size)
{
	if (size == 0)
		return 0;
	if (muxer_queue.cur_leng < size) {
		// printf("[%s]empty!\n", __func__);
		return 0;
	}
	muxer_queue.cur_leng -= size;
	if (muxer_queue.index_read + size > muxer_queue.total_leng) {
		// 若读指针已经位于接近末尾位置，则分段拷贝数据，循环队列
		int tail_leng = muxer_queue.total_leng - muxer_queue.index_read;
		memcpy(data,&muxer_queue.buffer[muxer_queue.index_read],tail_leng) ;
		int head_leng = size - tail_leng;
		memcpy(&data[tail_leng],&muxer_queue.buffer[0],head_leng) ;
		muxer_queue.index_read = head_leng;
	} else {
		memcpy(data,&muxer_queue.buffer[muxer_queue.index_read],size) ;
		muxer_queue.index_read += size;
	}
	return 1;
}
static int muxerQueueGetCurLeng(void)
{
	return muxer_queue.cur_leng;
}

static int32_t readH264Nalu(uint8_t *pPack, uint32_t nPackLen, unsigned int offSet, MP4ENC_NaluUnit *pNaluUnit)
{
	unsigned int i = offSet;
	while (i < nPackLen)
	{
		if (pPack[i++] == 0x00 && pPack[i++] == 0x00 && pPack[i++] == 0x01)// 开始码
		{
			unsigned int pos = i;
			while (pos < nPackLen)
			{
				if (pPack[pos++] == 0x00 && pPack[pos++] == 0x00 && pPack[pos++] == 0x01)
					break;

			}
			if (pos == nPackLen)
				pNaluUnit->size = pos - i;
			else
				pNaluUnit->size = (pos - 3) - i;

			pNaluUnit->type = pPack[i] & 0x1f;
			pNaluUnit->data = (unsigned char *)&pPack[i];
			return (pNaluUnit->size + i - offSet);
		}
	}
	return 0;

}

static int32_t mp4v2VideoWrite(MP4FileHandle hFile, MP4TrackId *pTrackId,uint8_t *pPack,uint32_t nPackLen, MP4ENC_INFO *stMp4Info)
{
	MP4ENC_NaluUnit stNaluUnit;
	memset(&stNaluUnit, 0, sizeof(stNaluUnit));
	int nPos = 0, nLen = 0;
	while ((nLen = readH264Nalu(pPack, nPackLen, nPos, &stNaluUnit)) != 0)
	{
		switch (stNaluUnit.type)
		{
			case 7:
				if (*pTrackId == MP4_INVALID_TRACK_ID)
				{
					video_start_tick = MyGetTickCount();
					*pTrackId = MP4AddH264VideoTrack(hFile,
							90000,
						   	90000 / stMp4Info->u32FrameRate,
						   	stMp4Info->u32Width,
						   	stMp4Info->u32Height,
						   	stNaluUnit.data[1], stNaluUnit.data[2], stNaluUnit.data[3], 3);
					if (*pTrackId == MP4_INVALID_TRACK_ID) {
						return 0;
					}
					MP4SetVideoProfileLevel(hFile, stMp4Info->u32Profile);
					MP4AddH264SequenceParameterSet(hFile,*pTrackId,stNaluUnit.data,stNaluUnit.size);
				}
				break;
			case 8:
				if (*pTrackId == MP4_INVALID_TRACK_ID)
				{
					break;
				}
				MP4AddH264PictureParameterSet(hFile,*pTrackId,stNaluUnit.data,stNaluUnit.size);
				break;
			case 5:
			case 1:
				{
					if (*pTrackId == MP4_INVALID_TRACK_ID)
					{
						break;
					}
					int nDataLen = stNaluUnit.size + 4;
					unsigned char *data = (unsigned char *)malloc(nDataLen);
					data[0] = stNaluUnit.size >> 24;
					data[1] = stNaluUnit.size >> 16;
					data[2] = stNaluUnit.size >> 8;
					data[3] = stNaluUnit.size & 0xff;
					memcpy(data + 4, stNaluUnit.data, stNaluUnit.size);
					video_frame_cnt++;
					uint64_t dur = MP4_INVALID_DURATION;
					uint64_t diff_time = MyGetTickCount() - video_start_tick;
					if (diff_time) {
						float dwFrameRate = video_frame_cnt * 1000.0 / (float)diff_time;
						dur = 90000 / dwFrameRate;
					}
					if (!MP4WriteSample(hFile, *pTrackId, data, nDataLen, dur, 0, stNaluUnit.type == 5?1:0))
					{
						free(data);
						return 0;
					}
					free(data);

				}
				break;
			default :
				break;

		}
		nPos += nLen;

	}

	return 1;

}

int mp4MuxerAudioInit(int channels, long sample, int bits)
{
	if (!mp4_fd)
		return -1;
	unsigned char *pp = NULL;
	unsigned long pp_leng = 0;
	int mPCMBitSize = 16;
	audio_sample = sample;
	faac_fd = faacEncOpen(sample,channels,&inputSample, &maxOutputBytes);
	faacEncConfigurationPtr faac_cfg = faacEncGetCurrentConfiguration(faac_fd);
	faac_cfg->inputFormat = FAAC_INPUT_16BIT;
	faac_cfg->outputFormat = 0; /*0 - raw; 1 - ADTS*/
	faac_cfg->bitRate = sample;  //库内部默认为64000
	faac_cfg->useTns = 0;
	faac_cfg->allowMidside = 0;
	faac_cfg->shortctl = SHORTCTL_NORMAL;
	faac_cfg->aacObjectType = LOW;
	faac_cfg->mpegVersion = MPEG4;//MPEG2
	faacEncSetConfiguration(faac_fd,faac_cfg);
	faacEncGetDecoderSpecificInfo(faac_fd,&pp,&pp_leng);
	mPCMBufferSize = inputSample * mPCMBitSize / 8;
	aacData = (unsigned char *)malloc(maxOutputBytes);   //编码后输出数据（也就是AAC数据）存放位置
	pcm_buffer = (unsigned char *)malloc(mPCMBufferSize);   //编码后输出数据（也就是AAC数据）存放位置
	muxerQueueInit(1024 *10);
	audio = MP4AddAudioTrack(mp4_fd,sample,1024,MP4_MPEG4_AUDIO_TYPE);
	MP4SetAudioProfileLevel(mp4_fd,2);
	MP4SetTrackESConfiguration(mp4_fd, audio, pp, pp_leng);
	return 0;
}

int mp4MuxerInit(int width,int height,char *file_name)
{
	char *p[4] = {"isom","iso2","avc1","mp41"};
	mp4_fd = MP4CreateEx(file_name,
			0,1,1,"isom",0x00000200, p, 4);
	enc_info.u32Width = width;
	enc_info.u32Height = height;
	enc_info.u32FrameRate = 15;
	enc_info.u32Profile = 0x01;
	video_start_tick = 0;
	video_frame_cnt = 0;
	return 0;
}

int mp4MuxerAppendVideo(uint8_t* data, int size)
{
	if (!mp4_fd)
		return -1;
	mp4v2VideoWrite(mp4_fd,&video,data,size,&enc_info);
	return 0;
}

int mp4MuxerAppendAudio(uint8_t* data, int size_in)
{
	if (!mp4_fd)
		return -1;
	muxerQueueWrite(data,size_in);
	if (muxerQueueGetCurLeng() < mPCMBufferSize)
		return 0;
	while (muxerQueueRead(pcm_buffer,mPCMBufferSize) != 0) {
		int ret = faacEncEncode(
				faac_fd, (int*) pcm_buffer, mPCMBufferSize / 2, aacData, maxOutputBytes);
		if (ret > 0)
			MP4WriteSample(mp4_fd, audio, aacData, ret, MP4_INVALID_DURATION, 0, 1);
	}
	return 1;
}

int mp4MuxerStop(void)
{
	if (aacData)
		free(aacData);
	if (pcm_buffer)
		free(pcm_buffer);
	if (faac_fd)
		faacEncClose(faac_fd);
	MP4Close(mp4_fd, 0);
	video = MP4_INVALID_TRACK_ID;
	audio = MP4_INVALID_TRACK_ID;
	mp4_fd = NULL;
	muxerQueueUninit();

	video_start_tick = 0;
	video_frame_cnt = 0;
	return 0;
}

