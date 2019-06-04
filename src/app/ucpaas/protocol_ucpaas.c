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
typedef struct UserStruct {
	char id[32];
	char nick_name[32];
	char token[256];
	int scope;
}UserStruct;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static UserStruct user_local;
static UserStruct *user_other;


static char userid[32] = "Tc19053151c36eb7d3c7ddc5";
static char calleeUid[32] = "13267233004";
static char user_token[512] = "eyJBbGciOiJIUzI1NiIsIkFjY2lkIjoiY2I3MzhjZmNkNmFlYTkxMDZiZTk5OTc2NzZlNzJhMDIiLCJBcHBpZCI6ImFjNTdhMDc3ZGIwYjRjY2JhNzEwNTU5Yzk4NzlkYmQ1IiwiVXNlcmlkIjoiVGMxOTA1MjhkZjZhMjE0Y2UyOGFlNTJjIn0=.9LOQ4nvBr9B8m9AP7Fo43wYGo1e9QVEnbnZgp0CUzkY=";

static FILE* fw = NULL;
static int g_isVideoCall = 0;

static int g_isAutoAnswer = 0;
static int g_previewEn = 1;
static int g_externalAVEn = 1;

static char userids[][32] = {
    "Tc190527dabba36d826d1f1",
    "Tc190524396a4d95de7e0156",
    "Tc190528df6a214ce28ae52c"
};

static char tokens[][256] = {
    "eyJBbGciOiJIUzI1NiIsIkFjY2lkIjoiY2I3MzhjZmNkNmFlYTkxMDZiZTk5OTc2NzZlNzJhMDIiLCJBcHBpZCI6ImFjNTdhMDc3ZGIwYjRjY2JhNzEwNTU5Yzk4NzlkYmQ1IiwiVXNlcmlkIjoiVGMxOTA1MjdkYWJiYTM2ZDgyNmQxZjAifQ==.AGu9Ok35iq1BihAHnnsbQvHjrtVGXTZC3YkfnSvPnRE=",
    "eyJBbGciOiJIUzI1NiIsIkFjY2lkIjoiY2I3MzhjZmNkNmFlYTkxMDZiZTk5OTc2NzZlNzJhMDIiLCJBcHBpZCI6ImFjNTdhMDc3ZGIwYjRjY2JhNzEwNTU5Yzk4NzlkYmQ1IiwiVXNlcmlkIjoiVGMxOTA1MjQzOTZhNGQ5NWRlN2UwMTU2In0=.rT9zenbvSSsxil2410Dew25v4FkTe9ltWw9raKEv6i4=",
    "eyJBbGciOiJIUzI1NiIsIkFjY2lkIjoiY2I3MzhjZmNkNmFlYTkxMDZiZTk5OTc2NzZlNzJhMDIiLCJBcHBpZCI6ImFjNTdhMDc3ZGIwYjRjY2JhNzEwNTU5Yzk4NzlkYmQ1IiwiVXNlcmlkIjoiVGMxOTA1MjhkZjZhMjE0Y2UyOGFlNTJjIn0=.9LOQ4nvBr9B8m9AP7Fo43wYGo1e9QVEnbnZgp0CUzkY="
};
static int userid_num = sizeof(userids)/sizeof(userids[0]);


static int TUCS_extern_capture_init();
static void startVideoIncomingThread();
static void stopVideoIncomingThread();


// UCS connect status callback
void connect_event_cb(int ev_reason)
{
    printf("%s: ev_reason[%d]\n", __FUNCTION__, ev_reason);
}

// UCS dailing out failed
void dial_failed_cb(const char* callid, int reason)
{
    printf("[%s] callid[%s] reason[%d]\n", __FUNCTION__, callid, reason);
    if (g_isVideoCall && g_externalAVEn)
    {
        stopVideoIncomingThread();
    }
}

// call alerting
void on_alerting_cb(const char* callid)
{
    printf("[%s] callid[%s]\n", __FUNCTION__, callid);
    if (g_isVideoCall && g_previewEn && g_externalAVEn)
    {
        startVideoIncomingThread();
    }
}

