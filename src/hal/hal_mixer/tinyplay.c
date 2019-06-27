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

struct cmd {
    unsigned int card;
    unsigned int device;
    int flags;
    struct pcm_config config;
    unsigned int bits;
};


static struct cmd g_cmd;
static struct pcm *pcm;

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

static int sample_is_playable(struct cmd *cmd)
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


void rvMixerPlayInit(void)
{
    g_cmd.card = 0;
    g_cmd.device = 0;
    g_cmd.flags = PCM_OUT;
    g_cmd.config.period_size = 512;
    g_cmd.config.period_count = 4;
    g_cmd.config.channels = 2;
    g_cmd.config.rate = 48000;
    g_cmd.config.format = PCM_FORMAT_S16_LE;
    g_cmd.config.silence_threshold = 1024 * 2;
    g_cmd.config.stop_threshold = 1024 * 2;
    g_cmd.config.start_threshold = 1024;
    g_cmd.bits = 16;
}

int rvMixerPlayOpen(int sample,int channle,int bit)
{
    if (bit == 8) {
        g_cmd.config.format = PCM_FORMAT_S8;
    } else if (bit == 16) {
        g_cmd.config.format = PCM_FORMAT_S16_LE;
    } else if (bit == 24) {
        g_cmd.config.format = PCM_FORMAT_S24_3LE;
    } else if (bit == 32) {
        g_cmd.config.format = PCM_FORMAT_S32_LE;
    }
	g_cmd.config.rate = sample;
	g_cmd.config.channels = channle;
	printf("playing %u ch, %u hz, %u bit\n",
			g_cmd.config.channels,
			g_cmd.config.rate,bit);
	sample_is_playable(&g_cmd);
    pcm = pcm_open(g_cmd.card,
                        g_cmd.device,
                        g_cmd.flags,
                        &g_cmd.config);
    if (pcm == NULL) {
        fprintf(stderr, "failed to allocate memory for pcm\n");
        return -1;
    } else if (!pcm_is_ready(pcm)) {
        fprintf(stderr, "failed to open for pcm %u,%u\n", g_cmd.card, g_cmd.device);
        pcm_close(pcm);
        return -1;
    }
	return pcm_get_file_descriptor(pcm);
}

void rvMixerPlayClose(void)
{
	pcm_close(pcm);
}

int rvMixerPlayWrite(void *data,int size)
{
	int ret = pcm_writei(pcm, data, pcm_bytes_to_frames(pcm, size));
	return g_cmd.config.period_count * ret;
}

