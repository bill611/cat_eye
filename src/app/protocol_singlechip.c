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
extern void resetAutoSleepTimerShort(void);
extern IpcServer* ipc_main;

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define PIR_TIMER_INTERVAL 10
#define PIR_ACTIVE_COUNT 5

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
static int cap_end = 0; // 是否抓图结束，结束后可以上传图片

static void cmdSleep(void)
{
	IpcData ipc_data;
	protocol_hardcloud->enableSleepMpde();
	ipc_data.dev_type = IPC_DEV_TYPE_MAIN;
	ipc_data.cmd = IPC_UART_SLEEP;
	if (ipc_main)
		ipc_main->sendData(ipc_main,IPC_UART,&ipc_data,sizeof(ipc_data));
}

static void* threadPirTimer(void *arg)
{
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
	uint64_t picture_id = *(uint64_t *)arg;
	while (cap_end == 0) {
		usleep(100000);
	}
	cap_end = 0;
	protocol_hardcloud->uploadPic(FAST_PIC_PATH);
	protocol_hardcloud->reportCapture(picture_id);
	return NULL;
}

static void* threadAlarm(void *arg)
{
	ReportAlarmData *alarm_data = (ReportAlarmData *)arg;
	while (cap_end == 0) {
		usleep(100000);
	}
	char pic_buf_jpg[100 * 1024];
	unsigned char *pic_buf_yuv = NULL;
	int yuv_len = 0;
	int w,h;
	cap_end = 0;
	FILE *fp = fopen(alarm_data->file_path,"rb");
	int leng = fread(pic_buf_jpg,1,sizeof(pic_buf_jpg),fp);
	fclose(fp);
	jpegToYuv420sp((unsigned char *)pic_buf_jpg, leng,&w,&h, &pic_buf_yuv, &yuv_len);
	if (my_video) {
		if (my_video->faceRecognizer(pic_buf_yuv,w,h,&alarm_data->age,&alarm_data->sex) == 0)
			alarm_data->has_people = 1;
		else
			alarm_data->has_people = 0;
	}
	if (pic_buf_yuv)
		free(pic_buf_yuv);

	sqlInsertRecordAlarm(alarm_data->date,
			alarm_data->type,
			alarm_data->has_people,
			alarm_data->age,
			alarm_data->sex,
			alarm_data->picture_id);
	protocol_hardcloud->uploadPic(FAST_PIC_PATH);
	protocol_hardcloud->reportAlarm(alarm_data);
	return NULL;
}
static void* threadFace(void *arg)
{
	ReportFaceData *face_data = (ReportFaceData *)arg;
	while (cap_end == 0) {
		usleep(100000);
	}

	sqlInsertRecordFaceNoBack(face_data->date,
			face_data->user_id,
			face_data->nick_name,
			face_data->picture_id);
	protocol_hardcloud->uploadPic(FAST_PIC_PATH);
	protocol_hardcloud->reportFace(face_data);
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
				// my_video->videoCallOutAll();
#endif
				if (pir_cycle_end == 1)
					break;
				pir_cycle_end = 1;
				char url[256] = {0};
				picture_id = atoll(ipc_data->data.file.name);
				sprintf(url,"%s/%s_0.jpg",QINIU_URL,ipc_data->data.file.name);
				sqlInsertRecordCapNoBack(ipc_data->data.file.date,picture_id);
				sqlInsertPicUrlNoBack(picture_id,url);
				createThread(threadCapture,&picture_id);
			}
			break;
		case IPC_VIDEO_CAPTURE_END:
			cap_end = 1;
			break;
		case IPC_UART_POWEROFF:
			my_video->hideVideo();
			formVideoLayerGotoPoweroff();
			break;
		case IPC_UART_PIR:
			resetAutoSleepTimerShort();
			pir_act_timer = PIR_TIMER_INTERVAL;
			pir_disact_timer = 0;
			if (pir_cycle_end == 1)
				break;
			if (++pir_act_count == PIR_ACTIVE_COUNT) {
				pir_cycle_end = 1;
				// 上传报警记录
				static ReportAlarmData alarm_data;
				IpcData ipc_data;
				char url[256] = {0};
				getFileName(ipc_data.data.file.name,ipc_data.data.file.date);
				sprintf(ipc_data.data.file.path,"%s%s_0.jpg",FAST_PIC_PATH,ipc_data.data.file.name);
				sprintf(url,"%s/%s_0.jpg",QINIU_URL,ipc_data.data.file.name);

				strcpy(alarm_data.date,ipc_data.data.file.date);
				strcpy(alarm_data.file_path,ipc_data.data.file.path);
				alarm_data.type = ALARM_TYPE_PEOPLES;
				alarm_data.picture_id = atoll(ipc_data.data.file.name);
				sqlInsertPicUrlNoBack(alarm_data.picture_id,url);
				createThread(threadAlarm,&alarm_data);

				ipc_data.dev_type = IPC_DEV_TYPE_MAIN;
				ipc_data.cmd = IPC_VIDEO_CAPTURE;
				if (ipc_main)
					ipc_main->sendData(ipc_main,IPC_CAMMER,&ipc_data,sizeof(ipc_data));
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
	// 上传人脸识别记录		
	static ReportFaceData face_data;
	IpcData ipc_data;
	char url[256] = {0};
	getFileName(ipc_data.data.file.name,ipc_data.data.file.date);
	sprintf(ipc_data.data.file.path,"%s%s_0.jpg",FAST_PIC_PATH,ipc_data.data.file.name);
	sprintf(url,"%s/%s_0.jpg",QINIU_URL,ipc_data.data.file.name);

	strcpy(face_data.date,ipc_data.data.file.date);
	strcpy(face_data.file_path,ipc_data.data.file.path);
	strcpy(face_data.nick_name,nick_name);
	strcpy(face_data.user_id,user_id);
	face_data.picture_id = atoll(ipc_data.data.file.name);
	sqlInsertPicUrlNoBack(face_data.picture_id,url);
	createThread(threadFace,&face_data);

	ipc_data.dev_type = IPC_DEV_TYPE_MAIN;
	ipc_data.cmd = IPC_VIDEO_CAPTURE;
	if (ipc_main)
		ipc_main->sendData(ipc_main,IPC_CAMMER,&ipc_data,sizeof(ipc_data));
}

void registSingleChip(void)
{
	protocol_singlechip = (ProtocolSinglechip *) calloc(1,sizeof(ProtocolSinglechip));
	protocol_singlechip->cmdSleep = cmdSleep;
	protocol_singlechip->deal = deal;
	protocol_singlechip->hasPeople = hasPeople;
	createThread(threadPirTimer,NULL);
}
