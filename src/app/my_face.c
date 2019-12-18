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
#include "config.h"
#include "debug.h"
#include "sql_handle.h"
#include "thread_helper.h"
#include "protocol.h"
#include "my_audio.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern char *rdfaceGetFaceVersion(void);
extern char *rdfaceGetDeviceKey(void);

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

static int init(void)
{
	if (g_config.face_enable == 0)
		return 0;
	// 不重复初始化
	if (face_init_finished == 1)
		return 1;
#ifdef USE_FACE
	pthread_mutex_lock(&mutex);
	face_init_finished = 0;
	if(rdfaceInit() < 0) {
		rdfaceUninit();
		DPRINT("rdfaceInit error!");
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	face_init_finished = 1;
	pthread_mutex_unlock(&mutex);
#endif
	return 1;
}

static int regist(MyFaceRegistData *data)
{
	if (g_config.face_enable == 0)
		return -1;
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
	if (g_config.face_enable == 0)
		return -1;
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

static int recognizer(char *image_buff,int w,int h)
{
	if (g_config.face_enable == 0)
		return 0;
#ifdef USE_FACE
	if (face_init_finished == 0)
        return 0;
	if (pthread_mutex_trylock(&mutex) != 0)
		return 1;
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
	return 1;
}

static void uninit(void)
{
	if (face_init_finished == 0)
		return;
#ifdef USE_FACE
	pthread_mutex_lock(&mutex);
	face_init_finished = 0;
	rdfaceUninit();
	pthread_mutex_unlock(&mutex);
#endif
	printf("face uninited\n");
}

static char *getVersion(void)
{
#ifdef USE_FACE
	return rdfaceGetFaceVersion();	
#else
	return "未检测到人脸识别算法";	
#endif
}
static char *getDeviceKey(void)
{
#ifdef USE_FACE
	return rdfaceGetDeviceKey();	
#else
	return "未检测到人脸识别算法";	
#endif
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
	my_face->getVersion = getVersion;
	my_face->getDeviceKey = getDeviceKey;

	face_init_finished = 0;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);
}
