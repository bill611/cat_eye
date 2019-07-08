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
#include <sys/time.h>
#include "cJSON.h"
#include "tcp_client.h"
#include "my_http.h"
#include "my_mqtt.h"
#include "json_dec.h"
#include "sql_handle.h"
#include "thread_helper.h"
#include "config.h"
#include "my_ntp.h"
#include "externfunc.h"
#include "my_dns.h"
#include "my_video.h"
#include "jpeg_enc_dec.h"
#include "debug.h"
#include "protocol.h"

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
// 正式地址
// #define HARD_COULD_API "http://iot.taichuan.net/v1/Mqtt"
#define HARD_COULD_API "http://84.internal.taichuan.net:8080/v1"
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
	CE_Report = 6011,			//	Client	Post	猫眼数据上报
	CE_Reset = 6012,			//	Server	Post	重置猫眼配置与数据
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
	char service_host[64];
	int service_port;
};

struct DebugInfo{
	int cmd;
	char *info;
};
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
Protocol*pro_hardcloud;
static MyHttp *http = NULL;
static MyMqtt *mqtt = NULL;
static int mqtt_connect_state,ntp_connect_state;
static struct DebugInfo dbg_info[] = {
	{Sys_TestData,"Sys_TestData"},
	{Sys_UploadLog,"Sys_UploadLog"},
	{Sys_Control,"Sys_Control"},
	{CE_PostAwaken,"CE_PostAwaken"},
	{CE_SendAwaken,"CE_SendAwaken"},
	{CE_SetSelfIntercom,"CE_SetSelfIntercom"},
	{CE_SetTargetsIntercom,"CE_SetTargetsIntercom"},
	{CE_GetIntercoms,"CE_GetIntercoms"},
	{CE_GetFaces,"CE_GetFaces"},
	{CE_SetFace,"CE_SetFace"},
	{CE_RemoveFace,"CE_RemoveFace"},
	{CE_Snap,"CE_Snap"},
	{CE_GetConfig,"CE_GetConfig"},
	{CE_SetConfig,"CE_SetConfig"},
	{CE_Report,"CE_Report"},
	{CE_Reset,"CE_Reset"},
};

struct OPTS opts;
static char subTopic[TOPIC_NUM][100] = {{0,0}};
static int g_id = 0;
static char *qiniu_server_token = NULL;

static void printProInfo(int cmd)
{
	int i;
	for (i=0; i<sizeof(dbg_info)/sizeof(struct DebugInfo); i++) {
		if (cmd == dbg_info[i].cmd) {
			printf("[%d,%s]\n", cmd,dbg_info[i].info);
			return;
		}
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief getTimestamp 获取当前时间戳
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static time_t getTimestamp(void)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_sec;
}
static cJSON * packData(int api,int mode,int id)
{
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root,"api",api);
	cJSON_AddNumberToObject(root,"time",getTimestamp());
	cJSON_AddNumberToObject(root,"mode",mode);
	cJSON_AddNumberToObject(root,"id",id);
	return root;
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
			dec->destroy(dec);
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
	ntp_connect_state = 1;
}
static void mqttConnectFailure(void* context)
{
	printf("[%s]\n", __func__);
}

static void sysTestData(int api,int id,CjsonDec *dec)
{
	char *send_buff;
	if(dec->changeCurrentObj(dec,"body")) {
		printf("change body fail\n");
		return;
	}
	cJSON *data = dec->getValueObject(dec,"data");
	cJSON *root = packData(api,4,id);
	cJSON_AddItemToObject(root,"body",data);
	send_buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mqtt->send(opts.pubTopic,strlen(send_buff),send_buff);
	free(send_buff);

}

/* ---------------------------------------------------------------------------*/
/**
 * @brief ceGetIntercoms 获取对讲信息
 *
 * @param dec
 */
