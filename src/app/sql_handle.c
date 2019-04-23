/*
 * =============================================================================
 *
 *       Filename:  sqlHandle.c
 *
 *    Description:  数据库存储操作接口
 *
 *        Version:  1.0
 *        Created:  2018-05-21 22:48:19
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
#include <stdint.h>
#include <pthread.h>
#include "externfunc.h"
#include "sqlite3.h"
#include "sqlite.h"
#include "sql_handle.h"
#include "debug.h"
#include "config.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int sqlCheck(TSqlite *sql);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static pthread_mutex_t mutex;

TSqliteData sql_local = {
	.file_name = DATABSE_PATH"device.db",
	.sql = NULL,
	.checkFunc = sqlCheck,
};

static int sqlCheck(TSqlite *sql)
{
    int ret;
	char *string = "CREATE TABLE  DeviceList(\
ID char(32) PRIMARY KEY,\
ProductKey char(32),\
DevType INTEGER,\
Addr INTEGER,\
Channel INTEGER,\
EleQuantity INTEGER,\
AirConditionSpeed INTEGER,\
AirConditionMode INTEGER,\
AirConditionTemp INTEGER,\
MideaSlaveData BLOB,\
MotionCurtainArm INTEGER,\
InfraredArm INTEGER,\
DoorContactArm INTEGER\
	   	)";
    if (sql == NULL)
        goto sqlCheck_fail;

    ret = LocalQueryOpen(sql,"select ID from DeviceList limit 1");
    sql->Close(sql);
	if (ret == 1) {
		backData(sql->file_name);
		return TRUE;
	}

sqlCheck_fail:
    saveLog("sql locoal err\n");
	if (recoverData(sql_local.file_name) == 0) {
		saveLog("creat new db\n");
		LocalQueryExec(sql_local.sql,string);
	} else {
		sql_local.sql->Destroy(sql_local.sql);
		sql_local.sql = CreateLocalQuery(sql_local.sql->file_name);
	}
	return FALSE;
}

int sqlGetDeviceCnt(void)
{
	LocalQueryOpen(sql_local.sql,"select * from DeviceList ");
	return sql_local.sql->RecordCount(sql_local.sql);
}

void sqlGetDevice(char *id,
		int *dev_type,
		uint16_t *addr,
		uint16_t *channel,
		char *product_key,int index)
{
	int i;
	LocalQueryOpen(sql_local.sql,"select * from DeviceList ");
	for (i=0; i<index; i++) {
		sql_local.sql->Next(sql_local.sql);
	}
	if (id)
		LocalQueryOfChar(sql_local.sql,"ID",id,32);
	if (dev_type)
		*dev_type = LocalQueryOfInt(sql_local.sql,"DevType");
	if (addr)
		*addr = LocalQueryOfInt(sql_local.sql,"Addr");
	if (channel)
		*channel = LocalQueryOfInt(sql_local.sql,"Channel");
	if (product_key)
		LocalQueryOfChar(sql_local.sql,"ProductKey",product_key,32);
}
void sqlGetDeviceEnd(void)
{
	sql_local.sql->Close(sql_local.sql);
}

void sqlInsertDevice(char *id,
		int dev_type,
		uint16_t addr,
		uint16_t channel,
		char *product_key)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "INSERT INTO DeviceList(ID,DevType,Addr,Channel,ProductKey) VALUES('%s','%d','%d','%d','%s')",
			id, dev_type,addr,channel,product_key);
	saveLog("%s\n",buf);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}

void sqlDeleteDevice(char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "Delete From DeviceList Where ID=\"%s\"", id);
	DPRINT("%s\n",buf);
	saveLog("%s\n",buf);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}

void sqlClearDevice(void)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "Delete From DeviceList");
	DPRINT("%s\n",buf);
	saveLog("%s\n",buf);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}
int sqlGetDeviceId(uint16_t addr,char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select ID From DeviceList Where Addr=\"%d\"", addr);
	LocalQueryOpen(sql_local.sql,buf);
	int ret = sql_local.sql->RecordCount(sql_local.sql);
	if (ret)
		LocalQueryOfChar(sql_local.sql,"ID",id,32);
	// DPRINT("ret:%d,id:%s\n", ret,id);
	sql_local.sql->Close(sql_local.sql);
	pthread_mutex_unlock(&mutex);
	return ret;
}

void sqlSetEleQuantity(int value,char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "UPDATE DeviceList SET EleQuantity ='%d' Where id = \"%s\"",
			value,id);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}
int sqlGetEleQuantity(char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select EleQuantity From DeviceList Where ID=\"%s\"", id);
	LocalQueryOpen(sql_local.sql,buf);
	int ret = LocalQueryOfInt(sql_local.sql,"EleQuantity");
	sql_local.sql->Close(sql_local.sql);
	pthread_mutex_unlock(&mutex);
	return ret;
}
void sqlSetMideaAddr(char *id,void *data,int size)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "UPDATE DeviceList SET MideaSlaveData=?,Channel=4 \
		   	Where id = \"%s\"", id);
	printf("%s\n",buf);
	sql_local.sql->prepare(sql_local.sql,buf);
	sql_local.sql->reset(sql_local.sql);
	sql_local.sql->bind_blob(sql_local.sql,data,size);
	sql_local.sql->step(sql_local.sql);
	sql_local.sql->finalize(sql_local.sql);
	sql_local.sql->Close(sql_local.sql);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}

void sqlGetMideaAddr(char *id,void *data)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select MideaSlaveData From DeviceList Where ID=\"%s\"", id);
	sql_local.sql->prepare(sql_local.sql,buf);
	sql_local.sql->reset(sql_local.sql);
	sql_local.sql->step(sql_local.sql);
	sql_local.sql->getBlobData(sql_local.sql,data);
	sql_local.sql->finalize(sql_local.sql);
	sql_local.sql->Close(sql_local.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlSetAirConditionPara(char *id,int temp,int mode,int speed)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "UPDATE DeviceList SET AirConditionSpeed ='%d',\
		   	AirConditionMode ='%d',AirConditionTemp ='%d' Where id = \"%s\"",
			speed,mode,temp,id);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}
void sqlGetAirConditionPara(char *id,int *temp,int *mode,int *speed)
{
	pthread_mutex_lock(&mutex);
	char buf[128];
	sprintf(buf, "select AirConditionTemp,AirConditionMode,AirConditionSpeed From DeviceList Where ID=\"%s\"", id);
	LocalQueryOpen(sql_local.sql,buf);
	if (temp)
		*temp = LocalQueryOfInt(sql_local.sql,"AirConditionTemp");
	if (mode)
		*mode = LocalQueryOfInt(sql_local.sql,"AirConditionMode");
	if (speed)
		*speed = LocalQueryOfInt(sql_local.sql,"AirConditionSpeed");
	sql_local.sql->Close(sql_local.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlSetInfraredArmStatus(char *id,int arm_status)
{
	pthread_mutex_lock(&mutex);
	char buf[128];
	sprintf(buf, "UPDATE DeviceList SET InfraredArm ='%d',\
		   	 Where id = \"%s\"",
			arm_status,id);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}
void sqlGetInfraredArmStatus(char *id,int *arm_status)
{
	pthread_mutex_lock(&mutex);
	char buf[128];
	sprintf(buf, "select InfraredArm From DeviceList Where ID=\"%s\"", id);
	LocalQueryOpen(sql_local.sql,buf);
	if (arm_status)
		*arm_status = LocalQueryOfInt(sql_local.sql,"InfraredArm");
	sql_local.sql->Close(sql_local.sql);
	pthread_mutex_unlock(&mutex);
}
void sqlSetDoorContactArmStatus(char *id,int arm_status)
{
	pthread_mutex_lock(&mutex);
	char buf[128];
	sprintf(buf, "UPDATE DeviceList SET DoorContactArm ='%d',\
		   	 Where id = \"%s\"",
			arm_status,id);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}
void sqlGetDoorContactArmStatus(char *id,int *arm_status)
{
	pthread_mutex_lock(&mutex);
	char buf[128];
	sprintf(buf, "select DoorContactArm From DeviceList Where ID=\"%s\"", id);
	LocalQueryOpen(sql_local.sql,buf);
	if (arm_status)
		*arm_status = LocalQueryOfInt(sql_local.sql,"DoorContactArm");
	sql_local.sql->Close(sql_local.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlInit(void)
{
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex, &mutexattr);

	LocalQueryLoad(&sql_local);
	sql_local.checkFunc(sql_local.sql);
	if (!sql_local.sql) {
		DPRINT("sql err\n");
		saveLog("sql err\n");
	}
}
