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
#include <dirent.h>
#include <errno.h>
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
#include "queue.h"
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
#define HARD_COULD_API "https://iot.taichuan.net/v1"
// 测试地址
// #define HARD_COULD_API "http://84.internal.taichuan.net:8080/v1"
enum {
	MODE_SEND_NEED_REPLY = 1,
	MODE_SEND_NONEED_REPLY = 2,
	MODE_REPLY = 4,
};
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

struct QueueList {
	Queue *upload;  // 上传七牛队列
	Queue *report_capture; // 上报抓拍日志队列
	Queue *report_alarm; // 上报报警队列
	Queue *report_face; // 上报人脸记录队列
	Queue *report_talk; // 上报通话记录队列
};
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
ProtocolHardcloud *protocol_hardcloud;
static MyHttp *http = NULL;
static MyMqtt *mqtt = NULL;
static int mqtt_connect_state,ntp_connect_state;
static int heart_start = 0;
static int heart_end = 0;
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
static struct QueueList queue_list;

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
	char *send_buff = NULL;
	if(dec->changeCurrentObj(dec,"body")) {
		printf("change body fail\n");
		return;
	}
	cJSON *data = dec->getValueObject(dec,"data");
	cJSON *root = packData(api,MODE_REPLY,id);
	cJSON_AddItemToObject(root,"body",data);
	send_buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mqtt->send(opts.pubTopic,strlen(send_buff),send_buff);
	free(send_buff);

}

/* ---------------------------------------------------------------------------*/
/**
 * @brief ceSetSelfIntercom 设置本机对讲信息
 *
 * @param dec
 */
