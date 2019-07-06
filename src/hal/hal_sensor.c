/*
 * =============================================================================
 *
 *       Filename:  hal_sensor.c
 *
 *    Description:  室内接近传感器
 *
 *        Version:  1.0
 *        Created:  2019-07-06 15:30:59
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
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include "hal_sensor.h"

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
static int sensor_fd = -1;

void halSensorInit(void)
{
	sensor_fd = open ("/dev/input/event2", O_RDONLY);
	if(sensor_fd <= 0) {
		printf("open /dev/input/event2 device error!\n");
		return ;
	}
}

int halSensorGetState(void)
{
	if (sensor_fd < 0) {
		return HAL_SENSER_ERR;
	}
	struct input_event s_event;
	if(read (sensor_fd, &s_event, sizeof(struct input_event)) == sizeof(struct input_event) <= 0)
		return HAL_SENSER_ERR;
	if(s_event.type != EV_ABS)
		return HAL_SENSER_ERR;
	if(s_event.code != ABS_DISTANCE)
		return HAL_SENSER_ERR;

	if(s_event.value == 0)
		return HAL_SENSOR_INACTIVE;
	else if(s_event.value == 1)
		return HAL_SENSOR_ACTIVE;

	return HAL_SENSER_ERR;
}
void halSensorUninit(void)
{
	if (sensor_fd > 0)
		close(sensor_fd);
}
