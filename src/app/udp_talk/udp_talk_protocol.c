/*
 * =============================================================================
 *
 *       Filename:  protocol_video.c
 *
 *    Description:  太川对讲协议
 *
 *        Version:  1.0
 *        Created:  2018-12-13 14:28:55
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
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/prctl.h>

#include "state_machine.h"
#include "udp_talk_protocol.h"
/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define DBG_PROTOCOL 1
#if DBG_PROTOCOL > 0
	#define DBG_P( ... ) \
    do { \
        printf("[VIDEO DEBUG]"); \
        printf( __VA_ARGS__ );  \
    }while(0)
#else
	#define DBG_P( ... )
#endif

#define NELEMENTS(array)        /* number of elements in an array */ \
	        (sizeof (array) / sizeof ((array) [0]))

#define LOADFUNC(func) \
do {\
	if (in->func)\
		priv->func = in->func;\
	else\
		priv->func = func##Default;\
} while (0)

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* 呼叫命令 */
enum {
	CMD_CALL,		/* CMD_CALL 呼叫 */
	CMD_ANSWER,		/* CMD_ANSWER 保留，测试时不用 */
	CMD_TALK,		/* CMD_TALK 摘机，被呼叫方发送该消息 */
	CMD_OVER,		/* CMD_OVER 挂机 */
	CMD_UNLOCK,		/* CMD_UNLOCK 开锁 */
	CMD_LEAVEWORD,	/* 留言 */
	CMD_FORTIFY,	//
	CMD_REPEAL,		//
	CMD_PICTURE,	//
	CMD_TEXT,		//
	CMD_CLOSEAD,	//
	CMD_MUSICPLAY,	//
	CMD_MUSICSTOP,	//
	CMD_CLOSEMEDIA,
	CMD_SENDPORT1,
	CMD_SHAKEHANDS=0x80,		// 呼叫前握手命令
	ASW_OK=0x100,	//呼叫回复成功
	ASW_FAIL,		//呼叫回复失败
	MNT_OK,			//监视成功
	MNT_BUSY,		//监视忙
	MNT_VERFAIL,	//监视检验MD5失败
	MNT_REFUSE,		//
	CMD_SENDPORT,	//
	CMD_TRANSCALL,	//转发呼叫命令
	CMD_TRANSOVER,	//转发呼叫结束命令
	CMD_TRANSTALK,	//一户多机转发摘机
	ASW_NOEXISTS,	//用户不存在
	ASW_TRANSCALL_PERMITTED=0x0110,	//一户多机分机接受呼叫
	ASW_TRANSCALL_DENIED=0x0111,	//一户多机分机拒绝呼叫
	CMD_ELEVATOR_PERMIT=0x0200,		//电梯互访
	ASW_ELEVATOR_SUCCESS=0x0201,	//互访呼梯成功
	ASW_ELEVATOR_FAIL=0x0202,		//互访呼梯失败
	CMD_SHAKEHANDS_ASW=0x300,	// 呼叫前握手包应答命令
	CMD_OVER_TIMEOUT = 0x400,		/* 超时挂机 */
};

// 状态机
enum {
	EVENT_SHAKEHANDS, 	//主动呼叫发送握手
	EVENT_SHAKEHANDS_ASW, 	//对方呼叫返回握手
	EVENT_TYPE_CENTER, 	//类型管理中心
	EVENT_TYPE_HM, 		//类型室内机
	EVENT_TYPE_DMK, 	//类型门口机
	EVENT_TYPE_HDMK, 	//类型门口机
	EVENT_DIAL,        	//拨号
	EVENT_CALL,        	//呼叫
	EVENT_TALK,        	//通话
	EVENT_TALK_EX,      //作为主机收到分机接听指令
	EVENT_RING,    		//被呼叫
	EVENT_RING_EX,    	//作为分机时被呼叫
	EVENT_FAIL_COMM,    //失败:连接
	EVENT_FAIL_SHAKE,   //失败:握手
	EVENT_FAIL_BUSY,   	//失败:正忙
	EVENT_FAIL_ABORT,   //失败:通话中中断
	EVENT_HANGUP, 		//挂机

};

enum {
	ST_IDLE,			//空闲
	ST_SHAKEHANDS,		//主动发送握手状态
	ST_SHAKEHANDS_ASW,	//对方呼叫已返回握手状态
	ST_JUDGE_TYPE,		//判断设备类型状态3000还是U9
	ST_DIAL,			//正在拨号
	ST_CALL,			//呼叫
	ST_TALK,			//对讲
	ST_RING,			//被呼叫，响铃
};

enum {
	DO_NO,   			//不做操作，只改变状态
	DO_SHAKEHANDS,   	//发送握手命令
	DO_SHAKEHANDS_ASW, 	//回复握手命令
	DO_JUDGE_TYPE, 		//监视时判断类型
	DO_DIAL,          	//发送拨号命令
	DO_CALL,    		//发送呼叫命令
	DO_TALK,        	//执行对讲流程
	DO_TALK_EX,        	//分机接听，主机保持转发状态
	DO_RING,        	//执行响铃流程
	DO_FAIL_COMM,    	//执行连接失败
	DO_FAIL_SHAKE,    	//执行握手失败
	DO_FAIL_BUSY,       //执行正忙失败
	DO_FAIL_ABORT,   	//执行通话中中断
	DO_RET_FAIL,       	//回复失败
	DO_HANGUP, 		//执行挂机命令
};

/* 包头 */
typedef struct
{
    unsigned int ID;
    unsigned int Size;
    unsigned int Type;
}COMMUNICATION;

typedef struct _COMMUNICATION_CALL
{
	unsigned int Cmd;					//Command
	char CallID[16];					//呼叫方编号
	char CallName[32];					//呼叫方名称
	unsigned int CallType;				//Type
	unsigned int VideoType;			//视频类型，0 H264, 1 Mpeg4
	unsigned short VideoWidth;			//视频类型  0 320X240 1 352X288 2 640X480 3 720X576
	unsigned short VideoHeight;			//视频类型  0 320X240 1 352X288 2 640X480 3 720X576
} COMMUNICATION_CALL;

//一户多机主机转发呼叫
typedef struct _TCALLFJ {
	unsigned int ID;
	unsigned int Size;
	unsigned int Type;
	unsigned int Cmd;					//Command
	char CallID[16];					//呼叫方编号
	char CallName[32];					//呼叫方名称
	unsigned int CallType;				//Type
	unsigned int VideoType;				//视频类型，0 H264, 1 Mpeg4
	unsigned short VideoWidth;			//视频类型  0 320X240 1 352X288 2 640X480 3 720X576
	unsigned short VideoHeight;			//视频类型  0 320X240 1 352X288 2 640X480 3 720X576
	char CallIP[16];					//呼叫方IP地址
	unsigned int CallPort;				//呼叫方端口号
	unsigned int TransByCenter;			//户户对讲通过管理中心转发0:否 1:是
	unsigned int CenterIP;				//管理中心IP地址
	char Add[12];						//保留
} TCALLFJ;

//分机数据结构
typedef struct {
	char IP[16];
	unsigned int dwIP;
	int Port;
	int DevType;
	unsigned long dwLastTick;
	unsigned char bCalling;
	unsigned char bTalk;
} TRegisiterDev;

//分机登记
typedef struct
{
	unsigned int ID;
	unsigned int Size;
	unsigned int Type;
	char Reserve[32];
} TFJRegStruct;

// 状态机传递参数
typedef struct _STMData {
	int flag;
	char mCallIP[16];
	int device_type;
	int device_index;
	char ip[16];
	int port;
	struct _COMMUNICATION_CALL p_call;
	struct _TCALLFJ p_call_ex;
	struct _VideoTrans *video;
}STMData;

typedef struct _UdpCallCmd {
	unsigned int cmd;
	void (*proc)(struct _VideoTrans *,
			char *ip,int port,char *data);	//呼叫对讲协议处理
}UdpCallCmd;

typedef struct _STMDo {
	int action;
	int (*proc)(struct _VideoTrans *video,STMData *data);
}STMDo;

