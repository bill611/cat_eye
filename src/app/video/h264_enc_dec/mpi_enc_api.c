/*
 * =============================================================================
 *
 *       Filename:  mpi_enc_api.c
 *
 *    Description:  h264编码rkmpp
 *
 *        Version:  1.0
 *        Created:  2019-06-20 17:32:29
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
#define MODULE_TAG "mpi_enc_test"
#include <string.h>
#include <mpp/rk_mpi.h>

#include <mpp/mpp_log.h>
#include <mpi/mpp_mem.h>
#include <mpi/mpp_env.h>
#include <mpi/mpp_time.h>
#include <mpi/mpp_common.h>

#include "utils.h"
#include "mpi_enc_api.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
typedef struct {
    MppCodingType   type;
    RK_U32          width;
    RK_U32          height;
    MppFrameFormat  format;
} MpiEncTestCmd;

typedef struct {
    // global flow control flag
    RK_U32 frm_eos;
    RK_U32 pkt_eos;
    RK_U32 frame_count;
    RK_U64 stream_size;

    // src and dst
    FILE *fp_input;
    FILE *fp_output;

    // base flow context
    MppCtx ctx;
    MppApi *mpi;
    MppEncPrepCfg prep_cfg;
    MppEncRcCfg rc_cfg;
    MppEncCodecCfg codec_cfg;

    // input / output
    MppBuffer frm_buf;
    MppEncSeiMode sei_mode;

    // paramter for resource malloc
    RK_U32 width;
    RK_U32 height;
    RK_U32 hor_stride;
    RK_U32 ver_stride;
    MppFrameFormat fmt;
    MppCodingType type;

    // resources
    size_t frame_size;
    /* NOTE: packet buffer may overflow */
    size_t packet_size;

    // rate control runtime parameter
    RK_S32 gop;
    RK_S32 fps;
    RK_S32 bps;
} MpiEncTestData;

typedef struct _H264EncodePriv{
	unsigned char *sps_pps_head;
	int sps_pps_head_size;
	MpiEncTestData *p;
}H264EncodePriv;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
H264Encode *my_h264enc;

static OptionInfo mpi_enc_cmd[] = {
    {"i",               "input_file",           "input bitstream file"},
    {"o",               "output_file",          "output bitstream file, "},
    {"w",               "width",                "the width of input picture"},
    {"h",               "height",               "the height of input picture"},
    {"f",               "format",               "the format of input picture"},
    {"t",               "type",                 "output stream coding type"},
    {"n",               "max frame number",     "max encoding frame number"},
};

