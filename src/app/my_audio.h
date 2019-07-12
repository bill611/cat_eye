/*
 * =============================================================================
 *
 *       Filename:  my_audio.h
 *
 *    Description:  播放相关音频文件
 *
 *        Version:  1.0
 *        Created:  2019-06-27 13:26:32 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MY_AUDIO_H
#define _MY_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	void myAudioPlayRecognizer(char *usr_name);
	void myAudioPlayRing(void);
	void myAudioStopPlay(void);
	void myAudioPlayDingdong(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
