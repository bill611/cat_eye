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
#define MPI_DEC_STREAM_SIZE         (SZ_4K)
#define MAX_FILE_NAME_LENGTH        256

typedef struct {
    MppCtx          ctx;
    MppApi          *mpi;

    /* end of stream flag when set quit the loop */
    RK_U32          eos;

    /* buffer for stream data reading */
    char            *buf;

    /* input and output */
    MppBufferGroup  frm_grp;
    MppPacket       packet;
    size_t          packet_size;
    MppFrame        frame;

    FILE            *fp_output;
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

static int decode_simple(MpiDecLoopData *data,size_t read_size,unsigned char *out_data)
{
    RK_U32 pkt_done = 0;
    RK_U32 err_info = 0;
    MPP_RET ret = MPP_OK;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    char   *buf = data->buf;
    MppPacket packet = data->packet;
    MppFrame  frame  = NULL;
    size_t packet_size = data->packet_size;

	printf("%s():%d\n", __func__,__LINE__);
    // write data to packet
    mpp_packet_write(packet, 0, buf, read_size);
	printf("%s():%d\n", __func__,__LINE__);
    // reset pos and set valid length
    // mpp_packet_set_pos(packet, buf);
	printf("%s():%d\n", __func__,__LINE__);
    mpp_packet_set_length(packet, read_size);

	printf("size:%d\n", read_size);
	printf("%s():%d\n", __func__,__LINE__);
    do {
        RK_S32 times = 5;
        // send the packet first if packet is not done
        if (!pkt_done) {
            ret = mpi->decode_put_packet(ctx, packet);
            if (MPP_OK == ret)
                pkt_done = 1;
        }
	printf("%s():%d\n", __func__,__LINE__);

        // then get all available frame and release
		do {
			RK_S32 get_frm = 0;
			RK_U32 frm_eos = 0;

try_again:
	printf("%s():%d\n", __func__,__LINE__);
			ret = mpi->decode_get_frame(ctx, &frame);
			if (MPP_ERR_TIMEOUT == ret) {
				if (times > 0) {
					times--;
					msleep(2);
					goto try_again;
				}
				mpp_err("decode_get_frame failed too much time\n");
			}
			if (MPP_OK != ret) {
				mpp_err("decode_get_frame failed ret %d\n", ret);
				break;
			}

	printf("%s():%d\n", __func__,__LINE__);
			if (frame) {
	printf("%s():%d\n", __func__,__LINE__);
				if (mpp_frame_get_info_change(frame)) {
					RK_U32 width = mpp_frame_get_width(frame);
					RK_U32 height = mpp_frame_get_height(frame);
					RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
					RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
					RK_U32 buf_size = mpp_frame_get_buf_size(frame);

					printf("decode_get_frame get info changed found\n");
					printf("decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d",
							width, height, hor_stride, ver_stride, buf_size);

					/*
					 * NOTE: We can choose decoder's buffer mode here.
					 * There are three mode that decoder can support:
					 *
					 * Mode 1: Pure internal mode
					 * In the mode user will NOT call MPP_DEC_SET_EXT_BUF_GROUP
					 * control to decoder. Only call MPP_DEC_SET_INFO_CHANGE_READY
					 * to let decoder go on. Then decoder will use create buffer
					 * internally and user need to release each frame they get.
					 *
					 * Advantage:
					 * Easy to use and get a demo quickly
					 * Disadvantage:
					 * 1. The buffer from decoder may not be return before
					 * decoder is close. So memroy leak or crash may happen.
					 * 2. The decoder memory usage can not be control. Decoder
					 * is on a free-to-run status and consume all memory it can
					 * get.
					 * 3. Difficult to implement zero-copy display path.
					 *
					 * Mode 2: Half internal mode
					 * This is the mode current test code using. User need to
					 * create MppBufferGroup according to the returned info
					 * change MppFrame. User can use mpp_buffer_group_limit_config
					 * function to limit decoder memory usage.
					 *
					 * Advantage:
					 * 1. Easy to use
					 * 2. User can release MppBufferGroup after decoder is closed.
					 *    So memory can stay longer safely.
					 * 3. Can limit the memory usage by mpp_buffer_group_limit_config
					 * Disadvantage:
					 * 1. The buffer limitation is still not accurate. Memory usage
					 * is 100% fixed.
					 * 2. Also difficult to implement zero-copy display path.
					 *
					 * Mode 3: Pure external mode
					 * In this mode use need to create empty MppBufferGroup and
					 * import memory from external allocator by file handle.
					 * On Android surfaceflinger will create buffer. Then
					 * mediaserver get the file handle from surfaceflinger and
					 * commit to decoder's MppBufferGroup.
					 *
					 * Advantage:
					 * 1. Most efficient way for zero-copy display
					 * Disadvantage:
					 * 1. Difficult to learn and use.
					 * 2. Player work flow may limit this usage.
					 * 3. May need a external parser to get the correct buffer
					 * size for the external allocator.
					 *
					 * The required buffer size caculation:
					 * hor_stride * ver_stride * 3 / 2 for pixel data
					 * hor_stride * ver_stride / 2 for extra info
					 * Total hor_stride * ver_stride * 2 will be enough.
					 *
					 * For H.264/H.265 20+ buffers will be enough.
					 * For other codec 10 buffers will be enough.
					 */

					if (NULL == data->frm_grp) {
						/* If buffer group is not set create one and limit it */
						ret = mpp_buffer_group_get_internal(&data->frm_grp, MPP_BUFFER_TYPE_ION);
						if (ret) {
							mpp_err("get mpp buffer group failed ret %d\n", ret);
							break;
						}

						/* Set buffer to mpp decoder */
						ret = mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, data->frm_grp);
						if (ret) {
							mpp_err("set buffer group failed ret %d\n", ret);
							break;
						}
					} else {
						/* If old buffer group exist clear it */
						ret = mpp_buffer_group_clear(data->frm_grp);
						if (ret) {
							mpp_err("clear buffer group failed ret %d\n", ret);
							break;
						}
					}

					/* Use limit config to limit buffer count to 24 with buf_size */
					ret = mpp_buffer_group_limit_config(data->frm_grp, buf_size, 24);
					if (ret) {
						mpp_err("limit buffer group failed ret %d\n", ret);
						break;
					}

					/*
					 * All buffer group config done. Set info change ready to let
					 * decoder continue decoding
					 */
					ret = mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
					if (ret) {
						mpp_err("info change ready failed ret %d\n", ret);
						break;
					}
				} else {
	printf("%s():%d\n", __func__,__LINE__);
					err_info = mpp_frame_get_errinfo(frame) | mpp_frame_get_discard(frame);
					if (err_info) {
						printf("decoder_get_frame get err info:%d discard:%d.\n",
								mpp_frame_get_errinfo(frame), mpp_frame_get_discard(frame));
					}
					RK_U32 width = mpp_frame_get_width(frame);
					RK_U32 height = mpp_frame_get_height(frame);
					RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
					RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
					RK_U32 buf_size = mpp_frame_get_buf_size(frame);
					data->frame_count++;
					printf("decode_get_frame get frame %d,size:%d\n", data->frame_count,buf_size);
					memcpy(out_data,frame,buf_size);
					// if (data->fp_output && !err_info)
					// dump_mpp_frame_to_file(frame, data->fp_output);
				}
				frm_eos = mpp_frame_get_eos(frame);
				mpp_frame_deinit(&frame);
				frame = NULL;
				get_frm = 1;
			}

	printf("%s():%d\n", __func__,__LINE__);
			// try get runtime frame memory usage
			if (data->frm_grp) {
				size_t usage = mpp_buffer_group_usage(data->frm_grp);
				if (usage > data->max_usage)
					data->max_usage = usage;
			}

	printf("%s():%d\n", __func__,__LINE__);
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

	printf("%s():%d\n", __func__,__LINE__);
			// if (get_frm)
				// continue;
			break;
		} while (1);

	printf("%s():%d\n", __func__,__LINE__);
        // if (data->frame_num > 0 && data->frame_count >= data->frame_num) {
            // data->eos = 1;
            // printf("reach max frame number %d\n", data->frame_count);
            // break;
        // }

        if (pkt_done)
            break;
	printf("%s():%d\n", __func__,__LINE__);

        /*
         * why sleep here:
         * mpi->decode_put_packet will failed when packet in internal queue is
         * full,waiting the package is consumed .Usually hardware decode one
         * frame which resolution is 1080p needs 2 ms,so here we sleep 3ms
         * * is enough.
         */
        msleep(3);
	printf("%s():%d\n", __func__,__LINE__);
    } while (1);

	printf("%s():%d\n", __func__,__LINE__);
    return ret;
}


