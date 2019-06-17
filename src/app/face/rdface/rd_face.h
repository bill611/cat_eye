/*
 * =============================================================================
 *
 *       Filename:  rd_face.h
 *
 *    Description:  阅面人脸识别算法接口
 *
 *        Version:  1.0
 *        Created:  2019-06-17 17:13:03 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _RD_FACE_H
#define _RD_FACE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


	int rdfaceInit(void);
	void rdfaceUninit(void);
	int rdfaceRegist(unsigned char *image_buff,int w,int h,float **out_feature);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
