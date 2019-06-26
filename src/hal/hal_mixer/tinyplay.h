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

	int rvMixerOpen(int sample,int channle,int bit);
	void rvMixerClose(void);
	int rvMixerWrite(void *data,int size);
	void rvMixerInit(void);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
