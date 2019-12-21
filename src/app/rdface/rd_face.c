/*
 * =============================================================================
 *
 *       Filename:  rd_face.c
 *
 *    Description:  阅面人脸识别算法
 *
 *
 *        Version:  1.0
 *        Created:  2019-06-18 19:34:21
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
#include <arm_neon.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "config.h"
#include "RSCommon.h"
#include "readsense_face_sdk.h"
#include "ion/ion.h"
#include "video_ion_alloc.h"
#include "thread_helper.h"
#include "rd_face.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define RECOGNIZE_INTAVAL 60

#define WIDTH_MAX 1280
#define HEIGHT_MAX 720
#define FACE_MODEL_PATH "/root/usr/face_model/"
#define APP_ID "dfe52c64a6a368bf181c76b512d2b3fe"

#define RECOGNITION_TIME 0

#define DPRINT(...)           \
do {                          \
    printf("\033[1;32m");  \
    printf("[FACE->%s,%d]",__func__,__LINE__);   \
    printf(__VA_ARGS__);      \
    printf("\033[0m");        \
} while (0)

typedef struct _DebugInfo {
	int opt;
	const char *content;
}DebugInfo;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static const char* g_fd_weights_path 			= FACE_MODEL_PATH"fd.weights.bin";
static const char* g_fl_weights_path 			= FACE_MODEL_PATH"fl.weights.bin";
static const char* g_fle_light_weights_path 	= FACE_MODEL_PATH"fle_light.weights.bin";
static const char* g_fle_infrared_weights_path 	= FACE_MODEL_PATH"fle_infrared.weights.bin";
static const char* g_fgs_weights_path 			= FACE_MODEL_PATH"fgs.weights.bin";
static const char* g_fr_lite_weights_path 		= FACE_MODEL_PATH"fr_lite.weights.bin";
static const char* g_fq_weights_path 			= FACE_MODEL_PATH"fq.weights.bin";
static const char* g_fla_weights_path 			= FACE_MODEL_PATH"fla.weights.bin";
static const char* g_fags_weights_path 			= FACE_MODEL_PATH"fgas.weights.bin";

static struct video_ion model_ion;
static struct video_ion raw_ion;
static struct video_ion dst_ion;
static struct video_ion internal_ion;

static int dsp_fd;
static float feature_last[FACE_RECOGNITION_FEATURE_DIMENSION]; // 最后一次人脸特征值
static int track_id_last;	// 最后一次人脸信息
static int recognize_intaval = 0; // 识别结果处理间隔
static int thread_start = 0;
static int thread_end = 1;

static void* threadTimer1s(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	thread_start = 1;
	thread_end = 0;
    while (thread_start) {
        if (recognize_intaval)
            recognize_intaval--;
        sleep(1);
    }
	thread_end = 1;
    return NULL;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rdfaceInit 初始化人脸识别算法
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
int rdfaceInit(void)
{
	model_ion.fd = -1;
	model_ion.client = -1;


	DPRINT("=====into rdfaceInit()====\n");


	if (video_ion_alloc_rational(&model_ion, 24 * 1024, 1024, 1, 1)) {
		DPRINT("-----> model_ion alloc failed(24MB)\n");
		goto exit;
	}

	DPRINT("==>>alloc model_ion.size:%d,(%dK) !!\n",model_ion.size,model_ion.size/1024);

	raw_ion.fd = -1;
	raw_ion.client = -1;
	if (video_ion_alloc(&raw_ion, WIDTH_MAX, HEIGHT_MAX)) {
		DPRINT("-----> raw_ion alloc failed(%dK)\n",WIDTH_MAX*HEIGHT_MAX/1024);
		goto exit_model;
	}
	DPRINT("==>>alloc raw_ion.size:%d,(%dK)\n",raw_ion.size,raw_ion.size/1024);


	dst_ion.fd = -1;
	dst_ion.client = -1;
	if (video_ion_alloc_rational(&dst_ion, 1024, 1024, 1, 1)) {
		DPRINT("-----> dst_ion alloc failed(1MB)\n");
		goto exit_raw;
	}
	DPRINT("==>>alloc dst_ion.size:%d,(%dK)\n",dst_ion.size,dst_ion.size/1024);


	internal_ion.fd = -1;
	internal_ion.client = -1;
	if (video_ion_alloc_rational(&internal_ion, 1024, 1024, 1, 1)) {
		DPRINT("-----> internal_ion alloc failed(1M)\n");
		goto exit_dst;
	}
	DPRINT("==>>alloc internal_ion.size:%d,(%dK)\n",internal_ion.size,internal_ion.size/1024);

	dsp_fd = open("/dev/dsp", O_RDWR);
	if (dsp_fd < 0) {
		DPRINT("------> opend dsp dev failed.\n");
		goto exit_internal;
	}

	DPRINT("dsp face version_number: %s\n", readsense_face_sdk_get_version_number());
	DPRINT("dsp face device_key: %s\n", readsense_face_sdk_get_device_key());

	//初始化接口
	if(readsense_initial_face_sdk(model_ion.buffer, 20, 5,
				g_fd_weights_path, g_fl_weights_path,
				g_fle_light_weights_path, g_fle_infrared_weights_path,
				g_fgs_weights_path, 0,
				g_fr_lite_weights_path, g_fq_weights_path,
				0,g_fags_weights_path,
				APP_ID,
				// 2dfe86e156957ca1
				// "ea15e2876d514524c873b6159b86f2f5d715819598ee3b116e51cb2338b8ffbe5556ef1d9e7ee21739588fd631923ffd80df7158e7e6fb561eac7faa8252f67e"
				// f07440307d514cc7
				g_config.f_license
				// "424b15c4f0f7f373f7a893544bd758a95bda33c689ab124c66efdb9d93533afc5024f5c8abb68150991e642f54c8e13b34b8ed968aedd024d7d0bc72622ffedb"
				)) {

		DPRINT("%s:------> readsense_initial_face_sdk failed.\n", __func__);
		goto exit_dsp;
	}

	DPRINT("=====exit rdfaceInit()====\n");
    createThread(threadTimer1s,NULL);
	return 0;

exit_dsp:
	close(dsp_fd);
exit_internal:
	video_ion_free(&internal_ion);
exit_dst:
	video_ion_free(&dst_ion);
exit_raw:
	video_ion_free(&raw_ion);
exit_model:
	video_ion_free(&model_ion);
exit:
	return -1;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rdfaceUninit 释放人脸识别算法资源
 */