typedef struct _VideoPriv {
	StMachine *st_machine; 		// 状态机处理
	VideoProtoclType pro_type;		// 对讲协议类型
	TRegisiterDev RegDev[MAXFJCOUNT]; // 分机
	int enable;					// 使能对讲
	int port;					// 对讲端口
	char *master_ip;			// 主机IP
	char *room_id;			// 终端编号(门牌号)
	char *room_name;			// 本机终端名称
	int packet_id;				// 发送包ID
	int call_cmd;				// 对讲协议命令字 TP_CALL
	int device_type;			// 发送的设备类型 TYPE_USER
	int leave_word;				// 是否有未读留言0无 1有

	int  st_run;				//状态机运行状态
	int  st_run_exit;			//状态机退出状态
	int  time_call;				//对讲计时
	int  time_shake;			//握手计时
	int  time_comm;				//通信计时
	int dwPeerType;				//对讲方的设备类型
	int m_CodeType;				//视频编码类型
	int dwInCameraWidth;		//呼入Camera的大小
	int dwInCameraHeight;		//呼入Camera的大小
	char cPeerRoom[32];			//对方名称
	char cPeerID[16];			//对方ID
	char cPeerIP[16];			//对方IP
	int PeerPort;				//对方端口
	int leave_word_state;		//是否在留言状态
	int master_trans;           //是否主机转发的呼叫指令 1主机转发 0非主机转发
	int call_dir;               // 0 呼入 1呼出
}VideoPriv;
/*-----------------video trans end----------------*/

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void videoTransHandleCallTime(VideoTrans *This);

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
VideoTrans *protocol_video = NULL;

static STMData *stm_data = NULL;
static StMachine *video_st_machine_bz;
static StMachine *video_st_machine_u9;

static char *st_debug_ev[] = {
	"EVENT_SHAKEHANDS", 	//主动呼叫发送握手
	"EVENT_SHAKEHANDS_ASW", 	//对方呼叫返回握手
	"EVENT_TYPE_CENTER", 	//类型管理中心
	"EVENT_TYPE_HM", 		//类型室内机
	"EVENT_TYPE_DMK", 	//类型门口机
	"EVENT_TYPE_HDMK", 	//类型门口机
	"EVENT_DIAL",        	//拨号
	"EVENT_CALL",        	//呼叫
	"EVENT_TALK",        	//通话
	"EVENT_TALK_EX",      //作为主机收到分机接听指令
	"EVENT_RING",    		//被呼叫
	"EVENT_RING_EX",    	//作为分机时被呼叫
	"EVENT_FAIL_COMM",    //失败:连接
	"EVENT_FAIL_SHAKE",   //失败:握手
	"EVENT_FAIL_BUSY",   	//失败:正忙
	"EVENT_FAIL_ABORT",   //失败:通话中中断
	"EVENT_HANGUP", 		//挂机
};
static char *st_debug_st[] = {
	"ST_IDLE",			//空闲
	"ST_SHAKEHANDS",		//主动发送握手状态
	"ST_SHAKEHANDS_ASW",	//对方呼叫已返回握手状态
	"ST_JUDGE_TYPE",		//判断设备类型状态3000还是U9
	"ST_DIAL",			//正在拨号
	"ST_CALL",			//呼叫
	"ST_TALK",			//对讲
	"ST_RING",			//被呼叫，响铃
};
static char *st_debug_do[] = {
	"DO_NO",   			//不做操作，只改变状态
	"DO_SHAKEHANDS",   	//发送握手命令
	"DO_SHAKEHANDS_ASW", 	//回复握手命令
	"DO_JUDGE_TYPE", 		//监视时判断类型
	"DO_DIAL",          	//发送拨号命令
	"DO_CALL",    		//发送呼叫命令
	"DO_TALK",        	//执行对讲流程
	"DO_TALK_EX",        	//分机接听，主机保持转发状态
	"DO_RING",        	//执行响铃流程
	"DO_FAIL_COMM",    	//执行连接失败
	"DO_FAIL_SHAKE",    	//执行握手失败
	"DO_FAIL_BUSY",       //执行正忙失败
	"DO_FAIL_ABORT",   	//执行通话中中断
	"DO_RET_FAIL",       	//回复失败
	"DO_HANGUP", 		//执行挂机命令
};
static StateTableDebug st_debug = {
	.ev = st_debug_ev,
	.st = st_debug_st,
	.todo = st_debug_do,
};
/* ---------------------------------------------------------------------------*/
/**
 * @brief 通话状态机
 */
/* ---------------------------------------------------------------------------*/
static StateTable stm_video_state_bz[] = {

{EVENT_SHAKEHANDS,		ST_IDLE,		ST_SHAKEHANDS,		DO_SHAKEHANDS},

{EVENT_SHAKEHANDS_ASW,	ST_IDLE,		ST_SHAKEHANDS_ASW,	DO_SHAKEHANDS_ASW},
{EVENT_SHAKEHANDS_ASW,	ST_RING,		ST_RING,			DO_SHAKEHANDS_ASW},
{EVENT_SHAKEHANDS_ASW,	ST_JUDGE_TYPE,	ST_JUDGE_TYPE,		DO_SHAKEHANDS_ASW},
{EVENT_SHAKEHANDS_ASW,	ST_DIAL,		ST_DIAL,			DO_SHAKEHANDS_ASW},
{EVENT_SHAKEHANDS_ASW,	ST_CALL,		ST_CALL,			DO_SHAKEHANDS_ASW},
{EVENT_SHAKEHANDS_ASW,	ST_TALK,		ST_TALK,			DO_SHAKEHANDS_ASW},

{EVENT_RING,		ST_SHAKEHANDS_ASW,	ST_SHAKEHANDS_ASW,	DO_JUDGE_TYPE},
{EVENT_TYPE_HM,		ST_SHAKEHANDS_ASW,	ST_RING,			DO_RING},
{EVENT_TYPE_DMK,	ST_SHAKEHANDS_ASW,	ST_RING,			DO_RING},
{EVENT_TYPE_HDMK,	ST_SHAKEHANDS_ASW,	ST_RING,			DO_RING},
{EVENT_DIAL,		ST_SHAKEHANDS,		ST_DIAL,			DO_DIAL},
{EVENT_TYPE_HM,		ST_JUDGE_TYPE,	ST_IDLE,			DO_NO},
{EVENT_TYPE_DMK,	ST_JUDGE_TYPE,	ST_IDLE,			DO_NO},
{EVENT_TYPE_HDMK,	ST_JUDGE_TYPE,	ST_IDLE,			DO_NO},

{EVENT_DIAL,		ST_IDLE,		ST_DIAL,			DO_DIAL},

{EVENT_RING_EX,		ST_IDLE,		ST_RING,			DO_RING},

{EVENT_RING,		ST_IDLE,		ST_JUDGE_TYPE,		DO_JUDGE_TYPE},
{EVENT_RING,		ST_DIAL,		ST_DIAL,			DO_RET_FAIL},
{EVENT_RING,		ST_CALL,		ST_CALL,			DO_RET_FAIL},
{EVENT_RING,		ST_TALK,		ST_TALK,			DO_RET_FAIL},
{EVENT_RING,		ST_RING,		ST_RING,			DO_RET_FAIL},
{EVENT_TYPE_CENTER,	ST_JUDGE_TYPE,	ST_RING,			DO_RING},

{EVENT_CALL,		ST_DIAL,		ST_CALL,			DO_CALL},

{EVENT_TALK,		ST_CALL,		ST_TALK,			DO_TALK},
{EVENT_TALK,		ST_RING,		ST_TALK,			DO_TALK},

{EVENT_TALK_EX,		ST_RING,		ST_TALK,			DO_TALK_EX},

{EVENT_FAIL_COMM,	ST_DIAL,		ST_IDLE,			DO_FAIL_COMM},
{EVENT_FAIL_COMM,	ST_SHAKEHANDS,	ST_IDLE,			DO_FAIL_COMM},

{EVENT_FAIL_BUSY,	ST_DIAL,		ST_IDLE,			DO_FAIL_BUSY},
{EVENT_FAIL_SHAKE,	ST_SHAKEHANDS,	ST_IDLE,			DO_FAIL_SHAKE},
{EVENT_FAIL_SHAKE,	ST_SHAKEHANDS_ASW,	ST_IDLE,		DO_FAIL_SHAKE},

{EVENT_FAIL_ABORT,	ST_CALL,		ST_IDLE,			DO_FAIL_ABORT},
{EVENT_FAIL_ABORT,	ST_TALK,		ST_IDLE,			DO_FAIL_ABORT},
{EVENT_FAIL_ABORT,	ST_RING,		ST_IDLE,			DO_FAIL_ABORT},

{EVENT_HANGUP,	ST_SHAKEHANDS,	ST_IDLE,			DO_HANGUP},
{EVENT_HANGUP,	ST_SHAKEHANDS_ASW,	ST_IDLE,		DO_HANGUP},
{EVENT_HANGUP,	ST_JUDGE_TYPE,	ST_IDLE,			DO_HANGUP},
{EVENT_HANGUP,	ST_RING,		ST_IDLE,			DO_HANGUP},
{EVENT_HANGUP,	ST_DIAL,		ST_IDLE,			DO_HANGUP},
{EVENT_HANGUP,	ST_CALL,		ST_IDLE,			DO_HANGUP},
{EVENT_HANGUP,	ST_TALK,		ST_IDLE,			DO_HANGUP},

};

