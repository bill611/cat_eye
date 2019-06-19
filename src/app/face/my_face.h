/*
 * =============================================================================
 *
 *       Filename:  my_face.h
 *
 *    Description:  人脸识别算法接口
 *
 *        Version:  1.0
 *        Created:  2019-06-17 15:26:22 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MY_FACE_H
#define _MY_FACE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	typedef struct _MyFace{
		int (*deleteOne)(char *id);
		int (*regist)(unsigned char *image_buff,int w,int h,char *id,char *nick_name,char *url);
		void (*recognizer)(char *image_buff,int w,int h);
		void (*uninit)(void);
	}MyFace;

	extern MyFace *my_face;
	void myFaceInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
