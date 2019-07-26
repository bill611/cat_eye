/*
 * =============================================================================
 *
 *       Filename:  mpi_dec_api.h
 *
 *    Description:  mpp api h264解码
 *
 *        Version:  1.0
 *        Created:  2019-06-20 15:44:41 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MPI_DEC_API_H
#define _MPI_DEC_API_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
	struct _H264DecodePriv;
	typedef struct _H264Decode {
		struct _H264DecodePriv *priv;
		int (*decode)(struct _H264Decode *This,unsigned char *in_data,int in_size,unsigned char *out_data,int *out_w,int *out_h);
		int (*init)(struct _H264Decode *This,int w,int h);
		int (*unInit)(struct _H264Decode *This);
	}H264Decode;
	H264Decode *my_h264dec;
	void myH264DecInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
