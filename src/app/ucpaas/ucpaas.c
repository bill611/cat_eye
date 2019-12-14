/*
 * =============================================================================
 *
 *       Filename:  protocol_ucpaas.c
 *
 *    Description:  云之讯对讲协议
 *
 *        Version:  1.0
 *        Created:  2019-06-04 16:09:13
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "ucpaas.h"
#include "sql_handle.h"
#include "thread_helper.h"
#include "ucpaas/UCSService.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define DPRINT(...)           \
do {                          \
    printf("\033[1;33m");  \
    printf("[UCPASS->%s,%d]",__func__,__LINE__);   \
    printf(__VA_ARGS__);      \
    printf("\033[0m");        \
} while (0)

typedef struct _DebugInfo {
	int opt;
	const char *content;
}DebugInfo;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/

static Callbacks call_backs;
static int g_externalAVEn = 1;
static int connect_state = 0;
static int call_status = 0;
static unsigned char *rec_buf = NULL;
static unsigned int rec_buf_len = 0;
static int send_for_reciev_video = 0; // 为了接收视频，发送视频数据保持连接


static const char * ucDebugInfo(int ev_reason)
{
	static DebugInfo debug_info[] = {
		{eUCS_REASON_SUCCESS,"eUCS_REASON_SUCCESS"},
		{eUCS_REASON_TCP_CONNECTED,"eUCS_REASON_TCP_CONNECTED"},
		{eUCS_REASON_TCP_RECONNECTED,"eUCS_REASON_TCP_RECONNECTED"},
		{eUCS_REASON_TCP_DISCONNECTED,"eUCS_REASON_TCP_DISCONNECTED"},
		{eUCS_REASON_TCP_SEND,"eUCS_REASON_TCP_SEND"},
		{eUCS_REASON_TCP_RECV,"eUCS_REASON_TCP_RECV"},
		{eUCS_REASON_TCP_TRANS_EMPTY,"eUCS_REASON_TCP_TRANS_EMPTY"},
		{eUCS_REASON_TCP_TRANS_TARGET_UNEXIST,"eUCS_REASON_TCP_TRANS_TARGET_UNEXIST"},
		{eUCS_REASON_TCP_TRANS_TARGET_OFFLINE,"eUCS_REASON_TCP_TRANS_TARGET_OFFLINE"},
		{eUCS_REASON_TCP_TRANS_PROXY_ERROR,"eUCS_REASON_TCP_TRANS_PROXY_ERROR"},
		{eUCS_REASON_TCP_TRANS_SEND_TIMEOUT,"eUCS_REASON_TCP_TRANS_SEND_TIMEOUT"},
		{eUCS_REASON_LOGIN_SUCCESS,"eUCS_REASON_LOGIN_SUCCESS"},
		{eUCS_REASON_TOKEN_INVALID,"eUCS_REASON_TOKEN_INVALID"},
		{eUCS_REASON_LOGIN_ARG_ERR,"eUCS_REASON_LOGIN_ARG_ERR"},
		{eUCS_REASON_LOGIN_SYS_ERR,"eUCS_REASON_LOGIN_SYS_ERR"},
		{eUCS_REASON_LOGIN_PWD_ERR,"eUCS_REASON_LOGIN_PWD_ERR"},
		{eUCS_REASON_INVALID_PROXYS_NUM,"eUCS_REASON_INVALID_PROXYS_NUM"},
		{eUCS_REASON_RINGING,"eUCS_REASON_RINGING"},
		{eUCS_REASON_CONNECTING,"eUCS_REASON_CONNECTING"},
		{eUCS_REASON_INVALID_CALLED,"eUCS_REASON_INVALID_CALLED"},
		{eUCS_REASON_CALLED_OFFLINE,"eUCS_REASON_CALLED_OFFLINE"},
		{eUCS_REASON_CALLED_BUSY,"eUCS_REASON_CALLED_BUSY"},
		{eUCS_REASON_CALLED_REJECT,"eUCS_REASON_CALLED_REJECT"},
		{eUCS_REASON_CALLED_NO_ANSWER,"eUCS_REASON_CALLED_NO_ANSWER"},
		{eUCS_REASON_CALLED_FROZEN,"eUCS_REASON_CALLED_FROZEN"},
		{eUCS_REASON_CALLER_FROZEN,"eUCS_REASON_CALLER_FROZEN"},
		{eUCS_REASON_CALLER_EXPIRED,"eUCS_REASON_CALLER_EXPIRED"},
		{eUCS_REASON_NO_BALANCE,"eUCS_REASON_NO_BALANCE"},
		{eUCS_REASON_MSG_TIMEOUT,"eUCS_REASON_MSG_TIMEOUT"},
		{eUCS_REASON_BLACKlIST,"eUCS_REASON_BLACKlIST"},
		{eUCS_REASON_HANGUP_BYPEER,"eUCS_REASON_HANGUP_BYPEER"},
		{eUCS_REASON_HANGUP_MYSELF,"eUCS_REASON_HANGUP_MYSELF"},
		{eUCS_REASON_MEDIA_NOT_ACCEPT,"eUCS_REASON_MEDIA_NOT_ACCEPT"},
		{eUCS_REASON_RTPP_TIMEOUT,"eUCS_REASON_RTPP_TIMEOUT"},
		{eUCS_REASON_MEDIA_UPDATE_FAILED,"eUCS_REASON_MEDIA_UPDATE_FAILED"},
		{eUCS_REASON_CALLER_CANCEL,"eUCS_REASON_CALLER_CANCEL"},
		{eUCS_REASON_FORBIDDEN,"eUCS_REASON_FORBIDDEN"},
		{eUCS_REASON_RTP_RECEIVED_TIMEOUT,"eUCS_REASON_RTP_RECEIVED_TIMEOUT"},
		{eUCS_REASON_CALLID_NOT_EXIST,"eUCS_REASON_CALLID_NOT_EXIST"},
		{eUCS_REASON_NOT_LOGGED_IN,"eUCS_REASON_NOT_LOGGED_IN"},
		{eUCS_REASON_INTERNAL_SRV_ERROR,"eUCS_REASON_INTERNAL_SRV_ERROR"},
	};
	int i;
	for (i=0; i<sizeof(debug_info)/sizeof(DebugInfo); i++) {
		if (ev_reason == debug_info[i].opt)
			return debug_info[i].content;
	}
}

// UCS connect status callback
static void connect_event_cb(int ev_reason)
{
    DPRINT("ev_reason[%s]\n", ucDebugInfo(ev_reason));
	if (ev_reason == eUCS_REASON_LOGIN_SUCCESS) {
		connect_state = 1;
	}
}

// UCS dailing out failed
static void dial_failed_cb(const char* callid, int reason)
{
    DPRINT("callid[%s] reason[%s]\n", callid, ucDebugInfo(reason));
    int result = 0;
    if (call_backs.dialFail)
        call_backs.dialFail(&result);
	call_status = 0;
}

// call alerting
static void on_alerting_cb(const char* callid)
{
    DPRINT("callid[%s]\n", callid);
	if (call_backs.dialRet)
		call_backs.dialRet((void *)callid);
}

// new call incoming
static void on_incomingcall_cb(const char* callid, int calltype,
    const char* caller_uid, const char* caller_name,
    const char* userdata)
{
    DPRINT("callid[%s] calltype[%d], caller_uid[%s] caller_name[%s] userdata[%s]\n",
         callid, calltype, caller_uid, caller_name, userdata);
	if (call_backs.incomingCall)
		call_backs.incomingCall((void *)caller_uid);
	call_status = 1;
}

// call answer
static void on_answer_cb(const char* callid)
{
    DPRINT("callid[%s]\n",  callid);
    if (call_backs.answer)
        call_backs.answer((void *)callid);
}

// call hangup
static void on_hangup_cb(const char* callid, int reason)
{
    DPRINT("callid[%s] reason[%s]\n", callid,ucDebugInfo(reason));
	call_status = 0;
	send_for_reciev_video = 0;
	int type = 0;
	if (reason == eUCS_REASON_HANGUP_MYSELF)
		type = 1;
	else if (reason == eUCS_REASON_HANGUP_BYPEER)
		type = 2;
	if (call_backs.hangup)
		call_backs.hangup(&type);
}

// received dtmf
static void received_dtmf_cb(int dtmf_code)
{
    DPRINT("dtmf_code[%d]\n", dtmf_code);
}

// network quality report between call session
static void network_state_report_cb(int reason, const char* netstate)
{
    DPRINT("reason[%s] netstate[%s]\n", ucDebugInfo(reason), netstate);
}

// UCS through data received callback
static void transdata_received_cb(const char* from_userId,
                        const char* callid,
                        const char* trans_data)
{
    if ( NULL != trans_data)
    {
		if (call_backs.receivedCmd)
			call_backs.receivedCmd(from_userId,(void *) trans_data);
        DPRINT("transdata_received_cb: trans_data[%s]\n", trans_data);
    }
}

static void transdata_send_result_cb(int err_code)
{
    DPRINT("transdata_send_result_cb: err_code[%s]\n",ucDebugInfo(err_code) );
	if (call_backs.sendCmd)
		call_backs.sendCmd(&err_code);
}

// UCS invoke it to change video stream
static void set_video_framerate_cb(unsigned int target_width, unsigned int target_height,
    unsigned int frame_rate, unsigned int bitrate)
{
    DPRINT("set_video_framerate_cb: resolution[%dx%d fps[%d] bitrate[%d]\n",
        target_width, target_height, frame_rate, bitrate);
}

// UCS init external playout device with given parameters
// sample_rate -- playout audio sample rate, 16k
// bytes_per_sample -- bytes of per sample, always 2 bytes = 16bits
// num_of_channels -- number of playout channels, always = 1
static void init_playout_cb(unsigned int sample_rate,
    unsigned int bytes_per_sample,
    unsigned int num_of_channels)
{
    DPRINT("rate:%d,sample:%d,channle:%d \n",
			sample_rate,
			bytes_per_sample,
			num_of_channels);
	if (call_backs.initAudio)
		call_backs.initAudio(8000,16,2);
}

// UCS init external recording device with given parameters
// sample_rate -- recording audio sample rate, 16000
// bytes_per_sample -- bytes of per sample, always 2 bytes = 16bits
// num_of_channels -- number of recording channels, always = 1
static void init_recording_cb(unsigned int sample_rate,
    unsigned int bytes_per_sample,
    unsigned int num_of_channels)
{
    DPRINT("rate:%d,sample:%d,channle:%d \n",
			sample_rate,
			bytes_per_sample,
			num_of_channels);
	if (call_backs.startRecord)
		call_backs.startRecord(sample_rate,bytes_per_sample,num_of_channels);
}

// UCS read recording 10ms pcm data from external audio device
// audio_data -- recording pcm data
// size -- want to read data len
static int read_recording_data_cb(char * audio_data,
    unsigned int size)
{
	if (call_backs.recording)
		call_backs.recording(audio_data,size);
	return 0;
}

// UCS write playout 10ms pcm data to external audio device
// audio_data -- playout data for external audio device
// size -- playout data length
static int write_playout_data_cb(const char* audio_data,
    unsigned int size)
{
	if (call_backs.playAudio)
		call_backs.playAudio(audio_data,size);
	return 0;
}

static int TUCS_Init()
{
    char version[UCS_MAX_VERSION_STR_LEN] = { 0 };
    UCS_cb_vtable_t vtable;

    memset(&vtable, 0x00, sizeof(UCS_cb_vtable_t));
    vtable.connect_cb = connect_event_cb;
    vtable.dial_failed_cb = dial_failed_cb;
    vtable.on_alerting_cb = on_alerting_cb;
    vtable.on_answer_cb = on_answer_cb;
    vtable.on_incomingcall_cb = on_incomingcall_cb;
    vtable.on_hangup_cb = on_hangup_cb;
    vtable.received_dtmf_cb = received_dtmf_cb;
    vtable.transdata_received_cb = transdata_received_cb;
    vtable.transdata_send_result_cb = transdata_send_result_cb;
    vtable.set_video_framerate_cb = set_video_framerate_cb;
    vtable.init_playout_cb = init_playout_cb;
    vtable.init_recording_cb = init_recording_cb;
    vtable.write_data_cb = write_playout_data_cb;
    vtable.read_data_cb = read_recording_data_cb;
    UCS_RegisterCallbackVtable(&vtable);

    if (UCS_Init() < 0) {
        DPRINT("ucs init failed.\n");
        return -1;
    }

    UCS_SetLogEnable(0, NULL);

    UCS_GetVersion(version);
	DPRINT("ucpaas version %s\n",version);

    return 0;
}

static int TUCS_Destory()
{
    return UCS_Destory();
}


void ucsDial(char *user_id)
{
	if (connect_state == 0)
		return;
    UCS_Dial(user_id, eUCS_CALL_TYPE_VIDEO_CALL);
	call_status = 1;
}

void ucsAnswer(void)
{
	if (connect_state == 0)
		return;
	UCS_CallAnswer();
}

void ucsHangup(void)
{
	if (connect_state == 0)
		return;
	if (call_status == 0)
		return;
	UCS_CallHangUp();
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief ucsSendCmd 云之讯cmd会将cmd内容打包成json,所以传入的cmd碰到"需要加转义
 * 字符，例如:
 *
 * {\\\"messageType\\\":%d,\\\"deviceType\\\":%d,\\\"deviceNumber\\\":\\\"%s\\\"}
 *
 * @param cmd
 * @param user_id
 */
