/*
 * =====================================================================================
 *
 *       Filename:  MyGpioCtr.c
 *
 *    Description:  GPIO控制
 *
 *        Version:  1.0
 *        Created:  2015-12-24 09:06:46
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
/* ----------------------------------------------------------------*
 *                      include head files
 *-----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>

#include "debug.h"
#include "thread_helper.h"
#include "hal_gpio.h"
#include "my_gpio.h"


/* ----------------------------------------------------------------*
 *                  extern variables declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                  internal functions declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define GPIO_MAX_INPUT_TASK 32
typedef struct _MyGpioTable {
	int       portid;
	int       mask;
	int       active;		//有效值
	int 	  default_value;	//默认值
	int  	  active_time;

	int		  current_value;
	int		  portnum;
	int 	  flash_times;
	int 	  flash_set_time;
	int 	  flash_delay_time;
	int 	  flash_even_flag;
	int		  delay_time;
}MyGpioTable;

typedef struct _MyGpioInputTask {
	int port;
	void *arg;
	void (* thread)(void *,int);
}MyGpioInputTask;

typedef struct _MyGpioPriv {
	MyGpioTable *table;
	pthread_mutex_t mutex;
	int task_num;
}MyGpioPriv;

 
#if (defined ANYKA)
#define GPIO_ZIGBEE_POWER		84,-1,1,IO_ACTIVE
#define GPIO_WIFI_POWER			83,-1,1,IO_ACTIVE
#define GPIO_LED_WIFI			80,-1,0,IO_INACTIVE  // 双色灯，绿
#define GPIO_LED_RESET			-1,-1,0,IO_NO_EXIST
#define GPIO_LED_ONLINE			79,-1,0,IO_INACTIVE  // 双色灯，红
#define GPIO_LED_NET_IN			-1,-1,0,IO_NO_EXIST
#define GPIO_LED_POWER			23,-1,0,IO_ACTIVE  // 单色灯，红

#define GPIO_RESET				85,-1,0,IO_INPUT
#define GPIO_MODE				80,-1,0,IO_INPUT
#else

#define GPIO_ZIGBEE_POWER		'c',15,1,IO_ACTIVE
#define GPIO_WIFI_POWER			'e',0, 1,IO_ACTIVE
#define GPIO_LED_WIFI			'c',14,0,IO_INACTIVE
#define GPIO_LED_RESET			'd',11,0,IO_INACTIVE
#define GPIO_LED_ONLINE			'd',10,0,IO_INACTIVE
#define GPIO_LED_NET_IN			'c',13,0,IO_ACTIVE
#define GPIO_LED_POWER			'c',0, 0,IO_ACTIVE

#define GPIO_RESET				'e',1,0,IO_INPUT
#define GPIO_MODE				'd',3,0,IO_INPUT
#endif
/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/

static MyGpioTable gpio_normal_tbl[]={
	{GPIO_ZIGBEE_POWER,	0},
	{GPIO_WIFI_POWER,	0},
	{GPIO_LED_WIFI,		0},
	{GPIO_LED_RESET,	0},
	{GPIO_LED_ONLINE,	0},
	{GPIO_LED_NET_IN,	0},
	{GPIO_LED_POWER,	0},

	{GPIO_RESET,		30},
	{GPIO_MODE,			1},
};

MyGpio *gpio = NULL;
/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioSetValue GPIO口输出赋值，不立即执行
 *
 * @param This
 * @param port IO口
 * @param Value 有效IO_ACTIVE or 无效IO_INACTIVE
 */
