/*
 * =============================================================================
 *
 *       Filename:  CjsonDec.h
 *
 *    Description:  json编解码封装
 *
 *        Version:  1.0
 *        Created:  2016-07-06 17:23:53 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _CJSON_DEC_H
#define _CJSON_DEC_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "cJSON.h"

	typedef struct _CjsonDec {
		cJSON *root;
		cJSON *current;
		cJSON *front;

		void (*getValueChar)(struct _CjsonDec *,char *name,char **value);
		int (*getValueInt)(struct _CjsonDec *,char *name);
		int (*getArrayInt)(struct _CjsonDec *,int item,char *name);
		void (*getArrayChar)(struct _CjsonDec *,int item,char *name,char **value);
		int (*changeCurrentObj)(struct _CjsonDec *,char *name);
		void (*changeCurrentArrayObj)(struct _CjsonDec *,int item);
		void (*changeObjFront)(struct _CjsonDec *);
		int (*getArraySize)(struct _CjsonDec *);
		void (*print)(struct _CjsonDec *);
		void (*destroy)(struct _CjsonDec *);
		
	}CjsonDec;
	CjsonDec *cjsonDecCreate(char *data);
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
