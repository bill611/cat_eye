/*
 * =============================================================================
 *
 *       Filename:  sensor_detector.c
 *
 *    Description:  传感器检测
 *
 *        Version:  1.0
 *        Created:  2019-07-06 15:47:47
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
#include <unistd.h>
#include <stdlib.h>
#include "hal_sensor.h"
#include "hal_battery.h"
#include "thread_helper.h"
#include "sensor_detector.h"
#include "externfunc.h"
#include "sql_handle.h"
#include "protocol.h"
#include "my_video.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
Sensors *sensor;

static void* theadProximity(void *arg)
{
	int state = HAL_SENSER_ERR;
	int state_old = HAL_SENSER_ERR;
	halSensorInit();	
	while (1) {
		state = halSensorGetState();
		if (state != state_old) {
			if (state == HAL_SENSOR_INACTIVE) {
				printf("out 50cm\n");
			} else {
				printf("in 50cm\n");
			}
		}
		state_old = state;
		usleep(200000);
	};
	return NULL;
}
static void* theadEle(void *arg)
{
	int power_old = 0;
	int power_state_old = 0;
	int report_low_power = 0; // 发送低电量报警
	while (1) {
		// 更新充电状态
		int power_state = halBatteryGetState();
		if (power_state_old != power_state) {
			power_state_old = power_state;
			if (sensor->interface->uiUpadteEleState)
				sensor->interface->uiUpadteEleState(power_state);
		}
		// 更新电量
		int power = halBatteryGetEle();
		if (power_old == 0 || power_old != power) {
			power_old = power;
			if (sensor->interface->uiUpadteElePower)
				sensor->interface->uiUpadteElePower(power);
			if (power < 20) {
				if ( 	power_state == BATTERY_NORMAL
					&& 	report_low_power == 0) {
					report_low_power = 1;
					ReportAlarmData alarm_data;
					alarm_data.type = ALARM_TYPE_LOWPOWER;
					getDate(alarm_data.date,sizeof(alarm_data.date));
					sqlInsertRecordAlarm(alarm_data.date,alarm_data.type,0,0,0,0);
					protocol_hardcloud->reportAlarm(&alarm_data);
				}
			} else {
				report_low_power = 0;
			}
		}
		sleep(1);
	}
	return NULL;
}

static int getElePower(void)
{
	return halBatteryGetEle();
}
static int getEleState(void)
{
	return halBatteryGetState();
}

void sensorDetectorInit(void)
{
	sensor = (Sensors *) calloc(1,sizeof(Sensors ));
	sensor->interface = (SensorsInterface *) calloc(1,sizeof(SensorsInterface));
	sensor->getElePower = getElePower;
	sensor->getEleState = getEleState;
	createThread(theadProximity,NULL);
	createThread(theadEle,NULL);
}
