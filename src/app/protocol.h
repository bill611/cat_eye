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
		void (*reload)(void);
		void (*dial)(int type,char *user_id,char *ui_title);
		void (*answer)(char *ui_title);
		void (*hangup)(void);
		void (*connect)(void);
		void (*reconnect)(void);

		void (*uiShowFormVideo)(int type,char *name);
		void (*uiHangup)(void);
		void (*uiAnswer)(char *name);

		void (*sendCmd)(char *cmd,char *user_id);
		void (*receivedCmd)(const char *user_id,void *arg);
		void (*sendVideo)(void *data,int size);
		void (*initAudio)(void);
		void (*playAudio)(const char *data,unsigned int size);
		void (*startRecord)(void);
		void (*recording)(char *data,unsigned int size);
		void (*playVideo)(const unsigned char* frame_data, const unsigned int data_len);
	}ProtocolTalk;
	extern ProtocolTalk *protocol_talk;

	void protocolInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
