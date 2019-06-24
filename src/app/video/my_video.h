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
		void (*showVideo)(void);  // 显示视频
		void (*hideVideo)(void);  // 隐藏视频
        void (*faceStart)(void);  // 开启人脸识别功能
        int (*faceRegist)( unsigned char *image_buff,
                int w,int h,
                char *id,char *nick_name,char *url);// 注册人脸
        void (*faceDelete)(char *id); // 删除人脸
		void (*capture)(int count);
		void (*recordStart)(int count);
		void (*recordStop)(void);
	}MyVideo;
	extern MyVideo *my_video;
	void myVideoInit(void);
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
