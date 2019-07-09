/*
 * =============================================================================
 *
 *       Filename:  protocol_video.h
 *
 *    Description:  太川对讲协议
 *
 *        Version:  1.0
 *        Created:  2018-12-13 14:21:32
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _VIDEO_PROTOCOL_H
#define _VIDEO_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>

#define COMM_TIME		3 	// 呼叫超时时间S 
#define SHAKE_TIME		6 	// 握手超时时间S
#define TALK_TIME		180 // 对讲时间
#define RING_TIME		31  // 响铃时间
#define LEAVE_WORD_TIME	11  // 留言时间

#define CALL_OK					0
#define CALL_NOHOST				-1
#define CALL_FAIL				-2
#define CALL_BUSY				-3
#define CALL_WAIT				-4

#define MAXFJCOUNT  6
#define FJ_REGIST_TIME  (65000)  // 分机注册时间间隔 64秒，兼容芯唐版本时间

	typedef enum _VideoProtoclType {
		VIDEOTRANS_PROTOCOL_3000, // 3000协议
		VIDEOTRANS_PROTOCOL_U9,   // U9协议
	}VideoProtoclType;

	typedef enum _VideoCallDir{
		VIDEOTRANS_CALL_DIR_IN, // 呼入
		VIDEOTRANS_CALL_DIR_OUT, // 呼出
	}VideoCallDir;

	typedef enum _VideoUiStatus{
		VIDEOTRANS_UI_NONE,				// 不做处理
		VIDEOTRANS_UI_SHAKEHANDS,		// 3000握手命令
		VIDEOTRANS_UI_CALLIP,			// 呼出
		VIDEOTRANS_UI_RING,				// 呼入响铃
		VIDEOTRANS_UI_LEAVE_WORD,		// 留言
		VIDEOTRANS_UI_RETCALL,			// 呼出到管理中心，室内机收到回应
		VIDEOTRANS_UI_RETCALL_MONITOR,	// 呼出到门口机，户门口机收到回应,显示开锁按钮
		VIDEOTRANS_UI_FAILCOMM,			// 通信异常
		VIDEOTRANS_UI_FAILSHAKEHANDS,	// 握手异常
		VIDEOTRANS_UI_FAILBUSY,			// 对方忙
		VIDEOTRANS_UI_FAILABORT,		// 突然中断
		VIDEOTRANS_UI_ANSWER,			// 本机接听
		VIDEOTRANS_UI_ANSWER_EX, 		// 分机接听
		VIDEOTRANS_UI_OVER,				// 挂机
	}VideoUiStatus;

	typedef enum _VideoProtocolStatus {
		VIDEOTRANS_STATUS_NULL,		// 不在通话或响铃状态
		VIDEOTRANS_STATUS_RING,		// 响铃状态，倒计时30秒
		VIDEOTRANS_STATUS_TALK,		// 通话状态，倒计时180秒
	}VideoProtocolStatus;


	struct _VideoPriv;
	struct _VideoInterface;
	typedef struct _VideoTrans {
		struct _VideoPriv *priv;
		struct _VideoInterface *interface;
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 使能通话对讲，（无机身码时不能对讲）
		 *
		 * @param 
		 */
		/* ---------------------------------------------------------------------------*/
		void (*enable)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief  呼叫
		 *
		 * @param
		 * @param ip 呼叫对方IP
		 */
		/* ---------------------------------------------------------------------------*/
		void (*call)(struct _VideoTrans *,char *ip);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 接听
		 *
		 * @param
		 */
		/* ---------------------------------------------------------------------------*/
		void (*answer)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 留言
		 *
		 * @param 
		 */
		/* ---------------------------------------------------------------------------*/
		void (*leaveWord)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 挂机
		 *
		 * @param
		 */
		/* ---------------------------------------------------------------------------*/
		void (*hangup)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 呼叫超时调用此接口
		 *
		 * @param 
		 */
		/* ---------------------------------------------------------------------------*/
        void (*callBackOverTime)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 通话中开锁
		 *
		 * @param
		 */
		/* ---------------------------------------------------------------------------*/
		void (*unlock)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief  获取通话方IP
		 *
		 * @param
		 *
		 * @returns 返回对方IP
		 */
		/* ---------------------------------------------------------------------------*/
		char *(*getPeerIP)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 获取通话端口
		 *
		 * @param
		 *
		 * @returns 返回端口
		 *
		 */
		/* ---------------------------------------------------------------------------*/
		int (*getTalkPort)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief  获得通话对方名称
		 *
		 * @param
		 *
		 * @returns 返回名称
		 */
		/* ---------------------------------------------------------------------------*/
		char *(*dispCallName)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 判断是否在空闲状态
		 *
		 * @param
		 *
		 * @returns 1空闲 0非空闲
		 */
		/* ---------------------------------------------------------------------------*/
		int (*getIdleStatus)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 判断是否在通话状态
		 *
		 * @param
		 *
		 * @returns  1通话 0非通话
		 */
		/* ---------------------------------------------------------------------------*/
		int (*getTalkStatus)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief  获取是否有未读留言
		 *
		 * @param 
		 *
		 * return  1有未读留言 0无未读留言
		 */
		/* ---------------------------------------------------------------------------*/
		int (*hasUnreadLeaveWord)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 清除未读留言
		 *
		 * @param 
		 */
		/* ---------------------------------------------------------------------------*/
		void (*clearUnreadLeaveWord)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 分机注册函数，接收其他分机向本机注册
		 *
		 * @param 
		 * @param ip 分机IP
		 * @param port 分机端口
		 * @param type 分机协议类型 TP_PHONE_REGISTER
		 * @param data 分机数据
		 */
		/* ---------------------------------------------------------------------------*/
		void (*registDev)(struct _VideoTrans *,char *ip,int port,int type,char *data); 
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief  获取已注册分机结构体的首地址，itesdk里重新定义结构体使用
		 *
		 * @param 
		 *
		 * @returns 分机结构体数组首地址
		 */
		/* ---------------------------------------------------------------------------*/
		void *(*getRegistDev)(struct _VideoTrans *); 
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief  获取指定下标分机的IP
		 *
		 * @param 
		 * @param index 指定下标
		 *
		 * @returns IP
		 */
		/* ---------------------------------------------------------------------------*/
		char *(*getRegistOneDevIp)(struct _VideoTrans *,int index); 
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 作为分机时，向主机注册地址，
		 *
		 * @param 
		 * @param type 注册分机的类型 TP_U9_REGISTER
		 */
		/* ---------------------------------------------------------------------------*/
		void (*registToMaster)(struct _VideoTrans *,int type); 
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief  获得当前通话倒计时显示
		 *
		 * @param
		 * @param disp_string 显示字符
		 * @returns 1成功 0失败
		 */
		/* ---------------------------------------------------------------------------*/
		VideoProtocolStatus (*dispCallTime)(struct _VideoTrans *,int *disp_time);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 切换协议时重新创建状态机线程
		 *
		 * @param type 协议类型
		 */
		/* ---------------------------------------------------------------------------*/
		void (*resetProtocol)(struct _VideoTrans *,VideoProtoclType type);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 接收通话对接协议
		 *
		 * @param 
		 * @param ip 对方IP
		 * @param port 对方端口
		 * @param data 协议数据
		 * @param size 数据大小
		 */
		/* ---------------------------------------------------------------------------*/
        void (*cmdHandle)(struct _VideoTrans *,
                char *ip,int port, char *data,int size);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 销毁通话功能
		 *
		 * @param
		 */
		/* ---------------------------------------------------------------------------*/
		void (*destroy)(struct _VideoTrans *);
	}VideoTrans;

	// 需要实现的接口
	typedef struct _VideoInterface {
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief uint64_t 获取系统时间tick
		 *
		 * @param 
		 */
		/* ---------------------------------------------------------------------------*/
		uint64_t (*getSystemTick)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief uint64_t 计算当前tick与上一次记录的差值
		 *
		 * @param 
		 * @param cur 当前tick
		 * @param old 上一次tick
		 *
		 * return 差值
		 */
		/* ---------------------------------------------------------------------------*/
		uint64_t (*getDiffSysTick)(struct _VideoTrans *,uint64_t cur,uint64_t old);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 使用UDP底层发送协议
		 *
		 * @param 
		 * @param ip 目的IP
		 * @param port 目的端口
		 * @param data 数据
		 * @param size 数据大小
		 * @param enable_call_back 是否使用回调函数
		 */
		/* ---------------------------------------------------------------------------*/
		void (*udpSend)(struct _VideoTrans *,
                char *ip,int port,void *data,int size,int enable_call_back);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 保存通话记录
		 *
		 * @param 
		 * @param dir VideoCallDir 呼入或者呼出
		 * @param name
		 * @param ip
		 */
		/* ---------------------------------------------------------------------------*/
		void (*saveRecordAsync)(struct _VideoTrans *,VideoCallDir dir,char *name,char *ip);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief  发送状态消息,用于UI提示当前状态及回调处理
		 *
		 * @param
		 * @param status VideoUiStatus
		 */
		/* ---------------------------------------------------------------------------*/
		void (*sendMessageStatus)(struct _VideoTrans *,VideoUiStatus status);

		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 显示呼叫窗口
		 *
		 * @param
		 */
		/* ---------------------------------------------------------------------------*/
		void (*showCallWindow)(struct _VideoTrans *);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief int 判断是否为管理中心Center,门口机Dmk,户门口机HDmk
		 *
		 * @param 
		 * @param ip 输入IP
		 * @return -1非判断目标，>=0目标下标,比如0,第0个管理中心等
		 */
		/* ---------------------------------------------------------------------------*/
		int (*isCenter)(struct _VideoTrans *,char *ip);
		int (*isDmk)(struct _VideoTrans *,char *ip);
		int (*isHDmk)(struct _VideoTrans *,char *ip);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief  取得门口机Dmk，户门口机HDmk名称
		 *
		 * @param 
		 * @param index 目标下标
		 *
		 * @returns 返回名称字符串
		 */
		/* ---------------------------------------------------------------------------*/
		char *(*getDmkName)(struct _VideoTrans *,int index);
		char *(*getHDmkName)(struct _VideoTrans *,int index);
	}VideoInterface;

	/* ---------------------------------------------------------------------------*/
	/**
	 * @brief videoTransCreate 创建对讲对象
	 *
	 * @param interface 需实现的接口函数
	 * @param port 端口
	 * @param call_cmd 呼叫命令 TP_CALL
	 * @param device_type 设备类型 TYPE_USER
	 * @param type 3000协议orU9协议
	 * @param master_ip 主机IP
	 * @param room_name 本机住户地址名称
	 * @param room_id 本机住户ID
	 *
	 * @returns 
	 */
	/* ---------------------------------------------------------------------------*/
	extern VideoTrans * videoTransCreate(VideoInterface *interface,
			int port,
			int call_cmd,
			int device_type,
			VideoProtoclType type,
			char *master_ip,
			char *room_name,
			char *room_id);

	extern VideoTrans *video;
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
