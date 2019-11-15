/*
 * Copyright (C) 2017 Hertz Wang 1989wanghang@163.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses
 *
 * Any non-GPL usage of this software or parts of this software is strictly
 * forbidden.
 *
 */

#ifdef NDEBUG
#undef NDEBUG
#endif
#ifndef DEBUG
#define DEBUG
#endif

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <easymedia/buffer.h>
#include <easymedia/key_string.h>
#include <easymedia/media_config.h>
#include <easymedia/utils.h>

#include <easymedia/decoder.h>
#include "mpi_dec_api.h"

struct SpsPpsTemp {
	char buf[64];
	int size;
};

extern "C" {
int split_h264_separate(const uint8_t *buffer, size_t length,int *sps_length,int *pps_length) ;
}
static std::shared_ptr<easymedia::VideoDecoder> g_mpp_dec;
static struct SpsPpsTemp sps_pps_tmp;
static char h264_sps_pps[64];
static std::string param;
static std::string mpp_codec = "rkmpp";

static void dump_output(const std::shared_ptr<easymedia::MediaBuffer> &out,int *out_size,unsigned char *out_data,int *out_w,int *out_h) 
{
	auto out_image = std::static_pointer_cast<easymedia::ImageBuffer>(out);
	// hardware always need aligh width/height, we write the whole buffer with
	// virtual region which may contains invalid data
	// printf("out:%d\n",out_image->GetValidSize() );
	if (out_image->GetValidSize() <= 0)
		return;
	const ImageInfo &info = out_image->GetImageInfo();
	// fprintf(stderr, "got one frame, format: %s <%dx%d>in<%dx%d>\n",
			// PixFmtToString(info.pix_fmt), info.width, info.height,
			// info.vir_width, info.vir_height);
	*out_size = CalPixFmtSize(out_image->GetPixelFormat(), out_image->GetVirWidth(),
			out_image->GetVirHeight());
	*out_w = info.vir_width;
	*out_h = info.vir_height;
	if (*out_size) 
		memcpy(out_data,out_image->GetPtr(),*out_size);
}

static bool get_output_and_process(easymedia::VideoDecoder *mpp_dec,int *out_size,unsigned char *out_data,int *out_w,int *out_h) 
{
	auto out_buffer = mpp_dec->FetchOutput();
	if (!out_buffer && errno != 0) {
		fprintf(stderr, "fatal error %m\n");
		return false;
	}
  if (out_buffer && !out_buffer->IsValid() && !out_buffer->IsEOF()) {
    // got a image info buffer
    // fprintf(stderr, "got info frame\n");
    // fetch the real frame
    out_buffer = mpp_dec->FetchOutput();
  }
	if (out_buffer) {
		dump_output(std::static_pointer_cast<easymedia::ImageBuffer>(out_buffer),out_size,out_data,out_w,out_h);
	}
	return true;
}

static int mpiH264Decode(H264Decode *This,unsigned char *in_data,int in_size,unsigned char *out_data,int *out_w,int *out_h)
{
	int sps_length = 0,pps_length = 0;
	unsigned char *i_frame = NULL;
	unsigned char *frame = in_data;
	int frame_size = in_size;
	if (in_data[0] == 0 && in_data[1] == 0 && in_data[2] == 0 && in_data[3] == 1
			&& in_data[4] == 0x6 && in_data[5] == 0x5) {
		return 0;
	}
	int frame_type = split_h264_separate(in_data,in_size,&sps_length,&pps_length);
	if (frame_type == 7 || frame_type == 8) {
		if (frame_type == 7) {
			if (memcmp(h264_sps_pps,in_data,sps_length)) {
				my_h264dec->unInit(my_h264dec);
				my_h264dec->init(my_h264dec,1024,600);
				memcpy(h264_sps_pps,in_data,sps_length);
			}
		}
		memcpy(&sps_pps_tmp.buf[sps_pps_tmp.size],in_data,in_size);
		sps_pps_tmp.size += in_size;
		return 0;
	}

	if (sps_pps_tmp.size) {
		i_frame = (unsigned char *) calloc(1,in_size + sps_pps_tmp.size);
		memcpy(i_frame,sps_pps_tmp.buf,sps_pps_tmp.size);
		memcpy(i_frame + sps_pps_tmp.size,in_data,in_size);
		frame = i_frame;
		frame_size += sps_pps_tmp.size;
	}
	
	// printf("size:%d\n",frame_size );
	memset(&sps_pps_tmp,0,sizeof(sps_pps_tmp));
	std::shared_ptr<easymedia::MediaBuffer> buffer;
	buffer = easymedia::MediaBuffer::Alloc(frame_size);
	assert(buffer);
	memcpy(buffer->GetPtr(),frame,frame_size);
	buffer->SetValidSize(frame_size);
	buffer->SetUSTimeStamp(easymedia::gettimeofday());
	int ret = g_mpp_dec->SendInput(buffer);
	get_output_and_process(g_mpp_dec.get(),&ret,out_data,out_w,out_h);
	if (i_frame)
		free(i_frame);
	return ret;

}

static int mpiH264DecInit(H264Decode *This,int width,int height)
{
	printf("[%s]\n", __func__);
	g_mpp_dec = easymedia::REFLECTOR(Decoder)::Create<easymedia::VideoDecoder>(
			mpp_codec.c_str(), param.c_str());
	if (!g_mpp_dec) {
		fprintf(stderr, "Create decoder %s failed\n", mpp_codec.c_str());
		return -1;
	}
	return 0;
}

static int mpiH264DecUnInit(H264Decode *This)
{
	printf("[%s]\n", __func__);
	g_mpp_dec = NULL;
	memset(h264_sps_pps,0,sizeof(h264_sps_pps));
	return 0;
}
#if 1
void myH264DecInit(void)
{
	int split_mode = 0;//= input_dir_path.empty() ? 1 : 0;
	int timeout = -1;
	my_h264dec = (H264Decode *)calloc(1,sizeof(H264Decode));
	my_h264dec->init = mpiH264DecInit;
	my_h264dec->unInit = mpiH264DecUnInit;
	my_h264dec->decode = mpiH264Decode;

	PARAM_STRING_APPEND(param, KEY_INPUTDATATYPE, VIDEO_H264);
	// PARAM_STRING_APPEND_TO(param, KEY_MPP_GROUP_MAX_FRAMES, 4);

	PARAM_STRING_APPEND_TO(param, KEY_MPP_SPLIT_MODE, split_mode);
	PARAM_STRING_APPEND_TO(param, KEY_OUTPUT_TIMEOUT, timeout);

	easymedia::REFLECTOR(Decoder)::DumpFactories();
}
#endif
