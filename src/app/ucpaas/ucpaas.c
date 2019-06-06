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

#include "debug.h"
#include "ucpaas.h"
#include "sql_handle.h"
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
typedef struct _DebugInfo {
	int opt;
	const char *content;
}DebugInfo;

typedef struct _Callbacks {
    void (*dial)(void *arg);
    void (*answer)(void *arg);
    void (*hangup)(void *arg);
    void (*dialRet)(void *arg);
    void (*incomingCall)(void *arg);
    void (*sendCmd)(void *arg);
    void (*receivedCmd)(const char *user_id,void *arg);
    void (*initAudio)(void);
    void (*startRecord)(void);
    void (*recording)(char *data,unsigned int size);
    void (*playAudio)(const char *data,unsigned int size);
}Callbacks;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/

static Callbacks call_backs;

static FILE* fw = NULL;
static int g_isVideoCall = 0;

static int g_isAutoAnswer = 0;
static int g_previewEn = 1;
static int g_externalAVEn = 1;

static int TUCS_extern_capture_init();
static void startVideoIncomingThread();
static void stopVideoIncomingThread();


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
    DPRINT("%s: ev_reason[%s]\n", __FUNCTION__, ucDebugInfo(ev_reason));
}

// UCS dailing out failed
static void dial_failed_cb(const char* callid, int reason)
{
    int result = 0; 
    if (call_backs.dial)
        call_backs.dial(&result);
    DPRINT("[%s] callid[%s] reason[%s]\n", __FUNCTION__, callid, ucDebugInfo(reason));
	return ;
    if (g_isVideoCall && g_externalAVEn)
    {
        stopVideoIncomingThread();
    }
}

// call alerting
static void on_alerting_cb(const char* callid)
{
	if (call_backs.dialRet)
		call_backs.dialRet((void *)callid);
    DPRINT("[%s] callid[%s]\n", __FUNCTION__, callid);
	return;
    if (g_isVideoCall && g_previewEn && g_externalAVEn)
    {
        startVideoIncomingThread();
    }
}

// new call incoming
static void on_incomingcall_cb(const char* callid, int calltype,
    const char* caller_uid, const char* caller_name,
    const char* userdata)
{
    DPRINT("[%s] callid[%s] calltype[%d], caller_uid[%s] caller_name[%s] userdata[%s]\n", 
        __FUNCTION__, callid, calltype, caller_uid, caller_name, userdata);
	if (call_backs.incomingCall)
		call_backs.incomingCall((void *)caller_uid);
	return;
    g_isVideoCall = calltype;
    if (g_isVideoCall && g_previewEn && g_externalAVEn)
    {
        startVideoIncomingThread();
    }

    if (g_isAutoAnswer)
    {
        UCS_CallAnswer();
    }
}

// call answer
static void on_answer_cb(const char* callid)
{
    DPRINT("[%s] callid[%s]\n", __FUNCTION__, callid);
    if (call_backs.answer)
        call_backs.answer((void *)callid);
	return;
    if (g_isVideoCall && g_externalAVEn)
    {
        startVideoIncomingThread();
    }
}

// call hangup
static void on_hangup_cb(const char* callid, int reason)
{
    DPRINT("[%s] callid[%s] reason[%s]\n", __FUNCTION__, callid,ucDebugInfo(reason));
	if (call_backs.hangup)
		call_backs.hangup((void *)callid);
	return;
    if (fw)
	{
		fclose(fw);
	}

    if (g_isVideoCall && g_externalAVEn)
    {
        stopVideoIncomingThread();
    }
}

// received dtmf
static void received_dtmf_cb(int dtmf_code)
{
    DPRINT("[%s] dtmf_code[%d]\n", __FUNCTION__, dtmf_code);
}

