/*
 * =============================================================================
 *
 *       Filename:  protocol_hardcloud.c
 *
 *    Description:  猫眼相关硬件云协议
 *
 *        Version:  1.0
 *        Created:  2019-05-18 13:53:49
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "my_http.h"
#include "my_mqtt.h"
#include "json_dec.h"
#include "thread_helper.h"
#include "protocol.h"
#include "config.h"
#include "my_ntp.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define TOPIC_NUM 6
enum {
	Sys_TestData = 1,			//	Server	Send	测试数据指令 服务端发送测试内容到客户端 客户端必须立即将收到测试内容的data原封Return回来
	Sys_UploadLog = 2,			//	Server	Post	上传日志指令 要求客户端将异常等日志到服务器
	Sys_Control = 3,			//	Server	Send	控制(重启、启用、禁用）)
	CE_PostAwaken = 6000,		//	Server	Post	无条件唤醒
	CE_SendAwaken = 6001,		//	Server	Send	无条件唤醒
	CE_SetSelfIntercom = 6002,  //	Server	Post	设置自身对讲账号
	CE_SetTargetsIntercom = 6003,//	Server	Post	设置对方对讲账号
	CE_GetIntercoms = 6004,		//	Client	Send	获取所有对讲账号信息
	CE_GetFaces = 6005,			//	Server	Send	获取所有人脸信息
	CE_SetFace = 6006,			//	Server	Send	设置人脸
	CE_RemoveFace = 6007,		//	Server	Send	删除人脸
	CE_Snap = 6008,				//	Server	Send	抓拍
	CE_GetConfig = 6009,		//	Server	Send	获取猫眼配置
	CE_SetConfig = 6010,		//	Server	Send	设置猫眼配置
};
struct OPTS {
	char client_id[32];
	char username[32];
	char password[32];
	char host[16];
	int  port;
	char pubTopic[64];
	char platformUrl[64];
	char ntp_server_ip[64];

};
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
Protocol*pro_hardcloud;
static MyHttp *http = NULL;
static MyMqtt *mqtt = NULL;

// 正式地址
// static char *hard_could_api = "https://iot.taichuan.net/v1/Mqtt/GetSevice?num=";

// test地址
static char *hard_could_api = "http://84.internal.taichuan.net:8080/v1/Mqtt/GetSevice?num=";

struct OPTS opts;
static char subTopic[TOPIC_NUM][100] = {{0,0}};
static int g_id = 0;

/* ---------------------------------------------------------------------------*/
/**
 * @brief getTimestamp 获取当前时间戳
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static time_t getTimestamp(void)
{
	time_t timep;
	struct tm *p;
	p = localtime(&timep);
	timep = mktime(p);
	return timep;
}
/* ---------------------------------------------------------------------------*/
/*
  * @brief judgeHead 初始化JSON数据，
  *
  * @param data 数据  type:0表示整型，1表示字符型
  *
  * @returns state 为 true 时返回初始化的json指针 state 为 false 时返回空指针
  */
/* ---------------------------------------------------------------------------*/
static CjsonDec * judgeHead(char *data, char *state, int type)
{
	CjsonDec *dec = cjsonDecCreate(data);
	char *buff = NULL;
	if (!dec) {
		printf("json dec create fail!\n");
		return NULL;
	}
	if(0 == type) {
		int status = dec->getValueInt(dec, state);
		if(0 != status) {
			dec->print(dec);
			dec->destroy(dec);
			return NULL;
		}
	} else if(1 == type) {
		dec->getValueChar(dec,state,&buff);
		if (NULL == buff) {
			printf("judgeHead getValueChar null!!\n");
			return NULL;
		}
	}
	return dec;
}

