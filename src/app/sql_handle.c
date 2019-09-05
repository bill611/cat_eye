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
struct DBTables {
	char *name;
	char **title;
};

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

static char *table_face = "CREATE TABLE IF NOT EXISTS FaceInfo( \
ID INTEGER PRIMARY KEY,\
userId char(32) UNIQUE,\
nickName char(128),\
fileURL char(256),\
feature BLOB\
)";

// 视频和抓图都用此接口
static char *table_record_cap = "CREATE TABLE IF NOT EXISTS RecordCapture( \
ID INTEGER PRIMARY KEY,\
date_time char(64),\
picture_id INTEGER\
)";

static char *table_record_alarm = "CREATE TABLE IF NOT EXISTS RecordAlarm( \
ID INTEGER PRIMARY KEY,\
date_time char(64),\
type INTEGER, \
hasPeople INTEGER, \
age INTEGER, \
sex INTEGER, \
picture_id INTEGER\
)";

static char *table_record_talk = "CREATE TABLE IF NOT EXISTS RecordTalk( \
ID INTEGER PRIMARY KEY,\
date_time char(64),\
people char(64),\
callDir INTEGER, \
answered INTEGER,\
talkTime INTEGER,\
picture_id INTEGER\
)";
static char *table_record_face = "CREATE TABLE IF NOT EXISTS RecordFace( \
ID INTEGER PRIMARY KEY,\
date_time char(64),\
faceId char(64),\
nickName char(128),\
picture_id INTEGER\
)";

static char *table_url_pic = "CREATE TABLE IF NOT EXISTS PicUrl( \
ID INTEGER PRIMARY KEY,\
picture_id INTEGER, \
url char(128)\
)";

static char *table_url_rec = "CREATE TABLE IF NOT EXISTS RecordUrl( \
ID INTEGER PRIMARY KEY,\
record_id INTEGER, \
url char(128)\
)";

static struct DBTables db_tables[] = {
	{"UserInfo",		&table_user},
	{"FaceInfo",		&table_face},
	{"RecordCapture",	&table_record_cap},
	{"RecordAlarm",		&table_record_alarm},
	{"RecordTalk",		&table_record_talk},
	{"RecordFace",		&table_record_face},
	{"PicUrl",			&table_url_pic},
	{"RecordUrl",		&table_url_rec},
	{NULL,NULL}
};

static TSqliteData dbase = {
	.file_name = DATABSE_PATH"database.db",
	.sql = NULL,
	.checkFunc = sqlCheck,
};

