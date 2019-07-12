/*
 * =============================================================================
 *
 *       Filename:  Rtp.h
 *
 *    Description:  传输接口
 *
 *        Version:  1.0
 *        Created:  2016-03-01 14:28:41 
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * ============================================================================
 */
#ifndef _RTP_H
#define _RTP_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	struct _UdpTalkTransInterface;
	typedef struct _Rtp {
		
		int fpAudio;	//音频驱动打开句柄
		int bTransVideo;//是否传输视频
		int bTransAudio;//是否传输音频
		int silence;	//静音模式
		int frame_period;// 帧率
		int video_codec_type;	//视频编码类型

		struct _UdpTalkTransInterface *interface;

		int *(*getFpAudio)(struct _Rtp *);
		int (*setPeerIp)(struct _Rtp *,char *ip);
		void (*setSilence)(struct _Rtp *,int value);
		int (*getSilence)(struct _Rtp *);
		int (*init)(struct _Rtp *, const char *dest_ip, int Port);
		void (*buildConnect)(struct _Rtp *); 
		int (*getVideo)(struct _Rtp *,void *data); 
		void (*startAudio)(struct _Rtp *);
		void (*close)(struct _Rtp *);
		void (*Destroy)(struct _Rtp **);
	} Rtp;

	// 需要实现的接口
	typedef struct _UdpTalkTransInterface {
		void (*abortCallBack)(struct _Rtp *);
		void (*start)(struct _Rtp *);
		void (*receiveAudio)(struct _Rtp *,void *data,int size);
		void (*receiveEnd)(struct _Rtp *);
		void (*sendStart)(struct _Rtp *);
		void (*sendVideo)(struct _Rtp *,void *data,int size);
		int (*sendAudio)(struct _Rtp *,void *data,int size);
		void (*sendEnd)(struct _Rtp *);
	}UdpTalkTransInterface;

	Rtp * createRtp(UdpTalkTransInterface *,void *callBackData);
	extern Rtp * udp_talk_trans;
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
