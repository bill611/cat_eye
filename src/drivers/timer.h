/*
 * =============================================================================
 *
 *       Filename:  Timer.h
 *
 *    Description:  定时器20ms
 *
 *        Version:  v1.0
 *        Created:  2016-08-08 18:33:18 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _TIMER_H
#define _TIMER_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define LAYER_TIME_1S 1000

	struct _TimerPriv;
	typedef struct _Timer {
		struct _TimerPriv *priv;

		// 需要调用handle在已有定时器中运行
		
		void (*start)(struct _Timer *);
		void (*stop)(struct _Timer *);
		void (*destroy)(struct _Timer *);
		int (*handle)(struct _Timer *);
		void (*resetTick)(struct _Timer *);
		unsigned int (*getSystemTick)(void);

		// real timer 系统定时器,根据不同系统进行定制

		void (*realTimerCreate)(struct _Timer *, double times, void (*function)(int ,int ));
		void (*realTimerDelete)(struct _Timer *);
	} Timer;

	Timer * timerCreate(int speed,void (*function)(void *),void *arg);
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
