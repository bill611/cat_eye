/* tinyplay.c
**
** Copyright 2011, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
*/

#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

struct cmd {
    const char *filename;
    const char *filetype;
    unsigned int card;
    unsigned int device;
    int flags;
    struct pcm_config config;
    unsigned int bits;
};


#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

struct riff_wave_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t wave_id;
};

struct chunk_header {
    uint32_t id;
    uint32_t sz;
};

struct chunk_fmt {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct ctx {
    struct pcm *pcm;

    struct riff_wave_header wave_header;
    struct chunk_header chunk_header;
    struct chunk_fmt chunk_fmt;

    FILE *file;
};

static struct cmd g_cmd;
static struct ctx g_ctx;
int check_param(struct pcm_params *params, unsigned int param, unsigned int value,
                 char *param_name, char *param_unit)
{
    unsigned int min;
    unsigned int max;
    int is_within_bounds = 1;

    min = pcm_params_get_min(params, param);
    if (value < min) {
        fprintf(stderr, "%s is %u%s, device only supports >= %u%s\n", param_name, value,
                param_unit, min, param_unit);
        is_within_bounds = 0;
    }

    max = pcm_params_get_max(params, param);
    if (value > max) {
        fprintf(stderr, "%s is %u%s, device only supports <= %u%s\n", param_name, value,
                param_unit, max, param_unit);
        is_within_bounds = 0;
    }

    return is_within_bounds;
}

int sample_is_playable(const struct cmd *cmd)
{
    struct pcm_params *params;
    int can_play;

    params = pcm_params_get(cmd->card, cmd->device, PCM_OUT);
    if (params == NULL) {
        fprintf(stderr, "unable to open PCM %u,%u\n", cmd->card, cmd->device);
        return 0;
    }

    can_play = check_param(params, PCM_PARAM_RATE, cmd->config.rate, "sample rate", "hz");
    can_play &= check_param(params, PCM_PARAM_CHANNELS, cmd->config.channels, "sample", " channels");
    can_play &= check_param(params, PCM_PARAM_SAMPLE_BITS, cmd->bits, "bits", " bits");
    can_play &= check_param(params, PCM_PARAM_PERIOD_SIZE, cmd->config.period_size, "period size", "");
    can_play &= check_param(params, PCM_PARAM_PERIODS, cmd->config.period_count, "period count", "");

    pcm_params_free(params);

    return can_play;
}


void ctx_free(struct ctx *ctx)
{
    if (ctx == NULL) {
        return;
    }
    if (ctx->pcm != NULL) {
        pcm_close(ctx->pcm);
    }
}


int play_sample(struct ctx *ctx);

int tinyplay()
{
    struct cmd cmd;



    // if (ctx_init(&ctx, &cmd) < 0) {
        // return EXIT_FAILURE;
    // }

    /* TODO get parameters from context */
    printf("playing '%s': %u ch, %u hz, %u bit\n",
           cmd.filename,
           cmd.config.channels,
           cmd.config.rate,
           cmd.bits);

    // if (play_sample(&ctx) < 0) {
        // ctx_free(&ctx);
        // return EXIT_FAILURE;
    // }

    // ctx_free(&ctx);
    return EXIT_SUCCESS;
}

int play_sample(struct ctx *ctx)
{
    char *buffer;
    int size;
    int num_read;

    size = pcm_frames_to_bytes(ctx->pcm, pcm_get_buffer_size(ctx->pcm));
    buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "unable to allocate %d bytes\n", size);
        return -1;
    }


    do {
        num_read = fread(buffer, 1, size, ctx->file);
        if (num_read > 0) {
		if (pcm_writei(ctx->pcm, buffer, pcm_bytes_to_frames(ctx->pcm, num_read)) < 0) {
                fprintf(stderr, "error playing sample\n");
                break;
            }
        }
    } while (num_read > 0);

    free(buffer);
    return 0;
}

void rvMixerInit(void)
{
    g_cmd.filename = NULL;
    g_cmd.filetype = NULL;
    g_cmd.card = 0;
    g_cmd.device = 0;
    g_cmd.flags = PCM_OUT;
    g_cmd.config.period_size = 1024;
    g_cmd.config.period_count = 4;
    g_cmd.config.channels = 2;
    g_cmd.config.rate = 48000;
    g_cmd.config.format = PCM_FORMAT_S16_LE;
    g_cmd.config.silence_threshold = 1024 * 2;
    g_cmd.config.stop_threshold = 1024 * 2;
    g_cmd.config.start_threshold = 1024;
    g_cmd.bits = 16;
}

int rvMixerOpen(int sample,int channle,int bit)
{
    struct pcm_config config = g_cmd.config;

    if (bit == 8) {
        config.format = PCM_FORMAT_S8;
    } else if (bit == 16) {
        config.format = PCM_FORMAT_S16_LE;
    } else if (bit == 24) {
        config.format = PCM_FORMAT_S24_3LE;
    } else if (bit == 32) {
        config.format = PCM_FORMAT_S32_LE;
    }
	config.rate = sample;
	config.channels = channle;
	printf("playing %u ch, %u hz, %u bit\n",
			config.channels,                       
			config.rate,bit);                                 
    g_ctx.pcm = pcm_open(g_cmd.card,
                        g_cmd.device,
                        g_cmd.flags,
                        &config);
    if (g_ctx.pcm == NULL) {
        fprintf(stderr, "failed to allocate memory for pcm\n");
        return -1;
    } else if (!pcm_is_ready(g_ctx.pcm)) {
        fprintf(stderr, "failed to open for pcm %u,%u\n", g_cmd.card, g_cmd.device);
        pcm_close(g_ctx.pcm);
        return -1;
    }
	return pcm_get_file_descriptor(g_ctx.pcm);	
}

void rvMixerClose(void)
{
	pcm_close(g_ctx.pcm);
}

int rvMixerWrite(void *data,int size)
{
	return pcm_writei(g_ctx.pcm, data, size);
}

