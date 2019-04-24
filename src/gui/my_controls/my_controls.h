/*
 * =============================================================================
 *
 *       Filename:  my_controls.h
 *
 *    Description:  自定义控件接口
 *
 *        Version:  1.0
 *        Created:  2019-04-24 08:26:08 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MY_CONTROLS_H
#define _MY_CONTROLS_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

	typedef struct _MyControls {
		BOOL (*regist)(void);	
		void (*unregist)(void);	
		void (*bmpsLoad)(void *);	
		void (*bmpsRelease)(void *);	
	}MyControls;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
