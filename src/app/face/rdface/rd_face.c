/*
 * =============================================================================
 *
 *       Filename:  rd_face.c
 *
 *    Description:  阅面人脸识别算法
 *
3.rdfaceRecognizer 人脸检测追踪
4.getFaceFeature 人脸特征提取
5.get_fr_feature_similarity_norm_neon 人脸比对，结果大于0.4即可认为是同一人，但最好取人脸库相识度最大的。
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
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "fcntl.h"
#include "RSCommon.h"
#include "readsense_face_sdk.h"
#include "ion/ion.h"
#include "video_ion_alloc.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void fillFaceTrackBuf(unsigned char *image, int width, int height);
static int getFaceFeature(struct video_ion* image,  rs_point landmarks21[], float *outFRFeature);
int DB_similarity_contrast(float *feature , PERSON_INFO *person_info);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define WIDTH_MAX 1280
#define HEIGHT_MAX 720
#define FACE_MODEL_PATH "/data/face_model/"
#define APP_ID "dfe52c64a6a368bf181c76b512d2b3fe"

#define RECOGNITION_TIME 0

typedef struct tag_PERSON_INFO
{
	char name[32];
	int id;
	float DB_feature[10][FACE_RECOGNITION_FEATURE_DIMENSION];
}PERSON_INFO;

typedef struct tag_FACE_DB
{
	int face_num;
	PERSON_INFO person_data[10];
}FACE_DB;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static const char* g_fd_weights_path = FACE_MODEL_PATH"fd.weights.bin";
static const char* g_fl_weights_path = FACE_MODEL_PATH"fl.weights.bin";
static const char* g_fle_light_weights_path = FACE_MODEL_PATH"fle_light.weights.bin";
static const char* g_fle_infrared_weights_path = FACE_MODEL_PATH"fle_infrared.weights.bin";
static const char* g_fgs_weights_path = FACE_MODEL_PATH"fgs.weights.bin";
static const char* g_fr_lite_weights_path = FACE_MODEL_PATH"fr_lite.weights.bin";
static const char* g_fq_weights_path = FACE_MODEL_PATH"fq.weights.bin";
static const char* g_fla_weights_path = FACE_MODEL_PATH"fla.weights.bin";

static struct video_ion model_ion;
static struct video_ion raw_ion;
static struct video_ion dst_ion;
static struct video_ion internal_ion;
static int dsp_fd;

FACE_DB g_DB_face;
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


	printf("=-====into rdfaceInit()====\n");


	printf("====>>alloc model_ion!!\n");
	if (video_ion_alloc_rational(&model_ion, 32 * 1024, 1024, 1, 1)) {
		printf("%s: %d -----> model_ion alloc failed.\n", __func__, __LINE__);
		goto exit;
	}


	raw_ion.fd = -1;
	raw_ion.client = -1;
	if (video_ion_alloc(&raw_ion, WIDTH_MAX, HEIGHT_MAX)) {
		printf("%s: %d -----> raw_ion alloc failed.\n", __func__, __LINE__);
		goto exit_model;
	}
	printf("==>>alloc raw_ion.size:%d !!\n",raw_ion.size);


	dst_ion.fd = -1;
	dst_ion.client = -1;
	if (video_ion_alloc_rational(&dst_ion, 1024, 1024, 1, 1)) {
		printf("%s: %d -----> dst_ion alloc failed.\n", __func__, __LINE__);
		goto exit_raw;
	}


	internal_ion.fd = -1;
	internal_ion.client = -1;
	if (video_ion_alloc_rational(&internal_ion, 1024, 1024, 1, 1)) {
		printf("%s: %d -----> internal_ion alloc failed.\n", __func__, __LINE__);
		goto exit_dst;
	}

	dsp_fd = open("/dev/dsp", O_RDWR);
	if (dsp_fd < 0) {
		printf("------> opend dsp dev failed.\n");
		goto exit_internal;
	}

	printf("dsp face version_number: %s\n", readsense_face_sdk_get_version_number());
	printf("dsp face device_key: %s\n", readsense_face_sdk_get_device_key());

	//初始化接口
	if(readsense_initial_face_sdk(model_ion.buffer, 20, 5,
				g_fd_weights_path, g_fl_weights_path,
				g_fle_light_weights_path, g_fle_infrared_weights_path,
				g_fgs_weights_path, 0,
				g_fr_lite_weights_path, g_fq_weights_path,
				0,
				APP_ID,
				"f090638b22014521fee7b9de8c02fb071fc4d54e9062c84dc23daa289ae6f9b13dc6b467aed88b65033336ae725e72bfdacee34d1f41a8f57f22b3f0050216d3")) {

		printf("%s:------> readsense_initial_face_sdk failed.\n", __func__);
		goto exit_dsp;
	}

	printf("exit rdfaceInit()\n");
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
	printf("face->width:%d height:%d\n",width,height);
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
 * @param outFRFeature
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static int getFaceFeature(struct video_ion* image,  rs_point landmarks21[] , float *outFRFeature)
{
	float *pFRFeature = (float *)dst_ion.buffer;
	int j;

#if RECOGNITION_TIME
	struct timeval start_time,end_time;
	float cost_time;
	gettimeofday(&start_time,NULL);
#endif

	//提取人脸特征值
	if(readsense_face_recognition_lite(model_ion.buffer, (void*)model_ion.phys, image->buffer, (void*)image->phys,
				dst_ion.buffer, (void*)dst_ion.phys, internal_ion.buffer, (void*)internal_ion.phys,
				 dsp_fd,image->width, image->height, (float *)landmarks21)){
        printf("%s:------> readsense_face_recognition_lite failed.\n", __func__);
		return -1;
	}

#if RECOGNITION_TIME
	gettimeofday(&end_time,NULL);
	cost_time = (1000000*end_time.tv_sec) + end_time.tv_usec - (1000000*start_time.tv_sec) - start_time.tv_usec;
	printf("recognition cost time: %f ms\n", cost_time / 1000);
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
	printf("in rdfaceRegist\n");
	int *face_count = (int *)dst_ion.buffer;
	RSFT_FACE_RESULT *pFace = (RSFT_FACE_RESULT *)((int *)dst_ion.buffer+1);
	int ret;
	fillFaceTrackBuf(image_buff,w,h);

	//人脸追踪
	ret = readsense_face_detection(model_ion.buffer, (void*)model_ion.phys,
			raw_ion.buffer, (void*)raw_ion.phys, dst_ion.buffer, (void*)dst_ion.phys,
			internal_ion.buffer, (void*)internal_ion.phys, dsp_fd, raw_ion.width, raw_ion.height);
	printf("readsense_face_detection_and_landmark  return %d \n",ret);
	if ( ret !=0 ) {
		printf("%s:------> readsense_face_detection_and_landmark LICENCE_VALIDATE_FAIL.\n", __func__);
		return -1;
	}

	if (*face_count !=1 ) {
		printf("error : face num=%d\n",*face_count);
		return -1;
	}

	printf("\ntrackId: %d, blur: %f,front_prob:%f  ,position is: [%d, %d, %d, %d] \n",
			pFace->track_id, pFace->blur_prob, pFace->front_prob,
			pFace->left, pFace->top, pFace->right, pFace->bottom);


	float face_landmark[FACE_LANDMARK_NUM*2];
	memcpy(face_landmark, pFace->face_landmark, sizeof(face_landmark));

	//提特征
	*out_feature = (float *)dst_ion.buffer;
	*out_feature_size = FACE_RECOGNITION_FEATURE_DIMENSION;

	if(readsense_face_recognition_lite(model_ion.buffer, (void*)model_ion.phys, raw_ion.buffer, (void*)raw_ion.phys,
				dst_ion.buffer, (void*)dst_ion.phys, internal_ion.buffer, (void*)internal_ion.phys,
				dsp_fd,raw_ion.width, raw_ion.height, (float *)face_landmark)){
		printf("%s:------> readsense_face_recognition_lite failed.\n", __func__);
		return -1;
	}

	return 0;
}

int rdfaceRecognizer(void)
{
	int *face_count = (int *)dst_ion.buffer;
	RSFT_FACE_RESULT *pFace = (RSFT_FACE_RESULT *)((int *)dst_ion.buffer+1);
	RSFT_FACE_RESULT *pFaceALL =NULL;
	int ret;
	printf("in rdfaceRecognizer\n");

	//人脸检测追踪
	ret = readsense_face_tracking(model_ion.buffer, (void*)model_ion.phys,
		raw_ion.buffer, (void*)raw_ion.phys, dst_ion.buffer, (void*)dst_ion.phys,
		internal_ion.buffer, (void*)internal_ion.phys, dsp_fd, raw_ion.width, raw_ion.height);

	printf("\nreadsense_face_tracking  return %d \n",ret);
	if ( ret ==RSFT_LICENCE_VALIDATE_FAIL ) {
		printf("%s:------> readsense_face_tracking LICENCE_VALIDATE_FAIL.\n", __func__);
		return -1;
	}

#if 1
	printf("===>face num=%d\n",*face_count);
	if (*face_count == 0) {
		printf("error : face num=0\n");
		goto exit;
	}
#endif
	int faceCount=*face_count;

	pFaceALL = (RSFT_FACE_RESULT *)malloc(faceCount*sizeof(RSFT_FACE_RESULT));
	if(pFaceALL==NULL) {
		printf("pFaceALL  malloc error!\n");
		return -1;
	}
	memcpy(pFaceALL,pFace,faceCount*sizeof(RSFT_FACE_RESULT));

	int i=0;

	for(i=0; i<faceCount ; i++) {
		printf("face_count:%d , facenum:%d: ,trackId: %d, blur: %f,front_prob:%f  ,position is: [%d, %d, %d, %d] \n",
				faceCount,i,pFaceALL[i].track_id, pFaceALL[i].blur_prob, pFaceALL[i].front_prob,
				pFaceALL[i].left, pFaceALL[i].top, pFaceALL[i].right, pFaceALL[i].bottom);

		float face_landmark[FACE_LANDMARK_NUM*2];
		memcpy(face_landmark, pFaceALL[i].face_landmark, sizeof(face_landmark));

		int j=0;
		printf("face_landmark===> ");
		for(j=0;j<FACE_LANDMARK_NUM*2;j++)
		{
			printf(" %f ",face_landmark[j]);

		}
		printf("\n");


		float *quality_value = (float *)dst_ion.buffer;
		if(readsense_face_quality(model_ion.buffer,(void*)model_ion.phys,
						raw_ion.buffer, (void*)raw_ion.phys,dst_ion.buffer, (void*)dst_ion.phys,
						internal_ion.buffer, (void*)internal_ion.phys, dsp_fd, raw_ion.width, raw_ion.height,
						face_landmark))
		{
			printf("%s:------> readsense_face_quality failed.\n", __func__);
			goto exit;
		}
		printf("==>quality_value:%f\n",*quality_value);
		if(*quality_value<0.25)
		{
			printf("Low quality=%f\n",*quality_value);
			continue;
		}
	#if	0
		int *liveness_flag =(int *)dst_ion.buffer;
		if(readsense_face_liveness_infrared(model_ion.buffer,(void*)model_ion.phys,
						raw_ion.buffer, (void*)raw_ion.phys,dst_ion.buffer, (void*)dst_ion.phys,
						internal_ion.buffer, (void*)internal_ion.phys, dsp_fd, raw_ion.width, raw_ion.height,
						face_landmark))
		{
			printf("%s:------> readsense_face_liveness_infrared failed.\n", __func__);
			goto exit;
		}
		if( 0  && *liveness_flag==0)
		{
			printf("not liveness!!\n");
			continue;
		}
	#endif

	#if 1
		float feature[FACE_RECOGNITION_FEATURE_DIMENSION];
		//提取特征值
		ret = getFaceFeature(&raw_ion,  (rs_point *)face_landmark,feature);

		PERSON_INFO person_info;
		//对比
		if(DB_similarity_contrast(feature,&person_info))
		{
			printf("Identify  successful :  %s \n", person_info.name);
		}
		else
		{
			printf("Identify failure!!\n");
		}
	#endif
	}



	free(pFaceALL);

	return ret;
exit:
	free(pFaceALL);
	return -1;
}




#if 1


static float get_fr_feature_similarity_norm_neon(float *feature1, float *feature2, int dimension)
{
	int i;
	int iter = dimension >> 5;
	float32x4_t sum1 = vdupq_n_f32(0.0);
	float32x4_t sum2 = vdupq_n_f32(0.0);
	for (i = 0; i < iter; i++)
	{
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


int DB_similarity_contrast(float *feature , PERSON_INFO *person_info)
{
	int ret=0;
	int i=0;
	float similarity_ret=0;

	for(i=0;i<g_DB_face.face_num;i++)
	{
#define TRACK_TIME 0
#if TRACK_TIME
		struct timeval start_time,end_time;
		float cost_time;

		gettimeofday(&start_time,NULL);
#endif
		similarity_ret = get_fr_feature_similarity_norm_neon(g_DB_face.person_data[i].DB_feature, feature,FACE_RECOGNITION_FEATURE_DIMENSION );
#if TRACK_TIME
		gettimeofday(&end_time,NULL);
		cost_time = (1000000*end_time.tv_sec) + end_time.tv_usec - (1000000*start_time.tv_sec) - start_time.tv_usec;
	//	if (cost_time > 1000)
		{
			printf("get_fr_feature_similarity_norm_neon cost time: %f ms\n", cost_time / 1000);
		}
#endif
		printf("(%d)----cmp ret=%f \n",i,similarity_ret);
		if(similarity_ret>0.4)
		{
			ret=1;
			person_info->id = i;

			memcpy(person_info->name, g_DB_face.person_data[i].name,strlen(g_DB_face.person_data[i].name));

			break;
		}
	}
	return ret;
}
#endif





int rktest(int argc,char *argv[])
{

	char *path = NULL;
	char *info = NULL;
	int mode=0;

	// fillFaceTrackBuf(  buff, WIDTH_MAX , HEIGHT_MAX);


		//识别
		rdfaceRecognizer();
		readsense_clear_tracking_state();

	return 0;
}


