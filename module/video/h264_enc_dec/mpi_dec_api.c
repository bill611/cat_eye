/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <mpp/rk_mpi.h>

#include <mpp/mpp_log.h>
#include <mpi/mpp_mem.h>
#include <mpi/mpp_env.h>
#include <mpi/mpp_time.h>
#include <mpi/mpp_common.h>

#include "utils.h"
#include "mpi_dec_api.h"

#define MPI_DEC_LOOP_COUNT          4
#define MPI_DEC_STREAM_SIZE         1024*600*3
#define MAX_FILE_NAME_LENGTH        256

typedef struct {
    MppApi          *mpi;

    /* end of stream flag when set quit the loop */
    RK_U32          eos;

    /* buffer for stream data reading */
    char            *buf;

    /* input and output */
    MppBufferGroup  frm_grp;
    MppPacket       packet;
    MppFrame        frame;

    RK_S32          frame_count;
    size_t          max_usage;
} MpiDecLoopData;

typedef struct {
    MppCodingType   type;
    RK_U32          width;
    RK_U32          height;
    RK_U32          debug;

    RK_S32          timeout;
    size_t          pkt_size;

    // report information
    size_t          max_usage;
} MpiDecTestCmd;

H264Decode *my_h264dec;
static MpiDecTestCmd  cmd_ctx;
// base flow context
static MppCtx ctx          = NULL;
static MppApi *mpi         = NULL;
// input / output
static MppPacket packet    = NULL;
static MppFrame  frame     = NULL;
static char *buf           = NULL;
static MpiDecLoopData data;

