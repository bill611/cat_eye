/*
 * =====================================================================================
 *
 *       Filename:  FormMain.h
 *
 *    Description:  主窗口
 *
 *        Version:  1.0
 *        Created:  2016-02-23 15:34:38
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
#ifndef _FORM_MAIN_H
#define _FORM_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "commongdi.h"

	typedef struct _FormMainTimers {
		void (*proc)(void);
		int time;
	}FormMainTimers;
	
	typedef struct _FormMain {
		int status;
		FormMainTimers *timer;
		int (*loop)(void);

		void (*timerStart)(int idc_timer);
		void (*timerStop)(int idc_timer);
		int (*timerGetState)(int idc_timer);

	} FormMain;


	FormMain * formMainCreate(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
