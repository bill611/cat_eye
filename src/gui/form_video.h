/*
 * =============================================================================
 *
 *       Filename:  form_video.h
 *
 *    Description:  视频通话，录像等
 *
 *        Version:  1.0
 *        Created:  2019-04-30 14:16:25
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _VIDEO_H
#define _VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	enum {
		FORM_VIDEO_TYPE_CAPTURE, 	// 抓拍
		FORM_VIDEO_TYPE_RECORD,  	// 录像
		FORM_VIDEO_TYPE_TALK,  		// 门口机通话
		FORM_VIDEO_TYPE_MONITOR,  	// APP监视
		FORM_VIDEO_TYPE_OUTDOOR,  	// 监视门口
	};

	int createFormVideo(HWND hVideoWnd,int type,void (*callback)(void),int count);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
