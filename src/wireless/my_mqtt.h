/*
 * =============================================================================
 *
 *       Filename:  my_mqtt.h
 *
 *    Description:  封装mqtt接口
 *
 *        Version:  1.0
 *        Created:  2019-05-21 11:34:04 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MY_MQTT_H
#define _MY_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	typedef struct _MyMqtt {
		int (*send)(char *pub_topic,int len,void *payload);		
		int (*subcribe)(char* topic,
				void (*onSuccess)(void * context),
				void (*onFailure)(void *context)	);
		int (*connect)(char *url,char *client_id, int keepalive_interval,char *username,char *password,
				int (*callBack)(void* context, char* topicName, int topicLen, void* payload),
				void (*onSuccess)(void * context),
				void (*onFailure)(void *context)	);
	}MyMqtt;

	MyMqtt * myMqttCreate(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
