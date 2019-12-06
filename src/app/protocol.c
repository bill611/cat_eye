/*
 * =============================================================================
 *
 *       Filename:  protocol.c
 *
 *    Description:  猫眼相关协议
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
#include <arpa/inet.h>
#include "my_http.h"
#include "my_mqtt.h"
#include "my_update.h"
#include "json_dec.h"
#include "udp_server.h"
#include "tcp_client.h"
#include "thread_helper.h"
#include "externfunc.h"
#include "protocol.h"
#include "qrenc.h"
#include "my_video.h"
#include "config.h"
#include "timer.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
void registHardCloud(void);
void registTalk(void);
void registSingleChip(void);

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
enum {
	TP_CALL,            //呼叫对讲
	TP_TESTROUTE=0x200, //路由环回测试命令，类似于PING命令的作用
	TP_NATBURROW,       //NAT路由器穿透命令
	TP_RTPBURROW,       //RTP音视频穿透命令
	TP_DEVCHECK,        //设备测试
	TP_UPDATEPROC,      //程序更新通知
	TP_UPDATEEND,       //返回数据服务中心更新完成的消息
	TP_GETAPPVERSION,   //取程序版本号
	TP_ELEVATOR,        //电梯联动
	TP_UPDATEMSG,       //服务器IP更新信息,房号配置表更换信息
	TP_GETREMOTEPWD,    //获取设置服务器远程密码
	TP_SETREMOTEPWD,    //获取设置服务器远程密码
	TP_SENDMACTOSRV,    //发送MAC地址，IP地址及房号信息到服务器
	TP_CHANGEROOMID,    //更改房号
	TP_TRANCMDBYSRV,    //通过服务器转发包
	TP_TRANSRTP,        //发送RTP包
	TP_TRANSELECTRIC,   //转发送电器控制包
	TP_SENDSOFTVER,                 //发送本设备版本信息
	TP_RETUPDATEMSG,                //返回升级信息  0x211
	TP_BUTTONALARM,     //紧急按钮报警
	TP_LOCALDEVID,      //获取本机唯一编号
	TP_ADVERTISEMENT,    //中控机接收远程广告发布， 为了向U9中控机兼容，才更改值
	TP_LOCALHARDCODE = 0x520,   //获取本机硬件码
};

enum
{
    REQSERVERINF,           // 终端向服务器申请 ID 命令号
    RESPONSESERVERINF       // 服务器向终端返回 ID 命令号
};

enum {
	TYPE_CENTER,          //管理中心
    TYPE_DOOR,              //门口机
    TYPE_XC,                //保留
    TYPE_USER,              //中控主机
    TYPE_REMOTE,            //基于PC上的呼叫程序，如网站的对讲程序
    TYPE_XCDOOR,            //保留
    TYPE_FDOOR,             //户门口机
    TYPE_ZNBOX,             //智能箱
    TYPE_NETRF,             //网络RF模块
    TYPE_DEVID_SRV=0x100,   //取设备ID服务器
    TYPE_DEVHARDCODE_SRV=0x101, //取设备硬件码服务器
    TYPE_SINGLECHIP = 0x120,    //单片机设备
    TYPE_ALL=0xFFFFFFFF     //所有设备
};

//搜索设备信息包定义
enum {
	CMD_GETSTATUS,      //读设备信息
	CMD_RETSTATUS           //返回设备信息
};

/* 包头 */
typedef struct
{
    unsigned int ID;
    unsigned int Size;
    unsigned int Type;
}COMMUNICATION;

typedef struct
{
    unsigned int ID;        //包头ID号
    unsigned int Size;      //包大小
    unsigned int Type;      //包类型
    unsigned int Cmd;       //命令
    unsigned int ReqType;   //请求信息设备类型
    unsigned long long int DevID;            //设备ID
    char Addition[32];      //附加信息
} TResDeviceInf;

typedef struct
{
    unsigned int ID;        //包头ID号
    unsigned int Size;      //包大小
    unsigned int Type;      //包类型，必须为TP_DEVCHECK
    unsigned int Cmd;       //命令,必须为CMD_GETSTATUS
    unsigned int DevType;   //设备类型
    char Addition[20];      //附加信息
} TGetDeviceInfo;

//返回设备信息包定义
typedef struct
{
    unsigned int ID;        //包头ID号
    unsigned int Size;      //包大小
    unsigned int Type;      //包类型，必须为TP_DEVCHECK
    unsigned int Cmd;       //命令,必须为CMD_RETSTATUS
    unsigned int DevType;   //设备类型(以数值方式描述)
    char Code[16];          //设备虚拟编号
    char Name[32];          //设备名称
    char IP[16];            //设备IP地址
    char dType[20];         //设备类型(以字符方式描述)
    char IMEI[24];          //设备机身号
    char Addition[20];      //附加信息
} TRetDeviceInfo;