static int sqlCheck(TSqlite *sql)
{
    if (sql == NULL)
        goto sqlCheck_fail;
	int ret = 1;
	int i;
	for (i=0; db_tables[i].name != NULL; i++) {
		char buf[64];
		sprintf(buf,"select ID from %s limit 1",db_tables[i].name);
		int count = LocalQueryOpen(sql,buf);
		sql->Close(sql);
		if (count == 0) {
			ret = 0;
		}
	}
	if ( ret == 1 ) {
		backData((char *)sql->file_name);
		return TRUE;
	}

sqlCheck_fail:
    DPRINT("sql locoal err\n");
	if (recoverData(dbase.file_name) == 0) {
		for (i=0; db_tables[i].name != NULL; i++) {
			DPRINT("creat new db:%s\n",db_tables[i].name);
			LocalQueryExec(dbase.sql,*db_tables[i].title);
		}
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

int sqlGetUserInfoUseScopeStart(int scope)
{
	char buf[128];
	sprintf(buf,"select userId,nickName from UserInfo where scope = %d",scope );
	pthread_mutex_lock(&mutex);
	LocalQueryOpen(dbase.sql,buf);
	return dbase.sql->RecordCount(dbase.sql);
}

void sqlGetUserInfosUseScope(
		char *user_id,
		char *nick_name)
{
	if (user_id)
		LocalQueryOfChar(dbase.sql,"userId",user_id,32);
	if (nick_name)
		LocalQueryOfChar(dbase.sql,"nickName",nick_name,128);
	dbase.sql->Next(dbase.sql);
}

void sqlGetUserInfosUseScopeIndex(
		char *user_id,
		int scope,
		int index)
{
	char buf[128];
	sprintf(buf,"select userId,nickName from UserInfo where scope = %d",scope );
	pthread_mutex_lock(&mutex);
	LocalQueryOpen(dbase.sql,buf);
	while (index) {
		dbase.sql->Next(dbase.sql);
		index--;
	}
	if (user_id)
		LocalQueryOfChar(dbase.sql,"userId",user_id,32);
	pthread_mutex_unlock(&mutex);
}

int sqlGetUserInfoUseType(
		int type,
		char *user_id,
		char *login_token,
		char *nick_name,
		int *scope)
{
	char buf[128];
	sprintf(buf,"select * from UserInfo where type = %d",type );
	pthread_mutex_lock(&mutex);
	LocalQueryOpen(dbase.sql,buf);
	int ret = dbase.sql->RecordCount(dbase.sql);
	if (ret) {
		*scope = LocalQueryOfInt(dbase.sql,"scope");
		if (user_id)
			LocalQueryOfChar(dbase.sql,"userId",user_id,32);
		if (login_token)
			LocalQueryOfChar(dbase.sql,"loginToken",login_token,256);
		if (nick_name)
			LocalQueryOfChar(dbase.sql,"nickName",nick_name,128);
	}
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
	return ret;
}
int sqlGetUserInfoUseUserId(
		char *user_id,
		char *nick_name,
		int *scope)
{
	char buf[128];
	sprintf(buf,"select * from UserInfo where userId = \"%s\"",user_id );
	pthread_mutex_lock(&mutex);
	LocalQueryOpen(dbase.sql,buf);
	int ret = dbase.sql->RecordCount(dbase.sql);
	if (ret) {
		*scope = LocalQueryOfInt(dbase.sql,"scope");
		if (nick_name)
			LocalQueryOfChar(dbase.sql,"nickName",nick_name,128);
	}
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
	return ret;
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

void sqlDeleteDeviceUseTypeNoBack(int type)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "Delete From UserInfo Where type=%d", type);
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
void sqlInsertFace(char *user_id,
		char *nick_name,
		char *url,
		void *feature,
		int size)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select nickName From FaceInfo Where userId=\"%s\"", user_id);
	LocalQueryOpen(dbase.sql,buf);
	int ret = dbase.sql->RecordCount(dbase.sql);
	dbase.sql->Close(dbase.sql);
	if (ret == 0) {
		sprintf(buf, "INSERT INTO FaceInfo(userId,nickName,fileURL,feature) VALUES(?,?,?,?)");
	} else {
		sprintf(buf, "UPDATE FaceInfo SET userId=?,nickName=?,fileURL=?,feature=? WHERE userId = \"%s\"",user_id);
	}
	dbase.sql->prepare(dbase.sql,buf);
	dbase.sql->bind_reset(dbase.sql);
	dbase.sql->bind_text(dbase.sql,user_id);
	dbase.sql->bind_text(dbase.sql,nick_name);
	dbase.sql->bind_text(dbase.sql,url);
	dbase.sql->bind_blob(dbase.sql,feature,size);
	dbase.sql->step(dbase.sql);
	dbase.sql->finalize(dbase.sql);
	dbase.sql->Close(dbase.sql);
	dbase.checkFunc(dbase.sql);
	pthread_mutex_unlock(&mutex);
}

int sqlGetFaceCount(void)
{
	pthread_mutex_lock(&mutex);
	LocalQueryOpen(dbase.sql,"select userId From FaceInfo");
	int ret = dbase.sql->RecordCount(dbase.sql);
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
	return ret;

}
void sqlGetFaceStart(void)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select userId,nickName,fileURL,feature From FaceInfo ");
	dbase.sql->prepare(dbase.sql,buf);

}
int sqlGetFace(char *user_id,char *nick_name,char *url,void *feature)
{
	int ret = dbase.sql->step(dbase.sql);
	if (ret != SQLITE_ROW)
		return 0;
	dbase.sql->get_reset(dbase.sql);
	dbase.sql->getBindText(dbase.sql,user_id);
	dbase.sql->getBindText(dbase.sql,nick_name);
	dbase.sql->getBindText(dbase.sql,url);
	dbase.sql->getBlobData(dbase.sql,feature);
	return 1;
}
void sqlGetFaceEnd(void)
{
	dbase.sql->finalize(dbase.sql);
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
}
void sqlDeleteFace(char *id)
{
	char buf[256];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "Delete From FaceInfo Where userId=\"%s\"", id);
	DPRINT("%s\n",buf);
	LocalQueryExec(dbase.sql,buf);
	dbase.checkFunc(dbase.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlInsertPicUrlNoBack(
		uint64_t picture_id,
		char *url)
{
	pthread_mutex_lock(&mutex);
    char buf[256];
	if (url) {
		sprintf(buf, "INSERT INTO PicUrl(picture_id,url) VALUES('%lld','%s')",
				picture_id,url);
    } else {
		sprintf(buf, "INSERT INTO PicUrl(picture_id,url) VALUES('%lld','0')",
				picture_id);
    }
    printf("%s\n", buf);
    LocalQueryExec(dbase.sql,buf);
	pthread_mutex_unlock(&mutex);
}
void sqlInsertRecordUrlNoBack(
		uint64_t picture_id,
		char *url)
{
	pthread_mutex_lock(&mutex);
    char buf[256];
	if (url) {
		sprintf(buf, "INSERT INTO RecordUrl(record_id,url) VALUES('%lld','%s')",
				picture_id,url);
    } else {
		sprintf(buf, "INSERT INTO RecordUrl(record_id,url) VALUES('%lld','0')",
				picture_id);
    }
    printf("%s\n", buf);
    LocalQueryExec(dbase.sql,buf);
	pthread_mutex_unlock(&mutex);
}

void sqlInsertRecordAlarm(
		char *date_time,
		int type,
		int has_people,
		int age,
		int sex,
		uint64_t picture_id)
{
	char buf[256];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "INSERT INTO RecordAlarm(date_time,type,hasPeople,age,sex,picture_id) VALUES('%s','%d','%d','%d','%d','%lld')",
			date_time, type,has_people,age,sex,picture_id);
	printf("%s\n", buf);
	LocalQueryExec(dbase.sql,buf);
	dbase.checkFunc(dbase.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlInsertRecordCapNoBack(
		char *date_time,
		uint64_t picture_id)
{
	char buf[256];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "INSERT INTO RecordCapture(date_time,picture_id) VALUES('%s','%lld')",
			date_time, picture_id);
	printf("%s\n", buf);
	LocalQueryExec(dbase.sql,buf);
	pthread_mutex_unlock(&mutex);
}

void sqlInsertRecordTalkNoBack(
		char *date_time,
		char *people,
		int call_dir,
		int answered,
		int talk_time,
		uint64_t picture_id)
{
	char buf[256];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "INSERT INTO RecordTalk(date_time,people,callDir,answered,talkTime,picture_id)\
            VALUES('%s','%s','%d','%d','%d','%lld')",
			date_time,people, call_dir,answered,talk_time,picture_id);
	printf("%s\n", buf);
	LocalQueryExec(dbase.sql,buf);
	pthread_mutex_unlock(&mutex);
}