static MPP_RET test_ctx_init(MpiEncTestData **data, MpiEncTestCmd *cmd)
{
    MpiEncTestData *p = NULL;
    MPP_RET ret = MPP_OK;

    if (!data || !cmd) {
        mpp_err_f("invalid input data %p cmd %p\n", data, cmd);
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_calloc(MpiEncTestData, 1);
    if (!p) {
        mpp_err_f("create MpiEncTestData failed\n");
        ret = MPP_ERR_MALLOC;
        goto RET;
    }

    // get paramter from cmd
    p->width        = cmd->width;
    p->height       = cmd->height;
    p->hor_stride   = MPP_ALIGN(cmd->width, 16);
    p->ver_stride   = MPP_ALIGN(cmd->height, 16);
    p->fmt          = cmd->format;
    p->type         = cmd->type;

    // update resource parameter
    if (p->fmt <= MPP_FMT_YUV420SP_VU)
        p->frame_size = p->hor_stride * p->ver_stride * 3 / 2;
    else if (p->fmt <= MPP_FMT_YUV422_UYVY) {
        // NOTE: yuyv and uyvy need to double stride
        p->hor_stride *= 2;
        p->frame_size = p->hor_stride * p->ver_stride;
    } else
        p->frame_size = p->hor_stride * p->ver_stride * 4;
    p->packet_size  = p->width * p->height;

RET:
    *data = p;
    return ret;
}

static MPP_RET test_ctx_deinit(MpiEncTestData **data)
{
    MpiEncTestData *p = NULL;

    if (!data) {
        mpp_err_f("invalid input data %p\n", data);
        return MPP_ERR_NULL_PTR;
    }

    p = *data;
    if (p) {
        MPP_FREE(p);
        *data = NULL;
    }

    return MPP_OK;
}

static MPP_RET test_mpp_setup(MpiEncTestData *p)
{
    MPP_RET ret;
    MppApi *mpi;
    MppCtx ctx;
    MppEncCodecCfg *codec_cfg;
    MppEncPrepCfg *prep_cfg;
    MppEncRcCfg *rc_cfg;

    if (NULL == p)
        return MPP_ERR_NULL_PTR;

    mpi = p->mpi;
    ctx = p->ctx;
    codec_cfg = &p->codec_cfg;
    prep_cfg = &p->prep_cfg;
    rc_cfg = &p->rc_cfg;

    /* setup default parameter */
    p->fps = 30;
    p->gop = 10;
    p->bps = p->width * p->height / 8 * p->fps;

    prep_cfg->change        = MPP_ENC_PREP_CFG_CHANGE_INPUT |
                              MPP_ENC_PREP_CFG_CHANGE_ROTATION |
                              MPP_ENC_PREP_CFG_CHANGE_FORMAT;
    prep_cfg->width         = p->width;
    prep_cfg->height        = p->height;
    prep_cfg->hor_stride    = p->hor_stride;
    prep_cfg->ver_stride    = p->ver_stride;
    prep_cfg->format        = p->fmt;
    prep_cfg->rotation      = MPP_ENC_ROT_0;
    ret = mpi->control(ctx, MPP_ENC_SET_PREP_CFG, prep_cfg);
    if (ret) {
        mpp_err("mpi control enc set prep cfg failed ret %d\n", ret);
        goto RET;
    }

    rc_cfg->change  = MPP_ENC_RC_CFG_CHANGE_ALL;
    rc_cfg->rc_mode = MPP_ENC_RC_MODE_CBR;
    rc_cfg->quality = MPP_ENC_RC_QUALITY_MEDIUM;

    if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_CBR) {
        /* constant bitrate has very small bps range of 1/16 bps */
        rc_cfg->bps_target   = p->bps;
        rc_cfg->bps_max      = p->bps * 17 / 16;
        rc_cfg->bps_min      = p->bps * 15 / 16;
    } else if (rc_cfg->rc_mode ==  MPP_ENC_RC_MODE_VBR) {
        if (rc_cfg->quality == MPP_ENC_RC_QUALITY_CQP) {
            /* constant QP does not have bps */
            rc_cfg->bps_target   = -1;
            rc_cfg->bps_max      = -1;
            rc_cfg->bps_min      = -1;
        } else {
            /* variable bitrate has large bps range */
            rc_cfg->bps_target   = p->bps;
            rc_cfg->bps_max      = p->bps * 17 / 16;
            rc_cfg->bps_min      = p->bps * 1 / 16;
        }
    }

    /* fix input / output frame rate */
    rc_cfg->fps_in_flex      = 0;
    rc_cfg->fps_in_num       = p->fps;
    rc_cfg->fps_in_denorm    = 1;
    rc_cfg->fps_out_flex     = 0;
    rc_cfg->fps_out_num      = p->fps;
    rc_cfg->fps_out_denorm   = 1;

    rc_cfg->gop              = p->gop;
    rc_cfg->skip_cnt         = 0;

    mpp_log("%s() bps %d fps %d gop %d\n",__func__,
            rc_cfg->bps_target, rc_cfg->fps_out_num, rc_cfg->gop);
    ret = mpi->control(ctx, MPP_ENC_SET_RC_CFG, rc_cfg);
    if (ret) {
        mpp_err("mpi control enc set rc cfg failed ret %d\n", ret);
        goto RET;
    }

    codec_cfg->coding = p->type;
    switch (codec_cfg->coding) {
    case MPP_VIDEO_CodingAVC : {
        codec_cfg->h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
                                 MPP_ENC_H264_CFG_CHANGE_ENTROPY |
                                 MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        codec_cfg->h264.profile  = 100;
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        codec_cfg->h264.level    = 31;
        codec_cfg->h264.entropy_coding_mode  = 1;
        codec_cfg->h264.cabac_init_idc  = 0;
        codec_cfg->h264.transform8x8_mode = 1;
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        codec_cfg->jpeg.change  = MPP_ENC_JPEG_CFG_CHANGE_QP;
        codec_cfg->jpeg.quant   = 10;
    } break;
    case MPP_VIDEO_CodingVP8 : {
    } break;
    case MPP_VIDEO_CodingHEVC : {
        codec_cfg->h265.change = MPP_ENC_H265_CFG_INTRA_QP_CHANGE;
        codec_cfg->h265.intra_qp = 26;
    } break;
    default : {
        mpp_err_f("support encoder coding type %d\n", codec_cfg->coding);
    } break;
    }
    ret = mpi->control(ctx, MPP_ENC_SET_CODEC_CFG, codec_cfg);
    if (ret) {
        mpp_err("mpi control enc set codec cfg failed ret %d\n", ret);
        goto RET;
    }

    /* optional */
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        goto RET;
    }