typedef struct
{
	char ip[64];            //升级的文件
	char file[256];               //提示信息
} TUpdateRevertFile;
#pragma pack (1)
typedef struct
{
	unsigned int ID;        //包头ID号
	unsigned int Size;      //包大小
	unsigned int Type;      //包类型，必须为TP_DEVCHECK
	unsigned int Cmd;       //命令,必须为CMD_GETSTATUS
	char reserve;
	unsigned long long int HardCode;    //硬件码
	char Addition[20];      //附加信息
} TGetDeviceBodyCode;   //获取设备机身码使用
#pragma pack ()

typedef struct _PacketsID {
    char IP[16];
    uint32_t id;
    uint64_t dwTick;        //时间
}PacketsID;
typedef struct _UdpCmdRead {
	unsigned int cmd;
	void (*udpCmdProc)( struct _SocketHandle *ABinding,
			struct _SocketPacket *AData);   //协议处理
}UdpCmdRead;

typedef struct _ProtocolPriv {

}ProtocolPriv;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
Protocol *protocol;
static unsigned int packet_id;
static int get_imie_end = 1;
static Timer *timer_protocol_1s = NULL; // 协议1s定时器
static Timer *timer_getimei_5s = NULL; // 获取机身码5s定时器
static void (*getImeiCallback)(int result); // 机身码获取回调函数
IpcServer* ipc_main = NULL;