static int mpiH264Decode(H264Decode *This,unsigned char *in_data,int in_size,unsigned char *out_data)
{

    memset(&data, 0, sizeof(data));
    data.ctx            = ctx;
    data.mpi            = mpi;
    data.eos            = 0;
    data.buf            = in_data;
    data.packet         = packet;
    data.packet_size    = MPI_DEC_STREAM_SIZE;
    data.frame          = frame;
    data.frame_count    = 0;
	decode_simple(&data,in_size,out_data);
}

static int mpiH264DecInit(H264Decode *This,int width,int height)
{
    MPP_RET ret         = MPP_OK;
    RK_U32 need_split   = 1;

	buf = mpp_malloc(char, MPI_DEC_STREAM_SIZE);
	ret = mpp_packet_init(&packet, buf, MPI_DEC_STREAM_SIZE);

	if (ret) {
		mpp_err("mpp_packet_init failed\n");
		return -1;
	}

    ret = mpp_create(&ctx, &mpi);
    if (MPP_OK != ret) {
        mpp_err("mpp_create failed\n");
		return -1;
    }
    // NOTE: decoder split mode need to be set before init
    MppParam param = &need_split;
    ret = mpi->control(ctx, MPP_DEC_SET_PARSER_SPLIT_MODE, param);
    if (MPP_OK != ret) {
        mpp_err("mpi->control failed\n");
		mpp_destroy(ctx);
		return -1;
    }
    ret = mpp_init(ctx, MPP_CTX_DEC, MPP_VIDEO_CodingAVC);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
		mpp_destroy(ctx);
        return -1;
    }
	return 0;
}

static int mpiH264DecUnInit(H264Decode *This)
{
    MPP_RET ret = mpi->reset(ctx);
    if (MPP_OK != ret) {
        mpp_err("mpi->reset failed\n");
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
