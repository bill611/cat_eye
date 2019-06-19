/*
 * =============================================================================
 *
 *       Filename:  my_video.h
 *
 *    Description:  视频接口
 *
 *        Version:  1.0
 *        Created:  2019-06-19 10:20:06 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MY_VIDEO_H
#define _MY_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	typedef struct _MyVideo {
		void (*init)(void);
		void (*start)(void);
		void (*stop)(void);
		void (*capture)(int count);
	}MyVideo;
	extern MyVideo *my_video;
	void myVideoInit(void);
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
