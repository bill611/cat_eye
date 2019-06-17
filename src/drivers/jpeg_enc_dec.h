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


	void jpegIncDecInit(void);

	int jpegToYuv420sp(unsigned char* jpeg_buffer, int jpeg_size, unsigned char* yuv_buffer, int* yuv_size, int* yuv_type);
	int yuv420spToJpeg(unsigned char* yuv_buffer,  int width, int height, 
			unsigned char** jpeg_buffer, int* jpeg_size);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