static int dump_mpp_frame_to_buf(MppFrame frame, unsigned char *out_data)
{
    RK_U32 width    = 0;
    RK_U32 height   = 0;
    RK_U32 h_stride = 0;
    RK_U32 v_stride = 0;
    MppFrameFormat fmt  = MPP_FMT_YUV420SP;
    MppBuffer buffer    = NULL;
    RK_U8 *base = NULL;

    if (NULL == frame)
        return 0;

    width    = mpp_frame_get_width(frame);
    height   = mpp_frame_get_height(frame);
    h_stride = mpp_frame_get_hor_stride(frame);
    v_stride = mpp_frame_get_ver_stride(frame);
    fmt      = mpp_frame_get_fmt(frame);
    buffer   = mpp_frame_get_buffer(frame);

    if (NULL == buffer)
        return 0;

    base = (RK_U8 *)mpp_buffer_get_ptr(buffer);

	switch (fmt) {
		case MPP_FMT_YUV422SP : 
			{
				/* YUV422SP -> YUV422P for better display */
				RK_U32 i, j;
				RK_U8 *base_y = base;
				RK_U8 *base_c = base + h_stride * v_stride;
				RK_U8 *tmp = mpp_malloc(RK_U8, h_stride * height * 2);
				RK_U8 *tmp_u = tmp;
				RK_U8 *tmp_v = tmp + width * height / 2;

				int index = 0;
				for (i = 0; i < height; i++, base_y += h_stride) {
					memcpy(&out_data[index],base_y,width);
					index += width;
					// fwrite(base_y, 1, width, fp);
				}

				for (i = 0; i < height; i++, base_c += h_stride) {
					for (j = 0; j < width / 2; j++) {
						tmp_u[j] = base_c[2 * j + 0];
						tmp_v[j] = base_c[2 * j + 1];
					}
					tmp_u += width / 2;
					tmp_v += width / 2;
				}

				memcpy(&out_data[index],tmp,width * height);
				index += width * height;
				mpp_free(tmp);
				return index;
			} break;
		case MPP_FMT_YUV420SP : 
			{
				int index = 0;
				RK_U32 i;
				RK_U8 *base_y = base;
				RK_U8 *base_c = base + h_stride * v_stride;

				for (i = 0; i < height; i++, base_y += h_stride) {
					// fwrite(base_y, 1, width, fp);
					memcpy(&out_data[index],base_y,width);
					index += width;
				}
				for (i = 0; i < height / 2; i++, base_c += h_stride) {
					// fwrite(base_c, 1, width, fp);
					memcpy(&out_data[index],base_c,width);
					index += width;
				}
				return index;
			} break;
		case MPP_FMT_YUV444SP : 
			{
				/* YUV444SP -> YUV444P for better display */
				int index = 0;
				RK_U32 i, j;
				RK_U8 *base_y = base;
				RK_U8 *base_c = base + h_stride * v_stride;
				RK_U8 *tmp = mpp_malloc(RK_U8, h_stride * height * 2);
				RK_U8 *tmp_u = tmp;
				RK_U8 *tmp_v = tmp + width * height;

				for (i = 0; i < height; i++, base_y += h_stride) {
					memcpy(&out_data[index],base_y,width);
					index += width;
				}

				for (i = 0; i < height; i++, base_c += h_stride * 2) {
					for (j = 0; j < width; j++) {
						tmp_u[j] = base_c[2 * j + 0];
						tmp_v[j] = base_c[2 * j + 1];
					}
					tmp_u += width;
					tmp_v += width;
				}

				memcpy(&out_data[index],tmp,width * height*2);
				// fwrite(tmp, 1, width * height * 2, fp);
				mpp_free(tmp);
				index += width * height;
				return index;
			} break;
		default : 
			{
				mpp_err("not supported format %d\n", fmt);
			} break;
	}
}
static int decode_simple(MpiDecLoopData *data,unsigned char *in_data,size_t read_size,unsigned char *out_data,int *out_w,int *out_h)
{
    RK_U32 pkt_done = 0;
    RK_U32 err_info = 0;
    MPP_RET ret = MPP_OK;
	RK_U32 buf_size = 0 ;
	int out_size = 0;

    // write data to packet
    mpp_packet_write(packet, 0, in_data, read_size);
    // reset pos and set valid length
	// mpp_packet_set_pos(packet, in_data);
    mpp_packet_set_length(packet, read_size);

    do {
        RK_S32 times = 5;
        // send the packet first if packet is not done
        if (!pkt_done) {
            ret = mpi->decode_put_packet(ctx, packet);
            if (MPP_OK == ret)
                pkt_done = 1;
        }

        // then get all available frame and release
		do {
			RK_S32 get_frm = 0;
			RK_U32 frm_eos = 0;

try_again:
			ret = mpi->decode_get_frame(ctx, &frame);
			if (MPP_ERR_TIMEOUT == ret) {
				if (times > 0) {
					times--;
					msleep(2);
					goto try_again;
				}
				printf("decode_get_frame failed too much time\n");
			}
			if (MPP_OK != ret) {
				printf("decode_get_frame failed ret %d\n", ret);
				break;
			}

			if (frame) {
				if (mpp_frame_get_info_change(frame)) {
					*out_w = mpp_frame_get_width(frame);
					*out_h = mpp_frame_get_height(frame);
					RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
					RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
					MppFrameFormat fmt      = mpp_frame_get_fmt(frame);
					buf_size = mpp_frame_get_buf_size(frame);

					printf("decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d[%d]\n",
							*out_w, *out_h, hor_stride, ver_stride, buf_size,fmt);

					if (NULL == data->frm_grp) {
						/* If buffer group is not set create one and limit it */
						ret = mpp_buffer_group_get_internal(&data->frm_grp, MPP_BUFFER_TYPE_ION);
						if (ret) {
							printf("get mpp buffer group failed ret %d\n", ret);
							break;
						}

						/* Set buffer to mpp decoder */
						ret = mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, data->frm_grp);
						if (ret) {
							printf("set buffer group failed ret %d\n", ret);
							break;
						}
					} else {
						/* If old buffer group exist clear it */
						ret = mpp_buffer_group_clear(data->frm_grp);
						if (ret) {
							printf("clear buffer group failed ret %d\n", ret);
							break;
						}
					}

					/* Use limit config to limit buffer count to 24 with buf_size */
					ret = mpp_buffer_group_limit_config(data->frm_grp, buf_size, 24);
					if (ret) {
						printf("limit buffer group failed ret %d\n", ret);
						break;
					}

					/*
					 * All buffer group config done. Set info change ready to let
					 * decoder continue decoding
					 */
					ret = mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
					if (ret) {
						printf("info change ready failed ret %d\n", ret);
						break;
					}
				} else {
					err_info = mpp_frame_get_errinfo(frame) | mpp_frame_get_discard(frame);
					if (err_info) {
						// printf("decoder_get_frame get err info:%d discard:%d.\n",
								// mpp_frame_get_errinfo(frame), mpp_frame_get_discard(frame));
					}
					// buf_size = mpp_frame_get_buf_size(frame);
					data->frame_count++;
					out_size =	dump_mpp_frame_to_buf(frame,out_data);
					// printf("decode_get_frame get frame %d,size:%d\n" ,data->frame_count,out_size);
				}
				frm_eos = mpp_frame_get_eos(frame);
				mpp_frame_deinit(&frame);
				frame = NULL;
				get_frm = 1;
			}

			// try get runtime frame memory usage
			if (data->frm_grp) {
				size_t usage = mpp_buffer_group_usage(data->frm_grp);
				if (usage > data->max_usage)
					data->max_usage = usage;
			}

			// if last packet is send but last frame is not found continue
			// if (pkt_done && !frm_eos) {
				// msleep(10);
				// continue;
			// }

			if (frm_eos) {
				printf("found last frame\n");
				break;
			}

			// if (data->frame_num > 0 && data->frame_count >= data->frame_num) {
			// data->eos = 1;
			// break;
			// }

			// if (get_frm)
				// continue;
			break;
		} while (1);

        // if (data->frame_num > 0 && data->frame_count >= data->frame_num) {
            // data->eos = 1;
            // printf("reach max frame number %d\n", data->frame_count);
            // break;
        // }

        if (pkt_done)
            break;

        /*
         * why sleep here:
         * mpi->decode_put_packet will failed when packet in internal queue is
         * full,waiting the package is consumed .Usually hardware decode one
         * frame which resolution is 1080p needs 2 ms,so here we sleep 3ms
         * * is enough.
         */
        msleep(3);
    } while (1);

    return out_size;
}


