/*
 * =============================================================================
 *
 *       Filename:  my_video.h
 *
 *    Description:  视频接口,所有功能控制接口
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
	enum HangupType{ // 挂机方式
		HANGUP_TYPE_BUTTON,   			// 手动点击挂机
		HANGUP_TYPE_OVERTIME_ANSWER, 	// 接听后超时挂机
		HANGUP_TYPE_OVERTIME_UNANSWER, 	// 未接听超时挂机
		HANGUP_TYPE_PEER, 	// 对方挂机
	};

	typedef struct _MyVideo {
		void (*showLocalVideo)(void);  // 显示本地视频
		void (*showPeerVideo)(void);  // 显示远程视频
		void (*hideVideo)(void);  // 隐藏视频
        int (*faceRegist)( unsigned char *image_buff,
                int w,int h,
                char *id,char *nick_name,char *url);// 注册人脸
        void (*faceDelete)(char *id); // 删除人脸
        int (*faceRecognizer)( unsigned char *image_buff,
                int w,int h,
                int *age,int *sex);// 识别人脸

		void (*capture)(int type,int count,char *nick_name,char *user_id);

		int (*recordStart)(int type); // 返回0启动失败 1启动成功
		void (*recordWriteCallback)(char *data,int size);
		void (*recordStop)(void);
		
		void (*videoCallOut)(char *user_id);
		void (*videoCallOutAll)(void);
		void (*videoCallIn)(char *user_id);
		void (*videoAnswer)(int dir,int dev_type); // dir 0本机接听 1对方接听
		void (*videoHangup)(enum HangupType hangup_type); // 0对方挂机 1本机挂机
		int (*videoGetCallTime)(void);
		int (*videoGetRecordTime)(void);

		int (*update)(int type,char *ip,int port,char *file_path); // 升级
		int (*delaySleepTime)(int type); // 延长睡眠时间0短 1长

		int (*isVideoOn)(void); // 是否在对讲过程，包括呼叫过程，对讲过程
		int (*isTalking)(void); // 是否正在通话
		void (*resetCallTime)(int); // 重置剩余通话时间
	}MyVideo;
	extern MyVideo *my_video;
	void myVideoInit(void);
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
