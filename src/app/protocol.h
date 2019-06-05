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

	// 局域网协议
	struct _ProtocolPriv;
	typedef struct _Protocol {
		struct _ProtocolPriv *priv;
		void (*getImei)(void (*callBack)(int result));
		int (*isNeedToUpdate)(char *version,char *content);
	}Protocol;
	extern Protocol *protocol;

	// 对讲协议
	struct _ProtocolTalkPriv;
	typedef struct _ProtocolTalk {
		struct _ProtocolTalkPriv *priv;
		void (*dial)(char *user_id,void (*callBack)(void *arg));
		void (*answer)(void (*callBack)(void *arg));
		void (*hangup)(void (*callBack)(void *arg));
	}ProtocolTalk;
	extern ProtocolTalk *protocol_talk;

	void protocolInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