void sqlInsertRecordFaceNoBack(
		char *date_time,
		char *face_id,
		char *nick_name,
		uint64_t picture_id)
{
	char buf[256];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "INSERT INTO RecordFace(date_time,faceId,nickName,picture_id) VALUES('%s','%s','%s','%lld')",
			date_time,face_id,nick_name,picture_id);
	printf("%s\n", buf);
	LocalQueryExec(dbase.sql,buf);
	pthread_mutex_unlock(&mutex);
}

int sqlGetCapInfo(
		uint64_t picture_id,
		char *date_time)
{
	char buf[128];
	sprintf(buf,"select * from RecordCapture where picture_id = %lld",picture_id );
	pthread_mutex_lock(&mutex);
	LocalQueryOpen(dbase.sql,buf);
	int ret = dbase.sql->RecordCount(dbase.sql);
	printf("buf:%s,ret:%d\n", buf,ret);
	if (ret) {
		if (date_time)
			LocalQueryOfChar(dbase.sql,"date_time",date_time,64);
	}
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
	return ret;
}
void sqlCheckBack(void)
{
	dbase.checkFunc(dbase.sql);
}

int sqlGetPicInfoStart(uint64_t picture_id)
{
	char buf[128];
	sprintf(buf,"select url from PicUrl where picture_id = %lld",picture_id );
	pthread_mutex_lock(&mutex);
	LocalQueryOpen(dbase.sql,buf);
	return dbase.sql->RecordCount(dbase.sql);
}

void sqlGetPicInfos(char *url)
{
	if (url)
		LocalQueryOfChar(dbase.sql,"url",url,128);
	dbase.sql->Next(dbase.sql);
}
void sqlGetPicInfoEnd(void)
{
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
}

int sqlGetRecordInfoStart(uint64_t picture_id)
{
	char buf[128];
	sprintf(buf,"select url from RecordUrl where record_id = %lld",picture_id );
	pthread_mutex_lock(&mutex);
	LocalQueryOpen(dbase.sql,buf);
	return dbase.sql->RecordCount(dbase.sql);
}

void sqlGetRecordInfos(char *url)
{
	if (url)
		LocalQueryOfChar(dbase.sql,"url",url,128);
	dbase.sql->Next(dbase.sql);
}
void sqlGetRecordInfoEnd(void)
{
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
}

int sqlGetAlarmInfoUseDateType(
		char *date_time,
		int type,
		int *has_people)
{
	char buf[128];
	sprintf(buf,"select * from RecordAlarm where date_time = \"%s\" and type = %d",
			date_time,type );
	pthread_mutex_lock(&mutex);
	LocalQueryOpen(dbase.sql,buf);
	int ret = dbase.sql->RecordCount(dbase.sql);
	printf("buf:%s,ret:%d\n", buf,ret);
	if (ret) {
		if (date_time)
			LocalQueryOfChar(dbase.sql,"date_time",date_time,64);
		*has_people = LocalQueryOfInt(dbase.sql,"hasPeople");
	}
	dbase.sql->Close(dbase.sql);
	pthread_mutex_unlock(&mutex);
	return ret;
}

static void* threadSqlUpload(void *arg)
{
    
}

void sqlInit(void)
{
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

	LocalQueryLoad(&dbase);
	dbase.checkFunc(dbase.sql);
	if (!dbase.sql) {
		DPRINT("sql err\n");
	}
}