/* ---------------------------------------------------------------------------*/
static void ceGetIntercoms(CjsonDec *dec)
{
	if(dec->changeCurrentObj(dec,"body")) {
		printf("change body fail\n");
		return;
	}
	// 保存本机对讲信息
	if(dec->changeCurrentObj(dec,"self")) {
		printf("change self fail\n");
		return;
	}
	char *user_id = NULL;
	char *login_token = NULL;
	char *nick_name = NULL;
	int scope = 0 ;
	dec->getValueChar(dec,"userId",&user_id);
	dec->getValueChar(dec,"loginToken",&login_token);
	dec->getValueChar(dec,"nickName",&nick_name);
	scope = dec->getValueInt(dec,"scope");
	sqlClearDevice();
	sqlInsertUserInfo(user_id,login_token,nick_name,USER_TYPE_CATEYE,scope);
	if (user_id)
		free(user_id);
	if (login_token)
		free(login_token);
	if (nick_name)
		free(nick_name);
	// 保存对方对讲信息
	dec->changeObjFront(dec); // 返回上一层
	if(dec->changeCurrentObj(dec,"targets")) {
		printf("targets self fail\n");
		return;
	}
	int size = dec->getArraySize(dec);
	int i;
	for (i=0; i<size; i++) {
		user_id = NULL;
		nick_name = NULL;
		scope = 0 ;
		dec->getArrayChar(dec,i,"userId",&user_id);
		dec->getArrayChar(dec,i,"nickName",&nick_name);
		scope = dec->getArrayInt(dec,i,"scope");
		sqlInsertUserInfoNoBack(user_id,NULL,nick_name,USER_TYPE_OTHERS,scope);
		if (user_id)
			free(user_id);
		if (nick_name)
			free(nick_name);
	}
	sqlCheckBack();

	struct tm *tm_now = getTime();
	g_config.timestamp = tm_now->tm_hour + tm_now->tm_mday * 24 + tm_now->tm_mon * 30 * 24;
	ConfigSavePrivate();
	protocol_talk->reload();
	protocol_talk->reconnect();
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief ceGetFaces 获取人脸信息
 *
 * @param api
 * @param id
 * @param dec
 */
/* ---------------------------------------------------------------------------*/
static void ceGetFaces(int api,int id,CjsonDec *dec)
{
	char *send_buff;
	char user_id[32];
	char nick_name[128];
	char url[256];
	cJSON *root = packData(api,4,id);
	cJSON *arry = cJSON_CreateArray();
	sqlGetFaceStart();
	while (1) {
		memset(user_id,0,sizeof(user_id));
		memset(nick_name,0,sizeof(nick_name));
		memset(url,0,sizeof(url));
		int ret = sqlGetFace(user_id,nick_name,url,NULL);
		if (ret == 0)
			break;
		printf("id:%s,name:%s,url:%s\n", user_id,nick_name,url);
		cJSON *obj = cJSON_CreateObject();
		cJSON_AddStringToObject(obj,"id",user_id);
		cJSON_AddStringToObject(obj,"nickName",nick_name);
		cJSON_AddStringToObject(obj,"fileURL",url);
		cJSON_AddItemToArray(arry, obj);
	};
	sqlGetFaceEnd();

	cJSON_AddItemToObject(root,"body",arry);
	send_buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mqtt->send(opts.pubTopic,strlen(send_buff),send_buff);
	free(send_buff);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief ceSetFace 设置人脸信息
 *
 * @param api
 * @param id
 * @param dec
 */
/* ---------------------------------------------------------------------------*/
static void ceSetFace(int api,int id,CjsonDec *dec)
{
	char *send_buff;
	cJSON *root;
	int result = 0;

	if(dec->changeCurrentObj(dec,"body")) {
		printf("change body fail\n");
		goto send_return;
	}
	char *user_id = NULL;
	char *nick_name = NULL;
	char *url = NULL;
	int scope = 0 ;
	dec->getValueChar(dec,"fileURL",&url);
	dec->getValueChar(dec,"id",&user_id);
	dec->getValueChar(dec,"nickName",&nick_name);
	if (url) {
		int w,h;
		int yuv_len = 0;
		char *buff_img = NULL;
		unsigned char *yuv = NULL;
		int leng = http->post(url,NULL,&buff_img);
		jpegToYuv420sp((unsigned char *)buff_img, leng,&w,&h, &yuv, &yuv_len);
		if (my_video)
			if (my_video->faceRegist(yuv,w,h,user_id,nick_name,url) == 0)
				result = 1;
		if (buff_img)
			free(buff_img);
		if (yuv)
			free(yuv);
	}
    if (url)
		free(url);
	if (user_id)
		free(user_id);
	if (nick_name)
		free(nick_name);

send_return:
	root = packData(api,4,id);
	if (result)
		cJSON_AddStringToObject(root,"body","true");
	else
		cJSON_AddStringToObject(root,"body","false");
	send_buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mqtt->send(opts.pubTopic,strlen(send_buff),send_buff);
	free(send_buff);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief ceRemoveFace 删除人脸信息
 *
 * @param api
 * @param id
 * @param dec
 */
/* ---------------------------------------------------------------------------*/
static void ceRemoveFace(int api,int id,CjsonDec *dec)
{
	char *send_buff;
	int result = 0;
	cJSON *root;
	if(dec->changeCurrentObj(dec,"body")) {
		printf("change body fail\n");
		goto send_return;
	}
	char *user_id = NULL;
	dec->getValueChar(dec,"id",&user_id);
	if (user_id) {
		if (my_video)
			my_video->faceDelete(user_id);
		free(user_id);
	}
	result = 1;
send_return:
	root = packData(api,4,id);
	if (result)
		cJSON_AddStringToObject(root,"body","true");
	else
		cJSON_AddStringToObject(root,"body","false");
	send_buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mqtt->send(opts.pubTopic,strlen(send_buff),send_buff);
	free(send_buff);
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
	printProInfo(api);
	switch (api)
	{
		case Sys_TestData :
			sysTestData(api,id,dec);
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
			ceGetIntercoms(dec);
			break;
		case CE_GetFaces :
			ceGetFaces(api,id,dec);
			break;
		case CE_SetFace :
			ceSetFace(api,id,dec);
			break;
		case CE_RemoveFace :
			ceRemoveFace(api,id,dec);
			break;
		case CE_Snap :
			break;
		case CE_GetConfig :
			break;
		case CE_SetConfig :
			break;
		case CE_Report :
			break;
		case CE_Reset :
			break;
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
	struct tm *tm_now = getTime();
	int timestamp_now = tm_now->tm_hour + tm_now->tm_mday * 24 + tm_now->tm_mon * 30 * 24;
	printf("timestamp :now:%d,old:%d,div:%d\n",
		 timestamp_now, g_config.timestamp,timestamp_now - g_config.timestamp);
	if (timestamp_now - g_config.timestamp <= 12) {
		return;
	}
	char *send_buff;
	cJSON *root = packData(CE_GetIntercoms,1,++g_id);
	cJSON_AddStringToObject(root,"body","");
	send_buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mqtt->send(opts.pubTopic,strlen(send_buff),send_buff);
	free(send_buff);
}

static void ntpGetTimeCallback(void)
{
	mqtt_connect_state = 1;
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
	char *mqtt_server_content = NULL;
	char url[128] = {0};
	sprintf(url,"%s/Mqtt/GetSevice?num=%s",HARD_COULD_API,g_config.imei);
	while (1) {
		if (mqtt_server_content) {
			free(mqtt_server_content);
			mqtt_server_content = NULL;
		}
		http->post(url,NULL,&mqtt_server_content);

		CjsonDec *dec = NULL;
		char *buff = NULL;
		// printf("[%s]\n",mqtt_server_content );
		dec = judgeHead(mqtt_server_content,"code", 0);
		if (!dec) {
			printf("[%s,%d] judge head fail!\n", __func__,__LINE__);
			goto retry;
		}
		if(0 != dec->changeCurrentObj(dec,"data"))
			goto retry;

		dec->print(dec);
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
		if(0 != dec->changeCurrentObj(dec,"catEyeService"))
			goto retry;
		dec->getValueChar(dec,"host",&buff);
		if(NULL != buff) {
			printf("service_host: %s\n", buff);
			strcpy((char*)&opts.service_host, buff);
			free(buff);
		}
		opts.service_port = dec->getValueInt(dec,"port");
		printf("service_port: %d\n", opts.service_port);

		ntpTime(opts.ntp_server_ip,ntpGetTimeCallback);
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
		if (dec)
			dec->destroy(dec);
		break;
retry:
		if (dec)
			dec->destroy(dec);
		sleep(5);
	}
	if (mqtt_server_content) {
		free(mqtt_server_content);
		mqtt_server_content = NULL;
	}

	while (1) {
		char *qiniu_server= NULL;
		char url[128] = {0};
		CjsonDec *dec = NULL;
		sprintf(url,"%s/Storage/GetUploadToken?num=%s&code=%s&bucketName=%s",
				HARD_COULD_API,
				g_config.imei,
				g_config.hardcode,
				"tc-cat_eye");
		int ret = http->post(url,NULL,&qiniu_server);
		printf("[%d]%s\n",ret,url );
		if (ret == 0)
			goto retry_qiniu;

		dec = cjsonDecCreate(qiniu_server);
		if (!dec) {
			printf("[%s,%d] get data fail!\n", __func__,__LINE__);
			goto retry_qiniu;
		}
		dec->getValueChar(dec,"data",&qiniu_server_token);
		if (!qiniu_server_token) {
			printf("get qiniu token null!!\n");
			dec->destroy(dec);
			goto retry_qiniu;
		}
		printf("qiniu token:%s\n", qiniu_server_token);
		if (qiniu_server)
			free(qiniu_server);
		break;

retry_qiniu:
		if (dec)
			dec->destroy(dec);
		if (qiniu_server)
			free(qiniu_server);
		sleep(1);
	}
	sleep(1);
	return NULL;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief tcpHeartThread 硬件云心跳包
 *
 * @param arg
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static void* tcpHeartThread(void *arg)
{
	char ip[16];
	int connect_flag = 0;
	while (1) {
		memset(ip,0,sizeof(ip));
		if (opts.service_host[0] == 0) {
			sleep(1);
			continue;
		}
		dnsGetIp(opts.service_host,ip);
		if (connect_flag == 0) {
			if (tcp_client->Connect(tcp_client,ip,opts.service_port,5000) < 0){
				printf("connect fail,:%s,%d\n",ip,opts.service_port);
				sleep(1);
				continue;
			} else  {
				connect_flag = 1;
			}
		} 
		tcp_client->SendBuffer(tcp_client,g_config.imei,strlen(g_config.imei));	
		
		sleep(30);
	}
	return NULL;
}

static void* getIntercomsThread(void *arg)
{
	while (1) {
		if (mqtt_connect_state && ntp_connect_state) {
			getIntercoms();
			break;
		}
		sleep(1);
	}
	return NULL;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief registHardCloud 注册硬件云
 */
/* ---------------------------------------------------------------------------*/
void registHardCloud(void)
{
	mqtt_connect_state = 0;
	ntp_connect_state = 0;
	tcpClientInit();
	http = myHttpCreate();
	mqtt = myMqttCreate();
	createThread(initThread,NULL);
	createThread(tcpHeartThread,NULL);
	createThread(getIntercomsThread,NULL);

}

