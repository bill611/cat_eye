#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mp4v2/mp4v2.h"
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
	audio_sample = sample;
	audio = MP4AddAudioTrack(mp4_fd,sample,sample/15,MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE);
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
	if (!mp4_fd)
		return -1;
	if (audio_start_tick == 0)
		audio_start_tick = MyGetTickCount();
	audio_frame_cnt++;
	uint64_t dur = MP4_INVALID_DURATION;
	uint64_t diff_time = MyGetTickCount() - video_start_tick;
	if (diff_time) {
		float dwFrameRate = audio_frame_cnt * 1000.0 / (float)diff_time;
		dur = audio_sample / dwFrameRate;
	}
	MP4WriteSample(mp4_fd, audio, data, size, dur, 0, 1);
	return 0;	
}

int mp4MuxerStop(void)
{
	MP4Close(mp4_fd, 0);
	mp4_fd = NULL;

	video_start_tick = 0;
	audio_start_tick = 0;
	video_frame_cnt = 0;
	audio_frame_cnt = 0;
	return 0;
}

