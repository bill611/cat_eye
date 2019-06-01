/*
 * =============================================================================
 *
 *       Filename:  Timer.c
 *
 *    Description:  定时器
 *
 *        Version:  v1.0
 *        Created:  2016-08-08 18:33:01
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
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#include "timer.h"
#include "thread_helper.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

typedef struct _TimerPriv {
	timer_t real_id;
	pthread_mutex_t mutex;

	int enable;
	unsigned int speed;
	unsigned int count;
	unsigned int count_old;
	int thread_end;

	void (*func)(void *);
	void * arg;
}TimerPriv;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/

static void timerStart(Timer *This)
{
	if (This->priv->enable) {
		printf("Timer already start\n");
		return;
	}
	This->priv->count_old = This->priv->count = This->getSystemTick();
	This->priv->enable = 1;
}

static void timerStop(Timer *This)
{
	if (This->priv->enable == 0) {
		printf("Timer stopped\n");
		return;
	}
	This->priv->enable = 0;
}
static int timerIsStop(Timer *This)
{
	return This->priv->enable == 0 ?1:0;	
}

static void timerDestroy(Timer *This)
{
	This->stop(This);
	This->realTimerDelete(This);
	while (This->priv->thread_end == 0) {
		usleep(10000);
	}
	if (This->priv)
		free(This->priv);
	if (This)
		free(This);
}

static void* timerThread(void *arg)
{
	Timer *This = (Timer *)arg;
	This->priv->thread_end = 0;
	while(This->priv->real_id) {
		if (This->priv->enable == 0) {
			usleep(10000);
			continue ;
		}
		
		if ((This->priv->count - This->priv->count_old) >= This->priv->speed) {
			if (This->priv->func)
				This->priv->func(This->priv->arg);
			pthread_mutex_lock(&This->priv->mutex);
			This->priv->count_old = This->priv->count;
			pthread_mutex_unlock(&This->priv->mutex);
		} else {
			pthread_mutex_lock(&This->priv->mutex);
			This->priv->count = This->getSystemTick();
			pthread_mutex_unlock(&This->priv->mutex);
		}
		usleep(10000);
	}
	This->priv->thread_end = 1;
	return NULL;
}

static int timerHandle(Timer *This)
{
	if (This->priv->enable == 0) {
		return 0;
	}
	int ret = 0;
	if ((This->priv->count - This->priv->count_old) >= This->priv->speed) {
		if (This->priv->func) {
			This->priv->func(This->priv->arg);
			ret = 1;
		}
		This->priv->count_old = This->priv->count;
		// This->priv->count = 0;
	} else {
		// This->priv->count++;
		This->priv->count = This->getSystemTick();
	}
	return ret;
}

static unsigned int getSystemTickDefault(void)
{
	struct  timeval tv;
	gettimeofday(&tv,NULL);
	return ((tv.tv_usec / 1000) + tv.tv_sec  * 1000 );
}

static void timerResetTick(Timer *This)
{
	if (This->priv->enable == 0)
		return ;
	pthread_mutex_lock(&This->priv->mutex);
	This->priv->count_old = This->priv->count = This->getSystemTick();
	pthread_mutex_unlock(&This->priv->mutex);
}

static void realTimerCreateDefault(Timer *This,double value,void (*function)(int timerid,int arg))
{
	int result;
	pthread_mutexattr_t mutexattr;

	// 嵌套式线程锁
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&This->priv->mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

	This->priv->real_id = (timer_t)1;
	createThread(timerThread,This);

	// timer_create (CLOCK_REALTIME, NULL, &This->priv->real_id);
	// timer_connect (This->priv->real_id, function,0);
	// timer_settime (This->priv->real_id, 0, value, NULL);
}

static void realTimerDeleteDefault(Timer *This)
{
	// timer_delete(This->priv->real_id);
	This->priv->real_id = 0;
}

Timer * timerCreate(int speed,void (*function)(void *),void *arg)
{
	Timer *This = (Timer *) calloc(1,sizeof(Timer));
	if (!This) {
		printf("timer alloc fail\n");
		return NULL;
	}
	This->priv = (TimerPriv *) calloc(1,sizeof(TimerPriv));
	if (!This->priv){
		printf("timer alloc fail\n");
		free(This);
		return NULL;
	}
	This->priv->speed = speed;
	This->priv->func = function;
	This->priv->arg = arg;

	This->start = timerStart;
	This->stop = timerStop;
	This->destroy = timerDestroy;
	This->handle = timerHandle;
	This->getSystemTick = getSystemTickDefault;
	This->resetTick = timerResetTick;
	This->isStop = timerIsStop;
	realTimerCreateDefault(This,0,NULL);
	// This->realTimerCreate = realTimerCreateDefault;
	This->realTimerDelete = realTimerDeleteDefault;
	return This;
}
