/*
 * =============================================================================
 *
 *       Filename:  protocol_singlechip.c
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
#include "debug.h"
#include "uart.h"
#include "config.h"
#include "protocol.h"
#include "sql_handle.h"
#include "my_video.h"
#include "my_audio.h"
#include "externfunc.h"
#include "gui/screen.h"
#include "jpeg_enc_dec.h"
#include "thread_helper.h"
#include "form_videlayer.h"
#include "ipc_server.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern void screenAutoCloseStop(void);
extern IpcServer* ipc_main;

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define PIR_TIMER_INTERVAL 10
#define PIR_ACTIVE_COUNT 10

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
ProtocolSinglechip *protocol_singlechip;
// 从触发到未触发转变，需要经过一定时间等待稳定后才重新开始计数
static int pir_act_count = 0; 	 // PIR传感器连续触发倒计时，若3s内没有再次触发，则进入未触发倒计时
static int pir_act_timer = 0; // PIR传感器触发倒计时，10秒内没有触发，清零
static int pir_disact_timer = 0; // PIR传感器触发倒计时，10秒内没有触发，清零
static int pir_cycle_end = 0; // PIR触发周期结束,一个周期内，只触发一次人脸或门铃
static uint64_t picture_id = 0;

static void cmdSleep(void)
{
#ifdef AUTO_SLEEP
	IpcData ipc_data;
	protocol_hardcloud->enableSleepMpde();
	ipc_data.dev_type = IPC_DEV_TYPE_MAIN;
	ipc_data.cmd = IPC_UART_SLEEP;
	if (ipc_main)
		ipc_main->sendData(ipc_main,IPC_UART,&ipc_data,sizeof(ipc_data));
#endif
}

static void* threadPirTimer(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	while (1) {
		if (pir_act_timer) {
			// printf("[%s]act:%d\n", __func__,pir_act_timer);
			if (--pir_act_timer == 0 ) {
				pir_disact_timer = PIR_TIMER_INTERVAL;
			}
		}
		if (pir_disact_timer) {
			// printf("[%s]disact:%d\n", __func__,pir_disact_timer);
			if (--pir_disact_timer == 0) {
				pir_act_count = 0;
				pir_cycle_end = 0;
			}
		}
		sleep(1);
	}
	return NULL;
}
static void* threadCapture(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	uint64_t picture_id = *(uint64_t *)arg;
	protocol_hardcloud->uploadPic(FAST_PIC_PATH,picture_id);
	protocol_hardcloud->reportCapture(picture_id);
	return NULL;
}

static void deal(IpcData *ipc_data)
{
	switch(ipc_data->cmd)
	{
		case IPC_UART_KEY_POWER:
			screensaverStart(0);
			screenAutoCloseStop();
			Screen.ReturnMain();
			break;
		case IPC_UART_KEYHOME:
			formVideoLayerScreenOn();
			break;
		case IPC_UART_DOORBELL:
			{
#ifdef USE_UDPTALK
				formVideoLayerScreenOn();
#endif
#ifdef USE_UCPAAS
				my_video->videoCallOutAll();
#endif
			}
			break;
		case IPC_UART_CAPTURE:
			{
				if (pir_cycle_end == 1)
					break;
				pir_cycle_end = 1;
				char url[256] = {0};
				picture_id = atoll(ipc_data->data.file.name);
				sprintf(url,"%s/%s_%s_0.jpg",QINIU_URL,g_config.imei,ipc_data->data.file.name);
				sqlInsertRecordCapNoBack(ipc_data->data.file.date,picture_id);
				sqlInsertPicUrlNoBack(picture_id,url);
				createThread(threadCapture,&picture_id);
			} break;
		case IPC_UART_POWEROFF:
			formVideoLayerGotoPoweroff();
			break;
		case IPC_UART_PIR:
			my_video->delaySleepTime(0);
			pir_act_timer = PIR_TIMER_INTERVAL;
			pir_disact_timer = 0;
			if (pir_cycle_end == 1)
				break;
			if (++pir_act_count == PIR_ACTIVE_COUNT) {
				pir_cycle_end = 1;
				if (g_config.cap.type == 0)
					my_video->capture(CAP_TYPE_ALARM,g_config.cap.count,NULL,NULL);
				else {
					// 录像	
				}
			}
			break;
		default:
			break;
	}
}

static void hasPeople(char *nick_name,char *user_id)
{
	if (pir_cycle_end == 1)
		return;
	pir_cycle_end = 1;
	my_video->capture(CAP_TYPE_FACE,1,nick_name,user_id);
}

void registSingleChip(void)
{
	protocol_singlechip = (ProtocolSinglechip *) calloc(1,sizeof(ProtocolSinglechip));
	protocol_singlechip->cmdSleep = cmdSleep;
	protocol_singlechip->deal = deal;
	protocol_singlechip->hasPeople = hasPeople;
	createThread(threadPirTimer,NULL);
}
