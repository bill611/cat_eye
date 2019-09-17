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

typedef struct _MP4ENC_INFO
{
	unsigned int u32FrameRate;
	unsigned int u32Width;
	unsigned int u32Height;
	unsigned int u32Profile;

}MP4ENC_INFO;

static MP4ENC_INFO enc_info;
static MP4FileHandle mp4_fd = NULL;
static MP4TrackId video = MP4_INVALID_TRACK_ID;
static MP4TrackId audio = MP4_INVALID_TRACK_ID;
static uint64_t video_start_tick = 0;
static uint64_t audio_start_tick = 0;
static int video_frame_cnt = 0;
static int audio_frame_cnt = 0;
static int audio_sample = 0;
static unsigned long inputSample = 0;        //输入样本大小，在打开编码器时会得到此值
static unsigned long maxOutputBytes = 0;  //最大输出，编码后的输出数据大小不会高于这个值，也是打开编码器时获得
static int mPCMBufferSize = 0;    //一帧PCM缓存大小
static int mCountSize = 0;           //计算缓存大小
static char* mPCMBuffer;           //PCM缓存

static faacEncHandle faac_fd = NULL;

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
	faac_cfg->allowMidside = 1;
	faac_cfg->shortctl = SHORTCTL_NORMAL;
	faac_cfg->aacObjectType = LOW;
	faac_cfg->mpegVersion = MPEG4;//MPEG2
	faacEncSetConfiguration(faac_fd,faac_cfg);
	faacEncGetDecoderSpecificInfo(faac_fd,&pp,&pp_leng);
	mPCMBufferSize = inputSample * mPCMBitSize / 8;
	mPCMBuffer = (char *) malloc (mPCMBufferSize);
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
	audio_start_tick = 0;
	video_frame_cnt = 0;
	audio_frame_cnt = 0;
	return 0;
}

int mp4MuxerAppendVideo(uint8_t* data, int size)
{
	if (!mp4_fd)
		return -1;
	mp4v2VideoWrite(mp4_fd,&video,data,size,&enc_info);
	return 0;
}
int mp4MuxerAppendAudio(uint8_t* data, int size)
{
	int ret = 0;
	if (!mp4_fd)
		return -1;
	audio_frame_cnt++;
	if (mCountSize<mPCMBufferSize) {
		memcpy(mPCMBuffer +mCountSize,data,size);
		mCountSize += size;
	} else {
		mCountSize = 0;   //缓存区已满，重置记数
		unsigned char* aacData = (unsigned char *)malloc(maxOutputBytes);   //编码后输出数据（也就是AAC数据）存放位置

		//开始编码，faac_fd为编码器句柄，mPCMBuffer为PCM数据，inputSample为打开编码器时得到的输入样本数据
		//         //aacData为编码后数据存放位置，maxOutputBytes为编码后最大输出字节数，ret为编码后数据长度
		ret = faacEncEncode(faac_fd, (int32_t *)mPCMBuffer, inputSample,
				aacData, maxOutputBytes);
		//ret为0时不代表编码失败，而是编码速度较慢，导致缓存还未完全flush，可用一个循环继续调用编码接口，当 ret>0 时表示编码成功，且返回值为编码后数据长度
		// while (ret == 0) {
			// ret = faacEncEncode(faac_fd, (int32_t *)mPCMBuffer, inputSample, aacData, maxOutputBytes);
		// }

		if (ret > 0) {
			if (audio_start_tick == 0)
				audio_start_tick = MyGetTickCount();
			uint64_t dur = MP4_INVALID_DURATION;
			uint64_t diff_time = MyGetTickCount() - audio_start_tick;
			if (diff_time) {
				float dwFrameRate = audio_frame_cnt * 1000.0 / (float)diff_time;
				dur = audio_sample / dwFrameRate;
			}
			MP4WriteSample(mp4_fd, audio, aacData, ret, dur, 0, 1);
			// printf("encode voice success !ret:%d\n",ret);
			//到这里已经编码成功，aacData为编码后数据
			//我是写入一个自定义AACData结构体并放入队列进行处理，这里的aacData可按情况自行处理（如直接写入文件）
			// MP4WriteSample(mp4_fd, audio, aacData, ret, 1024, 0, 1);
			// memcpy(out_data,aacData,ret);
		} else {
			printf("encode failed!!\n");
		}
		if (aacData)
			free(aacData);

	}
	return ret;
}

int mp4MuxerStop(void)
{
	if (faac_fd)
		faacEncClose(faac_fd);
	if (mPCMBuffer)
		free(mPCMBuffer);
	MP4Close(mp4_fd, 0);
	mp4_fd = NULL;

	video_start_tick = 0;
	audio_start_tick = 0;
	video_frame_cnt = 0;
	audio_frame_cnt = 0;
	return 0;
}

