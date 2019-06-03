/*
*  Copyright (c) 2017 The UCPAAS project authors. All Rights Reserved.
*
*/

#ifndef __UCS_INCLUDE_UCSSERVICE_H__
#define __UCS_INCLUDE_UCSSERVICE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "ucs_defines.h"

/************************************************************************/
/* UCS enum definition                                                  */
/************************************************************************/



/************************************************************************/
/* UCS struct definition                                                */
/************************************************************************/
// UCS connect status callback
typedef void(*connect_event_cb_f)(int ev_reason);

// UCS dailing out failed
typedef void(*dial_failed_cb_f)(const char* callid, int reason);

// call alerting
typedef void(*on_alerting_cb_f)(const char* callid);

// new call incoming
typedef void(*on_incomingcall_cb_f)(const char* callid, int calltype,
    const char* caller_uid, const char* caller_name,
    const char* userdata);

// call answer
typedef void(*on_answer_cb_f)(const char* callid);

// call hangup
typedef void(*on_hangup_cb_f)(const char* callid, int reason);

// received dtmf
typedef void(*received_dtmf_cb_f)(int dtmf_code);

// network quality report between call session
typedef void(*network_state_report_cb_f)(int reason, const char* netstate);

// UCS through data received callback
typedef void(*transdata_received_cb_f)(const char* from_userId, 
                                        const char* callid,
                                        const char* trans_data);

// UCS send through data result callback
typedef void(*transdata_send_result_cb_f)(int err_code);

// UCS invoke it to change video stream
typedef void(*set_video_framerate_cb_f)(unsigned int target_width, unsigned int target_height,
                                        unsigned int frame_rate, unsigned int bitrate);

// UCS init external playout device with given parameters
// sample_rate -- playout audio sample rate, 16k
// bytes_per_sample -- bytes of per sample, always 2 bytes = 16bits
// num_of_channels -- number of playout channels, always = 1
typedef void(*init_playout_cb_f)(unsigned int sample_rate,
                                unsigned int bytes_per_sample,
                                unsigned int num_of_channels);

// UCS init external recording device with given parameters
// sample_rate -- recording audio sample rate, 16000
// bytes_per_sample -- bytes of per sample, always 2 bytes = 16bits
// num_of_channels -- number of recording channels, always = 1
typedef void(*init_recording_cb_f)(unsigned int sample_rate,
                                unsigned int bytes_per_sample,
                                unsigned int num_of_channels);

// UCS read recording 10ms pcm data from external audio device 
// audio_data -- recording pcm data
// size -- want to read data len
typedef int(*read_recording_data_cb_f)(char * audio_data,
                                       unsigned int size);

// UCS write playout 10ms pcm data to external audio device
// audio_data -- playout data for external audio device
// size -- playout data length
typedef int(*write_playout_data_cb_f)(const char* audio_data,
                                     unsigned int size);

typedef struct UCS_cb_vtable_t
{
    connect_event_cb_f          connect_cb;
    dial_failed_cb_f            dial_failed_cb;
    on_alerting_cb_f            on_alerting_cb;
    on_answer_cb_f              on_answer_cb;
    on_incomingcall_cb_f        on_incomingcall_cb;
    on_hangup_cb_f              on_hangup_cb;
    received_dtmf_cb_f          received_dtmf_cb;
    network_state_report_cb_f   network_state_report_cb;
    transdata_received_cb_f     transdata_received_cb;
    transdata_send_result_cb_f  transdata_send_result_cb;
    set_video_framerate_cb_f    set_video_framerate_cb;
    init_playout_cb_f           init_playout_cb;
    init_recording_cb_f         init_recording_cb;
    read_recording_data_cb_f    read_data_cb;
    write_playout_data_cb_f     write_data_cb;
} UCS_cb_vtable_t;

#define UCS_MAX_GROUP_DIAL_CALLEE_NUM   6
typedef struct UCS_groupdial_param_t
{
    char calleeUserId[UCS_MAX_GROUP_DIAL_CALLEE_NUM][UCS_MAX_USERID_STR_LEN];
    int  calleeNum;
} UCS_groupdial_param_t;

