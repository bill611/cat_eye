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
#include <string.h>
#include "rd_face.h"
#include "my_face.h"
#include "debug.h"
#include "sql_handle.h"
#include "thread_helper.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define SIMILAYRITY 0.4

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
    int ret = -1;
	if (rdfaceRegist(image_buff,w,h,&feature,&feature_len) < 0 ){
		DPRINT("regsit face err!\n");
        goto regist_end;
	}
    ret = 0;
	sqlInsertFace(id,nick_name,url,feature,feature_len);
regist_end:
    if (feature)
        free(feature);
    return ret;
}

static int deleteOne(char *id)
{
	sqlDeleteFace(id);
	return 0;
}

static int featureCompareCallback(float *feature)
{
    char user_id[32];
    char nick_name[128];
    char url[256];
    float feature_src[512];
    int result = -1;
    sqlGetFaceStart();
    while (1) {
        memset(user_id,0,sizeof(user_id));
        memset(nick_name,0,sizeof(nick_name));
        memset(url,0,sizeof(url));
        int ret = sqlGetFace(user_id,nick_name,url,feature_src);
        if (ret == 0)
            break;
        float ret1 = rdfaceGetFeatureSimilarity(feature_src,feature);
        if (ret1 > SIMILAYRITY) {
            printf("recognizer->id:%s,name:%s,url:%s\n", user_id,nick_name,url);
            result = 0;
            break; 
        }
    };
    sqlGetFaceEnd();
    return result;
}

static void recognizer(char *image_buff,int w,int h)
{
	int ret;
	if (sqlGetFaceCount()) {
        ret = rdfaceRecognizer(image_buff,w,h,featureCompareCallback);
	} else {
        ret = rdfaceRecognizer(image_buff,w,h,NULL);
	}
}

static void uninit(void)
{
	rdfaceUninit();
}

static void* myFaceInitThread(void *arg)
{
	my_face = (MyFace *) calloc(1,sizeof(MyFace));
	my_face->regist = regist;
	my_face->deleteOne = deleteOne;
	my_face->recognizer = recognizer;
	my_face->uninit = uninit;
	my_face->init = init;
}
void myFaceInit(void)
{
	createThread(myFaceInitThread,NULL);
}
