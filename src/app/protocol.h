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
		void (*dial)(char *user_id);
		void (*answer)(void);
		void (*hangup)(void);
		void (*connect)(void);
		void (*cbDialRet)(void (*callBack)(void *arg));

		void (*uiIncomingCall)(void *arg);
		void (*uiHangup)(void);

		void (*sendCmd)(char *cmd,char *user_id,void (*callBack)(void *arg));
		void (*receivedCmd)(void (*callBack)(const char *user_id,void *arg));
		void (*initAudio)(void (*callBack)(void));
		void (*playAudio)(void (*callBack)(const char *data,unsigned int size));
		void (*startRecord)(void (*callBack)(void));
		void (*recording)(void (*callBack)(char *data,unsigned int size));
		void (*playVideo)(const unsigned char* frame_data, const unsigned int data_len);
	}ProtocolTalk;
	extern ProtocolTalk *protocol_talk;

	void protocolInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
