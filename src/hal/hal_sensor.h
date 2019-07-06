/*
 * =============================================================================
 *
 *       Filename:  hal_sensor.h
 *
 *    Description:  室内接近传感器
 *
 *        Version:  1.0
 *        Created:  2019-07-06 15:38:53 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _HAL_SENSOR_H
#define _HAL_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	enum {
		HAL_SENSER_ERR = -1,
		HAL_SENSOR_INACTIVE,
		HAL_SENSOR_ACTIVE,
	};
	void halSensorInit(void);
	int halSensorGetState(void);
	void halSensorUninit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
