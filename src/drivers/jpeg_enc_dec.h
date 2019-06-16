/*
 * =============================================================================
 *
 *       Filename:  jpeg_enc_dec.h
 *
 *    Description:  jpeg编解码，jpeg9.0版本
 *
 *        Version:  1.0
 *        Created:  2019-06-15 20:45:37 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _JPEG_ENC_DEC_H
#define _JPEG_ENC_DEC_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


	int jpegDec (char * data,int data_len,char *out_data,int *out_len);
	int yuv420p_to_jpeg(const char * filename, const char* pdata,int image_width,int image_height, int quality);
	void jpegIncDecInit(void);
	int tjpeg2yuv(unsigned char* jpeg_buffer, int jpeg_size, unsigned char* yuv_buffer, int* yuv_size, int* yuv_type);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
