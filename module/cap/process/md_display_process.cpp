#include "md_display_process.h"
#include "thread_helper.h"
#include "h264_enc_dec/mpi_dec_api.h"
#include "jpeg_enc_dec.h"
#include "libyuv.h"

static FILE *fp = NULL;

DisplayProcess::DisplayProcess()
     : StreamPUBase("DisplayProcess", true, true)
{
}

DisplayProcess::~DisplayProcess()
{
}

bool DisplayProcess::processFrame(std::shared_ptr<BufferBase> inBuf,
                                        std::shared_ptr<BufferBase> outBuf)
{
	unsigned char *data = (unsigned char *)inBuf->getVirtAddr();
	if (fp == NULL)
		return true;
	unsigned char *jpeg_buf = NULL;
	int size = 0;
	yuv420spToJpeg(data,1280,720,&jpeg_buf,&size);
	if (jpeg_buf) {
		fwrite(jpeg_buf,1,size,fp);
		fflush(fp);
		fclose(fp);
		free(jpeg_buf);
	}
	fp = NULL;

    return true;
}


void DisplayProcess::capture(char *file_name)
{
	while (fp != NULL) {
		usleep(10000);	
	}
	fp = fopen(file_name,"wb");
}
