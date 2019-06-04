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
#include <unistd.h>
#include <string.h>
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
static char *table_user = "CREATE TABLE IF NOT EXISTS UserInfo( \
ID INTEGER PRIMARY KEY,\
userId char(32) UNIQUE,\
type INTEGER, \
loginToken char(128),\
nickName char(128),\
scope INTEGER\
)";

static TSqliteData dbase = {
	.file_name = DATABSE_PATH"database.db",
	.sql = NULL,
	.checkFunc = sqlCheck,
};

static int sqlCheck(TSqlite *sql)
{
    int ret;
    if (sql == NULL)
        goto sqlCheck_fail;

    ret = LocalQueryOpen(sql,"select ID from UserInfo limit 1");
    sql->Close(sql);
	if (ret == 1) {
		backData((char *)sql->file_name);
		return TRUE;
	}

sqlCheck_fail:
    DPRINT("sql locoal err\n");
	if (recoverData(dbase.file_name) == 0) {
		DPRINT("creat new db:%s\n",table_user);
		LocalQueryExec(dbase.sql,table_user);
	} else {
		dbase.sql->Destroy(dbase.sql);
		dbase.sql = CreateLocalQuery(dbase.sql->file_name);
	}
    sync();
	return FALSE;
}

int sqlGetUserInfoStart(int type)
{
	char buf[128];
	sprintf(buf,"select * from UserInfo where type = %d",type );
	pthread_mutex_lock(&mutex);
	LocalQueryOpen(dbase.sql,buf);
	return dbase.sql->RecordCount(dbase.sql);
}

void sqlGetUserInfos(
		char *user_id,
		char *nick_name,
		int *scope)
{
	*scope = LocalQueryOfInt(dbase.sql,"scope");
	if (user_id)
		LocalQueryOfChar(dbase.sql,"userId",user_id,32);
	if (nick_name)
		LocalQueryOfChar(dbase.sql,"nickName",nick_name,128);
	dbase.sql->Next(dbase.sql);
}

void sqlGetUserInfo(
		int type,
		char *user_id,
		char *login_token,
		char *nick_name,
		int *scope)
{
	char buf[128];
	sprintf(buf,"select * from UserInfo where type = %d",type );
	LocalQueryOpen(dbase.sql,buf);

	*scope = LocalQueryOfInt(dbase.sql,"scope");
	if (user_id)
		LocalQueryOfChar(dbase.sql,"userId",user_id,32);
	if (login_token)
		LocalQueryOfChar(dbase.sql,"loginToken",login_token,256);
	if (nick_name)
		LocalQueryOfChar(dbase.sql,"nickName",nick_name,128);
	dbase.sql->Close(dbase.sql);
}
void sqlGetUserInfoEnd(void)
{
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlInsertUserInfoNoBack(
		char *user_id,
		char *login_token,
		char *nick_name,
		int type,
		int scope)
{
	char buf[512];
	pthread_mutex_lock(&mutex);
	if (login_token)
		sprintf(buf, "INSERT INTO UserInfo(userId,loginToken,nickName,type,scope) VALUES('%s','%s','%s','%d','%d')",
				user_id, login_token,nick_name,type,scope);
	else
		sprintf(buf, "INSERT INTO UserInfo(userId,nickName,type,scope) VALUES('%s','%s','%d','%d')",
				user_id, nick_name,type,scope);
	printf("%s\n", buf);
	LocalQueryExec(dbase.sql,buf);
	pthread_mutex_unlock(&mutex);
}
void sqlInsertUserInfo(
		char *user_id,
		char *login_token,
		char *nick_name,
		int type,
		int scope)
{
	sqlInsertUserInfoNoBack(user_id,login_token,nick_name,type,scope);
	dbase.checkFunc(dbase.sql);
}

void sqlDeleteDevice(char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "Delete From UserInfo Where userId=\"%s\"", id);
	DPRINT("%s\n",buf);
	LocalQueryExec(dbase.sql,buf);
	dbase.checkFunc(dbase.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlClearDevice(void)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "Delete From UserInfo");
	DPRINT("%s\n",buf);
	LocalQueryExec(dbase.sql,buf);
	dbase.checkFunc(dbase.sql);
	pthread_mutex_unlock(&mutex);
}
int sqlGetDeviceId(uint16_t addr,char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select userId From UserInfo Where Addr=\"%d\"", addr);
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

void sqlCheckBack(void)
{
	dbase.checkFunc(dbase.sql);
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
	}
}
