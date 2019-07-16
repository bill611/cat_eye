/*
 * =============================================================================
 *
 *       Filename:  proximity_detector.c
 *
 *    Description:  接近传感器应用
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
#include "hal_sensor.h"
#include "thread_helper.h"
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
static void* proximityDetectorThread(void *arg)
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

void proximityDetectorInit(void)
{
	createThread(proximityDetectorThread,NULL);
}