static void mqttSubcribeSuccess(void* context)
{
}
static void mqttSubcribeFailure(void* context)
{
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief mqttConnectSuccess 链接成功后订阅相关主题
 *
 * @param context
 */
/* ---------------------------------------------------------------------------*/
static void mqttConnectSuccess(void* context)
{
	int i;
	for (i=0; i<TOPIC_NUM; i++) {
		if (subTopic[i][0] != '\0')
			mqtt->subcribe(subTopic[i], mqttSubcribeSuccess, mqttSubcribeFailure);
	}
}
static void mqttConnectFailure(void* context)
{
	printf("[%s]\n", __func__);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief mqttConnectCallBack 硬件云消息回调
 *
 * @param context
 * @param topicName
 * @param topicLen
 * @param payload
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static int mqttConnectCallBack(void* context, char* topicName, int topicLen, void* payload)
{
	CjsonDec *dec = cjsonDecCreate((char*)payload);
	if (NULL == dec) {
       printf("dealWithSubscription cjsonDecCreate fail!\n");
	   return -1;
	}
	dec->print(dec);
	char send_buff[128] = {0};
	int id = dec->getValueInt(dec, "id");
	int mode = dec->getValueInt(dec, "mode");
	int api= dec->getValueInt(dec, "api");
	printf("api:%d\n", api);
	switch (api)
	{
		case Sys_TestData :
			break;
		case Sys_UploadLog :
			break;
		case Sys_Control :
			break;
		case CE_PostAwaken :
			break;
		case CE_SendAwaken :
			break;
		case CE_SetSelfIntercom :
			break;
		case CE_SetTargetsIntercom :
			break;
		case CE_GetIntercoms :
			break;
		case CE_GetFaces :
			break;
		case CE_SetFace :
			break;
		case CE_RemoveFace :
			break;
		case CE_Snap :
			break;
		case CE_GetConfig :
			break;
		case CE_SetConfig :
			break;
	}

	if(1 == mode) {  //服务端需要回复
		sprintf(send_buff, "{\"api\": 4,\"time\": %ld,\"mode\": 4,  \"id\": %d,  \"body\": true}",
				 getTimestamp(), id);
		printf("send_buff:%s\n", send_buff);
		if (mqtt->send(opts.pubTopic,strlen(send_buff),send_buff) == 0) {
			goto connect_end ;
		}
	}

connect_end:
	dec->destroy(dec); 
	return 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief getIntercoms 获取所有对讲账号
 */
/* ---------------------------------------------------------------------------*/
static void getIntercoms(void)
{
	char send_buff[128] = {0};
	sprintf(send_buff, "{\"api\": %d,\"time\": %ld,\"mode\": 1,  \"id\": %d,  \"body\": {}}",
			CE_GetIntercoms,getTimestamp(), ++g_id);
	printf("send_buff[%s]:%s\n",opts.pubTopic, send_buff);
	mqtt->send(opts.pubTopic,strlen(send_buff),send_buff);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief initThread 初始化链接硬件云，mqtt链接方式,线程执行，直到链接上后退出
 *
 * @param arg
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static void* initThread(void *arg)
{
	char mqtt_server_content[1024*5] = {0};
	char url[128] = {0};
	sprintf(url,"%s%s",hard_could_api,g_config.imei);
	while (1) {
		http->post(url,NULL,mqtt_server_content);

		CjsonDec *dec = NULL;
		char *buff = NULL;
		dec = judgeHead(mqtt_server_content,"code", 0);
		if (!dec) {
			printf("[%s,%d] judge head fail!\n", __func__,__LINE__);
			goto retry;
		}
		if(0 != dec->changeCurrentObj(dec,"data"))
			goto retry;

		dec->getValueChar(dec,"ip",&buff);

		if (!buff) {
			printf("get ip [ip] null!!\n");
			dec->destroy(dec);
			goto retry;
		}
		strcpy((char*)&opts.host, buff);
		free(buff);
		opts.port = dec->getValueInt(dec, "port");

		strcpy((char*)&opts.client_id, g_config.imei);
		strcpy((char*)&opts.username, g_config.imei);
		strcpy((char*)&opts.password, g_config.hardcode);

		if(0 != dec->changeCurrentObj(dec,"subTopics"))
			goto retry;
		int size = dec->getArraySize(dec);

		dec->print(dec);

		int i;

		for (i=0; i<size; i++) {
			// 进入数组项
			dec->getArrayChar(dec,i,NULL,&buff);
			if (NULL == buff) {
				printf("func:%s getArrayChar failed!!\n",__func__);
				size = 0;
				break;
			} else {
				strcpy(subTopic[i],buff);
				printf("index:%d subTopic:%s\n",i,subTopic[i]);
			}

			free(buff);
		}

		dec->changeObjFront(dec);
		dec->getValueChar(dec,"pubTopic",&buff);
		if(NULL != buff) {
			printf("publish topic %s\n", buff);
			strcpy((char*)&opts.pubTopic, buff);
			printf("opts publish Topic:%s\n",opts.pubTopic);
			free(buff);
		}

		dec->getValueChar(dec,"ntpServer",&buff);
		if(NULL != buff) {
			printf("ntpServer: %s\n", buff);
			strcpy(opts.ntp_server_ip, buff);
			free(buff);
		}

		dec->getValueChar(dec,"platformURL",&buff);
		if(NULL != buff) {
			printf("platformURL: %s\n", buff);
			strcpy((char*)&opts.platformUrl, buff);
			free(buff);
		}

		dec->destroy(dec);
		ntpTime(opts.ntp_server_ip);
		sprintf(url, "%s:%d", opts.host, opts.port);
		int ret = mqtt->connect(url,opts.client_id,
				10,
				opts.username,
				opts.password,
				mqttConnectCallBack,
				mqttConnectSuccess,
				mqttConnectFailure);
		if (ret < 0)
			goto retry;
		break;
retry:
		sleep(5);
	}
	sleep(1);
	getIntercoms();
	return NULL;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief registHardCloud 注册硬件云
 */
/* ---------------------------------------------------------------------------*/
void registHardCloud(void)
{
	http = myHttpCreate();
	mqtt = myMqttCreate();
	createThread(initThread,NULL);
}

