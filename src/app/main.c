/*
 * =============================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  智能猫眼 
 *
 *        Version:  1.0
 *        Created:  2019-04-19 16:47:01
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
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "thread_helper.h"
#include "my_gpio.h"
#include "externfunc.h"
#include "protocol.h"
#include "sql_handle.h"
#include "config.h"
#include "form_videlayer.h"


/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void *gpioRegistThread(void *arg);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
typedef struct _GpioInputThread {
	struct GpioArgs args;
	void *(*func)(void *);
}GpioInputThread;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static GpioInputThread gpio_input_handle[] =
{
    {{NULL,ENUM_GPIO_MODE},		gpioRegistThread},
};

static void gpioInputRegist(void)
{
	unsigned int i;
	for (i=0; i<NELEMENTS(gpio_input_handle); i++) {
		gpio_input_handle[i].args.gpio = gpio;
		gpio->addInputThread(gpio,&gpio_input_handle[i].args,gpio_input_handle[i].func);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioRegistThread 注册按键
 *
 * @param arg
 * @param port 当前设备端口
 */
/* ---------------------------------------------------------------------------*/
static void *gpioRegistThread(void *arg)
{
	struct GpioArgs *This = arg;
	static int status = 0,status_old = 0;
	while (1) {
		status = This->gpio->inputHandle(This->gpio,This->port);
		if (status && status_old == 0) {
			printf("[%s]\n", __FUNCTION__);
		}
		status_old = status;
		usleep(10000);
	}
	return NULL;
}

static void * timer1sThread(void *arg)
{
	while(1) {
		sleep(1);
	}	
	return NULL;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief MiniGUIMain 主函数入口
 *
 * @param argc
 * @param argv[]
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
int MiniGUIMain(int argc, const char* argv[])
{
	printf("stat--->%s,:%s\n",DEVICE_SVERSION,DEVICE_KVERSION);
	configLoad();
    sqlInit();
	gpioInit();
	gpioInputRegist();
	createThread(timer1sThread,NULL);
	protocolInit();
	formVideoLayerCreate();
    return 0;
}