/* ---------------------------------------------------------------------------*/
static void ceSetSelfIntercom(CjsonDec *dec)
{
	if(dec->changeCurrentObj(dec,"body")) {
		printf("change body fail\n");
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

	sqlDeleteDeviceUseTypeNoBack(USER_TYPE_CATEYE);
	sqlInsertUserInfo(user_id,login_token,nick_name,USER_TYPE_CATEYE,scope);
	if (user_id)
		free(user_id);
	if (login_token)
		free(login_token);
	if (nick_name)
		free(nick_name);

	if (protocol_talk) {
		protocol_talk->reload();
		protocol_talk->reconnect();
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief ceSetTargetsIntercom 设置对方对讲账号
 *
 * @param dec
 */
/* ---------------------------------------------------------------------------*/
static void ceSetTargetsIntercom(CjsonDec *dec)
{
	if(dec->changeCurrentObj(dec,"body")) {
		printf("change body fail\n");
		return;
	}
	char *user_id = NULL;
	char *nick_name = NULL;
	int scope = 0 ;
	sqlDeleteDeviceUseTypeNoBack(USER_TYPE_OTHERS);
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

	if (protocol_talk) {
		protocol_talk->reload();
		protocol_talk->reconnect();
	}
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
	if (protocol_talk) {
		protocol_talk->reload();
		protocol_talk->reconnect();
	}
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
	cJSON *root = packData(api,MODE_REPLY,id);
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
	root = packData(api,MODE_REPLY,id);
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
	root = packData(api,MODE_REPLY,id);
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
	// dec->print(dec);
	if (my_video)
		my_video->delaySleepTime(1);
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
			ceSetSelfIntercom(dec);
			break;
		case CE_SetTargetsIntercom :
			ceSetTargetsIntercom(dec);
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
		default:
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
		if (protocol_talk) {
			protocol_talk->connect();
		}
		return;
	}
	char *send_buff;
	cJSON *root = packData(CE_GetIntercoms,MODE_SEND_NEED_REPLY,++g_id);
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

static void waitConnectService(void)
{
	while (1) {
		if (opts.service_host[0] == 0) {
			sleep(1);
			continue;
		}
		return;
	}
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
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
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

		// dec->print(dec);
		dec->getValueChar(dec,"ip",&buff);

		if (!buff) {
			printf("get ip [ip] null!!\n");
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

static void writeSleepScript(char *dst_ip,int dst_port)
{
	strcpy(g_config.wifi_lowpower.dst_ip,dst_ip);
	sprintf(g_config.wifi_lowpower.dst_port,"%d",dst_port);
}

static void enableSleepMpde(void)
{
	heart_start = 0;
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
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	char ip[16];
	int connect_flag = 0;
	char imei[32];
	char imei_len[8];
	char gateway[16];
	int send_interval = 0;
	heart_start = 1;
	memset(ip,0,sizeof(ip));
	// 等待连接上服务器
	waitConnectService();
	dnsGetIp(opts.service_host,ip);
	writeSleepScript(ip,opts.service_port);
	while (heart_start) {
		if (connect_flag == 1 && send_interval < 30) {
			send_interval++;
			char tcp_rec[64] = {0};
			int len = tcp_client->RecvBuffer(tcp_client,tcp_rec,sizeof(tcp_rec),1000);
			if (strcmp(tcp_rec,"AwakenAsync")== 0) {
				if (my_video)
					my_video->delaySleepTime(1);
			}
			goto loop_heart;
		}
		send_interval = 0;
		if (tcp_client->Connect(tcp_client,ip,opts.service_port,5000) < 0){
			printf("connect fail,:%s,%d\n",ip,opts.service_port);
			goto loop_heart;
		} else {
			connect_flag = 1;
			tcp_client->SendBuffer(tcp_client,g_config.imei,strlen(g_config.imei));
		}
loop_heart:
		sleep(1);
	}
	tcp_client->DisConnect(tcp_client);

	getLocalIP(g_config.wifi_lowpower.local_ip,gateway);
	getGateWayMac(gateway,g_config.wifi_lowpower.dst_mac);
	sprintf(imei,"\"%s\"",g_config.imei);	
	sprintf(imei_len,"%ld",strlen(g_config.imei));	
	excuteCmd("dhd_priv","wl","tcpka_conn_add","1",
			g_config.wifi_lowpower.dst_mac,
			g_config.wifi_lowpower.local_ip,
			g_config.wifi_lowpower.dst_ip,
			"0","1223",
			g_config.wifi_lowpower.dst_port,
			"1","0","1024","1062046","2130463","1",imei_len, imei,
			NULL);
	excuteCmd("dhd_priv","wl","tcpka_conn_enable","1","1","10","10","10",NULL);
	excuteCmd("dhd_priv","setsuspendmode","1",NULL);
	return NULL;
}

static void* getIntercomsThread(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	while (1) {
		if (mqtt_connect_state && ntp_connect_state) {
			getIntercoms();
			break;
		}
		sleep(1);
	}
	return NULL;
}

static void* threadUpload(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	UpLoadData up_data;
	char *qiniu_upload= NULL;
	DIR *dir;
	struct dirent *dirp;
	while (!qiniu_server_token) {
		sleep(1);
	}
	while (1) {
		if (queue_list.upload) {
			queue_list.upload->get(queue_list.upload,&up_data);
		} else {
			sleep(1);
			continue;
		}
		char file_name[64] = {0};
		sprintf(file_name,"%s_%lld",g_config.imei,up_data.picture_id);
		int file_len = strlen(file_name);
		if((dir=opendir(up_data.file_path)) == NULL) {
			printf("Open File %s Error %s\n",up_data.file_path,strerror(errno));
			return 0;
		}
		while((dirp=readdir(dir)) != NULL) {
			if (		(strcmp(".",dirp->d_name) == 0) 
					|| 	(strcmp("..",dirp->d_name) == 0)
					|| 	strncmp(file_name,dirp->d_name,file_len)) {
				continue;
			}
			char buf[64];
			sprintf(buf,"%s%s",up_data.file_path,dirp->d_name);
			printf("[%s]%s\n",__func__,buf);
			if (GetFileSize(buf) == 0) {
				printf("[%s]file size == 0\n",__func__);
				continue;
			}
			http->qiniuUpload("http://upload-z2.qiniup.com",
					NULL,qiniu_server_token,
					buf,
					dirp->d_name,
					&qiniu_upload);
			if (qiniu_upload) {
				printf("%s\n",qiniu_upload);
				free(qiniu_upload);
			}
			if (remove(buf) < 0) {
				printf("remove error:%s\n",strerror(errno));
			} else {
				printf("remove:%s\n",buf);
			}
		}
		sqlCheckBack();
		closedir(dir);
	}
	return NULL;
}
static void uploadPic(char *path,uint64_t pic_id)
{
	printf("[%s]pic_id:%lld\n", __func__,pic_id);
	if (pic_id == 0)
		return;
	UpLoadData up_data;
	strcpy(up_data.file_path,path);
	up_data.picture_id = pic_id;
	if (!queue_list.upload) {
		queue_list.upload = queueCreate("upload",QUEUE_BLOCK,sizeof(UpLoadData));
	}
	queue_list.upload->post(queue_list.upload,&up_data);
}

static void* threadReportCapture(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	uint64_t picture_id = 0;
	char date[64] = {0};
	int i;
	char *send_buff;
	waitConnectService();
	while (1) {
		if (queue_list.report_capture) {
			queue_list.report_capture->get(queue_list.report_capture,&picture_id);
		} else {
			sleep(1);
			continue;
		}
		int ret = sqlGetCapInfo(picture_id,date);
		// 封装jcson信息
		cJSON *root = packData(CE_Report,MODE_SEND_NONEED_REPLY,++g_id);
		cJSON *obj_body = cJSON_CreateObject();
		cJSON_AddStringToObject(obj_body,"dataType","capture");
		cJSON *arry_pic = cJSON_CreateArray();
		cJSON *arry_record = cJSON_CreateArray();
		if (ret) {
			int count = sqlGetPicInfoStart(picture_id);
			for (i=0; i<count; i++) {
				char url[128] = {0};
				cJSON *obj = cJSON_CreateObject();
				sqlGetPicInfos(url);
				cJSON_AddStringToObject(obj,"url",url);
				cJSON_AddItemToArray(arry_pic, obj);
			}
			sqlGetPicInfoEnd();

			count = sqlGetRecordInfoStart(picture_id);
			for (i=0; i<count; i++) {
				char url[128] = {0};
				cJSON *obj = cJSON_CreateObject();
				sqlGetRecordInfos(url);
				cJSON_AddStringToObject(obj,"url",url);
				cJSON_AddItemToArray(arry_record, obj);
			}
			sqlGetRecordInfoEnd();
		}
		cJSON *obj_data = cJSON_CreateObject();
		cJSON_AddStringToObject(obj_data,"date",date);
		cJSON_AddItemToObject(obj_data,"picture",arry_pic);
		cJSON_AddItemToObject(obj_data,"record",arry_record);
		cJSON_AddItemToObject(obj_body,"data",obj_data);
		cJSON_AddItemToObject(root,"body",obj_body);
		send_buff = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
		mqtt->send(opts.pubTopic,strlen(send_buff),send_buff);
		free(send_buff);
	}
	return NULL;
}
static void reportCapture(uint64_t pic_id)
{
	uint64_t data = pic_id;
	if (!queue_list.report_capture)
		queue_list.report_capture = queueCreate("re_cap",QUEUE_BLOCK,sizeof(uint64_t));
	queue_list.report_capture->post(queue_list.report_capture,&data);
}
static void* threadReportAlarm(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	ReportAlarmData alarm_data;
	int i;
	char *send_buff;
	waitConnectService();
	while (1) {
		if (queue_list.report_alarm) {
			queue_list.report_alarm->get(queue_list.report_alarm,&alarm_data);
		} else {
			sleep(1);
			continue;
		}

		// 封装jcson信息
		cJSON *root = packData(CE_Report,MODE_SEND_NONEED_REPLY,++g_id);
		cJSON *obj_body = cJSON_CreateObject();
		cJSON_AddStringToObject(obj_body,"dataType","alarmRecord");
		cJSON *obj_data = cJSON_CreateObject();
		cJSON_AddStringToObject(obj_data,"date",alarm_data.date);
		cJSON_AddNumberToObject(obj_data,"type",alarm_data.type);
		if (alarm_data.has_people)
			cJSON_AddTrueToObject(obj_data,"hasPeople");
		else
			cJSON_AddFalseToObject(obj_data,"hasPeople");
		cJSON_AddNumberToObject(obj_data,"sex",alarm_data.sex);
		cJSON_AddNumberToObject(obj_data,"age",alarm_data.age);
		cJSON *arry = cJSON_CreateArray();
		int count = sqlGetPicInfoStart(alarm_data.picture_id);
		for (i=0; i<count; i++) {
			char url[128] = {0};
			cJSON *obj = cJSON_CreateObject();
			sqlGetPicInfos(url);
			cJSON_AddStringToObject(obj,"url",url);
			cJSON_AddItemToArray(arry, obj);
		}
		sqlGetPicInfoEnd();
		cJSON_AddItemToObject(obj_data,"picture",arry);
		cJSON_AddItemToObject(obj_body,"data",obj_data);
		cJSON_AddItemToObject(root,"body",obj_body);
		send_buff = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
		mqtt->send(opts.pubTopic,strlen(send_buff),send_buff);
		free(send_buff);
	}
	return NULL;
}
static void reportAlarm(ReportAlarmData *data)
{
	if (!queue_list.report_alarm)
		queue_list.report_alarm = queueCreate("re_alarm",QUEUE_BLOCK,sizeof(ReportAlarmData));
	queue_list.report_alarm->post(queue_list.report_alarm,data);
}
static void* threadReportFace(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	ReportFaceData face_data;
	int i;
	char *send_buff;
	waitConnectService();
	while (1) {
		if (queue_list.report_face) {
			queue_list.report_face->get(queue_list.report_face,&face_data);
		} else {
			sleep(1);
			continue;
		}

		// 封装jcson信息
		cJSON *root = packData(CE_Report,MODE_SEND_NONEED_REPLY,++g_id);
		cJSON *obj_body = cJSON_CreateObject();
		cJSON_AddStringToObject(obj_body,"dataType","faceRecognitionRecord");
		cJSON *obj_data = cJSON_CreateObject();
		cJSON_AddStringToObject(obj_data,"date",face_data.date);
		cJSON_AddStringToObject(obj_data,"faceId",face_data.user_id);
		cJSON *obj_face_info = cJSON_CreateObject();
		cJSON_AddStringToObject(obj_face_info,"nickName",face_data.nick_name);
		cJSON_AddItemToObject(obj_data,"faceInfo",obj_face_info);
		int count = sqlGetPicInfoStart(face_data.picture_id);
		if (count) {
			char url[128] = {0};
			sqlGetPicInfos(url);
			cJSON_AddStringToObject(obj_data,"picture",url);
		}
		sqlGetPicInfoEnd();

		cJSON_AddItemToObject(obj_body,"data",obj_data);
		cJSON_AddItemToObject(root,"body",obj_body);
		send_buff = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
		mqtt->send(opts.pubTopic,strlen(send_buff),send_buff);
		free(send_buff);
	}
	return NULL;
}

static void reportFace(ReportFaceData *data)
{
	if (!queue_list.report_face)
		queue_list.report_face = queueCreate("re_face",QUEUE_BLOCK,sizeof(ReportFaceData));
	queue_list.report_face->post(queue_list.report_face,data);
}

static void* threadReportTalk(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	ReportTalkData talk_data;
	int i;
	char *send_buff;
	waitConnectService();
	while (1) {
		if (queue_list.report_talk) {
			queue_list.report_talk->get(queue_list.report_talk,&talk_data);
		} else {
			sleep(1);
			continue;
		}

		// 封装jcson信息
		cJSON *root = packData(CE_Report,MODE_SEND_NONEED_REPLY,++g_id);
		cJSON *obj_body = cJSON_CreateObject();
		cJSON_AddStringToObject(obj_body,"dataType","callRecord");
		cJSON *obj_data = cJSON_CreateObject();
		cJSON_AddStringToObject(obj_data,"date",talk_data.date);
		cJSON_AddStringToObject(obj_data,"people",talk_data.nick_name);
		cJSON_AddNumberToObject(obj_data,"callDir",talk_data.call_dir);
		if (talk_data.answered)
			cJSON_AddTrueToObject(obj_data,"autoAnswer");
		else
			cJSON_AddFalseToObject(obj_data,"autoAnswer");
		cJSON_AddNumberToObject(obj_data,"talkTime",talk_data.talk_time);

		cJSON *arry = cJSON_CreateArray();
		int count = sqlGetPicInfoStart(talk_data.picture_id);
		for (i=0; i<count; i++) {
			char url[128] = {0};
			cJSON *obj = cJSON_CreateObject();
			sqlGetPicInfos(url);
			cJSON_AddStringToObject(obj,"url",url);
			cJSON_AddItemToArray(arry, obj);
		}
		sqlGetPicInfoEnd();

		cJSON_AddItemToObject(obj_data,"picture",arry);
		cJSON_AddItemToObject(obj_body,"data",obj_data);
		cJSON_AddItemToObject(root,"body",obj_body);
		send_buff = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
		mqtt->send(opts.pubTopic,strlen(send_buff),send_buff);
		free(send_buff);
	}
	return NULL;
}
static void reportTalk(ReportTalkData *data)
{
	if (!queue_list.report_talk)
		queue_list.report_talk = queueCreate("re_talk",QUEUE_BLOCK,sizeof(ReportTalkData));
	queue_list.report_talk->post(queue_list.report_talk,data);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief registHardCloud 注册硬件云
 */
/* ---------------------------------------------------------------------------*/
void registHardCloud(void)
{
	protocol_hardcloud = (ProtocolHardcloud *) calloc(1,sizeof(ProtocolHardcloud));
	protocol_hardcloud->uploadPic = uploadPic;
	protocol_hardcloud->reportCapture = reportCapture;
	protocol_hardcloud->reportAlarm = reportAlarm;
	protocol_hardcloud->reportFace= reportFace;
	protocol_hardcloud->reportTalk= reportTalk;
	protocol_hardcloud->enableSleepMpde = enableSleepMpde;

	mqtt_connect_state = 0;
	ntp_connect_state = 0;
	tcpClientInit();
	http = myHttpCreate();
	mqtt = myMqttCreate();
	createThread(initThread,NULL);
	createThread(tcpHeartThread,NULL);
	createThread(getIntercomsThread,NULL);
	createThread(threadUpload,NULL);
	createThread(threadReportCapture,NULL);
	createThread(threadReportAlarm,NULL);
	createThread(threadReportFace,NULL);
	createThread(threadReportTalk,NULL);

}

