/*
 * =============================================================================
 *
 *       Filename:  mpi_enc_api.h
 *
 *    Description:  mpp api h264编码
 *
 *        Version:  1.0
 *        Created:  2019-06-20 15:51:26 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MPI_ENC_API_H
#define _MPI_ENC_API_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
	struct _H264EncodePriv;
	typedef struct _H264Encode {
		struct _H264EncodePriv *priv;
		void (*init)(struct _H264Encode *This,int w,int h);
		void (*unInit)(struct _H264Encode *This);
	}H264Encode;
	H264Encode *my_h264enc;
	void myH264EncInit(void);
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