typedef struct UCS_vqecfg_t
{
	unsigned char aec_enable;
	unsigned char agc_enable;
	unsigned char ns_enable;
} UCS_vqecfg_t;

/************************************************************************/
/* UCS API declare                                                      */
/************************************************************************/
/************************************************************
Function	: UCS_RegisterCallbackVtable
Description : set UCSService callbacks
Input		: token -- user login token
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_RegisterCallbackVtable(UCS_cb_vtable_t* vtable);

/************************************************************
Function	: UCS_Init
Description : Init UCSService, invoked after UCS_RegisterCallbackVtable()
Input		: none
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_Init();

/************************************************************
Function	: UCS_Destory
Description : Destory UCSService
Input		: 
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_Destory();

/************************************************************
Function	: UCS_GetVersion
Description : get UCS version
Input		: 
Output		: version
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_GetVersion(char* version);

/************************************************************
Function	: UCS_SetLogEnable
Description : enable/disable UCS logger
Input		: enable -- enable/disable UCS logger
            : logpath -- sdk log path, without file name. should like: "/mnt/mmc/"
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_SetLogEnable(int enable, const char* logpath);

/************************************************************
Function	: UCS_ViERingPreviewEnable
Description : enable/disable UCS video call ring preview
Input		: enable -- 1: enable, 0: disable
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/7/19
************************************************************/
int UCS_ViERingPreviewEnable(int enable);

/************************************************************
Function	: UCS_Connect
Description : connect to UCPAAS server
Input		: token -- user login token
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_Connect(const char* token);

/************************************************************
Function	: UCS_DisConnect
Description : disconnect connection with UCPAAS server
Input		: none
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_DisConnect();

/************************************************************
Function	: UCS_SendTransData
Description : Do through data sending
Input		: isOffline - 1: use offline push, 0: use normal tcp trans
              targetId -- target for send data to
              transData -- through data
              dataLen -- len of through data
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_SendTransData(const int isOffline, const char* targetId, 
                            const char* transData, const int dataLen);

/************************************************************
Function	: UCS_Dial()
Description : Do call outgoing
Input		: callId -- called userid
            : videoCall -- video call
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_Dial(const char* calledId, eUcsCallType callType);

/************************************************************
Function	: UCS_GroupDial
Description : Do group call outgoing
Input		: dialParam -- called info
            : videoCall -- video call
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_GroupDial(UCS_groupdial_param_t* dialParam, eUcsCallType callType);

/************************************************************
Function	: UCS_CallAnswer
Description : accept the incoming call
Input		: none
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_CallAnswer();

/************************************************************
Function	: UCS_CallHangUp
Description : hangup/reject the call
Input		: none
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_CallHangUp();

/************************************************************
Function	: UCS_SetExtAudioTransEnable
Description : enable/disable external audio pcm stream
Input		: enable -- 1 for enable, 0 disable           
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_SetExtAudioTransEnable(int enable);

/************************************************************
Function	: UCS_SetExtVideoStreamEnable
Description : enable/disable external video h.264 stream
Input		: enable -- 1 for enable, 0 disable
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_SetExtVideoStreamEnable(int enable);

/************************************************************
Function	: UCS_SetVqeCfg
Description : Enable/disable media engine voice quality enhancement
Input		: vqecfg -- voice quality enhancement config
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/8/21
************************************************************/
int UCS_SetVqeCfg(UCS_vqecfg_t* vqecfg);

/************************************************************
Function	: UCS_PushExternalVideoStream
Description : push external h264 stream to UCS for sending out
Input		: frameData -- h264 frame data
              dataLen   -- len of h264 frame data
Output		: none
Return		: 0 if succeed, else -1;
Remark		: None
Modified	: 2017/6/9
************************************************************/
int UCS_PushExternalVideoStream(const unsigned char* frameData, const unsigned int dataLen);

int UCS_get_video_frame(char *data, unsigned int* length, long long* timeStamp, int *frameType);

void UCS_video_start(int send_receive);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __UCS_INCLUDE_UCSSERVICE_H__ */
