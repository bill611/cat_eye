#ifndef READSENSE_FACE_SDK_H
#define READSENSE_FACE_SDK_H

#define RSFT_MAX_FACE_NUM 10

#define FACE_LANDMARK_NUM 21

#define FACE_RECOGNITION_FEATURE_DIMENSION 512

#ifdef __cplusplus
extern "C" {
#endif

const char * readsense_face_sdk_get_version_number();

const char * readsense_face_sdk_get_device_key();


//detect_frequency: recommend [5,20]
int readsense_initial_face_sdk(void * model_virt, 
				const int detect_frequency, const int detect_frequency_noface, 
				const char * fd_weights_path, const char * fl_weights_path, 
				const char * fle_light_weights_path, const char * fle_infrared_weights_path, 
				const char * fgs_weights_path, const char * fr_weights_path, 
				const char * fr_lite_weights_path, const char * fq_weights_path, 
				const char * fla_weights_path, const char * fgas_weights_path, 
				const char * app_id, const char * signature);




enum {
	RSFT_DETECT_FRAME_RETURN_VALUE = 1001,
	RSFT_TRACK_FRAME_RETURN_VALUE = 0,
	RSFT_LICENCE_VALIDATE_FAIL = 1
};

typedef struct tag_RSFT_FACE_RESULT
{
	int track_id;

	int left;
	int top;
	int right;
	int bottom;

	float blur_prob;
	float front_prob;

	float face_landmark[FACE_LANDMARK_NUM*2];

	int gender;//0: female, 1: male, -1: invalid value
	int age;//>=0, -1: invalid value
} RSFT_FACE_RESULT;

int readsense_face_tracking(void * model_virt, void * model_phys, void * raw_virt, void * raw_phys, 
				void * dst_virt, void * dst_phys, void * internal_virt, void * internal_phys,
				int dsp_fd, int raw_width, int raw_height);

// sample code of getting result after invoking <readsense_face_tracking>:
/*
	int face_num = *((int*)(dst_virt));
	printf("face_num:%d\n", face_num);
	RSFT_FACE_RESULT * pFTResult = (RSFT_FACE_RESULT *)((int*)(dst_virt) + 1);
	for (int i = 0; i < face_num; i++)
	{
		printf("facenum %d: %d,%d,%d,%d, track_id:%d\n", i, pFTResult[i].left, pFTResult[i].top, 
			pFTResult[i].right, pFTResult[i].bottom, pFTResult[i].track_id);
		printf("blur:%f, front:%f\n", pFTResult[i].blur_prob, pFTResult[i].front_prob);
		printf("face_landmark:\n");
		for (int k = 0; k < FACE_LANDMARK_NUM;k++)
		{
			printf("%f,%f\n", pFTResult[i].face_landmark[k*2], pFTResult[i].face_landmark[k*2+1]);
		}
	}
*/

int readsense_clear_tracking_state();

//reserve for debug, don't use generally.
int readsense_face_detection(void * model_virt, void * model_phys, void * raw_virt, void * raw_phys,
				void * dst_virt, void * dst_phys, void * internal_virt, void * internal_phys,
				int dsp_fd, int raw_width, int raw_height);

//reserve for debug, don't use generally.
int readsense_face_detection_and_landmark(void * model_virt, void * model_phys, void * raw_virt, void * raw_phys,
				void * dst_virt, void * dst_phys, void * internal_virt, void * internal_phys,
				int dsp_fd, int raw_width, int raw_height);




int readsense_face_quality(void * model_virt, void * model_phys, 
				void * raw_virt, void * raw_phys, void * dst_virt, void * dst_phys, 
				void * internal_virt, void * internal_phys, int dsp_fd, int raw_width, int raw_height, 
				float * landmark_position);
/*
	printf("face quality: %f\n", *((float *)dst_virt));
*/


int readsense_face_liveness_infrared(void * model_virt, void * model_phys,
				void * raw_virt, void * raw_phys, void * dst_virt, void * dst_phys,
				void * internal_virt, void * internal_phys, int dsp_fd, int raw_width, int raw_height,
				float * landmark_position);
/*
	printf("face liveness: %d\n", *((int *)dst_virt));
*/


int readsense_face_liveness_light(void * model_virt, void * model_phys,
				void * raw_virt, void * raw_phys, void * dst_virt, void * dst_phys,
				void * internal_virt, void * internal_phys, int dsp_fd, int raw_width, int raw_height,
				float * landmark_position);
/*
	printf("face liveness: %d\n", *((int *)dst_virt));
*/


int readsense_face_recognition_lite(void * model_virt, void * model_phys, 
				void * raw_virt, void * raw_phys, void * dst_virt, void * dst_phys, 
				void * internal_virt, void * internal_phys, int dsp_fd, int raw_width, int raw_height, 
				float * landmark_position);
/*
	printf("fr_feature:\n");
	for (int i = 0; i < FACE_RECOGNITION_FEATURE_DIMENSION; i++)
		printf("%f,", ((float *)dst_virt)i]);
	printf("\n");
*/

#ifdef __cplusplus
}
#endif

#endif
