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

	// 硬件云协议
	typedef struct _ProtocolHardCloud {
		void (*getImei)(void (*callBack)(int result));
		int (*isNeedToUpdate)(char *version,char *content);
	}ProtocolHardCloud;
	extern ProtocolHardCloud *pro_hardcloud;
	void protocolInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
