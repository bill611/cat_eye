/*
 * =============================================================================
 *
 *       Filename:  jpeg_enc_dec.c
 *
 *    Description:  jpeg编解码
 *
 *        Version:  1.0
 *        Created:  2019-06-15 20:45:18
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
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "debug.h"
#include "jpeglib.h"
#include "turbojpeg.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/**
 * @brief yuv420spToYuv420p NV12转YUV420P
 *
 * @param yuv420sp
 * @param yuv420p
 * @param width
 * @param height
 * nv12(yuv420sp) 4x4        yuv420p 4x4
 *        YYYY                YYYY 
 *        YYYY                YYYY
 *        YYYY                YYYY
 *        YYYY                YYYY
 *        UVUV                UUUU
 *        UVUV                VVVV
 */
/* ---------------------------------------------------------------------------*/
static void yuv420spToYuv420p(unsigned char* yuv420sp, unsigned char* yuv420p, int width, int height)
{
    if (yuv420sp == NULL || yuv420p == NULL)
        return;
    int i, j;
    int y_size = width * height;

    unsigned char* y = yuv420sp;
    unsigned char* uv = yuv420sp + y_size;

    unsigned char* y_tmp = yuv420p;
    unsigned char* u_tmp = yuv420p + y_size;
    unsigned char* v_tmp = yuv420p + y_size * 5 / 4;

    // y
    memcpy(y_tmp, y, y_size);

    // u
    for (j = 0, i = 0; j < y_size/2; j+=2, i++) {
        u_tmp[i] = uv[j];
        v_tmp[i] = uv[j+1];
    }
}


/* ---------------------------------------------------------------------------*/
/**
 * @brief yuv420pToYuv420sp 
 *
 * @param yuv420p
 * @param yuv420sp
 * @param width
 * @param height
 *
 * nv12(yuv420sp) 4x4        yuv420p 4x4
 *        YYYY                YYYY 
 *        YYYY                YYYY
 *        YYYY                YYYY
 *        YYYY                YYYY
 *        UVUV                UUUU
 *        UVUV                VVVV
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static int yuv420pToYuv420sp(unsigned char * yuv420p,unsigned char* yuv420sp,int width,int height)
{
    if (yuv420sp == NULL || yuv420p == NULL)
        return -1;
    
    int y_size = width * height;
    unsigned char* y = yuv420p;
    unsigned char* u = yuv420p + y_size;
    unsigned char* v = yuv420p + y_size * 5 / 4;

    unsigned char* y_tmp = yuv420sp;
    unsigned char* uv_tmp = yuv420sp + y_size;
    //Y
    memcpy(y_tmp, y, y_size);

    int i;
    for (i=0; i<y_size/2; i++) {
        if(i%2 == 0) {
           *uv_tmp=*u;
		   u++;
        } else {
           *uv_tmp=*v;
		   v++;
        }
		uv_tmp++;
    }
	printf("convert finish\n");
    return 0;
}
/*
 * Here's the routine that will replace the standard error_exit method:
 */

/* ---------------------------------------------------------------------------*/
/**
 * @brief yuv420spToJpeg 
 *
 * @param yuv_buffer
 * @param width
 * @param height
 * @param jpeg_buffer
 * @param jpeg_size
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
int yuv420spToJpeg(unsigned char* yuv_buffer,  int width, int height, 
                             unsigned char** jpeg_buffer, int* jpeg_size)
{
    tjhandle handle = NULL;
	unsigned char *yuv_buffer_420p;
    int quality = 50; // 编码质量50(1-100)
    int flags = 0;
    int padding = 1; // 1或4均可，但不能是0
    int need_size = 0;
    int ret = -1;

	if (!yuv_buffer)
		goto encode_err;

    int yuv_size = width * height * 3/2;
    handle = tjInitCompress();
	if (handle == NULL) {
		goto encode_err;
	}

    need_size = tjBufSizeYUV2(width, padding, height,TJSAMP_420);
    if (need_size != yuv_size) {
        printf("we detect yuv size: %d, but you give: %d, check again.\n", need_size, yuv_size);
		goto encode_end;
    }
	yuv_buffer_420p = (unsigned char *)calloc(width*height*3/2,1);
	yuv420spToYuv420p(yuv_buffer,yuv_buffer_420p,width,height);
    ret = tjCompressFromYUV(handle, yuv_buffer_420p, width, padding, height, 
            TJSAMP_420, jpeg_buffer, (unsigned long *)jpeg_size, quality, flags);
    if (ret < 0) {
        printf("compress to jpeg failed: %s\n", tjGetErrorStr());
    }

encode_end:
	tjDestroy(handle);
	if(yuv_buffer_420p)
		free(yuv_buffer_420p);
encode_err:
    return ret;
}

int jpegToYuv420sp(unsigned char* jpeg_buffer, 
		int jpeg_size, 
		int *out_width, int *out_height,
		unsigned char** yuv_buffer, int* yuv_size )
{
    tjhandle handle = NULL;
    int width, height, subsample, colorspace;
    int flags = 0;
    int padding = 1; // 1或4均可，但不能是0
    int ret = -1;

	if (!jpeg_buffer)
		return -1;
    handle = tjInitDecompress();
    if (handle == NULL) {
        printf("%s():%s\n",__func__,tjGetErrorStr() );
        goto jpeg_to_yuv_end;
    }
    tjDecompressHeader3(handle, jpeg_buffer, jpeg_size, &width, &height, &subsample, &colorspace);

	*out_width = width;
	*out_height = height;
    DPRINT("w: %d h: %d subsample: %d color: %d\n", width, height, subsample, colorspace);
	if (width < 0 || width > 1280 || height < 0 ||height > 720) {
        goto jpeg_to_yuv_end;
	}

    flags |= 0;

    // 注：经测试，指定的yuv采样格式只对YUV缓冲区大小有影响，实际上还是按JPEG本身的YUV格式来转换的
    *yuv_size = tjBufSizeYUV2(width, padding, height, subsample);

    unsigned char *yuv_buffer_420p =(unsigned char *)malloc(*yuv_size);
    if (yuv_buffer_420p == NULL) {
        printf("malloc buffer for rgb failed.\n");
        goto jpeg_to_yuv_end;
    }

    ret = tjDecompressToYUV2(handle, jpeg_buffer, jpeg_size, yuv_buffer_420p, width,
            padding, height, flags);
    if (ret < 0) {
        printf("compress to jpeg failed: %s\n", tjGetErrorStr());
    }
     *yuv_buffer =(unsigned char *)malloc(*yuv_size);
    yuv420pToYuv420sp(yuv_buffer_420p,*yuv_buffer,width,height);
    free(yuv_buffer_420p);

jpeg_to_yuv_end:
    tjDestroy(handle);

    return ret;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief jpegIncDecInit 必须要在c函数中有调用，否则cpp中提示找不到
 */
/* ---------------------------------------------------------------------------*/
void jpegIncDecInit(void)
{

}
