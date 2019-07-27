/*
 * =============================================================================
 *
 *       Filename:  singlechip.c
 *
 *    Description:  单片机通信协议
 *
 *        Version:  1.0
 *        Created:  2019-06-10 13:11:39
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
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "uart.h"
#include "protocol.h"
#include "config.h"
#include "my_audio.h"
#include "debug.h"
#include "externfunc.h"
#include "ipc_server.h"
#include "thread_helper.h"
#include "queue.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern int playVoice(char * file_name);

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define PACK_HEAD 0x5A
#define PACK_TAIL 0x5B

enum {
    CHECK_POWER = (1 << 0),
    CHECK_KEY_HOM = (1 << 1),
    CHECK_KEY_DOORBELL = (1 << 2),
    CHECK_KEY_PIR = (1 << 3),
};
enum{
	CMD_RESERVE = 0X01, // 备用
	CMD_POWER = 0X02, 				// ARM→单片机	0X02	1BYTE	0 关机 1进入低功耗模式 2 复位WIFI
	CMD_WIFI_PWD = 0X03,			// ARM→单片机	0X03	32BYTE	设置WIFI连接，数据格式： SSID：[0~15]BYTE； PASSWORD：[16~31]BYTE；
	CMD_WIFI_RESPONSE = 0X04, 		// 单片机→ARM	0X04	1BYTE	WIFI连接响应，参数说明： 0X00 WIFI连接成功； 0X01 WIFI连接失败；
	CMD_SERVER = 0X05, 				// ARM→单片机	0X05	NBYTE	设置连接服务器信息： 服务器IP：[0~3]BYTE； 设备信息：[4~N]BYTE；
	CMD_SERVER_RESPONSE = 0X06, 	// 单片机→ARM	0X06	1BYTE	服务器连接响应： 0X00 服务器连接成功； 0X01 服务器连接失败；
	CMD_SET_PIR = 0X07, 			// ARM→单片机	0X07	1BYTE	设置PIR距离： 0X00：近； 0X01：中； 0X02：远；
	CMD_SET_PIR_RESPONSE = 0X08, 	// 单片机→ARM	0X08	1BYTE	设置PIR响应： 0X00 PIR设置成功； 0X01 PIR设置失败；
	CMD_GET_CHECK = 0X41, 			// ARM→单片机	0X41	0BYTE	查询触发源；
	CMD_GET_CHECK_RESPONSE = 0X42, 	// 单片机→ARM	0X42	3BYTE	触发源BYTE0:
	/*
	   BIT0 开机按键触发1， 否则0；下同
	   BIT1 室内机按键触发；
	   BIT2 室外机按键触发；
	   BIT3 PIR信号触发；
	   BIT4 AP6212 WIFI信号触发；
	   BIT5 ESP WIFI信号触发；
	   BIT6 GSM 中断信号触发；
	   BIT7 USB 信号触发；
	   保留字节[1~2]BYTE，恒为0X00;
	   */
	CMD_REPORT_RESPONSE = 0XC8,				// 单片机→ARM	0XC8	3BYTE	触发源BYTE0:
	/*
	   BIT0 开机按键触发1， 否则0；下同
	   BIT1 室内机按键触发；
	   BIT2 室外机按键触发；
	   BIT3 PIR信号触发；
	   BIT4 AP6212 WIFI信号触发；
	   BIT5 ESP WIFI信号触发；
	   BIT6 GSM 中断信号触发；
	   BIT7 USB 信号触发；
	   保留字节[1~2]BYTE，恒为0X00;
	   */
	CMD_REPORT = 0XC9, 						// ARM→单片机	0XC9	0BYTE	ARM设备发出
	CMD_ERR_RESPONSE = 0xFF,				// 单片机→ARM	0XFF	1BYTE	0X00 数据接收成功； 0X01 数据校验失败； 0X02 参数错误；

};
typedef struct _ProtocolComm{
	uint8_t head;
	uint8_t leng;
	uint8_t id;
	uint8_t cmd;
	uint8_t data;
	uint8_t checksum;
	uint8_t tail;
}ProtocolComm;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static IpcServer* ipc_uart = NULL;
static Queue *video_queue = NULL;
static Queue *main_queue = NULL;
static uint8_t id = 0;

static void cmdPacket(uint8_t cmd,uint8_t id,uint8_t *data,int data_len)
{
	uint8_t *send_buff = NULL;
	int i;
	int leng = data_len + 6;
	send_buff = (uint8_t *)calloc(leng,sizeof(uint8_t));
	send_buff[0] = PACK_HEAD;
	send_buff[1] = leng;
	send_buff[2] = id;
	send_buff[3] = cmd;
	for (i=0; i<data_len; i++) {
		send_buff[4 + i] = data[i];
	}
	for (i=1; i<leng-2; i++) {
		send_buff[leng - 2] += send_buff[i];
	}
	send_buff[leng - 1] = PACK_TAIL; 
	if (uart)
		uart->send(uart,send_buff,leng);
	free(send_buff);
}

