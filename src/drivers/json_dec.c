/*
 * =============================================================================
 *
 *       Filename:  CjsonDec.c
 *
 *    Description:  json编解码封装
 *
 *        Version:  v1.0
 *        Created:  2016-07-06 17:23:16
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

#include "json_dec.h"
#include "debug.h"

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

/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonJugdeTypeInt 返回整型类型数据
 *
 * @param data 输入数据
 *
 * @returns 返回整型值
 */
/* ---------------------------------------------------------------------------*/
static int cjsonJugdeTypeInt(cJSON *data)
{
	if (!data) {
		return 0;
	}
	switch (data->type) 
	{
		case cJSON_False: return 0;
		case cJSON_True: return 1;
		case cJSON_Number: return data->valueint;
		default: return 0;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonJugdeTypeChar 返回字符串类型数据
 *
 * @param data 输入数据
 * @param value 输出数据，传入指针地址
 */
/* ---------------------------------------------------------------------------*/
static void cjsonJugdeTypeChar(cJSON *data,char **value)
{
	if (!data) {
		*value = NULL;
		return;
	}
	switch (data->type) 
	{
		case cJSON_NULL: 
			{
				*value = NULL;
			} break;
		case cJSON_String:
			{
				*value = (char *) malloc(strlen(data->valuestring)+1);
				//printf("valuestring:%s\n",data->valuestring);
				strcpy(*value,data->valuestring);
			} break;
		default: 
			break;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonChangeCurrentObj 切换当前json object
 *
 * @param This
 * @param name 项目名字
 */
/* ---------------------------------------------------------------------------*/
int cjsonChangeCurrentObj(CjsonDec *This,char *name)
{
	cJSON *data = NULL;
	data = cJSON_GetObjectItem(This->current,name);
	if(NULL != data)
	{
		This->front = This->current;
		This->current = data;
	//	printf("data %s\n", data->valuestring);
	}
	else
	{
		printf("cjsonChangeCurrentObj failed\n");
		return -1;
	}
	return 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonChangeCurrentArrayObj 切换成数组 object
 *
 * @param This
 * @param item 数组序号
 */
/* ---------------------------------------------------------------------------*/
static void cjsonChangeCurrentArrayObj(CjsonDec *This,int item)
{
	cJSON *data = NULL;
	data = cJSON_GetArrayItem(This->current,item);
	This->front = This->current;
	This->current = data;
}

static void cjsonChangeObjFront(CjsonDec *This)
{
	This->current = This->front;	
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonGetArraySize 返回数组个数
 *
 * @param This
 *
 * @returns 返回个数
 */
/* ---------------------------------------------------------------------------*/
static int cjsonGetArraySize(CjsonDec *This)
{
	return cJSON_GetArraySize(This->current);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonGetValueChar 对外接口，返回项目字符串
 *
 * @param This
 * @param name 项目
 * @param value 返回字符串的指针地址
 */
/* ---------------------------------------------------------------------------*/
static void cjsonGetValueChar(CjsonDec *This,char *name,char **value)
{
	cJSON *data = NULL;
	data = cJSON_GetObjectItem(This->current,name);
	if(NULL == data)
	{
		printf("cJSON_GetObjectItem:%s NULL\n", name);
		return;
		
	}
	cjsonJugdeTypeChar(data,value);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonGetValueInt 对外接口，返回整型
 *
 * @param This
 * @param name 项目
 *
 * @returns 返回值
 */
/* ---------------------------------------------------------------------------*/
static int cjsonGetValueInt(CjsonDec *This,char *name)
{
	cJSON *data = NULL;
	data = cJSON_GetObjectItem(This->current,name);
	return cjsonJugdeTypeInt(data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonGetArrayInt 对外接口，返回数组中的项目整型值
 *
 * @param This
 * @param item 数组序号
 * @param name 项目
 *
 * @returns 返回整型值
 */
/* ---------------------------------------------------------------------------*/
static int cjsonGetArrayInt(CjsonDec *This,int item,char *name)
{
	cJSON *data = NULL;
	cJSON *data_array = NULL;
	data = cJSON_GetArrayItem(This->current,item);
	data_array = cJSON_GetObjectItem(data,name);
	return cjsonJugdeTypeInt(data_array);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonGetArrayChar 对外接口，返回数组中的项目的字符串值
 *
 * @param This
 * @param item 数组序号
 * @param name 项目
 * @param value 指针地址
 */
/* ---------------------------------------------------------------------------*/
static void cjsonGetArrayChar(CjsonDec *This,int item,char *name,char **value)
{
	cJSON *data = NULL;
	cJSON *data_array = NULL;
	data = cJSON_GetArrayItem(This->current,item);
	if(NULL == data)
	{
		printf("cjsonGetArrayChar NULL\n");
		return;
	}
	switch (data->type) 
	{
		case cJSON_Object: 
		{
			data_array = cJSON_GetObjectItem(data,name);
			cjsonJugdeTypeChar(data_array,value);
		} 
		break;
		case cJSON_String:
		{
			cjsonJugdeTypeChar(data,value);
		} 
		break;
		default: 
			break;
	}
	return;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonPrint 格式化打印当前json obj 结构
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void cjsonPrint(CjsonDec *This)
{
	char *out = cJSON_Print(This->current);
	printf("%s\n", out);
	free(out);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonDestroy 销毁json
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void cjsonDestroy(CjsonDec *This)
{
	cJSON_Delete(This->root);
	free(This);	
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cjsonDecCreate 创建json类
 *
 * @param data HTTP获得的所有数据
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
CjsonDec *cjsonDecCreate(char *data)
{
	CjsonDec *This = (CjsonDec *) malloc(sizeof(CjsonDec));
	This->root = cJSON_Parse(data);
	if(NULL == This->root)
	{
		printf("cJSON_Parse failed\n");
		return NULL;
	}
	This->current = This->root;
	This->front = This->root;
    if (!This->root) {
		saveLog("Error before: [%s]\n",cJSON_GetErrorPtr());
		free(This);
		return NULL;
	}
	This->getValueInt = cjsonGetValueInt;
	This->getValueChar = cjsonGetValueChar;
	This->getArrayInt = cjsonGetArrayInt;
	This->getArrayChar = cjsonGetArrayChar;
	This->changeCurrentObj = cjsonChangeCurrentObj;
	This->changeCurrentArrayObj = cjsonChangeCurrentArrayObj;
	This->changeObjFront = cjsonChangeObjFront;
	This->getArraySize = cjsonGetArraySize;
	This->print = cjsonPrint;
	This->destroy = cjsonDestroy;

	return This;
}