/* ---------------------------------------------------------------------------*/
void ucsSendCmd(char *cmd,char *user_id)
{
	if (connect_state == 0)
		return;
	UCS_SendTransData(0, user_id, cmd, strlen(cmd));
}

void ucsSendVideo(const unsigned char* frameData, const unsigned int dataLen)
{
	if (connect_state == 0)
		return;
	// DPRINT("send:%d\n", dataLen);
	UCS_PushExternalVideoStream(frameData,dataLen);
}

static void* threadSendForRecive(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	send_for_reciev_video = 1;
	while (send_for_reciev_video) {
		if (rec_buf)	 {
			UCS_PushExternalVideoStream(rec_buf,rec_buf_len);
		}
		usleep(100000);
	}
	if (rec_buf) {
		free(rec_buf);
		rec_buf = NULL;
		rec_buf_len = 0;
	}
	return NULL;
}

void ucsReceiveVideo(unsigned char* frameData,
	   	unsigned int *dataLen,
		long long *timeStamp,
		int *frameType)
{
	if (connect_state == 0)
		return;
	UCS_get_video_frame(frameData, dataLen,timeStamp,frameType);
	if (rec_buf == NULL && *dataLen) {
		rec_buf = (unsigned char *)malloc (*dataLen);
		rec_buf_len = *dataLen;
		memcpy(rec_buf,frameData,*dataLen);
		createThread(threadSendForRecive,NULL);
	}
}

