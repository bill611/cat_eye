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
	enum {
		IPC_VIDEO_ON,
		IPC_VIDEO_OFF,
		IPC_VIDEO_FACE_ON,
		IPC_VIDEO_FACE_OFF,
		IPC_VIDEO_ENCODE_ON,
		IPC_VIDEO_ENCODE_OFF,
		IPC_VIDEO_DECODE_ON,
		IPC_VIDEO_DECODE_OFF,
		IPC_VIDEO_CAPTURE,
		IPC_VIDEO_RECORD_START,
		IPC_VIDEO_RECORD_STOP,
		IPC_UART_SLEEP,
		IPC_UART_POWER,
		IPC_UART_KEYHOME,
		IPC_UART_DOORBELL,
		IPC_UART_PIR,
	};
	enum {
		PROTOCOL_TALK_3000,
		PROTOCOL_TALK_OTHER,
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
		CAP_TYPE_FORMMAIN = 0,
		CAP_TYPE_TALK,
		CAP_TYPE_ALARM,
		CAP_TYPE_FACE,
	};

	enum {
		ALARM_TYPE_LOWPOWER,
		ALARM_TYPE_PEOPLES,
	};

	typedef struct _IpcData {
		int cmd;
		int dev_type;
		int leng;
        union {
            char array_buf[64];
            char cap_path[64];
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
		void (*hangup)(void);
		void (*connect)(void);
		void (*reconnect)(void);
		void (*unlock)(void);

		void (*uiShowFormVideo)(int type,char *name);
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

	// 对讲协议
	typedef struct _ProtocolHardcloud {
		void (*uploadPic)(void);
		void (*reportCapture)(uint64_t pic_id);
		void (*reportLowPower)(char *date);
	}ProtocolHardcloud;
	extern ProtocolHardcloud *protocol_hardcloud;

	// 单片机协议
	typedef struct _ProtocolSinglechip {
		void (*deal)(int cmd,char *data,int size);
		void (*cmdSleep)(void);
	}ProtocolSinglechip;
	extern ProtocolSinglechip *protocol_singlechip;

	void protocolInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
