//--------------------------------------------------------------
//
// Copyright (c) Nuvoton Technology Corp. All rights reserved.
//
//--------------------------------------------------------------

#ifndef _RV1108GPIO_H__
#define _RV1108GPIO_H__

#include <linux/ioctl.h>
#include <linux/types.h>

#ifdef	__cplusplus
extern "C"
{
#endif

// TO BE CONTINUED
//typedef struct {
//   int32_t *inbuf;
//   int32_t *outbuf;
//   int32_t i32Size;
//} aac_enc_ctx_s;
// For AAC encoder, pass the parameters

// TO BE CONTINUED
//typedef struct {
//   int32_t i32Size;
//   int32_t *inbuf;
//   int32_t *outbuf;
//} aac_dec_ctx_s;
// For AAC decoder, pass the parameters

#define GPIO_IOC_MAGIC 'h'
#define GPIO_IOC_MAXNR 100

/* ===========================================================
	S means "Set" through a ptr
	T means Tell" directly with the argument value
	G means "Get": reply by setting through a pointer
	Q means "Query": response is on the return value
	X means "eXchange": switch G and S atomically
	H means "sHift": switch T and Q atomically
  ============================================================ */

#define RV1108_IOCINIT			_IOW(GPIO_IOC_MAGIC, 1, unsigned int)
#define RV1108_IOCREAD			_IOR(GPIO_IOC_MAGIC, 2, unsigned int)
#define RV1108_IOCGPIO_CTRL	_IOW(GPIO_IOC_MAGIC, 3, unsigned int)
#define RV1108_IOCGPIO_GET		_IOR(GPIO_IOC_MAGIC, 4, unsigned int)
//#define RV1108_IOC_WATCHDOG	_IOR(GPIO_IOC_MAGIC, 5, unsigned int)
//#define RV1108_IOC_RD_IMEI		_IOR(GPIO_IOC_MAGIC, 6, unsigned int)
//#define RV1108_IOC_WR_IMEI		_IOW(GPIO_IOC_MAGIC, 7, unsigned int)
//#define RV1108_IOC_NET_STATUS	_IOR(GPIO_IOC_MAGIC, 8, unsigned int)

#ifdef	__cplusplus
}
#endif

#endif