// network quality report between call session
static void network_state_report_cb(int reason, const char* netstate)
{
    DPRINT("[%s] reason[%s] netstate[%s]\n", __FUNCTION__, ucDebugInfo(reason), netstate);
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
	if (call_backs.sendCmd)
		call_backs.sendCmd(&err_code);
    DPRINT("transdata_send_result_cb: err_code[%d]\n", err_code);
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
	if (call_backs.initAudio)
		call_backs.initAudio();
	return;
//    fw = fopen("/home/playout_t.pcm", "wb");
    fw = fopen("/var/upgrade/playout_t.pcm", "wb");
	if (NULL == fw)
	{
		DPRINT("Failed to open playout_t.pcm\n");
	}
}

// UCS init external recording device with given parameters
// sample_rate -- recording audio sample rate, 16000
// bytes_per_sample -- bytes of per sample, always 2 bytes = 16bits
// num_of_channels -- number of recording channels, always = 1
#define MAX_READ_STREAM_SIZE (1024 * 1024 * 6)
static char readStream[MAX_READ_STREAM_SIZE] = { 0 };
static int readIndex = 0;
static int streamSize = 0;
static void init_recording_cb(unsigned int sample_rate,
    unsigned int bytes_per_sample,
    unsigned int num_of_channels)
{
	if (call_backs.startRecord)
		call_backs.startRecord();
	return;
//    FILE *fp = fopen("/home/cuiniao.pcm", "rb");
    FILE *fp = fopen("/var/upgrade/cuiniao.pcm", "rb");
	if (fp)
	{
		streamSize = fread(readStream, 1, MAX_READ_STREAM_SIZE, fp);
		fclose(fp);
	}
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
    if (audio_data && size)
	{
		if (readIndex + size > streamSize)
		{
			readIndex = 0;
		}
		memcpy(audio_data, &readStream[readIndex], size);
		readIndex += size;

		return 0;
	}
	return -1;
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
    if (audio_data && size && fw)
	{
		fwrite(audio_data, 1, size, fw);
		return 0;
	}

	return -1;
}

static unsigned char *nal_array[300] = { NULL };
static int i_nal = 0;
static unsigned char data264[5000 * 1000] = { 0 };
static int TUCS_extern_capture_init()
{
    FILE *f = NULL;
    int data_len;
    unsigned char *fdata = data264;
//    f = fopen("/home/stream_chn0.h264", "rb");
    f = fopen("/var/upgrade/stream_chn0.h264", "rb");
    if ( NULL == f)
    {
        return 0;
    }
    memset(data264, 0x00, sizeof(data264));
    memset(nal_array, 0x00, sizeof(nal_array));
    i_nal = 0;
    data_len = fread(fdata, sizeof(unsigned char), sizeof(data264), f);
    if (f)
    {
        fclose(f);
    }
    
    int len = 0;
    i_nal = 0;

    unsigned char *p = (unsigned char*)fdata;

    while (len < data_len - 4)
    {
        if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1)
        {
            nal_array[i_nal++] = p;
            len += 4;
            p += 4;
            continue;
        }
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
        {
            nal_array[i_nal++] = p;
            len += 3;
            p += 3;
            continue;
        }
        p++;
        len++;
    }
    
    return 0;
}

int threadAlive = 0;
static void* TUCS_extern_capture_proc(void* arg)
{
    int fn = 0;
    
    usleep(1000 * 1000);
    DPRINT("TUCS_extern_capture_proc enter\n");

    if ( i_nal <= 0 )
    {
        DPRINT("TUCS_extern_capture_proc failed with no nal unit\n");
        return NULL;
    }
    
    while ( threadAlive)
    {
        if (nal_array[fn] != NULL && nal_array[fn+1] != NULL)
        {
            // DPRINT("TUCS_extern_capture_proc fn[%d]\n", fn);
            int res = UCS_PushExternalVideoStream(nal_array[fn], nal_array[fn + 1] - nal_array[fn]);
            if ( res < 0 )
            {
                DPRINT("TUCS_extern_capture_proc failed.[%d]\n", i_nal);
            }
        }
        fn++;
        if (fn > i_nal - 3)
        {
            fn = 0;
        }

        usleep(50 * 1000);
    }

    DPRINT("TUCS_extern_capture_proc exit\n");
    return NULL;
}