RET:
    return ret;
}


static int mpi_enc_test(H264Encode *This,MpiEncTestCmd *cmd)
{
    MPP_RET ret = MPP_OK;
    MppApi *mpi;
    MppCtx ctx;

    mpp_log("%s() start\n",__func__);

    ret = test_ctx_init(&This->priv->p, cmd);
    if (ret) {
        mpp_err_f("test data init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }
    MpiEncTestData *p = This->priv->p;

    ret = mpp_buffer_get(NULL, &p->frm_buf, p->frame_size);
    if (ret) {
        mpp_err_f("failed to get buffer for input frame ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    mpp_log("%s() encoder test start w %d h %d type %d\n",__func__,
            p->width, p->height, p->type);

    // encoder demo
    ret = mpp_create(&p->ctx, &p->mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = test_mpp_setup(p);
    if (ret) {
        mpp_err_f("test mpp setup failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }
    mpi = p->mpi;
    ctx = p->ctx;

	// H264格式并且为第一帧时，提取sps/pps
    if (p->type == MPP_VIDEO_CodingAVC) {
        MppPacket packet = NULL;
        ret = mpi->control(ctx, MPP_ENC_GET_EXTRA_INFO, &packet);
        if (ret) {
            mpp_err("mpi control enc get extra info failed\n");
            goto MPP_TEST_OUT;
        }

        /* get and write sps/pps for H.264 */
        if (packet) {
            void *ptr   = mpp_packet_get_pos(packet);
            This->priv->sps_pps_head_size  = mpp_packet_get_length(packet);

			This->priv->sps_pps_head = (unsigned char *)malloc(This->priv->sps_pps_head_size);
			memcpy(This->priv->sps_pps_head,ptr,This->priv->sps_pps_head_size);
        }
    }

MPP_TEST_OUT:
	return 1;
}


static int mpiH264EncEncode(H264Encode *This,
        unsigned char *in_data,
        unsigned char **out_data,
        int *frame_type)
{
    MPP_RET ret;
    MppApi *mpi;
    MppCtx ctx;
	MpiEncTestData *p = This->priv->p;
    *frame_type = 0;
	if (NULL == p) {
		printf("%s()%d\n", __func__,__LINE__);
        return 0;
	}

    mpi = p->mpi;
    ctx = p->ctx;


	MppFrame frame = NULL;
	MppPacket packet = NULL;
	void *buf = mpp_buffer_get_ptr(p->frm_buf);

	memcpy(buf,in_data,p->frame_size);

	ret = mpp_frame_init(&frame);
	if (ret) {
		mpp_err_f("mpp_frame_init failed\n");
		goto RET;
	}

	mpp_frame_set_width(frame, p->width);
	mpp_frame_set_height(frame, p->height);
	mpp_frame_set_hor_stride(frame, p->hor_stride);
	mpp_frame_set_ver_stride(frame, p->ver_stride);
	mpp_frame_set_fmt(frame, p->fmt);
	mpp_frame_set_buffer(frame, p->frm_buf);
	mpp_frame_set_eos(frame, p->frm_eos);

	ret = mpi->encode_put_frame(ctx, frame);
	if (ret) {
		mpp_err("mpp encode put frame failed\n");
		goto RET;
	}

	ret = mpi->encode_get_packet(ctx, &packet);
	if (ret) {
		mpp_err("mpp encode get packet failed\n");
		goto RET;
	}
	size_t len = 0;
	if (packet) {
		// write packet to file here
		void *ptr   = mpp_packet_get_pos(packet);
		int len  = mpp_packet_get_length(packet);
		ret = len;

		p->pkt_eos = mpp_packet_get_eos(packet);

		unsigned char *head = ptr;
		int I_Frame = 0;
		if (head[0] == 0 && head[1] == 0 && head[2] == 1) {
			if (head[3] == 0x65) {
				I_Frame = 1;
			}
		}
		if (I_Frame) {
			*out_data = (unsigned char*) malloc(This->priv->sps_pps_head_size + len);
			memcpy(*out_data,This->priv->sps_pps_head,This->priv->sps_pps_head_size);
			memcpy(&((*out_data)[This->priv->sps_pps_head_size]),ptr,len);
            *frame_type = 1;
		} else {
			*out_data = (unsigned char*) malloc(len);
			memcpy(*out_data,ptr,len);
            *frame_type = 0;
		}

		mpp_packet_deinit(&packet);

		// mpp_log_f("encoded %s frame %d size %d\n", I_Frame ? "I":"P",p->frame_count, len);
		p->stream_size += len;
		p->frame_count++;

		if (p->pkt_eos) {
			mpp_log("found last packet\n");
			mpp_assert(p->frm_eos);
		}
	}

RET:
    return ret;
}

static void mpiH264EncInit(H264Encode *This,int width,int height)
{
    MPP_RET ret = MPP_OK;
    MpiEncTestCmd  cmd_ctx;
    memset(&cmd_ctx, 0, sizeof(MpiEncTestCmd));
	cmd_ctx.type = MPP_VIDEO_CodingAVC;
	cmd_ctx.width = width;
	cmd_ctx.height = height;
	cmd_ctx.format = MPP_FMT_YUV420SP;

    mpp_env_set_u32("mpi_debug", 0x0);
	mpi_enc_test(This,&cmd_ctx);
}

static void mpiH264EncUnInit(H264Encode *This)
{
    MpiEncTestData *p = This->priv->p;

	if (This->priv->sps_pps_head)
		free(This->priv->sps_pps_head);
	This->priv->sps_pps_head = NULL;
    int ret = p->mpi->reset(p->ctx);
    if (ret) {
        mpp_err("mpi->reset failed\n");
    }
    if (p->ctx) {
        mpp_destroy(p->ctx);
        p->ctx = NULL;
    }

    if (p->frm_buf) {
        mpp_buffer_put(p->frm_buf);
        p->frm_buf = NULL;
    }

    if (MPP_OK == ret)
        mpp_log("%s()success total frame %d bps %lld\n",__func__,
                p->frame_count, (RK_U64)((p->stream_size * 8 * p->fps) / p->frame_count));
    else
        mpp_err("%s() failed ret %d\n",__func__, ret);

    test_ctx_deinit(&This->priv->p);
}

void myH264EncInit(void)
{
	my_h264enc = (H264Encode *)calloc(1,sizeof(H264Encode));
	my_h264enc->priv = (H264EncodePriv *)calloc(1,sizeof(H264EncodePriv));
	my_h264enc->init = mpiH264EncInit;	
	my_h264enc->unInit = mpiH264EncUnInit;	
	my_h264enc->encode = mpiH264EncEncode;	
}
