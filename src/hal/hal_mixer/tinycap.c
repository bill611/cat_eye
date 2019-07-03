/* tinycap.c
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
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include "gpio-rv1108.h"

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

typedef struct
{
	char ioname[16];	// GPIO名称
	int iovalue;		// GPIO 电平

} TioLevelCtrl;

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

int capturing = 1;
int prinfo = 1;

static struct pcm_config g_pcm_config;
static struct pcm *g_pcm;
static int dev_card,dev_device;

int rvMixerCaptureOpen(void)
{
	printf("[%s]\n", __func__);
    g_pcm = pcm_open(dev_card, dev_device, PCM_IN, &g_pcm_config);
    if (!g_pcm || !pcm_is_ready(g_pcm)) {
        fprintf(stderr, "Unable to open PCM device (%s)\n",
                pcm_get_error(g_pcm));
        return -1;
    }
	return pcm_get_file_descriptor(g_pcm);
}
void rvMixerCaptureClose(void)
{
	printf("[%s]\n", __func__);
	pcm_close(g_pcm);
}
int rvMixerCaptureRead(void *data,int size)
{
	int ret = pcm_readi(g_pcm, data, size);
	// printf("ret:%d\n", ret);
	return  g_pcm_config.period_count * ret;
}
void rvMixerCaptureInit(void)
{
    memset(&g_pcm_config, 0, sizeof(g_pcm_config));
    g_pcm_config.channels = 2;
    g_pcm_config.rate = 8000;
    g_pcm_config.period_size = 512;
    g_pcm_config.period_count = 4;
    g_pcm_config.format = PCM_FORMAT_S16_LE;
    g_pcm_config.start_threshold = 0;
    g_pcm_config.stop_threshold = 0;
    g_pcm_config.silence_threshold = 0;
	dev_card = 0;
	dev_device = 0;
}
