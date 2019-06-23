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
#include <unistd.h>
#include "face/rdface/rd_face.h"
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
#define RECOGNIZE_INTAVAL 30

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
MyFace *my_face;

static int face_init_finished = 0; // 初始化是否结束，未结束时不处理其他功能
static MyFaceData face_data_last;// 最后一次人脸识别结果，如果为同一人，则每30秒处理一次结果
static int recognize_intaval = 0; // 识别结果处理间隔

/* ---------------------------------------------------------------------------*/
/**
 * @brief threadInit 人脸识别初始化会阻塞，所以在线程执行
 *
 * @param arg
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static void* threadInit(void *arg)
{
	face_init_finished = 0;
	if(rdfaceInit() < 0) {
		rdfaceUninit();
		DPRINT("rdfaceInit error!");
		return NULL;
	}
	face_init_finished = 1;
    return NULL;
}
static void* threadTimer1s(void *arg)
{
    while (1) {
        if (recognize_intaval)
            recognize_intaval--;
        sleep(1); 
    }
    return NULL;
}

static int init(void)
{
    memset(&face_data_last,0,sizeof(MyFaceData));
    createThread(threadInit,NULL);
	return 0;
}

static int regist(MyFaceRegistData *data)
{
	if (face_init_finished == 0)
        goto regist_end;
	float *feature = NULL;
	int feature_len = 0;
    int ret = -1;
	if (rdfaceRegist(data->image_buff,data->w,data->h,&feature,&feature_len) < 0 ){
		DPRINT("regsit face err!\n");
        goto regist_end;
	}
    ret = 0;
	sqlInsertFace(data->id,data->nick_name,data->url,feature,feature_len);
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

static int featureCompareCallback(float *feature,void *face_data_out)
{
    MyFaceData data;
    float feature_src[512];
    int result = -1;
    sqlGetFaceStart();
    while (1) {
        memset(&data,0,sizeof(MyFaceData));
        int ret = sqlGetFace(data.user_id,data.nick_name,data.url,feature_src);
        if (ret == 0)
            break;
        float ret1 = rdfaceGetFeatureSimilarity(feature_src,feature);
        if (ret1 > SIMILAYRITY) {
            // printf("recognizer->id:%s,name:%s,url:%s\n", 
                    // data.user_id,data.nick_name,data.url);
            result = 0;
            if (face_data_out)
                memcpy(face_data_out,&data,sizeof(MyFaceData));
            break;
        }
    };
    sqlGetFaceEnd();
    return result;
}

static void recognizer(char *image_buff,int w,int h)
{
	if (face_init_finished == 0)
        return;
	int ret;
    MyFaceData face_data;
	if (sqlGetFaceCount()) {
        ret = rdfaceRecognizer(image_buff,w,h,featureCompareCallback,&face_data);
        if (ret == 0) {
            if (strcmp(face_data.nick_name,face_data_last.nick_name) == 0) {
                if (recognize_intaval == 0) {
                    recognize_intaval = RECOGNIZE_INTAVAL;
                    printf("recognizer->id:%s,name:%s,url:%s\n", 
                            face_data.user_id,face_data.nick_name,face_data.url);
                    // TODO;
                }
            } else {
                recognize_intaval = RECOGNIZE_INTAVAL;
                printf("recognizer->id:%s,name:%s,url:%s\n", 
                        face_data.user_id,face_data.nick_name,face_data.url);
                // TODO  
            }
            memcpy(&face_data_last,&face_data,sizeof(MyFaceData));
        }
	} else {
        ret = rdfaceRecognizer(image_buff,w,h,NULL,&face_data);
	}
}

static void uninit(void)
{
	rdfaceUninit();
	face_init_finished = 0;
}

void myFaceInit(void)
{
	my_face = (MyFace *) calloc(1,sizeof(MyFace));
	my_face->regist = regist;
	my_face->deleteOne = deleteOne;
	my_face->recognizer = recognizer;
	my_face->uninit = uninit;
	my_face->init = init;
    createThread(threadInit,NULL);
}
