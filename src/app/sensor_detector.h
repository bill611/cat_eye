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
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 更新UI电量
		 *
		 * @param 
		 */
		/* ---------------------------------------------------------------------------*/
		void (*uiUpadteElePower)(int power);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 更新UI充电状态
		 *
		 * @param 
		 */
		/* ---------------------------------------------------------------------------*/
		void (*uiUpadteEleState)(int state);
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief 电量为0时，提示电量过低，请充电的提示
		 *
		 * @param 
		 */
		/* ---------------------------------------------------------------------------*/
		void (*uiLowPowerToPowerOff)(void);
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
