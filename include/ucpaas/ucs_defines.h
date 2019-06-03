/*
*  Copyright (c) 2017 The UCPAAS project authors. All Rights Reserved.
*
*/
#ifndef __UCS_INCLUDE_BASE_DEFINES_H__
#define __UCS_INCLUDE_BASE_DEFINES_H__

#ifdef __cplusplus
extern "C"{
#endif /* End of #ifdef __cplusplus */

#define UCS_MAX_USERID_STR_LEN      32
#define UCS_MAX_PHONE_STR_LEN       32
#define UCS_MAX_BRAND_STR_LEN       32
#define UCS_MAX_CALLID_STR_LEN      32
#define UCS_MAX_IP_STR_LEN          48
#define UCS_MAX_NICKNAME_STR_LEN    64
#define UCS_MAX_VERSION_STR_LEN     64
#define UCS_MAX_USERDATA_STR_LEN    128
#define UCS_MAX_APPID_STR_LEN       128
#define UCS_MAX_TCP_TRANS_DATA_LEN  512

#define UCS_MIN_USER_TOKEN_STR_LEN  100
#define UCS_MAX_USER_TOKEN_STR_LEN  256

typedef enum
{
    eUCS_REASON_SUCCESS = 300000,

    /* 300001 ~ 300099 is connect reason */
    /* tcp connection established */
    eUCS_REASON_TCP_CONNECTED = 300001,
    /* tcp connection reconnected */
    eUCS_REASON_TCP_RECONNECTED = 300002,
    /* tcp connection disconnected */
    eUCS_REASON_TCP_DISCONNECTED = 300003,    
    eUCS_REASON_TCP_SEND = 300004,
    eUCS_REASON_TCP_RECV = 300005,

    /* tcp through data is empty */
    eUCS_REASON_TCP_TRANS_EMPTY = 300010,
    /* tcp through data too large */
    eUCS_REASON_TCP_TRANS_2_LARGE = 300011,
    /* tcp through targetid not exist */
    eUCS_REASON_TCP_TRANS_TARGET_UNEXIST = 300012,
    /* tcp through targetid offline */
    eUCS_REASON_TCP_TRANS_TARGET_OFFLINE = 300013,
    /* tcp through proxy server internal error */
    eUCS_REASON_TCP_TRANS_PROXY_ERROR = 300014,
    /* tcp through data send timeout */
    eUCS_REASON_TCP_TRANS_SEND_TIMEOUT = 300015,

    /* 300100 ~ 300199 is login reason */
    /* cloud platform login successed */
    eUCS_REASON_LOGIN_SUCCESS = 300100,
    /* login failed caused by invalid token */
    eUCS_REASON_TOKEN_INVALID = 300101,
    /* login failed caused by error arguments */
    eUCS_REASON_LOGIN_ARG_ERR = 300102,
    /* login failed caused by proxy server error */
    eUCS_REASON_LOGIN_SYS_ERR = 300103,
    /* login failed caused by wrong password */
    eUCS_REASON_LOGIN_PWD_ERR = 300104,
    /* login failed caused no proxys */
    eUCS_REASON_INVALID_PROXYS_NUM = 300105,

    /* 300200 ~ 300299 is call reason */
    eUCS_REASON_RINGING = 300200,
    eUCS_REASON_CONNECTING = 300201,
    /* dial failed caused by wrong called */
    eUCS_REASON_INVALID_CALLED = 300202,
    /* dial failed caused by called offline */
    eUCS_REASON_CALLED_OFFLINE = 300203,
    /* dial failed caused by called in busy */
    eUCS_REASON_CALLED_BUSY = 300204,
    /* dial failed caused by called reject the call */
    eUCS_REASON_CALLED_REJECT = 300205,
    /* dial failed caused by called not answer */
    eUCS_REASON_CALLED_NO_ANSWER = 300206,
    /* dial failed caused by called has frozen */
    eUCS_REASON_CALLED_FROZEN = 300207,
    /* dial failed caused by caller has frozen */
    eUCS_REASON_CALLER_FROZEN = 300208,
    /* dial failed caused by caller has expired */
    eUCS_REASON_CALLER_EXPIRED = 300209,
    /* dial failed caused by caller insufficient balance */
    eUCS_REASON_NO_BALANCE = 300210,
    /* dial failed caused by signal message timeout */
    eUCS_REASON_MSG_TIMEOUT = 300211,
    /* dial failed caused by caller in black list */
    eUCS_REASON_BLACKlIST = 300212,
    /* hangup by remote peer */
    eUCS_REASON_HANGUP_BYPEER = 300213,
    /* hangup by local */
    eUCS_REASON_HANGUP_MYSELF = 300214,
    /* call failed caused by media consultation failed */
    eUCS_REASON_MEDIA_NOT_ACCEPT = 300215,
    /* call hangup caused by relay server rtp received timeout */
    eUCS_REASON_RTPP_TIMEOUT    = 300216,
    /* call hangup caused by media update failed */
    eUCS_REASON_MEDIA_UPDATE_FAILED = 300217,
    /* dial cancel by caller */
    eUCS_REASON_CALLER_CANCEL = 300218,
    /* dial failed caused by forbidden dialing */
    eUCS_REASON_FORBIDDEN   = 300219,
    /* call hangup caused by local rtp packets received timeout */
    eUCS_REASON_RTP_RECEIVED_TIMEOUT = 300220,
    /* hangup failed caused by not exist call */
    eUCS_REASON_CALLID_NOT_EXIST    = 300221,
    /* dial failed caused by caller not login successed */
    eUCS_REASON_NOT_LOGGED_IN = 300222,
    /* dial failed caused by server internal error */
    eUCS_REASON_INTERNAL_SRV_ERROR = 300223,

    eUCS_REASON_UNDEFINE = 399999
} eUcsReason;

typedef enum
{
	eUCS_CALL_TYPE_AUDIO_FREE_CALL 		= 0x01,
	eUCS_CALL_TYPE_AUDIO_DIRECT_CALL 	= 0x02,
	eUCS_CALL_TYPE_VIDEO_CALL 			= 0x03
} eUcsCallType;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif // __UCS_INCLUDE_BASE_DEFINES_H__