static StateTable stm_video_state_u9[] = {

{EVENT_TYPE_HM,		ST_JUDGE_TYPE,	ST_RING,			DO_RING},
{EVENT_TYPE_DMK,	ST_JUDGE_TYPE,	ST_RING,			DO_RING},
{EVENT_TYPE_HDMK,	ST_JUDGE_TYPE,	ST_RING,			DO_RING},

{EVENT_DIAL,		ST_IDLE,		ST_DIAL,			DO_DIAL},

{EVENT_RING_EX,		ST_IDLE,		ST_RING,			DO_RING},

{EVENT_RING,		ST_IDLE,		ST_JUDGE_TYPE,		DO_JUDGE_TYPE},
{EVENT_RING,		ST_DIAL,		ST_DIAL,			DO_RET_FAIL},
{EVENT_RING,		ST_CALL,		ST_CALL,			DO_RET_FAIL},
{EVENT_RING,		ST_TALK,		ST_TALK,			DO_RET_FAIL},
{EVENT_RING,		ST_RING,		ST_RING,			DO_RET_FAIL},
{EVENT_TYPE_CENTER,	ST_JUDGE_TYPE,	ST_RING,			DO_RING},

{EVENT_CALL,		ST_DIAL,		ST_CALL,			DO_CALL},

{EVENT_TALK,		ST_CALL,		ST_TALK,			DO_TALK},
{EVENT_TALK,		ST_RING,		ST_TALK,			DO_TALK},

{EVENT_TALK_EX,		ST_RING,		ST_TALK,			DO_TALK_EX},

{EVENT_FAIL_COMM,	ST_DIAL,		ST_IDLE,			DO_FAIL_COMM},
{EVENT_FAIL_COMM,	ST_SHAKEHANDS,	ST_IDLE,			DO_FAIL_COMM},

{EVENT_FAIL_BUSY,	ST_DIAL,		ST_IDLE,			DO_FAIL_BUSY},
{EVENT_FAIL_SHAKE,	ST_SHAKEHANDS,	ST_IDLE,			DO_FAIL_SHAKE},
{EVENT_FAIL_SHAKE,	ST_SHAKEHANDS_ASW,	ST_IDLE,		DO_FAIL_SHAKE},

{EVENT_FAIL_ABORT,	ST_CALL,		ST_IDLE,			DO_FAIL_ABORT},
{EVENT_FAIL_ABORT,	ST_TALK,		ST_IDLE,			DO_FAIL_ABORT},
{EVENT_FAIL_ABORT,	ST_RING,		ST_IDLE,			DO_FAIL_ABORT},

{EVENT_HANGUP,	ST_SHAKEHANDS,	ST_IDLE,			DO_HANGUP},
{EVENT_HANGUP,	ST_SHAKEHANDS_ASW,	ST_IDLE,		DO_HANGUP},
{EVENT_HANGUP,	ST_JUDGE_TYPE,	ST_IDLE,			DO_HANGUP},
{EVENT_HANGUP,	ST_RING,		ST_IDLE,			DO_HANGUP},
{EVENT_HANGUP,	ST_DIAL,		ST_IDLE,			DO_HANGUP},
{EVENT_HANGUP,	ST_CALL,		ST_IDLE,			DO_HANGUP},
{EVENT_HANGUP,	ST_TALK,		ST_IDLE,			DO_HANGUP},

};

/* ---------------------------------------------------------------------------*/
/**
 * @brief sendCmd 发送主动呼叫命令
 *
 * @param IP 对方IP
 * @param Port 对方端口
 * @param Cmd 呼叫指令
 * @param bCallBack 呼叫结果是否有回调函数
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int sendCmd(VideoTrans * This,
		char *IP, int Port,
		int Cmd,
		int bCallBack)
{
	char cBuf[sizeof(COMMUNICATION)+sizeof(COMMUNICATION_CALL)];
	COMMUNICATION * pComm = (COMMUNICATION *)cBuf;
	COMMUNICATION_CALL *pCall = (COMMUNICATION_CALL *)&cBuf[sizeof(COMMUNICATION)];
	pComm->ID = This->priv->packet_id++;
	pComm->Size = sizeof(COMMUNICATION)+sizeof(COMMUNICATION_CALL);
	pComm->Type = This->priv->call_cmd;
	pCall->Cmd = Cmd;
	strncpy(pCall->CallID,This->priv->room_id,sizeof(pCall->CallID)-1);
	strncpy(pCall->CallName,This->priv->room_name,sizeof(pCall->CallName)-1);
	pCall->CallType = This->priv->device_type;

	pCall->VideoType = 0;
	pCall->VideoWidth = This->priv->dwInCameraWidth;
	pCall->VideoHeight = This->priv->dwInCameraHeight;
    This->interface->udpSend(This,
            IP, Port, cBuf, pComm->Size, bCallBack);
	return 1;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief sendCmdRevert 发送对方呼叫命令的回应
 *
 * @param IP 对方IP
 * @param Port 对方端口
 * @param Cmd 回应的命令
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int sendCmdRevert(VideoTrans * This,
		char *IP,
		int Port,
		int Cmd)
{
	char cBuf[sizeof(COMMUNICATION)+sizeof(COMMUNICATION_CALL)+4];
	COMMUNICATION * pComm = (COMMUNICATION *)cBuf;
	COMMUNICATION_CALL *pRevert = (COMMUNICATION_CALL *)&cBuf[sizeof(COMMUNICATION)];
	memset(cBuf,0,sizeof(COMMUNICATION)+sizeof(COMMUNICATION_CALL)+4);
	pComm->ID = This->priv->packet_id++;
	pComm->Size = sizeof(COMMUNICATION)+sizeof(COMMUNICATION_CALL);
	pComm->Type = This->priv->call_cmd;
	pRevert->Cmd = Cmd;
	pRevert->CallType = This->priv->device_type;

	pRevert->VideoType = 0;
	pRevert->VideoWidth = This->priv->dwInCameraWidth;
	pRevert->VideoHeight = This->priv->dwInCameraHeight;
	strncpy(pRevert->CallID,This->priv->room_id,sizeof(pRevert->CallID)-1);
	strncpy(pRevert->CallName,This->priv->room_name,sizeof(pRevert->CallName)-1);

	This->interface->udpSend(This,IP,Port,cBuf,pComm->Size,0);

	return 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief videoCallbackOvertime 呼叫回调函数，呼叫超时时调用
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void videoCallbackOvertime(VideoTrans *This)
{
	stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			sizeof(STMData));
	stm_data->flag = 0;
	This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_FAIL_COMM,stm_data);
	DBG_P("[%s]Call Time out, Failed!\n",__FUNCTION__);

}

/* ---------------------------------------------------------------------------*/
/**
 * @brief callbackRtp 传输视频时中断超时回调函数
 */
