/*
 * =============================================================================
 *
 *       Filename:  sqlite.c
 *
 *    Description:  数据库接口
 *
 *        Version:  virsion
 *        Created:  2018-05-22 10:39:45
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
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "sqlite3.h"
#include "sqlite.h"
#include "externfunc.h"
#include "debug.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
#define SQL_LOCK()   pthread_mutex_lock(&Query->sql_lock->mutex)
#define SQL_UNLOCK() pthread_mutex_unlock(&Query->sql_lock->mutex)
struct Sqlite_mutex {
	pthread_mutex_t mutex;
};

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/

//----------------------------------------------------------------------------
char *strupper(char *pdst,const char *pstr,int Size);

struct SqlitePrivate
{
	BOOL Active;
	sqlite3 *db;
    sqlite3_stmt *ppStmt;
	int bind_num;
	char **pData;
	char *SqlText;
	char *pErrMsg;
	int FieldCount; 
	int RecordCount;
	int RecNo;
//	int LastRowID;
};
//----------------------------------------------------------------------------
static char * strncpytrim(char *pDest,const char *pSrc,int Size)
{
	int i = Size;
	char *pTmp = pDest;
	while(*pSrc && --i) {
		*pDest++ = *pSrc++;
	}
	//去除尾导空格
	while(pTmp!=pDest) {
		if(*(--pDest)!=32) {
			pDest++;
			break;
		}
	}
	*pDest = 0;
	return pTmp;
}
//----------------------------------------------------------------------------
static char * TField_AsChar(PSQLiteField This,char *Buf,int Size)
{
	// 
    if(!This->Private->Active)
    {
	    Buf[0]=0;
        return Buf;
    }
    if(This->Private->RecNo==0)
    {
	    Buf[0]=0;
        return Buf;
	}
	if(This->Private->pData[This->Private->FieldCount*(This->Private->RecNo)+
		This->offset]==NULL) {
	    Buf[0]=0;
        return Buf;
	}
	strncpytrim(Buf,This->Private->pData[This->Private->FieldCount*(This->Private->RecNo)+
		This->offset],Size);
	return Buf;
}
//----------------------------------------------------------------------------
static int TField_AsInt(PSQLiteField This)
{
    if(!This->Private->Active)
    {
        return 0;
    }
    if(This->Private->RecNo==0)
    {
        return 0;
	}
	if(This->Private->pData[This->Private->FieldCount*(This->Private->RecNo)+
		This->offset]==NULL)
		return 0;
	return atoi(This->Private->pData[This->Private->FieldCount*(This->Private->RecNo)+
		This->offset]);
}
//----------------------------------------------------------------------------
static double TField_AsFloat(PSQLiteField This)
{
    if(!This->Private->Active)
    {
        return 0;
    }
    if(This->Private->RecNo==0)
    {
        return 0;
	}
	if(This->Private->pData[This->Private->FieldCount*(This->Private->RecNo)+
		This->offset]==NULL)
		return 0;
	return atof(This->Private->pData[This->Private->FieldCount*(This->Private->RecNo)+
		This->offset]);
}
//----------------------------------------------------------------------------
static void SQLite_Destroy(struct _TSqlite *This)
{
	if(This->Private->Active) {
		This->Close(This);
	}
	sqlite3_close(This->Private->db);
	if(This->Private->SqlText)
		free(This->Private->SqlText);
	free(This->Private);
	free(This);
}
//----------------------------------------------------------------------------
//打开
static BOOL SQLite_Open(struct _TSqlite *This)
{
	int i;
	if(This->Private->Active) {
		This->Close(This);
	}
	if(sqlite3_get_table(This->Private->db,This->Private->SqlText,&This->Private->pData,
			&This->Private->RecordCount,&This->Private->FieldCount,
			&This->Private->pErrMsg)==SQLITE_OK) {
		This->Fields = (PSQLiteField)malloc(sizeof(TSQLiteField)*This->Private->FieldCount);
		if(This->Fields==NULL) {
			// DPRINT("Memory not enough\n");
			sqlite3_free_table(This->Private->pData);
			This->Private->pData = NULL;
			return TRUE;
		}
		memset(This->Fields,0,sizeof(TSQLiteField)*This->Private->FieldCount);
		This->Private->Active = TRUE;
		for(i=0;i<This->Private->FieldCount;i++) {
			strupper(This->Fields[i].Name,This->Private->pData[i],SQL_NAME_MAX);
			This->Fields[i].offset = i;
			This->Fields[i].Private = This->Private;
			This->Fields[i].AsChar = TField_AsChar;
			This->Fields[i].AsInt = TField_AsInt;
			This->Fields[i].AsFloat = TField_AsFloat;
		}
		if(This->Private->FieldCount)
			This->Private->RecNo = 1;
		else
			This->Private->RecNo = 0;
		return TRUE;
	}
	if(This->Private->pErrMsg) {
		sqlite3_free(This->Private->pErrMsg);
		This->Private->pErrMsg = NULL;
	}
	return FALSE;
}
//----------------------------------------------------------------------------
//执行
static BOOL SQLite_ExecSQL(struct _TSqlite *This)
{
	if(This->Private->Active)
		This->Close(This);
	if(sqlite3_exec( This->Private->db,This->Private->SqlText,NULL,0,
		&This->Private->pErrMsg)==SQLITE_OK) {
		return TRUE;
	}
	if(This->Private->pErrMsg) {
		sqlite3_free(This->Private->pErrMsg);
		This->Private->pErrMsg = NULL;
	}
	return FALSE;
}
//----------------------------------------------------------------------------
//关闭
static void SQLite_Close(struct _TSqlite *This)
{
	if(This->Private->Active) {
		sqlite3_free_table(This->Private->pData);
		This->Private->pData = NULL;
		free(This->Fields);
		This->Private->Active = FALSE;
	}
}
//----------------------------------------------------------------------------
// 返回记录数量
static int SQLite_RecordCount(struct _TSqlite *This)
{
	return This->Private->RecordCount;
}
//----------------------------------------------------------------------------
// 返回表是否打开
static BOOL SQLite_Active(struct _TSqlite *This)
{
	return This->Private->Active;
}
//----------------------------------------------------------------------------
// 返回字段数量
static int SQLite_FieldCount(struct _TSqlite *This)
{
	return This->Private->FieldCount;
}
//----------------------------------------------------------------------------
/* 取得最后插入影响的ID. */
static int SQLite_LastRowId(struct _TSqlite *This)
{
	return (int)sqlite3_last_insert_rowid(This->Private->db);
}
//----------------------------------------------------------------------------
//跳到记录号
static void SQLite_SetRecNo(struct _TSqlite *This,int RecNo)
{
	if(RecNo>0 && RecNo<=This->Private->RecordCount)
		This->Private->RecNo = RecNo;
}
//----------------------------------------------------------------------------
//首记录
static void SQLite_First(struct _TSqlite *This)
{
	if(This->Private->pData)
		This->Private->RecNo = 1;
	else
		This->Private->RecNo = 0;
}
//----------------------------------------------------------------------------
//末记录
static void SQLite_Last(struct _TSqlite *This)
{
	if(This->Private->pData)
		This->Private->RecNo = This->Private->RecordCount;
	else
		This->Private->RecNo = 0;
}
//----------------------------------------------------------------------------
//上一记录
static void SQLite_Prior(struct _TSqlite *This)
{
	if(This->Private->pData) {
		if(This->Private->RecNo>1)
			This->Private->RecNo--;
	}
}
//----------------------------------------------------------------------------
//下一记录
static void SQLite_Next(struct _TSqlite *This)
{
	if(This->Private->pData) {
		if(This->Private->RecNo<This->Private->RecordCount)
			This->Private->RecNo++;
	}
}
//----------------------------------------------------------------------------
//返回记录号
static int SQLite_RecNo(struct _TSqlite *This)
{
	return This->Private->RecNo;
}
//----------------------------------------------------------------------------
//返回字段
static PSQLiteField SQLite_FieldByName(struct _TSqlite *This,char *Name)
{
	int i;
	char FieldName[SQL_NAME_MAX];
	if(!This->Private->Active)
		return NULL;
	strupper(FieldName,Name,SQL_NAME_MAX);
	for(i=0;i<This->Private->FieldCount;i++) {
		if(strcmp(FieldName,This->Fields[i].Name)==0)
			return &This->Fields[i];
	}
	DPRINT("SQL '%s' [%s] is not found\n",This->Private->SqlText,Name);
	return NULL;
}
//----------------------------------------------------------------------------
//取SQL命令行
static int SQLite_GetSQLText(struct _TSqlite *This,char *pBuf,int Size)
{
	if(!This->Private->SqlText)
		return FALSE;
	strncpy(pBuf,This->Private->SqlText,Size);
	return TRUE;
}
//----------------------------------------------------------------------------
//设置SQL命令行
static void SQLite_SetSQLText(struct _TSqlite *This,char *SqlCmd)
{
	int TextLen = strlen(SqlCmd)+1;
	if(This->Private->Active)
		return;
	This->Private->SqlText = (char *)realloc(This->Private->SqlText,TextLen);
	strcpy(This->Private->SqlText,SqlCmd);
}
static void SQLite_prepare(struct _TSqlite *This,char *SqlCmd)
{
    sqlite3_prepare(This->Private->db,
            SqlCmd,strlen(SqlCmd),
            &This->Private->ppStmt,NULL);
}
static void SQLite_reset(struct _TSqlite *This)
{
    sqlite3_reset(This->Private->ppStmt);
	This->Private->bind_num = 1;
}
static void SQLite_get_blob_data(struct _TSqlite *This,void *data)
{
	const void *p;
	p = sqlite3_column_blob(This->Private->ppStmt,0);
    int size = sqlite3_column_bytes(This->Private->ppStmt,0);
	memcpy(data,p,size);
}
static void SQLite_finalize(struct _TSqlite *This)
{
    sqlite3_finalize(This->Private->ppStmt);
}
static void SQLite_step(struct _TSqlite *This)
{
    sqlite3_step(This->Private->ppStmt);
}
static void SQLite_bind_int(struct _TSqlite *This,int arg)
{
    sqlite3_bind_int(This->Private->ppStmt,This->Private->bind_num,arg);
	This->Private->bind_num++;
}
static void SQLite_bind_blob(struct _TSqlite *This,void *data,int size)
{
    sqlite3_bind_blob(This->Private->ppStmt,This->Private->bind_num,data,size,NULL);
	This->Private->bind_num++;
}
static void SQLite_bind_text(struct _TSqlite *This,char *text)
{
    sqlite3_bind_text(This->Private->ppStmt,This->Private->bind_num,text,-1,SQLITE_STATIC);
	This->Private->bind_num++;
}
//----------------------------------------------------------------------------
TSqlite * CreateLocalQuery(const char *FileName)
{
	int ret;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);

    TSqlite * This = (TSqlite *)malloc(sizeof(TSqlite));
    memset(This,0,sizeof(TSqlite));
    This->Private = (struct SqlitePrivate*)malloc(sizeof(struct SqlitePrivate));
	This->sql_lock = (struct Sqlite_mutex *)malloc(sizeof(struct Sqlite_mutex));
    memset(This->Private,0,sizeof(struct SqlitePrivate));
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&This->sql_lock->mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

	ret = sqlite3_open(FileName, &This->Private->db);
	if(ret!=SQLITE_OK) {
		DPRINT("open db err:%s\n",sqlite3_errmsg(This->Private->db));
		sqlite3_close(This->Private->db);
		free(This->Private);
		free(This);
		return NULL;
	}

	This->file_name = FileName;
	This->Destroy = SQLite_Destroy;
    This->Open = SQLite_Open;
    This->ExecSQL  = SQLite_ExecSQL;
    This->Close = SQLite_Close;
    This->First = SQLite_First;
    This->Last = SQLite_Last;
    This->Prior = SQLite_Prior;
    This->Next = SQLite_Next;
    This->SetRecNo = SQLite_SetRecNo;
    This->RecNo = SQLite_RecNo;
    This->FieldByName = SQLite_FieldByName;
    This->GetSQLText = SQLite_GetSQLText;
    This->SetSQLText = SQLite_SetSQLText;
	This->Active = SQLite_Active;
	This->RecordCount = SQLite_RecordCount;
	This->FieldCount = SQLite_FieldCount;
	This->LastRowId = SQLite_LastRowId;

    This->prepare = SQLite_prepare;
    This->reset = SQLite_reset;
    This->finalize = SQLite_finalize;
    This->step = SQLite_step;
    This->bind_int = SQLite_bind_int;
    This->bind_text = SQLite_bind_text;
    This->bind_blob = SQLite_bind_blob;
	This->getBlobData = SQLite_get_blob_data;
    return This;
}
//----------------------------------------------------------------------------
BOOL LocalQueryOpen(TSqlite *Query,char *SqlStr)
{
	SQL_LOCK();
    Query->Close(Query);
    Query->SetSQLText(Query,SqlStr);
	BOOL ret = 	Query->Open(Query);
	SQL_UNLOCK();
    return ret;
}
//----------------------------------------------------------------------------
BOOL LocalQueryExec(TSqlite *Query,char *SqlStr)
{
	SQL_LOCK();
    Query->Close(Query);
    Query->SetSQLText(Query,SqlStr);
	BOOL ret = Query->ExecSQL(Query);
	SQL_UNLOCK();
    return ret;
}
//----------------------------------------------------------------------------
char* LocalQueryOfChar(TSqlite *Query,char *FieldName,char *cBuf,int Size)
{
	PSQLiteField Field;
	Field = Query->FieldByName(Query,FieldName);
    if(Field==NULL)
    {
        return cBuf;
    }

	return Field->AsChar(Field,cBuf,Size);
}
//----------------------------------------------------------------------------
int LocalQueryOfInt(TSqlite *Query,char *FieldName)
{
	PSQLiteField Field;
	Field = Query->FieldByName(Query,FieldName);
    if(Field==NULL)
    {
        return 0;
    }
    return Field->AsInt(Field);
}
//----------------------------------------------------------------------------
double LocalQueryOfFloat(TSqlite *Query,char *FieldName)
{
	PSQLiteField Field;
	Field = Query->FieldByName(Query,FieldName);
    if(Field==NULL)
    {
        return 0;
    }

    return Field->AsFloat(Field);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief sqlLoad 加载数据库文件并判断是否成功，若不成功则恢复备份文件，重新加载
 *
 * @param sql 数据库文件
 * @param file 数据库文件名
 */
/* ---------------------------------------------------------------------------*/
void LocalQueryLoad(TSqliteData *sql)
{
	sql->sql = CreateLocalQuery(sql->file_name);
	if (sql->sql) {
		DPRINT("Open %s successfully\n",sql->file_name);
	} else {
		DPRINT("Err:%s open failed\n",sql->file_name);
		char file_bak[32];
		sprintf(file_bak,"%s_bak",sql->file_name);
		if (fileexists(file_bak) == 1) {
			recoverData(sql->file_name);
			sql->sql = CreateLocalQuery(sql->file_name);
		}
	}
}


