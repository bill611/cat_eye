/*
 * =============================================================================
 *
 *       Filename:  protocol.h
 *
 *    Description:  猫眼相关协议
 *
 *        Version:  1.0
 *        Created:  2019-05-18 13:54:36
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
#include <stdint.h>
#include "ipc_server.h"
	enum {
		IPC_DEV_TYPE_MAIN,
		IPC_DEV_TYPE_UART,
		IPC_DEV_TYPE_VIDEO,
	};
	// IPC通信消息
	enum {
		IPC_VIDEO_ON,				// 打开本地视频
		IPC_VIDEO_OFF,				// 关闭本地视频
		IPC_VIDEO_FACE_ON,			// 打开人脸识别
		IPC_VIDEO_FACE_OFF,			// 关闭人脸识别
		IPC_VIDEO_ENCODE_ON,		// 打开h264编码
		IPC_VIDEO_ENCODE_OFF,		// 关闭h264编码
		IPC_VIDEO_DECODE_ON,		// 打开h264解码
		IPC_VIDEO_DECODE_OFF,		// 关闭h264解码
		IPC_VIDEO_CAPTURE,			// 抓拍图片
		IPC_VIDEO_CAPTURE_END,		// 抓拍图片结束
		IPC_VIDEO_RECORD_START,		// 开始录像
		IPC_VIDEO_RECORD_STOP,		// 停止录像
		IPC_UART_SLEEP,				// 进入休眠
		IPC_UART_KEY_POWER,			// 室内机电源按键短按
		IPC_UART_KEYHOME,			// 室内机按键触发
		IPC_UART_DOORBELL,			// 门铃按键触发
		IPC_UART_PIR,				// 门外pir触发
		IPC_UART_WIFI_WAKE,			// wifi唤醒触发
		IPC_UART_WIFI_RESET,		// wifi复位
		IPC_UART_POWEROFF,			// 室内机电源键长按关机
		IPC_UART_REMOVE_CAP,		// 当识别到为熟人后删除门铃拍照
		IPC_UART_CAPTURE,			// 开机前抓拍图片
		IPC_UART_GETVERSION,		// 获取单片机版本信息
	};
	enum {
		PROTOCOL_TALK_LAN,  // 局域网对讲
		PROTOCOL_TALK_CLOUD, // 云对讲
	};

	enum {
		USER_TYPE_CATEYE,
		USER_TYPE_OTHERS,
	};
	enum {
		DEV_TYPE_UNDEFINED = 0, //未定义
		DEV_TYPE_ENTRANCEMACHINE = 1, //智能门禁
		DEV_TYPE_HOUSEHOLDAPP = 2, //住户手机APP
		DEV_TYPE_INNERDOORMACHINE = 3, //室内主机
		DEV_TYPE_SECURITYSTAFFAPP = 4, //保安手机APP
		DEV_TYPE_HOUSEENTRANCEMACHINE = 5, //户门口机
	};

	enum {
		CAP_TYPE_FORMMAIN = 0,	// 主界面按抓拍键
		CAP_TYPE_TALK,			// 通话时按抓拍
		CAP_TYPE_ALARM,			// 徘徊报警抓拍
		CAP_TYPE_FACE,          // 人脸识别抓拍
		CAP_TYPE_DOORBELL,      // 进入主程序后抓拍
	};

	typedef enum _AlarmType{
		ALARM_TYPE_LOWPOWER,	// 低电量报警
		ALARM_TYPE_PEOPLES,		// 陌生人徘徊报警
	}AlarmType;

	typedef enum _CallDir{
		CALL_DIR_IN,	// 呼入
		CALL_DIR_OUT,	// 呼出
	}CalLDir;

	typedef struct _ReportAlarmData{
		char date[32];		// 日期
		AlarmType type;		// 报警类型
		int has_people;		// 徘徊报警时，判断是否有人
		uint64_t picture_id;// 徘徊报警时，抓拍图片
		int age;		// 年龄
		int sex;		// 性别
	}ReportAlarmData;

	typedef struct _ReportFaceData{
		char date[32];		// 日期
		uint64_t picture_id;// 识别人脸时，抓拍图片
		char nick_name[32];		// 名称
		char user_id[32];		// face_id
	}ReportFaceData;

	typedef struct _ReportTalkData{
		char date[32];		// 日期
		uint64_t picture_id;// 抓拍图片或录像
		CalLDir call_dir;	// 0呼入 1呼出
		int answered;		// true 接听 false未接听
		int talk_time;		// 通话时间
		char nick_name[32];		// 通话人
	}ReportTalkData;

	typedef struct _UpLoadData{
		char file_path[64];	// 图片本地路径
		uint64_t picture_id;// 徘徊报警时，抓拍图片
	}UpLoadData;

	typedef struct _IpcData {
		int cmd;
		int dev_type;
		int leng;
		int count;
		char s_version[3];
		int need_ring; // 主程序是否需要响铃声,0否，1是
        union {
            char array_buf[128];
			char cap_path[128];
			struct {
				char path[64];
				char date[32];	
				char name[32];	
			}file;
        }data;
	}IpcData;

	typedef struct _UserStruct {
		char id[32];
		char nick_name[32];
		char token[256];
		int scope;
	}UserStruct;

	// 局域网协议
	struct _ProtocolPriv;
	typedef struct _Protocol {
		struct _ProtocolPriv *priv;
		void (*getImei)(void (*callBack)(int result));
		int (*isNeedToUpdate)(char *version,char *content);
	}Protocol;
	extern Protocol *protocol;

	// 对讲协议
	typedef struct _ProtocolTalk {
		int type;			// 0,3000局域网对讲协议，非0其他对讲协议
		void (*reload)(void);
		void (*dial)(char *user_id,void (*callBack)(int result));
		void (*answer)(void);
		void (*hangup)(int need_transfer); // 0不需要转呼APP，1需要转呼APP
		void (*connect)(void);
		void (*reconnect)(void);
		void (*unlock)(void);

		void (*uiShowFormVideo)(int type,char *name,int dir);
		void (*uiHangup)(void);
		void (*uiAnswer)(char *name);

		void (*sendCmd)(char *cmd,char *user_id);
		void (*receivedCmd)(const char *user_id,void *arg);
		void (*sendVideo)(void *data,int size);
		void (*receiveVideo)(void *data,int *size);
		void (*initAudio)(void);
		void (*playAudio)(const char *data,unsigned int size);
		void (*startRecord)(void);
		void (*recording)(char *data,unsigned int size);
		void (*playVideo)(const unsigned char* frame_data, const unsigned int data_len);

		void (*udpCmd)(char *ip,int port, char *data,int size);
	}ProtocolTalk;
	extern ProtocolTalk *protocol_talk;

	// 平台协议
	typedef struct _ProtocolHardcloud {
		void (*uploadPic)(char *path,uint64_t pic_id);
		void (*reportCapture)(uint64_t pic_id);
		void (*reportAlarm)(ReportAlarmData *data);
		void (*reportFace)(ReportFaceData *data);
		void (*reportTalk)(ReportTalkData *data);
		void (*enableSleepMpde)(void);
	}ProtocolHardcloud;
	extern ProtocolHardcloud *protocol_hardcloud;

	// 单片机协议
	typedef struct _ProtocolSinglechip {
		void (*deal)(IpcData *ipc_data);	// 处理单片机协议
		void (*cmdSleep)(void);				// 发送进入睡眠模式
		void (*cmdPowerOff)(void);			// 发送进入关机
		void (*cmdWifiReset)(void);			// 复位wifi
		void (*cmdGetVersion)(void);		// 获取单片机版本号
		void (*hasPeople)(char *nick_name,char *user_id);// 人脸识别到熟人
	}ProtocolSinglechip;
	extern ProtocolSinglechip *protocol_singlechip;

	void protocolInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
