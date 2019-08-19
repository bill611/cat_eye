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
#include "rdface/rd_face.h"
#include "my_face.h"
#include "debug.h"
#include "sql_handle.h"
#include "thread_helper.h"
#include "protocol.h"
#include "my_audio.h"

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

static pthread_mutex_t mutex; // 初始化过程线程锁,初始化过程不可被打断
static int face_init_finished = 0; // 初始化是否结束，未结束时不处理其他功能

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
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
#ifdef USE_FACE
	pthread_mutex_lock(&mutex);
	face_init_finished = 0;
	if(rdfaceInit() < 0) {
		rdfaceUninit();
		DPRINT("rdfaceInit error!");
		pthread_mutex_unlock(&mutex);
		return NULL;
	}
	face_init_finished = 1;
	pthread_mutex_unlock(&mutex);
#endif
    return NULL;
}
static int init(void)
{
    createThread(threadInit,NULL);
	return 0;
}

static int regist(MyFaceRegistData *data)
{
#ifdef USE_FACE
	if (face_init_finished == 0)
        goto regist_end;
	pthread_mutex_lock(&mutex);
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
	pthread_mutex_unlock(&mutex);
    return ret;
#endif
}

static int recognizerOnce(MyFaceRecognizer *data)
{
#ifdef USE_FACE
	if (face_init_finished == 0)
        return -1;
	pthread_mutex_lock(&mutex);
	int ret = rdfaceRecognizerOnce(data->image_buff,data->w,data->h,&data->age,&data->sex);
	pthread_mutex_unlock(&mutex);
    return ret;
#endif
}
static int deleteOne(char *id)
{
	sqlDeleteFace(id);
	return 0;
}

static int featureCompareCallback(float *feature,
		void *face_data_out,
		int sex,
		int age)
{
#ifdef USE_FACE
    MyFaceData data;
	MyFaceData *p = (MyFaceData*)face_data_out;
    float feature_src[512];
    int result = -1;
	if (sqlGetFaceCount()) {
		sqlGetFaceStart();
		while (1) {
			memset(&data,0,sizeof(MyFaceData));
			int ret = sqlGetFace(data.user_id,data.nick_name,data.url,feature_src);
			if (ret == 0)
				break;
			float ret1 = rdfaceGetFeatureSimilarity(feature_src,feature);
			if (ret1 > SIMILAYRITY) {
				printf("similyrity:%f,:%s,name:%s,url:%s,sex:%d,age:%d\n", 
						ret1,data.user_id,data.nick_name,data.url,sex,age);
				result = 0;
				if (p)
					memcpy(p,&data,sizeof(MyFaceData));
				break;
			}
		};
		sqlGetFaceEnd();
		if (result == -1) {
			if (p) {
				p->age = age;
				p->sex = sex;
			}
			result = 1;
		}
	} else {
		if (p) {
			p->age = age;
			p->sex = sex;
		}
		result = 1;
	}
	return result;
#endif
}

static void recognizer(char *image_buff,int w,int h)
{
#ifdef USE_FACE
	if (face_init_finished == 0)
        return;
	if (pthread_mutex_trylock(&mutex) != 0)
		return ;
    MyFaceData face_data;
	int ret = rdfaceRecognizer(image_buff,w,h,featureCompareCallback,&face_data);
	if (ret == 0) {
		printf("recognizer->id:%s,name:%s,url:%s\n", 
				face_data.user_id,face_data.nick_name,face_data.url);
		if (protocol_singlechip)
			protocol_singlechip->hasPeople(face_data.nick_name,face_data.user_id);
		myAudioPlayRecognizer(face_data.nick_name);
	} else if (ret == 1) {
		printf("recognizer->age:%d,sex:%d\n", 
				face_data.age,face_data.sex);
	}
	pthread_mutex_unlock(&mutex);
#endif
}

static void uninit(void)
{
#ifdef USE_FACE
	pthread_mutex_lock(&mutex);
	face_init_finished = 0;
	rdfaceUninit();
	pthread_mutex_unlock(&mutex);
#endif
	printf("face uninited\n");
}

void myFaceInit(void)
{
	my_face = (MyFace *) calloc(1,sizeof(MyFace));
	my_face->regist = regist;
	my_face->recognizerOnce = recognizerOnce;
	my_face->deleteOne = deleteOne;
	my_face->recognizer = recognizer;
	my_face->uninit = uninit;
	my_face->init = init;

	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);
}
