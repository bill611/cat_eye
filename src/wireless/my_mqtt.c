/*
 * =============================================================================
 *
 *       Filename:  my_mqtt.c
 *
 *    Description:  封装mqtt接口
 *
 *        Version:  1.0
 *        Created:  2019-05-21 11:33:50
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"
#include "my_mqtt.h"

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
// #define DBG_MQTT
#ifdef DBG_MQTT
    #define DBG_P( ...   ) \
    do { \
              printf("[MQTT DEBUG]"); \
        printf( __VA_ARGS__   );  \
    }while(0)
#else
    #define DBG_P( ...   )
#endif

#define TRUE 1
#define FALSE 0
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
MyMqtt *my_mqtt = NULL;

static MQTTAsync client;
static MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

static int (*mqttConnectCallBack)(void* context, char* topicName, int topicLen, void* payload);
static void (*mqttConnectSuccess)(void* context);
static void (*mqttConnectFailure)(void* context);

static void (*mqttSubcribeSuccess)(void* context);
static void (*mqttSubcribeFailure)(void* context);

static void connectionLost(void* context, char* cause)
{
	int rc = 0;

	DBG_P("%s()\n",__func__);
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
		DBG_P("connectionLost Failed to start connect, return code %d\n", rc);
	return;
}

static int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message)
{
	DBG_P("%s()\n",__func__);
	if (mqttConnectCallBack)
		mqttConnectCallBack(context,topicName,topicLen,message->payload);

	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);
	return TRUE;
}

static void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	DBG_P("%s()\n",__func__);
	if (mqttConnectFailure)
		mqttConnectFailure(context);
}


static void onConnect(void* context, MQTTAsync_successData* response)
{
	DBG_P("%s()\n",__func__);
	if (mqttConnectSuccess)
		mqttConnectSuccess(context);
}

static void onSubscribeSuccess(void* context,MQTTAsync_successData* response)
{
	DBG_P("%s()\n",__func__);
	if (mqttSubcribeSuccess)
		mqttSubcribeSuccess(context);
}

static void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	DBG_P("%s()\n",__func__);
	if (mqttSubcribeFailure)
		mqttSubcribeFailure(context);
}
static int send(char *pub_topic,int len,void *payload)
{
	saveLog("mqtt send:%s\n", (char *)payload);
	if (MQTTAsync_send(client, pub_topic, len, payload, 2, 0, NULL) == MQTTASYNC_SUCCESS) 
		return TRUE;
	else
		return FALSE;
}
static int subcribe(char* topic,
		void (*onSuccess)(void * context),
		void (*onFailure)(void *context)	)
{
	int rc;
	MQTTAsync_responseOptions ropts = MQTTAsync_responseOptions_initializer;

	mqttSubcribeSuccess = onSuccess;
	mqttSubcribeFailure = onFailure;
	ropts.onSuccess = onSubscribeSuccess;
	ropts.onFailure = onSubscribeFailure;
	ropts.context = client;
	if ((rc = MQTTAsync_subscribe(client, topic, 2, &ropts)) != MQTTASYNC_SUCCESS) {
		DBG_P("Failed to start subscribe:%s, return code %d\n", topic, rc);
		return FALSE;
	}
	return TRUE;
}

static int connect(char *url,char *client_id, int keepalive_interval,char *username,char *password,
		int (*callBack)(void* context, char* topicName, int topicLen, void* payload),
		void (*onSuccess)(void * context),
		void (*onFailure)(void *context)	)
{
	int rc = MQTTAsync_create(&client, url, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTASYNC_SUCCESS) {
		MQTTAsync_destroy(&client);
		return FALSE;
	}
	mqttConnectCallBack = callBack;
	mqttConnectSuccess = onSuccess;
	mqttConnectFailure = onFailure;
	MQTTAsync_setCallbacks(client, client, connectionLost, messageArrived, NULL);
	conn_opts.keepAliveInterval = keepalive_interval;
	conn_opts.cleansession = 0;
	conn_opts.username = username;
	conn_opts.password = password;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
	conn_opts.automaticReconnect = 1;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
		DBG_P("myconnect Failed to start connect, return code %d\n", rc);
		return FALSE;
	}
	return TRUE;
}

MyMqtt * myMqttCreate(void)
{
	// 只创建一次接口
	if (my_mqtt)
		return my_mqtt;
	my_mqtt = (MyMqtt *) calloc(1,sizeof(MyMqtt));
	my_mqtt->subcribe = subcribe;
	my_mqtt->connect = connect;
	my_mqtt->send = send;

	return my_mqtt;
}