// new call incoming
void on_incomingcall_cb(const char* callid, int calltype,
    const char* caller_uid, const char* caller_name,
    const char* userdata)
{
    printf("[%s] callid[%s] calltype[%d], caller_uid[%s] caller_name[%s] userdata[%s]\n", 
        __FUNCTION__, callid, calltype, caller_uid, caller_name, userdata);
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
void on_answer_cb(const char* callid)
{
    printf("[%s] callid[%s]\n", __FUNCTION__, callid);
    if (g_isVideoCall && g_externalAVEn)
    {
        startVideoIncomingThread();
    }
}

// call hangup
void on_hangup_cb(const char* callid, int reason)
{
    printf("[%s] callid[%s]\n", __FUNCTION__, callid);

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
void received_dtmf_cb(int dtmf_code)
{
    printf("[%s] dtmf_code[%d]\n", __FUNCTION__, dtmf_code);
}

// network quality report between call session
void network_state_report_cb(int reason, const char* netstate)
{
    printf("[%s] reason[%d] netstate[%s]\n", __FUNCTION__, reason, netstate);
}

// UCS through data received callback
void transdata_received_cb(const char* from_userId,
                        const char* callid,
                        const char* trans_data)
{
    if ( NULL != trans_data)
    {
        printf("transdata_received_cb: trans_data[%s]\n", trans_data);
    }
}

void transdata_send_result_cb(int err_code)
{
    printf("transdata_send_result_cb: err_code[%d]\n", err_code);
}

// UCS invoke it to change video stream
void set_video_framerate_cb(unsigned int target_width, unsigned int target_height,
    unsigned int frame_rate, unsigned int bitrate)
{
    printf("set_video_framerate_cb: resolution[%dx%d fps[%d] bitrate[%d]\n",
        target_width, target_height, frame_rate, bitrate);
}

// UCS init external playout device with given parameters
// sample_rate -- playout audio sample rate, 16k
// bytes_per_sample -- bytes of per sample, always 2 bytes = 16bits
// num_of_channels -- number of playout channels, always = 1
void init_playout_cb(unsigned int sample_rate,
    unsigned int bytes_per_sample,
    unsigned int num_of_channels)
{
//    fw = fopen("/home/playout_t.pcm", "wb");
    fw = fopen("/var/upgrade/playout_t.pcm", "wb");
	if (NULL == fw)
	{
		printf("Failed to open playout_t.pcm\n");
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
void init_recording_cb(unsigned int sample_rate,
    unsigned int bytes_per_sample,
    unsigned int num_of_channels)
{
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
int read_recording_data_cb(char * audio_data,
    unsigned int size)
{
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
int write_playout_data_cb(const char* audio_data,
    unsigned int size)
{
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
int TUCS_extern_capture_init()
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
void* TUCS_extern_capture_proc(void* arg)
{
    int fn = 0;
    
    usleep(1000 * 1000);
    printf("TUCS_extern_capture_proc enter\n");

    if ( i_nal <= 0 )
    {
        printf("TUCS_extern_capture_proc failed with no nal unit\n");
        return NULL;
    }
    
    while ( threadAlive)
    {
        if (nal_array[fn] != NULL && nal_array[fn+1] != NULL)
        {
            // printf("TUCS_extern_capture_proc fn[%d]\n", fn);
            int res = UCS_PushExternalVideoStream(nal_array[fn], nal_array[fn + 1] - nal_array[fn]);
            if ( res < 0 )
            {
                printf("TUCS_extern_capture_proc failed.[%d]\n", i_nal);
            }
        }
        fn++;
        if (fn > i_nal - 3)
        {
            fn = 0;
        }

        usleep(50 * 1000);
    }

    printf("TUCS_extern_capture_proc exit\n");
    return NULL;
}

pthread_t tid;
void startVideoIncomingThread()
{
    printf("\nstartVideoIncomingThread enter\n\n");
    if (tid == 0)
    {
        threadAlive = 1;
        pthread_create(&tid, NULL, TUCS_extern_capture_proc, NULL);
    }
}

void stopVideoIncomingThread()
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

int TUCS_Init()
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
        printf("ucs init failed.\n");
        return -1;
    }

    UCS_SetLogEnable(0, NULL);

    UCS_GetVersion(version);
	printf("ucpaas version %s\n",version);

    UCS_vqecfg_t vqecfg = {1, 1, 1};
    UCS_SetVqeCfg(&vqecfg);

    TUCS_extern_capture_init();
    
    return 0;
}

int TUCS_Destory()
{
    return UCS_Destory();
}

void TUCS_SelectUserid()
{
    int selectid = 0;
    
    printf("--------------------------\n");
    for (int idx = 0; idx < userid_num; idx++)
    {
        printf("    %d: %s\n", idx, userids[idx]);
    }
    printf("Please select userid: ");
    scanf("%d", &selectid);
    printf("Select userid: %s\n", userids[selectid]);

    if (selectid > userid_num)
    {
        return;
    }
    
    strcpy(userid, userids[selectid]);
    strcpy(user_token, tokens[selectid]);

    if (strncmp(userid, "68491000051922", strlen(userid)) == 0)
    {
        strcpy(calleeUid, "68491000051923");
    }

    UCS_DisConnect();
    
    UCS_Connect(user_token);
}

void TUCS_GroupDial(int videoCall)
{
    char input[32] = { 0 };
    char calleds[UCS_MAX_GROUP_DIAL_CALLEE_NUM][32] = { 0 };
    int called_num = 0;
    UCS_groupdial_param_t group_dial;

    memset(&group_dial, 0x00, sizeof(UCS_groupdial_param_t));

    for (int idx = 0; idx < UCS_MAX_GROUP_DIAL_CALLEE_NUM; idx++)
    {
        printf("Please input called(end by 'd'):\n");
        fgets(input, 32, stdin);
        if (input[0] == 'd')
        {
            break;
        }
        else 
        {
            strncpy(group_dial.calleeUserId[group_dial.calleeNum++], input, strlen(input) - 1);
        }
    }

    if (!group_dial.calleeNum)
    {
        return;
    }

    for(int i = 0; i < group_dial.calleeNum; i++)
    {
        printf("callee %d: %s\n", i, group_dial.calleeUserId[i]);
    }
    if (videoCall)
    {
        UCS_GroupDial(&group_dial, eUCS_CALL_TYPE_VIDEO_CALL);
        g_isVideoCall = 1;
    }
    else
    {
        UCS_GroupDial(&group_dial, eUCS_CALL_TYPE_AUDIO_FREE_CALL);
        g_isVideoCall = 0;
    }
}

void help_usage()
{
    printf("******************Softphone Menu**************************\n");
    printf("        the User   Id           : %s\n", userid);
    printf("        the Callee Id           : %s\n", calleeUid);
    printf("**********************************************************\n");
    printf("    a. Connect                     ~ b. Disconnect\n");
    printf("    c. Set the Callee Id           ~ d. switch ring preview\n");
    printf("    e. Select Userid               ~ f. Enable VQE\n");
    printf("\n");
    printf("    1. Audio Call                  ~ 2. Video call\n");
    printf("    3. Video Group Call            ~ 4. Hangup the Call\n");
    printf("    5. Answer the Incoming Call    ~ 6. Send transdata\n");
    printf("    7. Audio Direct Call           ~ 8. Audio Group Call\n");
    printf("\n");
    printf("    q. Quit the program\n");
    printf("**********************************************************\n");
}

static void ucsInit(void)
{
	
}

int registUcpaas(void)
{
    char cmd;
    char c;
    TUCS_Init();

    sleep(1);
	sqlGetUserInfo(0,user_local.id,user_local.token,user_local.nick_name,&user_local.scope);
	int cnt = sqlGetUserInfoStart(1);
	printf("cnt:%d\n", cnt);
	int i;
	user_other = (UserStruct *) calloc(cnt,sizeof(UserStruct));
	UserStruct * p_user = user_other;
	for (i=0; i<cnt; i++) {
		sqlGetUserInfos(p_user->id,p_user->nick_name,&p_user->scope);
		printf("%d:info:%s,%s,%d\n",i,p_user->id,p_user->nick_name,p_user->scope );
		p_user++;
	}
	sqlGetUserInfoEnd();
	printf("id:%s\n",user_local.id );
	printf("token:%s\n",user_local.token );
    UCS_Connect(user_token);

    UCS_SetExtAudioTransEnable(g_externalAVEn);
    UCS_SetExtVideoStreamEnable(g_externalAVEn);
	return 0;
    while (cmd != 'q')
    {
        help_usage();
        
        cmd = 0;        
        while ((c = getchar()) != '\n')
        {
            cmd = c;
        }
        
        switch (cmd)
        {
        case 'a':
            UCS_Connect(user_token);
            break;

        case 'b':
            UCS_DisConnect();
            break;

        case 'c':
            printf("Please input the Callee userid:\n");
            fgets(calleeUid, 32, stdin);
            calleeUid[strlen(calleeUid)-1] = '\0';
            printf("You input callee uid: %s\n", calleeUid);
            break;

        case 'd':
            g_previewEn = !g_previewEn;
            printf("Video ring preview %s\n", g_previewEn ? "enable" : "disable");
            UCS_ViERingPreviewEnable(g_previewEn);            
            break;

        case 'e':
            TUCS_SelectUserid();
            getchar();
            break;

        case 'f':
        {
            UCS_vqecfg_t vqecfg;
            vqecfg.aec_enable = 1;
            vqecfg.agc_enable = 1;
            vqecfg.ns_enable = 1;
            UCS_SetVqeCfg(&vqecfg);
        }
        break;

        case '1':
            UCS_Dial(calleeUid, eUCS_CALL_TYPE_AUDIO_FREE_CALL);
            g_isVideoCall = 0;
            break;

        case '2':
            UCS_Dial(calleeUid, eUCS_CALL_TYPE_VIDEO_CALL);
            g_isVideoCall = 1;
            break;

        case '3':
        {
            TUCS_GroupDial(1);
        }
            break;

        case '4':
            UCS_CallHangUp();
            break;
        
        case '5':
            UCS_CallAnswer();
            break;

        case '6':
        {
            const char* testdata = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyz";
            UCS_SendTransData(0, calleeUid, testdata, strlen(testdata));
        }
            break;

        case '7':
        {
            UCS_Dial(calleeUid, eUCS_CALL_TYPE_AUDIO_DIRECT_CALL);
            g_isVideoCall = 0;
        }
			break;  

        case '8':
        {
            TUCS_GroupDial(0);
        }
            break;
        default:
            break;
        }
    }

    TUCS_Destory();

    return 0;
}
