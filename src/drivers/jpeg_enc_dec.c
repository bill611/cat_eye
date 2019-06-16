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



//NV21->YUV420P
static void yuv420sp_to_yuv420p(unsigned char* yuv420sp, unsigned char* yuv420p, int width, int height)  
{  
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
    for (j = 0, i = 0; j < y_size/2; j+=2, i++)  
    {  
        v_tmp[i] = uv[j];  
        u_tmp[i] = uv[j+1];  
    }  
}


static int yuv420p_to_yuv420sp(unsigned char * yuv420p,unsigned char* yuv420sp,int width,int height)
{
    if(yuv420p==NULL)
        return -1;
    int i=0,j=0;
    //Y
    for(i=0;i<width*height;i++)
    {
        yuv420sp[i]=yuv420p[i];
    }
 
    int m=0,n=0;
    for(int j=0;j<width*height/2;j++)
    {
        if(j%2==0)
           yuv420sp[j+width*height]=yuv420p[m++];
        else
           yuv420sp[j+width*height]=yuv420p[n++];
    }
	printf("convert finish\n");
}
/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


/*
 * Sample routine for JPEG decompression.  We assume that the source file name
 * is passed in.  We want to return 1 on success, 0 on error.
 */


int jpegDec (char * data,int data_len,char *out_data,int *out_len)
{
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct my_error_mgr jerr;
	/* More stuff */
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */

	/* In this example we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		return 0;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_mem_src(&cinfo, data,data_len);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void) jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.txt for more info.
	 */
	/* 源信息 */
	printf("image_width = %d\n", cinfo.image_width);
	printf("image_height = %d\n", cinfo.image_height);
	printf("num_components = %d\n", cinfo.num_components);
	printf("int_color_space = %d\n", cinfo.jpeg_color_space);
	/* Step 4: set parameters for decompression */
	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */
	cinfo.out_color_space = JCS_YCbCr;
	/* Step 5: Start decompressor */

	(void) jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	printf("output_width = %d\n", cinfo.output_width);
	printf("output_height = %d\n", cinfo.output_height);
	printf("output_components = %d\n", cinfo.output_components);
	printf("output_color_space = %d\n", cinfo.out_color_space);
	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */ 
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */
		memcpy(&out_data[cinfo.output_scanline * row_stride],buffer,row_stride);
		*out_len += row_stride;
		// put_scanline_someplace(buffer[0], row_stride);
	}
	printf("out_len:%d\n", *out_len);
	/* Step 7: Finish decompression */

	(void) jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	 * Here we postpone it until after no more JPEG errors are possible,
	 * so as to simplify the setjmp error logic above.  (Actually, I don't
	 * think that jpeg_destroy can do an error exit, but why assume anything...)
	 */

	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	/* And we're done! */
	return 1;
}

int yuv420p_to_jpeg(const char * filename, const char* pdata,int image_width,int image_height, int quality)
{   
    struct jpeg_compress_struct cinfo;  
    struct jpeg_error_mgr jerr;  
    cinfo.err = jpeg_std_error(&jerr);  
    jpeg_create_compress(&cinfo);  
 
    FILE * outfile;    // target file  
    if ((outfile = fopen(filename, "wb")) == NULL) {  
        fprintf(stderr, "can't open %s\n", filename);  
        return -1;  
    }  
    jpeg_stdio_dest(&cinfo, outfile);  
 
    cinfo.image_width = image_width;  // image width and height, in pixels  
    cinfo.image_height = image_height;  
    cinfo.input_components = 3;    // # of color components per pixel  
    cinfo.in_color_space = JCS_YCbCr;  //colorspace of input image  
    jpeg_set_defaults(&cinfo);  
    jpeg_set_quality(&cinfo, quality, TRUE );  
 
    //////////////////////////////  
    //  cinfo.raw_data_in = TRUE;  
    cinfo.jpeg_color_space = JCS_YCbCr;  
    cinfo.comp_info[0].h_samp_factor = 2;  
    cinfo.comp_info[0].v_samp_factor = 2;  
    /////////////////////////  
 
    jpeg_start_compress(&cinfo, TRUE);  
 
    JSAMPROW row_pointer[1];
 
    unsigned char *yuvbuf;
    if((yuvbuf=(unsigned char *)malloc(image_width*3))!=NULL)
        memset(yuvbuf,0,image_width*3);
 
    unsigned char *ybase,*ubase;
    ybase=pdata;
    ubase=pdata+image_width*image_height;  
    int j=0;
    while (cinfo.next_scanline < cinfo.image_height) 
    {
        int idx=0;
        for(int i=0;i<image_width;i++)
        { 
            yuvbuf[idx++]=ybase[i + j * image_width];
            yuvbuf[idx++]=ubase[j/2 * image_width+(i/2)*2];
            yuvbuf[idx++]=ubase[j/2 * image_width+(i/2)*2+1];    
        }
        row_pointer[0] = yuvbuf;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
        j++;
    }
	free(yuvbuf);
    jpeg_finish_compress(&cinfo);  
    jpeg_destroy_compress(&cinfo);  
    fclose(outfile);  
    return 0;  
}

int tjpeg2yuv(unsigned char* jpeg_buffer, int jpeg_size, unsigned char* yuv_buffer, int* yuv_size, int* yuv_type)
{
	tjhandle handle = NULL;
	int width, height, subsample, colorspace;
	int flags = 0;
	int padding = 1; // 1或4均可，但不能是0
	int ret = 0;

	handle = tjInitDecompress();
	if (handle == NULL) {
		printf("---------------%s\n", tjGetErrorStr() );
		return -1;
	}
	tjDecompressHeader3(handle, jpeg_buffer, jpeg_size, &width, &height, &subsample, &colorspace);

	printf("w: %d h: %d subsample: %d color: %d\n", width, height, subsample, colorspace);

	flags |= 0;

	*yuv_type = subsample;
	// 注：经测试，指定的yuv采样格式只对YUV缓冲区大小有影响，实际上还是按JPEG本身的YUV格式来转换的
	*yuv_size = tjBufSizeYUV2(width, padding, height, subsample);

	unsigned char *yuv_buffer_420p =(unsigned char *)malloc(*yuv_size);
	if (yuv_buffer_420p == NULL)
	{
		printf("malloc buffer for rgb failed.\n");
		return -1;
	}

	ret = tjDecompressToYUV2(handle, jpeg_buffer, jpeg_size, yuv_buffer_420p, width,
			padding, height, flags);
	if (ret < 0)
	{
		printf("compress to jpeg failed: %s\n", tjGetErrorStr());
	}
	yuv420p_to_yuv420sp(yuv_buffer_420p,yuv_buffer,width,height);
	free(yuv_buffer_420p);
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
