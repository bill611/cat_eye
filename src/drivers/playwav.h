/*
 * =====================================================================================
 *
 *       Filename:  playwav.h
 *
 *    Description:  .wav文件播放驱动
 *
 *        Version:  1.0
 *        Created:  2015-11-24 14:23:39 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
#ifndef _PLAY_WAV_H
#define _PLAY_WAV_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	int playwavfile(char * file_name);
	void playWavStop();

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