static int mpiH264Decode(H264Decode *This,unsigned char *in_data,int in_size,unsigned char *out_data,int *out_w,int *out_h)
{
	memset(buf,0,MPI_DEC_STREAM_SIZE);
	return decode_simple(&data,in_data,in_size,out_data,out_w,out_h);
}

static int mpiH264DecInit(H264Decode *This,int width,int height)
{
    MPP_RET ret         = MPP_OK;
    RK_U32 need_split   = 1;

    memset(&data, 0, sizeof(data));
    data.mpi            = mpi;
    data.eos            = 0;
    data.packet         = packet;
    data.frame          = frame;
    data.frame_count    = 0;

	buf = mpp_malloc(char, MPI_DEC_STREAM_SIZE);
	if (NULL == buf) {
		printf("mpi_dec_test malloc input stream buffer failed\n");
		return -1;
	}
	ret = mpp_packet_init(&packet, buf, MPI_DEC_STREAM_SIZE);

	if (ret) {
		printf("mpp_packet_init failed\n");
		return -1;
	}

    ret = mpp_create(&ctx, &mpi);
    if (MPP_OK != ret) {
        printf("mpp_create failed\n");
		return -1;
    }
    // NOTE: decoder split mode need to be set before init
    MppParam param = &need_split;
    ret = mpi->control(ctx, MPP_DEC_SET_PARSER_SPLIT_MODE, param);
    if (MPP_OK != ret) {
        printf("mpi->control failed\n");
		mpp_destroy(ctx);
		return -1;
    }
    ret = mpp_init(ctx, MPP_CTX_DEC, MPP_VIDEO_CodingAVC);
    if (MPP_OK != ret) {
        printf("mpp_init failed\n");
		mpp_destroy(ctx);
        return -1;
    }
	return 0;
}

static int mpiH264DecUnInit(H264Decode *This)
{
    MPP_RET ret = mpi->reset(ctx);
    if (MPP_OK != ret) {
        printf("mpi->reset failed\n");
        return -1;
    }
    if (packet) {
        mpp_packet_deinit(&packet);
        packet = NULL;
    }

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }

	if (buf) {
		mpp_free(buf);
		buf = NULL;
	}
    if (data.frm_grp) {
        mpp_buffer_group_put(data.frm_grp);
        data.frm_grp = NULL;
    }

	return 0;
}
void myH264DecInit(void)
{
	my_h264dec = (H264Decode *)calloc(1,sizeof(H264Decode));
	// my_h264dec->priv = (H264DecodePriv *)calloc(1,sizeof(H264DecodePriv));
	my_h264dec->init = mpiH264DecInit;
	my_h264dec->unInit = mpiH264DecUnInit;
	my_h264dec->decode = mpiH264Decode;
}