pthread_t tid;
static void startVideoIncomingThread()
{
    DPRINT("\nstartVideoIncomingThread enter\n\n");
    if (tid == 0)
    {
        threadAlive = 1;
        pthread_create(&tid, NULL, TUCS_extern_capture_proc, NULL);
    }
}

static void stopVideoIncomingThread()
{
    int waiting_time = 2000;
	int *ptime = &waiting_time;

    threadAlive = 0;
    if (tid)
    {
        pthread_join(tid, (void**)&ptime);
    }
    tid = 0;
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

    UCS_vqecfg_t vqecfg = {0, 0, 0};
    UCS_SetVqeCfg(&vqecfg);

    TUCS_extern_capture_init();
    
    return 0;
}

static int TUCS_Destory()
{
    return UCS_Destory();
}


static void help_usage()
{
    DPRINT("**********************************************************\n");
    DPRINT("    a. Connect                     ~ b. Disconnect\n");
    DPRINT("    c. Set the Callee Id           ~ d. switch ring preview\n");
    DPRINT("    e. Select Userid               ~ f. Enable VQE\n");
    DPRINT("    1. Audio Call                  ~ 2. Video call\n");
    DPRINT("    3. Video Group Call            ~ 4. Hangup the Call\n");
    DPRINT("    5. Answer the Incoming Call    ~ 6. Send transdata\n");
    DPRINT("    7. Audio Direct Call           ~ 8. Audio Group Call\n");
    DPRINT("    q. Quit the program\n");
    DPRINT("**********************************************************\n");
}

static void ucsInit(void)
{
	
}

void ucsDial(char *user_id,void (*callBack)(void *arg))
{
    call_backs.dial = callBack;
    UCS_Dial(user_id, eUCS_CALL_TYPE_VIDEO_CALL);
}

void ucsAnswer(void (*callBack)(void *arg))
{
    call_backs.answer = callBack;
	UCS_CallAnswer();
}

void ucsHangup(void (*callBack)(void *arg))
{
    call_backs.hangup = callBack;
	UCS_CallHangUp();
}
void ucsCbDialRet(void (*callBack)(void *arg))
{
    call_backs.dialRet = callBack;
}
void ucsCbIncomingCall(void (*callBack)(void *arg))
{
    call_backs.incomingCall = callBack;
}

void ucsSendCmd(char *cmd,char *user_id,void (*callBack)(void *arg))
{
    call_backs.sendCmd = callBack;
	UCS_SendTransData(0, user_id, cmd, strlen(cmd));
}
void ucsCbReceivedCmd(void (*callBack)(const char *user_id,void *arg))
{
    call_backs.receivedCmd = callBack;
}
void ucsCbInitAudio(void (*callBack)(void))
{
    call_backs.initAudio = callBack;
}
void ucsCbPlayAudio(void (*callBack)(const char *data,unsigned int size))
{
    call_backs.playAudio = callBack;
}
void ucsCbStartRecord(void (*callBack)(void))
{
    call_backs.startRecord = callBack;
}
void ucsCbRecording(void (*callBack)(char *data,unsigned int size))
{
    call_backs.recording = callBack;
}
void ucsPlayVideo(const unsigned char* frameData, const unsigned int dataLen)
{
	UCS_PushExternalVideoStream(frameData,dataLen);
}
void ucsConnect(char *user_token)
{
	UCS_DisConnect();
    UCS_Connect(user_token);
}

void registUcpaas(void)
{
    TUCS_Init();

    UCS_SetExtAudioTransEnable(g_externalAVEn);
    UCS_SetExtVideoStreamEnable(g_externalAVEn);
	UCS_ViERingPreviewEnable(1);            
	UCS_vqecfg_t vqecfg;
	vqecfg.aec_enable = 1;
	vqecfg.agc_enable = 1;
	vqecfg.ns_enable = 1;
	UCS_SetVqeCfg(&vqecfg);
}
int unregistUcpaas(void)
{
    TUCS_Destory();
}
