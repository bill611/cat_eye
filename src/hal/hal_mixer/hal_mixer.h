/*
 * =============================================================================
 *
 *       Filename:  tinyplay.h
 *
 *    Description:  rv1108音频接口
 *
 *        Version:  1.0
 *        Created:  2019-06-26 15:44:41 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _TINYPLAY_H
#define _TINYPLAY_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	int rvMixerPlayOpen(int sample,int channle,int bit);
	void rvMixerPlayClose(void);
	int rvMixerPlayWrite(void *data,int size);
	void rvMixerPlayInit(void);

	int rvMixerCaptureOpen(void);
	void rvMixerCaptureClose(void);
	int rvMixerCaptureRead(void *data,int size);
	void rvMixerCaptureInit(void);

	int rvMixerSetPlayVolume(int value);
	int rvMixerSetCaptureVolume(int value);
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
