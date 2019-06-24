/*
 * =============================================================================
 *
 *       Filename:  ucpaas.h
 *
 *    Description:  云之讯对讲接口
 *
 *        Version:  1.0
 *        Created:  2019-06-05 17:28:39 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _UCPAAS_SDK_H
#define _UCPAAS_SDK_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	void ucsDial(char *user_id,void (*callBack)(void *arg));
	void ucsAnswer(void (*callBack)(void *arg));
	void ucsHangup(void (*callBack)(void *arg));
	void ucsCbDialRet(void (*callBack)(void *arg));
	void ucsCbIncomingCall(void (*callBack)(void *arg));
	void ucsSendCmd(char *cmd,char *user_id,void (*callBack)(void *arg));
	void ucsCbReceivedCmd(void (*callBack)(const char *user_id,void *arg));
	void ucsCbInitAudio(void (*callBack)(void));
	void ucsCbPlayAudio(void (*callBack)(const char *data,unsigned int size));
	void ucsCbStartRecord(void (*callBack)(void));
	void ucsCbRecording(void (*callBack)(char *data,unsigned int size));
	void ucsSendVideo(const unsigned char* frameData, const unsigned int dataLen);
	void ucsConnect(char *user_token);
	void registUcpaas(void);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