/* ---------------------------------------------------------------------------*/
void callbackRtp(void)
{
	// TODO
	// VideoTrans * This = &video;
	// stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			// sizeof(STMData));
	// stm_data->flag = 3;
	// This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_FAIL_ABORT,stm_data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief videoTransMulitDevSendOver 挂断分机
 *
 * @param This
 * @param bTalkOver 1 分机接听，挂断其他分机 0 挂断所有分机
 */
/* ---------------------------------------------------------------------------*/
static void videoTransMulitDevSendOver(VideoTrans *This,int bTalkOver)
{
	int i;
	if(This->priv->master_ip[0] != 0)
		return;

	struct _TCALLFJ TransCall;

	memset(&TransCall,0,sizeof(TCALLFJ));
	TransCall.ID = This->priv->packet_id++;
	TransCall.Size = sizeof(TCALLFJ);
	TransCall.Type = This->priv->call_cmd;
	TransCall.Cmd = CMD_TRANSOVER;
	strncpy(TransCall.CallIP,This->priv->cPeerIP,15);
	TransCall.CallPort = This->priv->PeerPort;
	for(i=0;i<MAXFJCOUNT;i++) {
		if(This->priv->RegDev[i].bCalling && (!bTalkOver || !This->priv->RegDev[i].bTalk)) {
			This->priv->RegDev[i].bCalling = FALSE;
			This->priv->RegDev[i].bTalk = FALSE;
			// DBG_P("[%s]:over:%d,%s\n", __FUNCTION__,bTalkOver,This->priv->RegDev[i].IP);
			This->interface->udpSend(This,This->priv->RegDev[i].IP,This->priv->RegDev[i].Port,
					&TransCall,sizeof(TCALLFJ),0);
		}
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief stmDoNothing 不做任何事，只改变状态
 *
 * @param This
 * @param st_data
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int stmDoNothing(VideoTrans * This,STMData *st_data)
{
	return CALL_OK;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief stmJudgeType 判断呼叫来源类型，门口机或室内机
 *
 * @param This
 * @param st_data
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int stmJudgeType(VideoTrans * This,STMData *st_data)
{
	DBG_P("STMDO:JudgeType()\n");
	char *IP = st_data->ip;
	int Port= st_data->port;
	stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			sizeof(STMData));
	memcpy(stm_data,st_data,sizeof(STMData));
	stm_data->device_index = This->interface->isCenter(This,IP);
	stm_data->device_type = EVENT_TYPE_CENTER;
	if (stm_data->device_index != -1 ) {
		if (This->priv->pro_type == VIDEOTRANS_PROTOCOL_3000)
			sendCmdRevert(This,IP,Port,CMD_SHAKEHANDS);	//通知管理中心使用3000协议
		goto judge_end;
	}
	stm_data->device_index = This->interface->isDmk(This,IP);
	stm_data->device_type = EVENT_TYPE_DMK;
	if (stm_data->device_index != -1 )
		goto judge_end;

	stm_data->device_index = This->interface->isHDmk(This,IP);
	stm_data->device_type = EVENT_TYPE_HDMK;
	if (stm_data->device_index != -1 )
		goto judge_end;

	stm_data->device_type = EVENT_TYPE_HM;

judge_end:
	This->priv->st_machine->msgPost(This->priv->st_machine,
			stm_data->device_type,
			stm_data);
	return CALL_OK;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmCallIP 直接呼叫IP
 * 如果成功，视设置需要转输音频至对方，但不传输音频
 * @param This
 * @param IP 对方IP
 * @param Port 对方端口
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int stmCallIP(VideoTrans * This,STMData *st_data)
{
	DBG_P("STMDO:CallIP():%s\n",st_data->mCallIP);
	This->priv->PeerPort = This->priv->port;
	strcpy(This->priv->cPeerIP,st_data->mCallIP);
	sendCmd(This,This->priv->cPeerIP,This->priv->port,CMD_CALL,TRUE);
	This->interface->sendMessageStatus(This,VIDEOTRANS_UI_CALLIP);
	This->priv->time_comm = COMM_TIME;
	return CALL_OK;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmShakeHands 呼叫对讲前的握手命令
 * 在使用3000协议对讲时需要增加握手协议，以区别U9协议，2者互不能通信
 * 在呼叫管理中心时，需要先发握手，不需等回应，再发呼叫
 *
 * @param This
 * @param IP 对方IP
 * @param Port 对方端口
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int stmShakeHands(VideoTrans * This,STMData *st_data)
{
	DBG_P("STMDO:ShakeHands()\n");
	strcpy(This->priv->cPeerIP,st_data->mCallIP);
	sendCmd(This,This->priv->cPeerIP,This->priv->port,CMD_SHAKEHANDS,TRUE);
	This->interface->sendMessageStatus(This,VIDEOTRANS_UI_SHAKEHANDS);
	This->priv->time_shake = SHAKE_TIME;
	return CALL_OK;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmShakeHandsRet 呼叫对讲前的握手命令返回
 * 在使用3000协议对讲时需要增加握手协议，以区别U9协议，2者互不能通信
 *
 * @param This
 * @param IP 对方IP
 * @param Port 对方端口
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int stmShakeHandsRet(VideoTrans * This,STMData *st_data)
{
	DBG_P("STMDO:ShakeHandsRet()\n");
	sendCmdRevert(This,st_data->ip,st_data->port,CMD_SHAKEHANDS_ASW);
	This->priv->time_shake = SHAKE_TIME;
	return CALL_OK;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmRetCall 返回呼叫的结果
 *
 * @param This
 * @param RetCode ASW_OK 呼叫成功，开始建立RTP连接传输音频，但不传输
 * 视频
 *				  ASW_FAIL 呼叫失败，说明对方在线，但出于正忙状态
 */
/* ---------------------------------------------------------------------------*/
static int stmRetCall(VideoTrans *This,STMData *st_data)
{
	DBG_P("STMDO:RetIP():w:%d,h:%d\n",
			st_data->p_call.VideoWidth,st_data->p_call.VideoHeight);
	This->priv->dwInCameraWidth = st_data->p_call.VideoWidth;
	This->priv->dwInCameraHeight = st_data->p_call.VideoHeight;
	This->priv->time_call = RING_TIME;
	int index = This->interface->isDmk(This,st_data->ip);
	memset(This->priv->cPeerRoom,0,sizeof(This->priv->cPeerRoom));
	if (index != -1) {
		strncpy(This->priv->cPeerRoom,
				This->interface->getDmkName(This,index),
				sizeof(This->priv->cPeerRoom)-1);
		This->interface->sendMessageStatus(This,VIDEOTRANS_UI_RETCALL_MONITOR);
		goto ret_call_end;
	}
	index = This->interface->isHDmk(This,st_data->ip);
	if (index != -1) {
		strncpy(This->priv->cPeerRoom,
				This->interface->getHDmkName(This,index),
				sizeof(This->priv->cPeerRoom)-1);
		This->interface->sendMessageStatus(This,VIDEOTRANS_UI_RETCALL_MONITOR);
		goto ret_call_end;
	}

	strncpy(This->priv->cPeerRoom,
			st_data->p_call.CallName,
			sizeof(This->priv->cPeerRoom)-1);
	This->interface->sendMessageStatus(This,VIDEOTRANS_UI_RETCALL);

ret_call_end:
	This->priv->call_dir = VIDEOTRANS_CALL_DIR_OUT;
	This->interface->saveRecordAsync(This,VIDEOTRANS_CALL_DIR_OUT,
			This->priv->cPeerRoom,This->priv->cPeerIP);
	return CALL_OK;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmRing 对方主动呼叫,振铃，并显示IP地址，如果对方
 * 传输视频，须显示。
 * 当为3000系统时，如果是室内机呼叫，需要先接受过握手协议，才响应，
 * 如果是管理中心，先回握手协议，再响应
 *
 *
 * @param This
 * @param IP 对方IP
 * @param Port 对方端口
 * @param pCall 相关参数
 *
 * @returns
// 如果成功，返回非0,失败返回0
 */
/* ---------------------------------------------------------------------------*/
static int stmRing(VideoTrans * This,STMData *st_data)
{
	int i;
	char *IP = st_data->ip;
	int Port= st_data->port;
	COMMUNICATION_CALL *pCall = &st_data->p_call;

	This->priv->dwInCameraWidth = st_data->p_call.VideoWidth;
	This->priv->dwInCameraHeight = st_data->p_call.VideoHeight;

	This->priv->m_CodeType = pCall->VideoType;
	This->priv->dwPeerType = pCall->CallType;
	//设置对方IP，根据IP和端口号发送回复给对方
	strcpy(This->priv->cPeerIP,IP);
	strncpy(This->priv->cPeerID,pCall->CallID,sizeof(This->priv->cPeerID)-1);
	This->priv->PeerPort = This->priv->port;

	if (This->priv->master_trans)
		sendCmdRevert(This,IP,Port,ASW_TRANSCALL_PERMITTED);
	else
		sendCmdRevert(This,IP,Port,ASW_OK);
	memset(This->priv->cPeerRoom,0,sizeof(This->priv->cPeerRoom));
	if (st_data->device_type == EVENT_TYPE_DMK ) {
		strncpy(This->priv->cPeerRoom,
				This->interface->getDmkName(protocol_video,st_data->device_index),
				sizeof(This->priv->cPeerRoom)-1);
	} else if (st_data->device_type == EVENT_TYPE_HDMK ) {
		strncpy(This->priv->cPeerRoom,
				This->interface->getHDmkName(protocol_video,st_data->device_index),
				sizeof(This->priv->cPeerRoom)-1);
	} else {
		strncpy(This->priv->cPeerRoom,
				pCall->CallName,
				sizeof(This->priv->cPeerRoom)-1);
	}

	// 转发到分机-------
	uint64_t dwTick = This->interface->getSystemTick(This);
	TCALLFJ TransCall;
	memcpy(&(TransCall.Cmd),pCall,sizeof(COMMUNICATION_CALL));
	TransCall.ID = This->priv->packet_id++;
	TransCall.Size = sizeof(TCALLFJ);
	TransCall.Type = This->priv->call_cmd;
	TransCall.Cmd = CMD_TRANSCALL;
	strncpy(TransCall.CallIP,IP,15);
	TransCall.CallPort = Port;
	TransCall.TransByCenter = 0;
	TransCall.CenterIP = 0;
	for(i=0; i<MAXFJCOUNT; i++) {
		if(This->priv->RegDev[i].dwLastTick && This->interface->getDiffSysTick(This,dwTick, This->priv->RegDev[i].dwLastTick) < FJ_REGIST_TIME) {
			This->priv->RegDev[i].bCalling = TRUE;
			This->interface->udpSend(This,This->priv->RegDev[i].IP,This->priv->RegDev[i].Port,&TransCall,
					sizeof(TCALLFJ),0);
		} else {
			This->priv->RegDev[i].bCalling = FALSE;
		}
	}
	// ----------

	This->priv->time_call = RING_TIME;
	This->interface->sendMessageStatus(This, VIDEOTRANS_UI_RING);
	This->priv->call_dir = VIDEOTRANS_CALL_DIR_IN;
	This->interface->saveRecordAsync(This,VIDEOTRANS_CALL_DIR_IN,
			This->priv->cPeerRoom,This->priv->cPeerIP);
	return CALL_OK;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmAnswer 对方接听，进行通话(传输音频或音视频同时传输)
 *
 * @param This
 * @param IP 对方IP
 * @param Port 对方端口
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int stmAnswer(VideoTrans * This,STMData *st_data)
{
	DBG_P("STMDO:Answer()\n");
	This->priv->PeerPort = This->priv->port;
	if(This->priv->leave_word_state) {
		This->priv->time_call = LEAVE_WORD_TIME;
		This->interface->sendMessageStatus(This,VIDEOTRANS_UI_LEAVE_WORD);
		This->priv->leave_word = 1;
	} else {
		This->priv->time_call = TALK_TIME;
		This->interface->sendMessageStatus(This,VIDEOTRANS_UI_ANSWER);
	}

	if (This->priv->master_trans) {
		TCALLFJ Packet;
		memset(&Packet,0,sizeof(TCALLFJ));
		Packet.ID = This->priv->packet_id++;
		Packet.Size = sizeof(TCALLFJ);
		Packet.Type = This->priv->call_cmd;
		Packet.Cmd = CMD_TRANSTALK;
		This->interface->udpSend(This,This->priv->cPeerIP,This->priv->PeerPort,&Packet,Packet.Size,0);
	} else {
		sendCmd(This,
				This->priv->cPeerIP,
				This->priv->PeerPort,
				CMD_TALK,
				FALSE);
	}
	videoTransMulitDevSendOver(This,FALSE);
	return TRUE;
}

static int stmAnswerEx(VideoTrans * This,STMData *st_data)
{
	DBG_P("STMDO:AnswerEx()\n");
	int i;
	char *IP = st_data->ip;
	for(i=0;i<MAXFJCOUNT;i++) {
		if(strcmp(This->priv->RegDev[i].IP,IP)!=0 || !This->priv->RegDev[i].bCalling)
			continue;
		DBG_P("[%s]:%s\n", __FUNCTION__,This->priv->RegDev[i].IP);
		sendCmd(This,
				This->priv->cPeerIP,
				This->priv->PeerPort,
				CMD_TALK,
				FALSE);
		This->priv->time_call = TALK_TIME;

		This->priv->RegDev[i].bTalk = TRUE;
		//挂断其它分机
		videoTransMulitDevSendOver(This,TRUE);
		This->interface->sendMessageStatus(This,VIDEOTRANS_UI_ANSWER_EX);

		break;
	}
	return TRUE;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmRetFail 通话中有被监视，返回失败
 *
 * @param This
 * @param st_data
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int stmRetFail(VideoTrans * This,STMData *st_data)
{
	DBG_P("STMDO:RetFail()\n");
	sendCmdRevert(This,st_data->ip,st_data->port,ASW_FAIL);
	return CALL_OK;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmOver 结束对讲
 *
 * @param This
 * @param ByMyself 2超时或非正常挂机（对方返回忙或通话过程断线) 1本机主动按键挂机，0远程发起命令结束
 */
/* ---------------------------------------------------------------------------*/
static int stmOver(VideoTrans * This,STMData *st_data)
{
	DBG_P("STMDO:Over():ByMyself:%d\n",st_data->flag);
	int ByMyself = st_data->flag;
	This->priv->leave_word_state = FALSE;
	This->priv->time_call = 0;
	This->priv->master_trans = 0;

	if(ByMyself) {
		sendCmd(This,
				This->priv->cPeerIP,
				This->priv->PeerPort,
				CMD_OVER,
				FALSE);
	}
	videoTransMulitDevSendOver(This,FALSE);

	memset(This->priv->cPeerRoom,0,sizeof(This->priv->cPeerRoom));
	This->priv->cPeerIP[0] = 0;
	This->priv->PeerPort = 0;
	This->interface->sendMessageStatus(This,VIDEOTRANS_UI_OVER);
	DBG_P("stmOver\n");
	return CALL_OK;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmFail 返回失败
 *
 * @param This
 * @param st_data
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int stmFail(VideoTrans * This,STMData *st_data)
{
	DBG_P("STMDO:Fail(),type:%d\n",st_data->flag);
	int type = st_data->flag;
	if (type == 0) {
		This->interface->sendMessageStatus(This,VIDEOTRANS_UI_FAILCOMM);
	} else if (type == 1) {
		This->interface->sendMessageStatus(This,VIDEOTRANS_UI_FAILSHAKEHANDS);
	} else if (type == 2) {
		This->interface->sendMessageStatus(This,VIDEOTRANS_UI_FAILBUSY);
	} else if (type == 3) {
		st_data->flag = 1;
		stmOver(This,st_data);
		This->interface->sendMessageStatus(This,VIDEOTRANS_UI_FAILABORT);
	}

	return CALL_OK;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief videoCall 发送呼叫命令
 *
 * @param This
 * @param ip 被呼叫IP
 */
/* ---------------------------------------------------------------------------*/
static void videoCall(VideoTrans *This,char *ip)
{
	stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			sizeof(STMData));
	memcpy(stm_data->mCallIP,ip,sizeof(stm_data->mCallIP));

	stm_data->device_index = This->interface->isCenter(This,ip);
	stm_data->device_type = EVENT_TYPE_CENTER;
	if (stm_data->device_index != -1 ) {
		// 没获取机身码时不可以呼叫
		if (This->priv->enable) {
			if (This->priv->pro_type == VIDEOTRANS_PROTOCOL_3000)
				sendCmd(This,ip,This->priv->port,CMD_SHAKEHANDS,FALSE);
			This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_DIAL,stm_data);
			This->interface->showCallWindow(This);
		} else {
			This->interface->showCallWindow(This);
			This->interface->sendMessageStatus(This,VIDEOTRANS_UI_FAILABORT);
		}
		return;
	}

	stm_data->device_index = This->interface->isDmk(This,ip);
	stm_data->device_type = EVENT_TYPE_DMK;
	if (stm_data->device_index != -1 )
		goto call_end;

	stm_data->device_index = This->interface->isHDmk(This,ip);
	stm_data->device_type = EVENT_TYPE_HDMK;
	if (stm_data->device_index != -1 )
		goto call_end;

	stm_data->device_type = EVENT_TYPE_HM;

call_end:

	if (This->priv->enable) {
		if (This->priv->pro_type == VIDEOTRANS_PROTOCOL_3000)
			This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_SHAKEHANDS,stm_data);
		else
			This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_DIAL,stm_data);
		This->interface->showCallWindow(This);
	} else {
		This->interface->showCallWindow(This);
		This->interface->sendMessageStatus(This,VIDEOTRANS_UI_FAILABORT);
	}
}

static void videoAnswer(VideoTrans *This)
{
	This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_TALK,0);
}

static void videoLeaveWord(VideoTrans *This)
{
	This->priv->leave_word_state = TRUE;
	This->answer(This);
}

static void videoHangup(VideoTrans *This)
{
	stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			        sizeof(STMData));
	stm_data->flag = 1;
	This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_HANGUP,stm_data);
}


static char *videoGetPeerIP(VideoTrans *This)
{
	return This->priv->cPeerIP;
}
static int videoGetTalkPort(VideoTrans *This)
{
	return 8800;
}
static int videoGetIdleStatus(VideoTrans *This)
{
	if (This->priv->st_machine->getCurrentstate(This->priv->st_machine) == ST_IDLE)
		return 1;
	else
		return 0;
}
static int videoGetTalkStatus(VideoTrans *This)
{
	if (This->priv->st_machine->getCurrentstate(This->priv->st_machine) == ST_TALK)
		return 1;
	else
		return 0;
}

static void videosetStatusBusy(VideoTrans *This)
{
	This->priv->st_machine->setCurrentstate(This->priv->st_machine,ST_TALK);
}

static void videosetStatusIdle(VideoTrans *This)
{
	This->priv->st_machine->setCurrentstate(This->priv->st_machine,ST_IDLE);
}

static void videoUnlock(VideoTrans * This)
{
	if (This->priv->st_machine->getCurrentstate(This->priv->st_machine) == ST_IDLE)
		return;
	sendCmd(This,
			This->priv->cPeerIP,
			This->priv->PeerPort,
			CMD_UNLOCK,
			FALSE);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief videoTransDispCallTime 获得当前通话倒计时时间，用于显示与
 * 计时
 *
 * @param This
 *
 * @returns 通话时间
 */
/* ---------------------------------------------------------------------------*/
static VideoProtocolStatus videoTransDispCallTime(VideoTrans *This,int *disp_time)
{
	static int PreTime = 0;
	if(This->priv->time_call == PreTime) {
		return VIDEOTRANS_STATUS_NULL;
	}
	*disp_time = PreTime = This->priv->time_call;

	switch (This->priv->st_machine->getCurrentstate(This->priv->st_machine))
	{
		case ST_CALL:
		case ST_RING:
			return VIDEOTRANS_STATUS_RING;

		case ST_TALK:
			return VIDEOTRANS_STATUS_TALK;

		default :
			return VIDEOTRANS_STATUS_NULL;
	}
}

static char * videoTransDispCallName(VideoTrans *This)
{
	return This->priv->cPeerRoom;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief videoTransHandleCallTime 在1s定时器函数执行，通话时-1
 *
 * @param This
 *
 */
/* ---------------------------------------------------------------------------*/
static void videoTransHandleCallTime(VideoTrans *This)
{
	if (!This)
		return;

	if (This->priv->st_machine->getCurrentstate(This->priv->st_machine) == ST_IDLE)
		return;

	if(This->priv->time_call) {
		This->priv->time_comm = 0;
		This->priv->time_shake = 0;
		// DBG_P("time_call:%d\n", This->priv->time_call);
		if(--This->priv->time_call == 0) {
			stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
					sizeof(STMData));
			stm_data->flag = 1;
			This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_HANGUP,stm_data);
			return;
		}
	}
	if (This->priv->time_comm) {
		This->priv->time_shake = 0;
		DBG_P("time_comm:%d\n", This->priv->time_comm);
		if (--This->priv->time_comm == 0) {
			stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
					sizeof(STMData));
			stm_data->flag = 0;
			This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_FAIL_COMM,stm_data);
		}
	}
	if (This->priv->time_shake) {
		DBG_P("time_shake:%d\n", This->priv->time_shake);
		if (--This->priv->time_shake == 0) {
			stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
					sizeof(STMData));
			stm_data->flag = 1;
			This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_FAIL_SHAKE,stm_data);
		}
	}
	return ;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief videoTransDestroy 销毁对象，退出程序
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void videoTransDestroy(VideoTrans * This)
{
	This->priv->st_machine->destroy(&This->priv->st_machine);
	free(This->interface);
	free(This->priv);
	free(This);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpCallRing 通话协议:被呼叫
 *
 * @param This
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpCallRing(VideoTrans * This,char *ip,int port,char *data)
{
	if (!This->priv->enable)
		return;
	DBG_P("CMD_CALL:Ring()\n");
	COMMUNICATION_CALL *pCall = (COMMUNICATION_CALL *)&data[sizeof(COMMUNICATION)];
	stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			sizeof(STMData));
	strcpy(stm_data->ip,ip);
	stm_data->port = port;
	memcpy(&stm_data->p_call,pCall,sizeof(COMMUNICATION_CALL));
	This->priv->master_trans = 0;
	This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_RING,stm_data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpCallRingEx 作为分机时被呼叫
 *
 * @param This
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpCallRingEx(VideoTrans * This,char *ip,int port,char *data)
{
	DBG_P("CMD_TRANSCALL:RingEx()\n");
	TCALLFJ *pCall = (TCALLFJ *)data;
	stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			sizeof(STMData));
	strcpy(stm_data->ip,ip);
	stm_data->port = port;
	memcpy(&stm_data->p_call_ex,pCall,sizeof(TCALLFJ));
	This->priv->master_trans = 1;
	This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_RING_EX,data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpCallTalk 通话协议:对方接听
 *
 * @param This
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpCallTalk(VideoTrans * This,char *ip,int port,char *data)
{
	DBG_P("CMD_TALK:Answer()\n");
	This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_TALK,0);
}

static void udpCallTalkEx(VideoTrans * This,char *ip,int port,char *data)
{
	DBG_P("CMD_TRANSTALK:AnswerEx()\n");
	stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			sizeof(STMData));
	strcpy(stm_data->ip,ip);
	stm_data->port = port;
	This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_TALK_EX,stm_data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpCallUnlock 作为主机时，分机通话中开锁，主机转发开锁命令
 *
 * @param This
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpCallUnlock(VideoTrans * This,char *ip,int port,char *data)
{
	DBG_P("CMD_UNLOCK()\n");
	if(This->priv->master_ip[0] != 0)
		return;
	int i;
	for(i=0; i<MAXFJCOUNT; i++) {
		if(strcmp(ip,This->priv->RegDev[i].IP) == 0) {
			DBG_P("[%s]:%s\n", __FUNCTION__,This->priv->RegDev[i].IP);
			This->unlock(This);
		}
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief udpCallOver 通话协议:对方挂机
 *
 * @param This
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpCallOver(VideoTrans * This,char *ip,int port,char *data)
{
	DBG_P("CMD_OVER:Over():%s\n", ip);
	stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			sizeof(STMData));
	if (strcmp(ip,This->priv->cPeerIP) == 0)
		stm_data->flag = 0;
	else
		stm_data->flag = 1;
	This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_HANGUP,stm_data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpCallAswOk 通话协议:呼叫后对方应答OK
 *
 * @param This
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpCallAswOk(VideoTrans * This,char *ip,int port,char *data)
{
	DBG_P("CMD_ASW_OK:RetCall()IP:%s\n",ip);
	if (strcmp(ip,This->priv->cPeerIP) == 0){
		COMMUNICATION_CALL *pCall = (COMMUNICATION_CALL *)&data[sizeof(COMMUNICATION)];
		stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
				sizeof(STMData));
		strcpy(stm_data->ip,ip);
		stm_data->port = port;
		memcpy(&stm_data->p_call,pCall,sizeof(COMMUNICATION_CALL));
		This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_CALL,stm_data);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpCallAswFail 通话协议:呼叫后对方应答失败(正忙)
 *
 * @param This
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpCallAswFail(VideoTrans * This,char *ip,int port,char *data)
{
	DBG_P("CMD_ASW_FAIL:RetFail()IP:%s\n",ip);
	stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			sizeof(STMData));
	stm_data->flag = 2;
	This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_FAIL_BUSY,stm_data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpCallShakeHands 通话协议:对方发送握手协议
 *
 * @param This
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpCallShakeHands(VideoTrans * This,char *ip,int port,char *data)
{
	if (This->priv->pro_type != VIDEOTRANS_PROTOCOL_3000)
		return;
	DBG_P("CMD_SHAKEHANDS :shakeHandsRet()\n");
	stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			sizeof(STMData));
	strcpy(stm_data->ip,ip);
	stm_data->port = port;
	This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_SHAKEHANDS_ASW,stm_data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpCallShakeHandsAsw 通话协议:对方应答握手协议
 *
 * @param This
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpCallShakeHandsAsw(VideoTrans * This,char *ip,int port,char *data)
{
	if (This->priv->pro_type != VIDEOTRANS_PROTOCOL_3000)
		return;
	DBG_P("CMD_SHAKEHANDS_ASW :CallIP():%s\n",ip);
	stm_data = (STMData *)This->priv->st_machine->initPara(This->priv->st_machine,
			sizeof(STMData));
	memcpy(stm_data->mCallIP,ip,sizeof(stm_data->mCallIP));
	This->priv->st_machine->msgPost(This->priv->st_machine,EVENT_DIAL,stm_data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpCallDenied 通话协议：作为主机时分机拒绝接听
 *
 * @param This
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpCallDenied(VideoTrans * This,char *ip,int port,char *data)
{
	DBG_P("ASW_TRANSCALL_DENIED()\n");
	int i;
	for(i=0;i<MAXFJCOUNT;i++) {
		if(    strcmp(ip,This->priv->RegDev[i].IP) == 0
			&& This->priv->RegDev[i].bCalling) {
			DBG_P("[%s]:%s\n", __FUNCTION__,This->priv->RegDev[i].IP);
			This->priv->RegDev[i].bCalling = FALSE;
			break;
		}
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief 通话状态机执行函数，与枚举顺序一致
 */
/* ---------------------------------------------------------------------------*/
static STMDo stm_video_state_do[] = {
	{DO_NO,				stmDoNothing},
	{DO_SHAKEHANDS,		stmShakeHands},
	{DO_SHAKEHANDS_ASW,	stmShakeHandsRet},
	{DO_JUDGE_TYPE,		stmJudgeType},
	{DO_DIAL,			stmCallIP},
	{DO_CALL,			stmRetCall},
	{DO_TALK,			stmAnswer},
	{DO_TALK_EX,		stmAnswerEx},
	{DO_RING,			stmRing},
	{DO_FAIL_COMM,		stmFail},
	{DO_FAIL_SHAKE,		stmFail},
	{DO_FAIL_BUSY,		stmFail},
	{DO_FAIL_ABORT,		stmFail},
	{DO_RET_FAIL,		stmRetFail},
	{DO_HANGUP,			stmOver},
};

/* ---------------------------------------------------------------------------*/
/**
 * @brief videoTransSTMHanldeVideo 呼叫流程状态机执行体
 *
 * @param This
 * @param data 传入参数
 */
/* ---------------------------------------------------------------------------*/
static int videoTransSTMHanldeVideo(StMachine *This,int result,void *data,void *arg)
{
	STMData *st_data = (STMData *)data;
	stm_video_state_do[This->getCurRun(This)].proc(arg,st_data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief 通话协议处理表
 */
/* ---------------------------------------------------------------------------*/
static UdpCallCmd udp_call_cmd_handle[] = {
	{CMD_CALL,				udpCallRing},
	{CMD_TALK,				udpCallTalk},
	{CMD_OVER,				udpCallOver},
	{CMD_UNLOCK,			udpCallUnlock},
	{ASW_TRANSCALL_DENIED,	udpCallDenied},
	{ASW_OK,				udpCallAswOk},
	{ASW_FAIL,				udpCallAswFail},
	{CMD_SHAKEHANDS,		udpCallShakeHands},
	{CMD_SHAKEHANDS_ASW,	udpCallShakeHandsAsw},
};

static UdpCallCmd udp_call_ex_cmd_handle[] = {
	{CMD_TRANSCALL,		udpCallRingEx},
	{CMD_TRANSTALK,		udpCallTalkEx},
	{CMD_TRANSOVER,		udpCallOver},
};

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpCallHandle 通话协议处理
 *
 * @param This
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpCallHandle(VideoTrans *This,
	   	char *ip,int port, char *data,int size)
{
	if(size == sizeof(COMMUNICATION) + sizeof(COMMUNICATION_CALL)) {
		COMMUNICATION_CALL *pCall = (COMMUNICATION_CALL *)&data[sizeof(COMMUNICATION)];
		unsigned int i;
		for(i=0; i<NELEMENTS(udp_call_cmd_handle); i++) {
			if (udp_call_cmd_handle[i].cmd == pCall->Cmd) {
				udp_call_cmd_handle[i].proc(This,ip,port,data);
				return;
			}
		}
		DBG_P("[%s] : call cmd = 0x%04x\n",__FUNCTION__,pCall->Cmd);
	} else  if(size == sizeof(TCALLFJ)) {
		DBG_P("From :%s-->",ip);
		TCALLFJ *pCallFJ = (TCALLFJ*)data;
		unsigned int i;
		for(i=0; i<NELEMENTS(udp_call_ex_cmd_handle); i++) {
			if (udp_call_ex_cmd_handle[i].cmd == pCallFJ->Cmd) {
				udp_call_ex_cmd_handle[i].proc(This,ip,port,data);
				return;
			}
		}
	}
}

static void *videoTimerThread(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	while (1) {
		videoTransHandleCallTime(arg);
		sleep(1);
	}
	return NULL;
}
static void videoTimerThreadCreate(VideoTrans *arg)
{
	int result;
	pthread_t m_pthread;					//线程号
	pthread_attr_t threadAttr1;				//线程属性

	pthread_attr_init(&threadAttr1);		//附加参数
	//设置线程为自动销毁
	pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);
	result = pthread_create(&m_pthread,&threadAttr1,videoTimerThread,arg);
	if(result) {
		DBG_P("[%s] pthread failt,Error code:%d\n",__FUNCTION__,result);
	}
	pthread_attr_destroy(&threadAttr1);		//释放附加参数
}
static int videoGetLeaveWord(VideoTrans *This)
{
	return This->priv->leave_word;
}
static void videoClearLeaveWord(VideoTrans *This)
{
	This->priv->leave_word = 0;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief videoTransResetProtocol 切换协议时，重新创建状态机线程
 *
 * @param This
 * @param type 协议类型
 */
/* ---------------------------------------------------------------------------*/
static void videoTransResetProtocol(VideoTrans *This,VideoProtoclType type)
{
	This->priv->st_run = 0;
	while (This->priv->st_run_exit == 0) {
		usleep(10000);
	}

	if (This->priv->pro_type == VIDEOTRANS_PROTOCOL_3000) {
		This->priv->st_machine = video_st_machine_bz;
	} else {
		This->priv->st_machine = video_st_machine_u9;
	}
	This->priv->pro_type = type;
}

static void videoEnable(VideoTrans *This)
{
	This->priv->enable = 1;
}

static void videoRegistDev(VideoTrans *This,char *ip,int port,int type,char *data)
{
	if(This->priv->master_ip[0] != 0)
		return;
	TFJRegStruct *reg = (TFJRegStruct *)data;

	//主机
	uint64_t dwTick = This->interface->getSystemTick(This);
	int i,FreeIdx = -1;
	for(i=0; i<MAXFJCOUNT; i++) {
		if(strcmp(ip,This->priv->RegDev[i].IP)==0)
			break;

		if(	   (FreeIdx == -1)
			&& (   (This->priv->RegDev[i].dwLastTick == 0)
				|| (This->interface->getDiffSysTick(This,dwTick, This->priv->RegDev[i].dwLastTick) > FJ_REGIST_TIME)) ) {
			FreeIdx = i;
		}
	}
	if (i == MAXFJCOUNT) {
		if(FreeIdx != -1) {
			strncpy(This->priv->RegDev[FreeIdx].IP,ip,15);
			This->priv->RegDev[FreeIdx].dwIP = inet_addr(ip);
			// printf("UDPDevRegProc[%d] ABIP:%s Ip:%s,dwIP:%d\n",
					// FreeIdx,ABinding->IP,Public.RegDev[FreeIdx].IP, Public.RegDev[FreeIdx].dwIP);
			This->priv->RegDev[FreeIdx].Port = port;
			This->priv->RegDev[FreeIdx].DevType = reg->Type==type? 1 : 0;
			This->priv->RegDev[FreeIdx].dwLastTick = dwTick;
		}
	} else {
		This->priv->RegDev[i].Port = port;
		This->priv->RegDev[i].DevType = reg->Type==type? 1 : 0;
		This->priv->RegDev[i].dwLastTick = dwTick;
	}
}
static void videoRegistToMaster(VideoTrans *This,int type)
{
	TFJRegStruct Packet;
	memset(&Packet,0,sizeof(TFJRegStruct));
	Packet.ID = 0;
	Packet.Size = sizeof(TFJRegStruct);
	Packet.Type = type;
    This->interface->udpSend(This,
            This->priv->master_ip,This->priv->port,&Packet,Packet.Size,
            0);
}
static void *videoGetRegistDev(struct _VideoTrans *This)
{
	return This->priv->RegDev;
}

static char *videoGetRegistOneDevIp(struct _VideoTrans *This,int index)
{
	return This->priv->RegDev[index].IP;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief getSystemTickDefault 接口默认函数
 *
 * @param This
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static uint64_t getSystemTickDefault(struct _VideoTrans *This)
{
	DBG_P("[%s]\n", __FUNCTION__);
	return 0;
}
static uint64_t getDiffSysTickDefault(struct _VideoTrans *This,
		uint64_t cur,uint64_t oldThis)
{
	DBG_P("[%s]\n", __FUNCTION__);
	return 0;
}
static 	void sendMessageStatusDefault(VideoTrans *This,VideoUiStatus status)
{
	DBG_P("[%s]:%d\n", __FUNCTION__,status);
}
static 	void showCallWindowDefault(VideoTrans *This)
{
	DBG_P("[%s]\n", __FUNCTION__);
}

static void udpSendDefault(struct _VideoTrans *This,
		char *ip,int port,void *data,int size, int enable_call_back)
{
	DBG_P("[%s]\n", __FUNCTION__);
}
static void saveRecordAsyncDefault(struct _VideoTrans *This,VideoCallDir dir,char *name,char *ip)
{
	DBG_P("[%s]\n", __FUNCTION__);
}
static int isCenterDefault(struct _VideoTrans *This,char *ip)
{
	DBG_P("[%s]\n", __FUNCTION__);
	return -1;
}
static int isDmkDefault(struct _VideoTrans *This,char *ip)
{
	DBG_P("[%s]\n", __FUNCTION__);
	return -1;
}
static int isHDmkDefault(struct _VideoTrans *This,char *ip)
{
	DBG_P("[%s]\n", __FUNCTION__);
	return -1;

}
static char *getDmkNameDefault(struct _VideoTrans *This,int index)
{
	DBG_P("[%s]\n", __FUNCTION__);
	return "dmk default";
}
static char *getHDmkNameDefault(struct _VideoTrans *This,int index)
{

	DBG_P("[%s]\n", __FUNCTION__);
	return "hdmk default";
}

static void loadInterface( VideoInterface *priv,VideoInterface *in)
{
	LOADFUNC(getSystemTick);
	LOADFUNC(getDiffSysTick);
	LOADFUNC(udpSend);
	LOADFUNC(saveRecordAsync);
	LOADFUNC(sendMessageStatus);
	LOADFUNC(showCallWindow);
	LOADFUNC(isCenter);
	LOADFUNC(isDmk);
	LOADFUNC(isHDmk);
	LOADFUNC(getDmkName);
	LOADFUNC(getHDmkName);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief videoTransCreate 创建呼叫对讲对象
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
VideoTrans * videoTransCreate(VideoInterface *interface,
		int port,
		int call_cmd,
		int device_type,
		VideoProtoclType type,
		char *master_ip,
		char *room_name,
		char *room_id)
{
	VideoTrans * This = (VideoTrans *)calloc(1,sizeof(VideoTrans));
	if(This == NULL) {
		DBG_P("calloc video fail!\n");
		return NULL;
	}

	This->priv = (VideoPriv *)calloc(1,sizeof(VideoPriv));
	if (This->priv == NULL) {
		DBG_P("calloc video priv fail!\n");
		free(This);
		return NULL;
	}

	This->interface = (VideoInterface *)calloc(1,sizeof(VideoInterface));
	if (This->interface == NULL) {
		DBG_P("calloc video interface fail!\n");
		free(This->priv);
		free(This);
		return NULL;
	}

	This->priv->packet_id = (unsigned)time(NULL);
	This->priv->pro_type = type;
	This->priv->device_type = device_type;
	This->priv->port = port;
	This->priv->call_cmd = call_cmd;
	This->priv->master_ip = master_ip;
	This->priv->room_name = room_name;
	This->priv->room_id = room_id;

	loadInterface(This->interface,interface);
	// 创建协议状态机
	video_st_machine_bz = stateMachineCreate(ST_IDLE,
			stm_video_state_bz,
			NELEMENTS(stm_video_state_bz),
			0, videoTransSTMHanldeVideo,This,&st_debug);
	video_st_machine_u9 = stateMachineCreate(ST_IDLE,
			stm_video_state_u9,
			NELEMENTS(stm_video_state_u9),
			0, videoTransSTMHanldeVideo,This,&st_debug);
	if (This->priv->pro_type == VIDEOTRANS_PROTOCOL_3000)
		This->priv->st_machine = video_st_machine_bz;
	else
		This->priv->st_machine = video_st_machine_u9;

	This->destroy = videoTransDestroy;
	This->dispCallTime = videoTransDispCallTime;
	This->dispCallName = videoTransDispCallName;
	This->hangup = videoHangup;
	This->call = videoCall;
    This->cmdHandle= udpCallHandle;
	This->answer = videoAnswer;
	This->leaveWord = videoLeaveWord;
	This->getPeerIP = videoGetPeerIP;
	This->unlock = videoUnlock;
	This->getIdleStatus = videoGetIdleStatus;
	This->getTalkStatus = videoGetTalkStatus;
	This->setStatusBusy = videosetStatusBusy;
	This->setStatusIdle = videosetStatusIdle;
	This->getTalkPort = videoGetTalkPort;
	This->resetProtocol = videoTransResetProtocol;
	This->hasUnreadLeaveWord = videoGetLeaveWord;
	This->clearUnreadLeaveWord = videoClearLeaveWord;
	This->enable = videoEnable;
    This->callBackOverTime = videoCallbackOvertime;
    This->registDev = videoRegistDev;
	This->registToMaster = videoRegistToMaster;
	This->getRegistDev = videoGetRegistDev;
	This->getRegistOneDevIp = videoGetRegistOneDevIp;
	videoTimerThreadCreate(This);
	return This;
}

