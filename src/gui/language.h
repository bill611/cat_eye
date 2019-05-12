/*
 * =============================================================================
 *
 *       Filename:  language.h
 *
 *    Description:  语言包
 *
 *        Version:  1.0
 *        Created:  2016-12-02 09:42:15 
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _LANGUAGE_H
#define _LANGUAGE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	enum {
		WORD_NULL ,//空白
		WORD_WIFI_SET ,//wifi设置
		WORD_SCREEN_SET,//屏幕设置
		WORD_DOORBELL_SET,//门铃设置
		WORD_TIMER_SET,//时间设置
		WORD_MUTE_SET,//免扰设置
		WORD_ALARM_SET,//报警设置
		WORD_FACTORY,//恢复出厂
		WORD_LOCAL_SET,//本机设置
		WORD_SETTING,//设置
		WORD_RECORD,//记录
		WORD_CAPTURE,//抓拍
		WORD_VIDEO,//录像
		WORD_UNLOCK,//开门
		WORD_HANGUP,//挂断
		WORD_WIFI_CLOSED,//WIFI已关闭，请点击开始
		WORD_NET_NEARBY,//附近网络
	};
	typedef struct {
		char string[64];	
	}Language;
	extern const Language word[];


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
