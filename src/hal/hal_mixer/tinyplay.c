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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if 1
#include <tinyalsa/asoundlib.h>
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
#else
#include <alsa/asoundlib.h>

static snd_pcm_t *playback_handle;//PCM设备句柄pcm.h
static snd_pcm_hw_params_t *hw_params;//硬件信息和PCM流配置
static snd_pcm_info_t *info;
static unsigned period_time = 0;
static unsigned buffer_time = 0;
static snd_pcm_uframes_t period_frames = 0;
static snd_pcm_uframes_t buffer_frames = 0;
static snd_pcm_uframes_t chunk_size = 0;
static snd_pcm_uframes_t buffer_size;

void rvMixerPlayInit(void)
{
}
int rvMixerPlayOpen(int sample,int channle,int bit)
{
	snd_pcm_info_alloca(&info);
	//1. 打开PCM，最后一个参数为0意味着标准配置
	int ret = snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (ret < 0) {
		perror("snd_pcm_open");
		return -1;
	}
	ret = snd_pcm_info(playback_handle, info);
	if (ret < 0) {
		perror("info");
		return -1;
	}
	ret = snd_pcm_nonblock(playback_handle, 1);
	if (ret < 0) {
		perror("nonblock");
		return -1;
	}
	//2. 分配snd_pcm_hw_params_t结构体
	snd_pcm_hw_params_alloca(&hw_params);
	//3. 初始化hw_params
	ret = snd_pcm_hw_params_any(playback_handle, hw_params);
	if (ret < 0) {
		perror("snd_pcm_hw_params_any");
		return -1;
	}
	//4. 初始化访问权限
	ret = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_access");
		return -1;
	}
	//5. 初始化采样格式SND_PCM_FORMAT_U8,8位
	int pcm_formate = SND_PCM_FORMAT_S16_LE;
    if (bit == 8) {
        pcm_formate = SND_PCM_FORMAT_S8;
    } else if (bit == 16) {
        pcm_formate = SND_PCM_FORMAT_S16_LE;
    } else if (bit == 24) {
        pcm_formate = SND_PCM_FORMAT_S24_LE;
    } else if (bit == 32) {
        pcm_formate = SND_PCM_FORMAT_S32_LE;
    }
	ret = snd_pcm_hw_params_set_format(playback_handle, hw_params, pcm_formate);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_format");
		return -1;
	}
	//7. 设置通道数量
	ret = snd_pcm_hw_params_set_channels(playback_handle, hw_params, channle);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_channels");
		return -1;
	}
	//6. 设置采样率，如果硬件不支持我们设置的采样率，将使用最接近的
	int val = sample;
	ret = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &val, 0);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_rate_near");
		return -1;
	}
	ret = snd_pcm_hw_params_get_buffer_time_max(hw_params,
			                        &buffer_time, 0);
	if (period_time == 0 && period_frames == 0) {
		if (buffer_time > 0)
			period_time = buffer_time / 4;
		else
			period_frames = buffer_frames / 4;
	}
	if (period_time > 0)
		ret = snd_pcm_hw_params_set_period_time_near(playback_handle, hw_params,
				&period_time, 0);
	else
		ret = snd_pcm_hw_params_set_period_size_near(playback_handle, hw_params,
				&period_frames, 0);
	if (buffer_time > 0) {
       ret = snd_pcm_hw_params_set_buffer_time_near(playback_handle, hw_params,
			                                &buffer_time, 0);
	} else {
       ret = snd_pcm_hw_params_set_buffer_size_near(playback_handle, hw_params,
			                                &buffer_frames);
	}
	 int monotonic = snd_pcm_hw_params_is_monotonic(hw_params);
	 int can_pause = snd_pcm_hw_params_can_pause(hw_params);
	//8. 设置hw_params
	ret = snd_pcm_hw_params(playback_handle, hw_params);
	if (ret < 0) {
		perror("snd_pcm_hw_params");
		return -1;
	}
	return 1;
}
void rvMixerPlayClose(void)
{
	if (playback_handle)
		snd_pcm_close(playback_handle);
	playback_handle = NULL;
	chunk_size = 0;
}
int rvMixerPlayWrite(void *data,int size)
{
	if (chunk_size == 0) {
		chunk_size = size;
		snd_pcm_hw_params_get_period_size(hw_params, &chunk_size, 0);
		snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
		if (chunk_size == buffer_size) {
			printf("Can't use period equal to buffer size (%lu == %lu)",
					chunk_size, buffer_size);
			return 0;
		}
	}
	int ret = 0;
	if (playback_handle)
		ret = snd_pcm_writei(playback_handle, data, size);
	if (ret == -EPIPE) {
		/* EPIPE means underrun */
		fprintf(stderr, "underrun occurred\n");
		//完成硬件参数设置，使设备准备好
		snd_pcm_prepare(playback_handle);
	}
	return ret;
}
#endif