static void cmdCheckStatus(void)
{
	cmdPacket(CMD_GET_CHECK,id++,NULL,0);
}

static void cmdSleep(void)
{
	return;
	uint8_t data = 1;
	cmdPacket(CMD_POWER,id++,&data,1);
}

static void uartDeal(void)
{
	uint8_t buff[512]={0};
	uint8_t checksum = 0;
	int index;
	int i;
	int leng = 0;

	int len = uart->recvBuffer(uart,buff,sizeof(buff));
	if (len <= 0) {
		return;
	}
    DEBUG_UART("reci",buff,len);
	for(index=0; index<len; index++){
		if(buff[index] == PACK_HEAD){
			leng = buff[index + 1];
			if (index + leng > 512)
				return;
			if(buff[index + leng - 1] == PACK_TAIL){
				break;
			}
		}
	}

	for (i=index+1; i<leng - 2; i++) {
		checksum += buff[i];
	}
	if (checksum != buff[index + leng - 2])
		return;
	uint8_t cmd = buff[index + 3];
	uint8_t *data = &buff[index + 4];
	switch(cmd)
	{
		case CMD_SET_PIR_RESPONSE:
			break;
		case CMD_GET_CHECK_RESPONSE:
		case CMD_REPORT_RESPONSE:
			if (data[0] & CHECK_POWER) {
				IpcData ipc_data;
				ipc_data.cmd = IPC_UART_POWER;
				main_queue->post(main_queue,&ipc_data);
            } else if (data[0] & CHECK_KEY_HOM) {
				IpcData ipc_data;
				ipc_data.cmd = IPC_UART_KEYHOME;
				main_queue->post(main_queue,&ipc_data);
            } else if (data[0] & CHECK_KEY_DOORBELL) {
				IpcData ipc_data;
				ipc_data.cmd = IPC_UART_DOORBELL;
				main_queue->post(main_queue,&ipc_data);
				char path[64];
				sprintf(path,"%sdingdong.wav",AUDIO_PATH);	
				playVoice(path);
			} else if (data[0] & CHECK_KEY_PIR) {
				IpcData ipc_data;
				ipc_data.cmd = IPC_UART_PIR;
				main_queue->post(main_queue,&ipc_data);
			}
			cmdPacket(CMD_REPORT,0,NULL,0);
			break;
		case CMD_ERR_RESPONSE:
			break;
		default:
			break;
	}
}

static void registSingleChip(void)
{
	uartInit(uartDeal);
	cmdCheckStatus();
}
static void ipcCallback(char *data,int size )
{
	IpcData ipc_data;
	memcpy(&ipc_data,data,sizeof(IpcData));
	switch(ipc_data.cmd)
	{
		case IPC_UART_SLEEP:
			cmdSleep();
			break;
		default:
			break;
	}
}

static void waitIpcOpen(char *path)
{
	int fd = -1;
	while (1) {
		fd = access(path,0);
		if (fd != 0) {
			// printf("open ipc :%s fail,error:%s\n",path ,strerror(errno));
			sleep(1);
			continue;
		}
		close(fd);
		return;
	}
}
static void* threadIpcSendVideo(void *arg)
{
	Queue * queue = (Queue *)arg;
	waitIpcOpen(IPC_CAMMER);
	IpcData ipc_data;
	while (1) {
		queue->get(queue,&ipc_data);	
		ipc_data.dev_type = IPC_DEV_TYPE_UART;
		if (ipc_uart)
			ipc_uart->sendData(ipc_uart,IPC_CAMMER,&ipc_data,sizeof(IpcData));
	}
	return NULL;
}
static void* threadIpcSendMain(void *arg)
{
	Queue * queue = (Queue *)arg;
	waitIpcOpen(IPC_MAIN);
	IpcData ipc_data;
	while (1) {
		queue->get(queue,&ipc_data);	
		ipc_data.dev_type = IPC_DEV_TYPE_UART;
		if (ipc_uart)
			ipc_uart->sendData(ipc_uart,IPC_MAIN,&ipc_data,sizeof(IpcData));
	}
	return NULL;
}
int main(int argc, char *argv[])
{
	registSingleChip();

	video_queue = queueCreate("video_queue",QUEUE_BLOCK,sizeof(IpcData));
	main_queue = queueCreate("main_queue",QUEUE_BLOCK,sizeof(IpcData));
	createThread(threadIpcSendVideo,video_queue);
	createThread(threadIpcSendMain,main_queue);
	ipc_uart = ipcCreate(IPC_UART,ipcCallback);
	pause();
	return 0;
}
