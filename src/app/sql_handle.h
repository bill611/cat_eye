/*
 * =============================================================================
 *
 *       Filename:  sqlHandle.h
 *
 *    Description:  数据库操作接口
 *
 *        Version:  1.0
 *        Created:  2018-05-21 22:48:57 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _SQL_HANDLE_H
#define _SQL_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>
	extern int sqlGetDeviceCnt(void);
	extern void sqlGetDevice(char *id,
			int *dev_type,
			uint16_t *addr,
			uint16_t *channel,
			char *product_key,int index);
	extern void sqlGetDeviceEnd(void);
	extern void sqlInsertWifi(
		char *name,
		char *password,
		uint32_t enable,
		uint32_t security);
	extern void sqlDeleteDevice(char *id);
	extern int sqlGetDeviceId(uint16_t addr,char *id);
	extern void sqlInit(void);
	extern void sqlClearDevice(void);
	extern void sqlSetEleQuantity(int value,char *id);
	extern int sqlGetEleQuantity(char *id);
	extern void sqlSetMideaAddr(char *id,void *data,int size);
	extern void sqlGetMideaAddr(char *id,void *data);
	extern void sqlSetAirConditionPara(char *id,int temp,int mode,int speed);
	extern void sqlGetAirConditionPara(char *id,int *temp,int *mode,int *speed);
	extern void sqlSetInfraredArmStatus(char *id,int arm_status);
	extern void sqlGetInfraredArmStatus(char *id,int *arm_status);
	extern void sqlSetDoorContactArmStatus(char *id,int arm_status);
	extern void sqlGetDoorContactArmStatus(char *id,int *arm_status);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