/* ---------------------------------------------------------------------------*/
void rdfaceUninit(void)
{
	thread_start = 0;
	while(thread_end == 0) {
		thread_start = 0;
		usleep(10000);
	}
	close(dsp_fd);
	video_ion_free(&internal_ion);
	video_ion_free(&dst_ion);
	video_ion_free(&raw_ion);
	video_ion_free(&model_ion);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief fillFaceTrackBuf 填充图片数据
 *
 * @param image
 * @param width
 * @param height
 */
/* ---------------------------------------------------------------------------*/
static void fillFaceTrackBuf(unsigned char *image, int width, int height)
{
	// DPRINT("face->width:%d height:%d\n",width,height);
	raw_ion.width = width;
	raw_ion.height = height;
	raw_ion.size = width * height * 3 / 2;
	memcpy(raw_ion.buffer, image, raw_ion.size);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief getFaceFeature 人脸特征提取
 *
 * @param image
 * @param landmarks21[]
 * @param outFRFeature 特征值
 *
 * @returns 0 成功 -1 失败
 */
/* ---------------------------------------------------------------------------*/
static int getFaceFeature(struct video_ion* image,  rs_point landmarks21[] , float *outFRFeature)
{
	float *pFRFeature = (float *)dst_ion.buffer;

#if RECOGNITION_TIME
	struct timeval start_time,end_time;
	float cost_time;
	gettimeofday(&start_time,NULL);
#endif

	//提取人脸特征值
	if(readsense_face_recognition_lite(model_ion.buffer, (void*)model_ion.phys, image->buffer, (void*)image->phys,
				dst_ion.buffer, (void*)dst_ion.phys, internal_ion.buffer, (void*)internal_ion.phys,
				 dsp_fd,image->width, image->height, (float *)landmarks21)){
        DPRINT("%s:------> readsense_face_recognition_lite failed.\n", __func__);
		return -1;
	}

#if RECOGNITION_TIME
	gettimeofday(&end_time,NULL);
	cost_time = (1000000*end_time.tv_sec) + end_time.tv_usec - (1000000*start_time.tv_sec) - start_time.tv_usec;
	DPRINT("recognition cost time: %f ms\n", cost_time / 1000);
#endif

	memcpy(outFRFeature,pFRFeature,FACE_RECOGNITION_FEATURE_DIMENSION*sizeof(float));
	return 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rdfaceRegist 注册人脸
 *
 * @param image_buff 人脸图片,nv12格式
 * @param w 图片宽
 * @param h 图片高
 * @param out_feature 返回特征值
 *
 * @returns 0成功 -1失败
 */
/* ---------------------------------------------------------------------------*/
int rdfaceRegist(unsigned char *image_buff,int w,int h,float **out_feature,int *out_feature_size)
{
	DPRINT("in rdfaceRegist\n");
	int *face_count = (int *)dst_ion.buffer;
	RSFT_FACE_RESULT *pFace = (RSFT_FACE_RESULT *)((int *)dst_ion.buffer+1);
	int ret;
	if (w < 0 || w > 1280 || h < 0 || h > 720) {
		return -1;
	}
	fillFaceTrackBuf(image_buff,w,h);

	//人脸追踪,针对静态图片
	ret = readsense_face_detection_and_landmark(model_ion.buffer, (void*)model_ion.phys,
			raw_ion.buffer, (void*)raw_ion.phys, dst_ion.buffer, (void*)dst_ion.phys,
			internal_ion.buffer, (void*)internal_ion.phys, dsp_fd, raw_ion.width, raw_ion.height);
	if ( ret !=0 ) {
		DPRINT("%s:------> readsense_face_detection_and_landmark LICENCE_VALIDATE_FAIL.\n", __func__);
		return -1;
	}

	if (*face_count !=1 ) {
		DPRINT("error : face num=%d\n",*face_count);
		return -1;
	}

	DPRINT("\ntrackId: %d, blur: %f,front_prob:%f  ,position is: [%d, %d, %d, %d] \n",
			pFace->track_id, pFace->blur_prob, pFace->front_prob,
			pFace->left, pFace->top, pFace->right, pFace->bottom);

	//提特征
	*out_feature = (float*)malloc(FACE_RECOGNITION_FEATURE_DIMENSION*sizeof(float));
	*out_feature_size = FACE_RECOGNITION_FEATURE_DIMENSION*sizeof(float);

    return getFaceFeature(&raw_ion,  (rs_point *)pFace->face_landmark,*out_feature);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rdfaceRecognizer 探测并识别人脸
 *
 * @param image_buff 输入图像
 * @param w 宽
 * @param h 高
 * @param featureCompare 人脸特征比较回调函数,回调中调用rdfaceGetFeatureSimilarity
 *
 * @returns 0 成功 -1 失败
 */
/* ---------------------------------------------------------------------------*/
int rdfaceRecognizer(unsigned char *image_buff,int w,int h,
        int (*featureCompare)(float *feature,void *face_data_out,int gender,int age),void *face_data_out)
{
	int *face_count = (int *)dst_ion.buffer;
	RSFT_FACE_RESULT *pFace = (RSFT_FACE_RESULT *)((int *)dst_ion.buffer+1);
	RSFT_FACE_RESULT *pFaceALL =NULL;
	int ret = -1;

	fillFaceTrackBuf(image_buff,w,h);
	//人脸检测追踪,针对动态图像
	int result = readsense_face_tracking(model_ion.buffer, (void*)model_ion.phys,
		raw_ion.buffer, (void*)raw_ion.phys, dst_ion.buffer, (void*)dst_ion.phys,
		internal_ion.buffer, (void*)internal_ion.phys, dsp_fd, raw_ion.width, raw_ion.height);

	// DPRINT("\nreadsense_face_tracking  return %d \n",result);
	if ( result == RSFT_LICENCE_VALIDATE_FAIL ) {
		DPRINT("%s:------> readsense_face_tracking LICENCE_VALIDATE_FAIL.\n", __func__);
		goto exit;
	}

	// DPRINT("===>face num=%d\n",*face_count);
	if (*face_count == 0) {
		// DPRINT("error : face num=0\n");
		goto exit;
	}
	int faceCount = *face_count;

	pFaceALL = (RSFT_FACE_RESULT *)malloc(faceCount*sizeof(RSFT_FACE_RESULT));
	if(pFaceALL==NULL) {
		DPRINT("pFaceALL  malloc error!\n");
		goto exit;
	}
	memcpy(pFaceALL,pFace,faceCount*sizeof(RSFT_FACE_RESULT));

	int i=0;

	for(i=0; i<faceCount ; i++) {

		float face_landmark[FACE_LANDMARK_NUM*2];
		memcpy(face_landmark, pFaceALL[i].face_landmark, sizeof(face_landmark));


		float *quality_value = (float *)dst_ion.buffer;
		if(readsense_face_quality(model_ion.buffer,(void*)model_ion.phys,
						raw_ion.buffer, (void*)raw_ion.phys,dst_ion.buffer, (void*)dst_ion.phys,
						internal_ion.buffer, (void*)internal_ion.phys, dsp_fd, raw_ion.width, raw_ion.height,
						face_landmark)) {
			DPRINT("%s:------> readsense_face_quality failed.\n", __func__);
			goto exit;
		}
		// DPRINT("==>quality_value:%f\n",*quality_value);
		if(*quality_value < 0.25) {
			// DPRINT("Low quality=%f\n",*quality_value);
			continue;
		}

		if ((track_id_last == pFaceALL[i].track_id) && (track_id_last != 0)) {
			continue;
		}
		track_id_last = pFaceALL[i].track_id;
		float feature[FACE_RECOGNITION_FEATURE_DIMENSION];
		//提取特征值
		if (getFaceFeature(&raw_ion,  (rs_point *)face_landmark,feature) != 0)
			continue;
		float sim = rdfaceGetFeatureSimilarity(feature_last,feature);
		memcpy(feature_last,feature,sizeof(feature));
		// printf("sim:%f,s:%f\n",sim , SIMILAYRITY );
		if (sim > SIMILAYRITY || sim == 0) {
			if (recognize_intaval == 0)
				recognize_intaval = RECOGNIZE_INTAVAL;
			else {
				break;
			}
		}
		// DPRINT("face_count:%d , facenum:%d: ,trackId: %d, blur: %f,front_prob:%f, gender:%d,age:%d,position is: [%d, %d, %d, %d] \n",
				// faceCount,i,pFaceALL[i].track_id, pFaceALL[i].blur_prob, pFaceALL[i].front_prob,
				// pFaceALL[i].gender , pFaceALL[i].age,
				// pFaceALL[i].left, pFaceALL[i].top, pFaceALL[i].right, pFaceALL[i].bottom);
		if (!featureCompare)
			continue;
		ret = featureCompare(feature,face_data_out,pFaceALL[i].gender,pFaceALL[i].age);
		if (ret >= 0)
			break;
	}

exit:
	free(pFaceALL);
    // readsense_clear_tracking_state();
	return ret;
}


/* ---------------------------------------------------------------------------*/
/**
 * @brief rdfaceGetFeatureSimilarity 人脸特征对比，结果大于0.4，可以认为是
 * 同一人，但最好取人脸库相识度最大的
 *
 * @param feature1
 * @param feature2
 *
 * @returns 返回相似度 0-1之间,>0.4可以认为是同一人
 */
/* ---------------------------------------------------------------------------*/
float rdfaceGetFeatureSimilarity(float *feature1, float *feature2)
{
	int i;
    int dimension = FACE_RECOGNITION_FEATURE_DIMENSION;
	int iter = dimension >> 5;
	float32x4_t sum1 = vdupq_n_f32(0.0);
	float32x4_t sum2 = vdupq_n_f32(0.0);
	for (i = 0; i < iter; i++) {
		float32x4_t f1_0 = vld1q_f32(feature1);
		float32x4_t f2_0 = vld1q_f32(feature2);
		sum1 = vaddq_f32(sum1, vmulq_f32(f1_0, f2_0));

		float32x4_t f1_1 = vld1q_f32(feature1 + 4);
		float32x4_t f2_1 = vld1q_f32(feature2 + 4);
		sum2 = vaddq_f32(sum2, vmulq_f32(f1_1, f2_1));

		float32x4_t f1_2 = vld1q_f32(feature1 + 8);
		float32x4_t f2_2 = vld1q_f32(feature2 + 8);
		sum1 = vaddq_f32(sum1, vmulq_f32(f1_2, f2_2));

		float32x4_t f1_3 = vld1q_f32(feature1 + 12);
		float32x4_t f2_3 = vld1q_f32(feature2 + 12);
		sum2 = vaddq_f32(sum2, vmulq_f32(f1_3, f2_3));

		float32x4_t f1_4 = vld1q_f32(feature1 + 16);
		float32x4_t f2_4 = vld1q_f32(feature2 + 16);
		sum1 = vaddq_f32(sum1, vmulq_f32(f1_4, f2_4));

		float32x4_t f1_5 = vld1q_f32(feature1 + 20);
		float32x4_t f2_5 = vld1q_f32(feature2 + 20);
		sum2 = vaddq_f32(sum2, vmulq_f32(f1_5, f2_5));

		float32x4_t f1_6 = vld1q_f32(feature1 + 24);
		float32x4_t f2_6 = vld1q_f32(feature2 + 24);
		sum1 = vaddq_f32(sum1, vmulq_f32(f1_6, f2_6));

		float32x4_t f1_7 = vld1q_f32(feature1 + 28);
		float32x4_t f2_7 = vld1q_f32(feature2 + 28);
		sum2 = vaddq_f32(sum2, vmulq_f32(f1_7, f2_7));

		feature1 += 32;
		feature2 += 32;
	}
	sum1 = vaddq_f32(sum1, sum2);
	return vgetq_lane_f32(sum1, 0) + vgetq_lane_f32(sum1, 1) + vgetq_lane_f32(sum1, 2) + vgetq_lane_f32(sum1, 3);
}

int rdfaceRecognizerOnce(unsigned char *image_buff,int w,int h,int *age,int *sex)
{
	int *face_count = (int *)dst_ion.buffer;
	RSFT_FACE_RESULT *pFace = (RSFT_FACE_RESULT *)((int *)dst_ion.buffer+1);

	fillFaceTrackBuf(image_buff,w,h);
	//人脸检测追踪,针对动态图像
	int result = readsense_face_tracking(model_ion.buffer, (void*)model_ion.phys,
		raw_ion.buffer, (void*)raw_ion.phys, dst_ion.buffer, (void*)dst_ion.phys,
		internal_ion.buffer, (void*)internal_ion.phys, dsp_fd, raw_ion.width, raw_ion.height);

	// DPRINT("\nreadsense_face_tracking  return %d \n",result);
	if ( result == RSFT_LICENCE_VALIDATE_FAIL ) {
		DPRINT("%s:------> readsense_face_tracking LICENCE_VALIDATE_FAIL.\n", __func__);
		return -1;
	}

	// DPRINT("===>face num=%d\n",*face_count);
	if (*face_count == 0) {
		// DPRINT("error : face num=0\n");
		return -1;
	}
	*age = pFace->age;
	*sex = pFace->gender;

	return 0;
}

const char *rdfaceGetFaceVersion(void)
{
	return readsense_face_sdk_get_version_number();
}
const char *rdfaceGetDeviceKey(void)
{
	return readsense_face_sdk_get_device_key();	
}
