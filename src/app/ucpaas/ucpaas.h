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

	typedef struct _Callbacks {
		void (*dialFail)(void *arg);
		void (*answer)(void *arg);
		void (*hangup)(void *arg);
		void (*dialRet)(void *arg);
		void (*incomingCall)(void *arg);
		void (*sendCmd)(void *arg);
		void (*receivedCmd)(const char *user_id,void *arg);
		void (*initAudio)(unsigned int rate,unsigned int bytes_per_sample,unsigned int channle);
		void (*startRecord)(unsigned int rate,unsigned int bytes_per_sample,unsigned int channle);
		void (*recording)(char *data,unsigned int size);
		void (*playAudio)(const char *data,unsigned int size);
	}Callbacks;

	void ucsDial(char *user_id);
	void ucsAnswer(void);
	void ucsHangup(void);
	void ucsSendCmd(char *cmd,char *user_id);
	void ucsSendVideo(const unsigned char* frameData, const unsigned int dataLen);
	int ucsConnect(char *user_token);
    void ucsDisconnect(void);
	void registUcpaas(Callbacks *interface);
	void ucsReceiveVideo(unsigned char* frameData,
			unsigned int *dataLen,
			long long *timeStamp,
			int *frameType);
	int ucsConnectState(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