int ucsConnect(char *user_token)
{
	if (connect_state == 1)
		return 0;
    if (user_token[0] != 0) {
		UCS_Connect(user_token);
		// test 门口机
		// UCS_Connect("eyJBbGciOiJIUzI1NiIsIkFjY2lkIjoiY2I3MzhjZmNkNmFlYTkxMDZiZTk5OTc2NzZlNzJhMDIiLCJBcHBpZCI6ImFjNTdhMDc3ZGIwYjRjY2JhNzEwNTU5Yzk4NzlkYmQ1IiwiVXNlcmlkIjoiVGNjMTkwNDFiZDI1YjQ0ZWY0YmJjZDgifQ==.Du+oG8HxUJBfaLDfaCcVanWRNCSvLSxVnQOHFRDzMAA=");
        return 1;
    }
    return 0;
}
void ucsDisconnect(void)
{
	if (connect_state == 0)
		return;
	connect_state = 0;
    UCS_DisConnect();
}

void registUcpaas(Callbacks *interface)
{
	call_backs.dialFail = interface->dialFail;
	call_backs.answer = interface->answer;
	call_backs.hangup = interface->hangup;
	call_backs.dialRet = interface->dialRet;
	call_backs.incomingCall = interface->incomingCall;
	call_backs.sendCmd = interface->sendCmd;
	call_backs.receivedCmd = interface->receivedCmd;
	call_backs.initAudio = interface->initAudio;
	call_backs.startRecord = interface->startRecord;
	call_backs.recording = interface->recording;
	call_backs.playAudio = interface->playAudio;

	if (TUCS_Init() == 0) {
		UCS_SetExtAudioTransEnable(g_externalAVEn);
		UCS_SetExtVideoStreamEnable(g_externalAVEn);
		UCS_ViERingPreviewEnable(1);
		UCS_vqecfg_t vqecfg;
		vqecfg.aec_enable = 1;
		vqecfg.agc_enable = 1;
		vqecfg.ns_enable = 1;
		UCS_SetVqeCfg(&vqecfg);
	}

}
int unregistUcpaas(void)
{
    TUCS_Destory();
}

int ucsConnectState(void)
{
	return connect_state;	
}
