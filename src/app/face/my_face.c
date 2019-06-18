/*
 * =============================================================================
 *
 *       Filename:  my_face.c
 *
 *    Description:  人脸识别算法接口
 *
 *        Version:  1.0
 *        Created:  2019-06-17 15:24:43
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
#include "rd_face.h"
#include "my_face.h"
#include "debug.h"
#include "sql_handle.h"

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
MyFace *my_face;
static int init(void)
{
	if(rdfaceInit()<0) {
		rdfaceUninit();
		DPRINT("rdfaceInit error!");
		return -1;
	}
	return 0;
}
static int regist(unsigned char *image_buff,int w,int h,char *id,char *nick_name,char *url)
{
	float *feature = NULL;
	int feature_len = 0;
	if (rdfaceRegist(image_buff,w,h,&feature,&feature_len) < 0 ){
		DPRINT("regsit face err!\n");
		return -1;
	}
	sqlInsertFace(id,nick_name,url,feature,feature_len);
}
static int deleteOne(char *id)
{
	sqlDeleteFace(id);
	return 0;
}
static int recognizer(char *image_buff)
{
	
}
static void uninit(void)
{
	rdfaceUninit();
}

void myFaceInit(void)
{
	my_face = (MyFace *) calloc(1,sizeof(MyFace));
	my_face->init = init;
	my_face->regist = regist;
	my_face->deleteOne = deleteOne;
	my_face->recognizer = recognizer;
	my_face->uninit = uninit;
	my_face->init();
}
