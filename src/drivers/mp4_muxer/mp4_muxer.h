/*
 * =============================================================================
 *
 *       Filename:  MP4Muxer.h
 *
 *    Description:  MP4视频音频封装，利用ffmpeg库
 *
 *        Version:  1.0
 *        Created:  2019-09-12 09:30:11 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MP4_MUXER_H
#define _MP4_MUXER_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	int mp4MuxerAudioInit(int channels, long sample, int bits);
	int mp4MuxerInit(int width,int height,char *file_name);
	int mp4MuxerAppendVideo(uint8_t* data, int size);
	int mp4MuxerAppendAudio(uint8_t* data, int size);
	int mp4MuxerStop(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif
