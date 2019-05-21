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

static TSqliteData dbase = {
	.file_name = DATABSE_PATH"database.db",
	.sql = NULL,
	.checkFunc = sqlCheck,
};

static int sqlCheck(TSqlite *sql)
{
    int ret;
	char *string = "CREATE TABLE  WifiList(\
ID INTEGER PRIMARY KEY,\
name char(32),\
password char(64),\
enable INTEGER,\
security INTEGER,\
	   	)";
    if (sql == NULL)
        goto sqlCheck_fail;

    ret = LocalQueryOpen(sql,"select ID from WifiList limit 1");
    sql->Close(sql);
	if (ret == 1) {
		backData(sql->file_name);
		return TRUE;
	}

sqlCheck_fail:
    saveLog("sql locoal err\n");
	if (recoverData(dbase.file_name) == 0) {
		saveLog("creat new db\n");
		LocalQueryExec(dbase.sql,string);
	} else {
		dbase.sql->Destroy(dbase.sql);
		dbase.sql = CreateLocalQuery(dbase.sql->file_name);
        sync();
	}
	return FALSE;
}

int sqlGetDeviceCnt(void)
{
	LocalQueryOpen(dbase.sql,"select * from WifiList ");
	return dbase.sql->RecordCount(dbase.sql);
}

void sqlGetDevice(char *id,
		int *dev_type,
		uint16_t *addr,
		uint16_t *channel,
		char *product_key,int index)
{
	int i;
	LocalQueryOpen(dbase.sql,"select * from WifiList ");
	for (i=0; i<index; i++) {
		dbase.sql->Next(dbase.sql);
	}
	if (id)
		LocalQueryOfChar(dbase.sql,"ID",id,32);
	if (dev_type)
		*dev_type = LocalQueryOfInt(dbase.sql,"DevType");
	if (addr)
		*addr = LocalQueryOfInt(dbase.sql,"Addr");
	if (channel)
		*channel = LocalQueryOfInt(dbase.sql,"Channel");
	if (product_key)
		LocalQueryOfChar(dbase.sql,"ProductKey",product_key,32);
}
void sqlGetDeviceEnd(void)
{
	dbase.sql->Close(dbase.sql);
}

void sqlInsertWifi(
		char *name,
		char *password,
		uint32_t enable,
		uint32_t security)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "INSERT INTO WifiList(name,password,enable,security) VALUES('%s','%s','%d','%d')",
			name, password,enable,security);
	saveLog("%s\n",buf);
	LocalQueryExec(dbase.sql,buf);
	dbase.checkFunc(dbase.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlDeleteDevice(char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "Delete From WifiList Where ID=\"%s\"", id);
	DPRINT("%s\n",buf);
	saveLog("%s\n",buf);
	LocalQueryExec(dbase.sql,buf);
	dbase.checkFunc(dbase.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlClearDevice(void)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "Delete From WifiList");
	DPRINT("%s\n",buf);
	saveLog("%s\n",buf);
	LocalQueryExec(dbase.sql,buf);
	dbase.checkFunc(dbase.sql);
	pthread_mutex_unlock(&mutex);
}
int sqlGetDeviceId(uint16_t addr,char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select ID From WifiList Where Addr=\"%d\"", addr);
	LocalQueryOpen(dbase.sql,buf);
	int ret = dbase.sql->RecordCount(dbase.sql);
	if (ret)
		LocalQueryOfChar(dbase.sql,"ID",id,32);
	// DPRINT("ret:%d,id:%s\n", ret,id);
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
	return ret;
}

void sqlSetEleQuantity(int value,char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "UPDATE WifiList SET EleQuantity ='%d' Where id = \"%s\"",
			value,id);
	LocalQueryExec(dbase.sql,buf);
	dbase.checkFunc(dbase.sql);
	pthread_mutex_unlock(&mutex);
}
int sqlGetEleQuantity(char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select EleQuantity From WifiList Where ID=\"%s\"", id);
	LocalQueryOpen(dbase.sql,buf);
	int ret = LocalQueryOfInt(dbase.sql,"EleQuantity");
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
	return ret;
}
void sqlSetMideaAddr(char *id,void *data,int size)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "UPDATE WifiList SET MideaSlaveData=?,Channel=4 \
		   	Where id = \"%s\"", id);
	printf("%s\n",buf);
	dbase.sql->prepare(dbase.sql,buf);
	dbase.sql->reset(dbase.sql);
	dbase.sql->bind_blob(dbase.sql,data,size);
	dbase.sql->step(dbase.sql);
	dbase.sql->finalize(dbase.sql);
	dbase.sql->Close(dbase.sql);
	dbase.checkFunc(dbase.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlGetMideaAddr(char *id,void *data)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select MideaSlaveData From WifiList Where ID=\"%s\"", id);
	dbase.sql->prepare(dbase.sql,buf);
	dbase.sql->reset(dbase.sql);
	dbase.sql->step(dbase.sql);
	dbase.sql->getBlobData(dbase.sql,data);
	dbase.sql->finalize(dbase.sql);
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
}


void sqlInit(void)
{
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex, &mutexattr);

	LocalQueryLoad(&dbase);
	dbase.checkFunc(dbase.sql);
	if (!dbase.sql) {
		DPRINT("sql err\n");
		saveLog("sql err\n");
	}
}