/* ---------------------------------------------------------------------------*/
static int myGpioSetValue(MyGpio *This,int port,int  Value)
{
	MyGpioTable *table;
	table = This->priv->table+port;

#ifdef X86
	if (table->default_value == IO_NO_EXIST ) {   //输出
#else
	if (	(table->default_value == IO_INPUT)
		||  (table->default_value == IO_NO_EXIST) ) {   //输出
#endif
		// printf("[%d]set value fail,it is input or not exist!\n",port);
		return 0;
	}

	if (Value == IO_ACTIVE) {
		table->current_value = table->active;
	} else {
		table->current_value = !(table->active);
	}
	return 1;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioSetValueNow GPIO口输出赋值，并立即执行
 *
 * @param This
 * @param port IO口
 * @param Value 有效IO_ACTIVE or 无效IO_INACTIVE
 */
/* ---------------------------------------------------------------------------*/
static void myGpioSetValueNow(MyGpio *This,int port,int  Value)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (myGpioSetValue(This,port,Value) == 0)
		return;

	pthread_mutex_lock(&This->priv->mutex);
	if (table->current_value)
		halGpioOut(table->portid,table->mask,1);
	else
		halGpioOut(table->portid,table->mask,0);
	pthread_mutex_unlock(&This->priv->mutex);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioRead 读输入口的值
 *
 * @param This
 * @param port IO口
 *
 * @returns 真正的值 0或1
 */
/* ---------------------------------------------------------------------------*/
static int myGpioRead(MyGpio *This,int port)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (table->default_value != IO_INPUT) {
		goto return_value;
	}
#ifndef X86
	if (halGpioIn(table->portid,table->mask))
		table->current_value = 1;
	else
		table->current_value = 0;
#endif

return_value:
	return table->current_value;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioIsActive 读取输入IO值，并判断是否IO有效
 *
 * @param This
 * @param port IO口
 *
 * @returns 有效IO_ACTIVE or 无效IO_INACTIVE
 */
/* ---------------------------------------------------------------------------*/
static int myGpioIsActive(MyGpio *This,int port)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (table->default_value == IO_NO_EXIST) {
		return IO_NO_EXIST;
	}
	if (myGpioRead(This,port) == table->active){
		return IO_ACTIVE;
	} else {
		return IO_INACTIVE;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioFlashTimer GPIO闪烁执行函数，在单独的定时线程中执行
 * IO口电平变化到还原算一次
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void myGpioFlashTimer(MyGpio *This)
{
	int i;
	MyGpioTable *table;
	table = This->priv->table;
	for (i=0; i<This->io_num; i++) {
		if (table->default_value == IO_NO_EXIST) {
			table++;
			continue;
		}
		if (table->default_value == IO_INPUT) {
			table++;
			continue;
		}
		if ((table->flash_delay_time == 0) || (table->flash_times == 0)) {
			table++;
			continue;
		}
		if (--table->flash_delay_time == 0) {
			table->flash_even_flag++;
			if (table->flash_even_flag == 2) {  //亮灭算一次
				table->flash_times--;
				table->flash_even_flag = 0;
			}

			table->flash_delay_time = table->flash_set_time;

			if (myGpioIsActive(This,i) == IO_ACTIVE)
				myGpioSetValueNow(This,i,IO_INACTIVE);
			else
				myGpioSetValueNow(This,i,IO_ACTIVE);
		}
		table++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioFlashStart 设置GPIO闪烁，并执行
 *
 * @param This
 * @param port IO口
 * @param freq 频率 根据myGpioFlashTimer 执行时间决定
 * @param times 闪烁总次数 FLASH_FOREVER为循环闪烁
 */
/* ---------------------------------------------------------------------------*/
static void myGpioFlashStart(MyGpio *This,int port,int freq,int times)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (table->default_value == IO_NO_EXIST) {
		return;
	}
	if (table->flash_set_time != freq) {
		table->flash_delay_time = freq;
		table->flash_set_time = freq;
		table->flash_times = times;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioFlashStop GPIO停止闪烁
 *
 * @param This
 * @param port IO口
 */
/* ---------------------------------------------------------------------------*/
static void myGpioFlashStop(MyGpio *This,int port)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (table->default_value == IO_NO_EXIST) {
		return;
	}

	table->flash_delay_time = 0;
	table->flash_set_time = FLASH_STOP;
	table->flash_times = 0;
	table->flash_even_flag = 0;
	myGpioSetValue(This,port,IO_INACTIVE);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioSetActiveTime 设置输入IO口的有效电平时间
 *
 * @param This
 * @param port IO口
 * @param value 时间
 */
/* ---------------------------------------------------------------------------*/
static void myGpioSetActiveTime(struct _MyGpio *This,int port,int value)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (table->default_value != IO_INPUT) {
		return;
	}
	table->active_time = value;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioInputHandle 检测输入IO电平
 *
 * @param This
 * @param port
 *
 * @returns 1为有效 0为无效
 */
/* ---------------------------------------------------------------------------*/
static int myGpioInputHandle(struct _MyGpio *This,int port)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	int ret = myGpioIsActive(This,port);
	// printf("port:%d,ret:%d,delay_time:%d\n",
		 // port,ret,table->delay_time );
	if (ret != IO_ACTIVE) {
		table->delay_time = 0;
		return 0;
	}
	if (table->delay_time < table->active_time) {
		++table->delay_time;
		return 0;
	} else {
		return 1;
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioInit GPIO初始化，在创建IO对象后必须执行
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void myGpioInit(MyGpio *This)
{
	int i;
	MyGpioTable *table;
	table = This->priv->table;
	for (i=0; i<This->io_num; i++) {
		if (table->portid < 0) // 可以在配置文件将不用IO写-1
			table->default_value = IO_NO_EXIST;

		if (table->default_value == IO_NO_EXIST) {
			table++;
			continue;
		}

		if (table->default_value != IO_INPUT) {
			halGpioSetMode(table->portid,table->mask,HAL_OUTPUT);
		} else {
			halGpioSetMode(table->portid,table->mask,HAL_INPUT);
		}
		if (table->default_value != IO_INPUT)//设置默认值
			myGpioSetValueNow(This,i,table->default_value);

		table->flash_delay_time = 0;
		table->flash_set_time = FLASH_STOP;
		table->flash_times = 0;
		table->flash_even_flag = 0;

		table++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioDestroy 销毁GPIO对象
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void myGpioDestroy(MyGpio *This)
{
	free(This->priv);
	free(This);
	This = NULL;
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioHandle GPIO输出执行函数，真正对IO口赋值，在单独线程
 * 中执行，IO输出以脉冲形式，防止非正常情况导致IO口电平一时错误
 *
 * @param this
 */
/* ---------------------------------------------------------------------------*/
static void myGpioHandle(MyGpio *This)
{
	int i;
	MyGpioTable *table;
	table = This->priv->table;
	for (i=0; i<This->io_num; i++) {
		if (table->default_value == IO_NO_EXIST) {
			table++;
			continue;
		}
		if (table->default_value == IO_INPUT) {
			table++;
			continue;
		}

		if (table->current_value)
			halGpioOut(table->portid,table->mask,1);
		else
			halGpioOut(table->portid,table->mask,0);
		table++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioThread Gpio处理函数，50ms处理一次
 *
 * @param arg
 */
/* ---------------------------------------------------------------------------*/
static void * myGpioOutputThread(void *arg)
{
	MyGpio *This = arg;
	while (This != NULL) {
		myGpioHandle(This);
		myGpioFlashTimer(This);
		usleep(50000);// 50ms
	}
	return NULL;
}


static void myGpioAddInputThread(MyGpio *This,
		struct GpioArgs *args,
		void *(* thread)(void *))
{
	createThread(thread,args);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioPrivCreate 创建GPIO对象
 *
 * @param gpio_table GPIO列表
 *
 * @returns GPIO对象
 */
/* ---------------------------------------------------------------------------*/
MyGpio* myGpioPrivCreate(MyGpioTable *gpio_table,int io_num)
{

	MyGpio *This = (MyGpio *)calloc(1,sizeof(MyGpio));
	This->priv = (MyGpioPriv *)calloc(1,sizeof(MyGpioPriv));
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&This->priv->mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

	This->priv->table = gpio_table;
	This->io_num = io_num;
	This->SetValue = myGpioSetValue;
	This->SetValueNow = myGpioSetValueNow;
	This->FlashStart = myGpioFlashStart;
	This->FlashStop = myGpioFlashStop;
	This->Destroy = myGpioDestroy;
	This->IsActive = myGpioIsActive;
	This->setActiveTime = myGpioSetActiveTime;
	This->inputHandle = myGpioInputHandle;
	This->addInputThread = myGpioAddInputThread;
	myGpioInit(This);
	createThread(myGpioOutputThread,This);
	return This;
}

void gpioInit(void)
{
	DPRINT("gpio init\n");
	gpio = myGpioPrivCreate(gpio_normal_tbl,
			sizeof(gpio_normal_tbl) / sizeof(MyGpioTable));

}