static unsigned long long htonll(unsigned long long val)
{
#if 1
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
           return (((unsigned long long)htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl((int)(val >> 32));
    }
    else if (__BYTE_ORDER == __BIG_ENDIAN)
    {
           return val;
    }
#endif

}
static void udpLocalGetIMEI(SocketHandle *ABinding,SocketPacket *AData)
{
	if(AData->Size != sizeof(TResDeviceInf)) {
		return;
	}
	TResDeviceInf *GetPacket = (TResDeviceInf*)AData->Data;
	if(GetPacket->Cmd == RESPONSESERVERINF && GetPacket->ReqType==TYPE_DEVID_SRV) {
		get_imie_end = 1;
		printf("[%s]imei:%llX\n",__func__,htonll(GetPacket->DevID));
		sprintf(g_config.imei,"%llX",htonll(GetPacket->DevID));

	}
}

static void udpLocalGetHardCode(SocketHandle *ABinding,SocketPacket *AData)
{
	if(AData->Size != sizeof(TResDeviceInf)) {
		return;
	}
	TResDeviceInf *GetPacket = (TResDeviceInf*)AData->Data;
	if(GetPacket->Cmd == RESPONSESERVERINF && GetPacket->ReqType==TYPE_DEVHARDCODE_SRV) {
		get_imie_end = 1;
		printf("[%s]hardcode:%llx\n",__func__,GetPacket->DevID);
		sprintf(g_config.hardcode,"%llx",GetPacket->DevID);
	}
}

static void udpLocalgetMsg(SocketHandle *ABinding,SocketPacket *AData)
{
	printf("TP_DEVCHECK:LocalMsgGetProc():%s\n",ABinding->IP);
	if(AData->Size==sizeof(TGetDeviceInfo)) {
		printf("MsgGetProc():TGetDeviceInfo\n");
		//返回本机设备信息
		TGetDeviceInfo *GetDevMsg = (TGetDeviceInfo*)AData->Data;
		printf("GetDevMsg DevType:%d\n", GetDevMsg->DevType);
		if(GetDevMsg->Cmd==CMD_GETSTATUS &&
			(GetDevMsg->DevType==TYPE_DOOR || GetDevMsg->DevType==0xFFFFFFFF)) { //类型为中控主机或所有设备
			TRetDeviceInfo RetDevMsg;
			const char *pDestIP;
			memset(&RetDevMsg,0,sizeof(TRetDeviceInfo));
			RetDevMsg.ID = packet_id++;
			RetDevMsg.Size = sizeof(TRetDeviceInfo);
			RetDevMsg.Type = TP_DEVCHECK;
			RetDevMsg.Cmd = CMD_RETSTATUS;			//返回设备信息
			RetDevMsg.DevType = TYPE_DOOR;

			// strcpy(RetDevMsg.IP,Public.cLocalIP);	//IP

			strcpy(RetDevMsg.Name,"cat eye");

			strcpy(RetDevMsg.dType,"cat eye");		//类型
			sprintf(RetDevMsg.IMEI,"%s",g_config.imei);		//设备唯一编号
			sprintf(RetDevMsg.Addition,"%s-%s",DEVICE_SVERSION,DEVICE_KVERSION);

			udp_server->SendBuffer(udp_server,"255.255.255.255",ABinding->Port, 		//返回信息
					(char *)&RetDevMsg,sizeof(TRetDeviceInfo));
		}
	} else if(AData->Size==sizeof(TRetDeviceInfo)) {
		TRetDeviceInfo *pDevMsg = (TRetDeviceInfo*)AData->Data;
		if(pDevMsg->Cmd != CMD_RETSTATUS) {
			return;
		}
		if(pDevMsg->DevType != TYPE_DEVID_SRV) {
			return;
		}
		//获取设备唯一编号
		// if(get_imie_end == 1) {
			// return;
		// }
		// get_imie_end = 1;
		TResDeviceInf Packet;
		memset(&Packet,0,sizeof(TResDeviceInf));
		Packet.ID = packet_id++;
		Packet.Size = sizeof(TResDeviceInf);
		Packet.Type = TP_LOCALDEVID;
		Packet.Cmd = REQSERVERINF;
		Packet.ReqType = TYPE_DOOR;
		udp_server->AddTask(udp_server,
				ABinding->IP,ABinding->Port,&Packet,Packet.Size,5,NULL,NULL);
		printf("Get Local ID From %s\n",ABinding->IP);
	}
}
static void udpUpdateProc(SocketHandle *ABinding,SocketPacket *AData)
{
	printf("TP_RETUPDATEMSG:UpdateProc()\n");
	TUpdateRevertFile *cBuf = NULL;
	char *Packet = (char *)&AData->Data[sizeof(COMMUNICATION)];

	cBuf = (TUpdateRevertFile *)malloc(sizeof(TUpdateRevertFile));
	sprintf(cBuf->ip,"%s",ABinding->IP);
	sprintf(cBuf->file,"%s",Packet);
	if (my_video)
		my_video->update(UPDATE_TYPE_CENTER,ABinding->IP,0,Packet);

	if (cBuf)
		free(cBuf);
}
static void getIMEI(void)
{
	if (strlen(g_config.hardcode) < 2)
		return;
	TGetDeviceBodyCode Packet;
	memset(&Packet,0,sizeof(TGetDeviceBodyCode));
	Packet.ID = packet_id++;
	Packet.Size = sizeof(TGetDeviceBodyCode);
	Packet.Type = TP_LOCALDEVID;
	Packet.Cmd = CMD_GETSTATUS;
	Packet.HardCode = htonll((unsigned long long)strtoull(g_config.hardcode, NULL, 16));
	printf("%s(),hardcode:%s\n",__func__,g_config.hardcode);
	if(udp_server)
		udp_server->AddTask(udp_server,
				"255.255.255.255",
				7801, &Packet, Packet.Size, 5,NULL,NULL);
}
static void getHardCode(void)
{
	TGetDeviceBodyCode Packet;
	memset(&Packet,0,sizeof(TGetDeviceBodyCode));
	Packet.ID = packet_id++;
	Packet.Size = sizeof(TGetDeviceBodyCode);
	Packet.Type = TP_LOCALHARDCODE;
	Packet.Cmd = CMD_GETSTATUS;
	if(udp_server)
		udp_server->AddTask(udp_server,
				"255.255.255.255",
				7801, &Packet, Packet.Size, 5,NULL,NULL);
}

static void timerImei5sThread(void *arg)
{
	int *get_type = (int *)arg;
	timer_getimei_5s->stop(timer_getimei_5s);
	if (get_imie_end == 0) {
		getImeiCallback(0);
		return;
	}
	get_imie_end = 0;
	if (*get_type == 0) {
		*get_type = 1;
		getIMEI();
		timer_getimei_5s->start(timer_getimei_5s);
	} else {
		char buf[128] = {0};
		sprintf(buf,"%s/%s",g_config.imei,g_config.hardcode);
		qrcodeString(buf,QRCODE_IMIE);
		getImeiCallback(1);
		ConfigSavePrivate();
	}
}

static void getImei(void (*callBack)(int result))
{
	static int get_type = 0; //获取机身码或者硬件码
	if (timer_getimei_5s == NULL)
		timer_getimei_5s = timerCreate(TIMER_1S * 5,timerImei5sThread,&get_type);
	if (timer_getimei_5s->isStop(timer_getimei_5s) == 0) {
		printf("wait for getting end\n");
		return;
	}
	getImeiCallback = callBack;

	if (strlen(g_config.hardcode) < 2) {
		get_type = 0;
		getHardCode();// 获取硬件码
	} else {
		get_type = 1;
		getIMEI();// 获取机身码
	}

	timer_getimei_5s->start(timer_getimei_5s);
	get_imie_end = 0;
}

static int isNeedToUpdate(char *version,char *content)
{
	return my_update->needUpdate(my_update,version,content);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief udpKillTaskCondition 根据条件删除任务
 *
 * @param arg1 udp数据
 * @param arg2 传入参数
 *
 * @returns 删除0失败 1成功
 */
/* ---------------------------------------------------------------------------*/
static int udpKillTaskCondition(void *arg1,void *arg2)
{
    int id = *(int *)arg2;
    COMMUNICATION * pHead = (COMMUNICATION*)arg1;
    if(pHead->ID==id)
        return 1;

    return 0;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief udpUdpProtocolFilter 接收协议前过滤重复协议以及应答回复
 *
 * @param ABinding
 * @param AData
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int udpUdpProtocolFilter(SocketHandle *ABinding,SocketPacket *AData)
{
	static PacketsID packets_id[10] = {0};
    static int packet_pos = 0;
	uint64_t dwTick;
	int i;

    // 收到无效字节
    if (AData->Size < 4)
        return 0;

	// 收到4个字节为对方返回的应答包,即ID
	if (AData->Size == 4) {
        udp_server->KillTaskCondition(udp_server,
                udpKillTaskCondition,AData->Data);
		return 0;
	}

    // 收到无效字节
    if (AData->Size < sizeof(COMMUNICATION))
        return 0;

	// 收到其他协议则回复相同ID
    udp_server->SendBuffer(udp_server,ABinding->IP,ABinding->Port,
            AData->Data,4);

	dwTick = getMs();
    // 过滤相同IP发送的多次重复命令
	for (i=0; i<10; i++) {
        if (strcmp(ABinding->IP,packets_id[i].IP) == 0
                && *(uint32_t*)AData->Data == packets_id[i].id ){
            return 0;
        }
    }
    sprintf(packets_id[packet_pos].IP,"%s",ABinding->IP);
    packets_id[packet_pos].id = *(uint32_t*)AData->Data;
    packets_id[packet_pos++].dwTick = dwTick;
    packet_pos %= 10;
	return 1;
}
static void udpLocalCall(SocketHandle *ABinding,SocketPacket *AData)
{
	if (protocol_talk && protocol_talk->udpCmd)
		protocol_talk->udpCmd( ABinding->IP,ABinding->Port,
				AData->Data,AData->Size);
}

static UdpCmdRead udp_cmd_handle[] = {
	{TP_CALL,			udpLocalCall},
	{TP_DEVCHECK,		udpLocalgetMsg},
	{TP_LOCALDEVID,     udpLocalGetIMEI},
	{TP_LOCALHARDCODE,  udpLocalGetHardCode},
	{TP_RETUPDATEMSG,  	udpUpdateProc},
	{0,NULL},
};

static void udpSocketRead(SocketHandle *ABinding,SocketPacket *AData)
{
	if (udpUdpProtocolFilter(ABinding,AData) == 0)
		return;

	COMMUNICATION * head = (COMMUNICATION *)AData->Data;
	int i;
	for(i=0; udp_cmd_handle[i].udpCmdProc != NULL; i++) {
       if (udp_cmd_handle[i].cmd == head->Type) {
	          if (udp_cmd_handle[i].udpCmdProc)
	              udp_cmd_handle[i].udpCmdProc(ABinding,AData);
	          return;
	       }
	}
	// printf("[%s]:IP:%s,Cmd=0x%04x\n",__FUNCTION__,ABinding->IP,head->Type);
}


static void ipcCallback(char *data,int size )
{
	IpcData ipc_data;
	memcpy(&ipc_data,data,sizeof(IpcData));
	if (protocol_singlechip)
		protocol_singlechip->deal(&ipc_data);
}

void protocolInit(void)
{
	packet_id = (unsigned)time(NULL);
	udpServerInit(udpSocketRead,7800);

	protocol = (Protocol *) calloc(1,sizeof(Protocol));
	protocol->priv = (ProtocolPriv *) calloc(1,sizeof(ProtocolPriv));
	protocol->getImei = getImei;
	protocol->isNeedToUpdate = isNeedToUpdate;

	ipc_main = ipcCreate(IPC_MAIN,ipcCallback);
	registHardCloud();
	registTalk();
	registSingleChip();
}

