/*
 * =============================================================================
 *
 *       Filename:  sensor_detector.h
 *
 *    Description:  传感器检测
 *
 *        Version:  virsion
 *        Created:  2019-07-06 15:48:08 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _SENSOR_DETECTOR_H
#define _SENSOR_DETECTOR_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	typedef struct _SensorsInterface {
		void (*uiUpadteElePower)(int power);
		void (*uiUpadteEleState)(int state);
	}SensorsInterface;

	typedef struct _Sensors {
		SensorsInterface *interface;
		int (*getElePower)(void);
		int (*getEleState)(void);
	}Sensors;
	extern Sensors *sensor;
	void sensorDetectorInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
