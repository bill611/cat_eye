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

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
