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
#include "debug.h"
#include "uart.h"
#include "config.h"
#include "protocol.h"
#include "sql_handle.h"
#include "my_video.h"
#include "my_audio.h"
#include "externfunc.h"
#include "gui/screen.h"
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

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
ProtocolSinglechip *protocol_singlechip;

static void cmdSleep(void)
{
	return;
	IpcData ipc_data;
	enableSleepMpde();
	ipc_data.dev_type = IPC_DEV_TYPE_MAIN;
	ipc_data.cmd = IPC_UART_SLEEP;
	if (ipc_main)
		ipc_main->sendData(ipc_main,IPC_UART,&ipc_data,sizeof(ipc_data));
}

static void deal(int cmd,char *data,int size)
{
	switch(cmd)
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
#ifdef USE_UDPTALK
			formVideoLayerScreenOn();
#endif
#ifdef USE_UCPAAS
			printf("[%s,%d]cmd:%d\n", __func__,__LINE__,cmd);
			my_video->videoCallOutAll();
#endif
			break;
		case IPC_UART_POWEROFF:
			formVideoLayerGotoPoweroff();
			break;
		default:
			break;
	}
}

void registSingleChip(void)
{
	protocol_singlechip = (ProtocolSinglechip *) calloc(1,sizeof(ProtocolSinglechip));
	protocol_singlechip->cmdSleep = cmdSleep;
	protocol_singlechip->deal = deal;
}
