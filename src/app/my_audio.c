/*
 * =============================================================================
 *
 *       Filename:  my_audio.c
 *
 *    Description:  播放相关音频
 *
 *        Version:  1.0
 *        Created:  2019-06-27 13:26:10
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
#include <unistd.h>
#include "playwav.h"
#include "config.h"
#include "my_mixer.h"
#include "externfunc.h"
#include "thread_helper.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int loop_start = 0;
static int loop_end = 1;

/* ---------------------------------------------------------------------------*/
/**
 * @brief isNeedToPlay 判断是否在免扰时候可以播放音频
 *
 * @returns 0 不播放 1播放
 */
/* ---------------------------------------------------------------------------*/
int isNeedToPlay(void)
{
    if (g_config.mute.state == 0) {
		return 1;
    }
	struct tm *tm = getTime();
	int time_now = tm->tm_hour * 60 + tm->tm_min;
	if (g_config.mute.start_time <= g_config.mute.end_time) {
        if (time_now >= g_config.mute.start_time && time_now <= g_config.mute.end_time)
            return 0;
        else 
            return 1;
	} else {
        if (time_now <= g_config.mute.end_time)
			return 0;
        if (time_now >= g_config.mute.start_time)
			return 0;
		return 1;
	}
}
void myAudioStopPlay(void)
{
	loop_start = 0;
	excuteCmd("busybox","killall","aplay",NULL);
}

void myAudioPlayRecognizer(char *usr_name)
{
	if (isNeedToPlay() == 0)
		return;
	char path[64];
	sprintf(path,"%s%s.wav",AUDIO_PATH,usr_name);
	playwavfile(path);
}
static void* loopPlay(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	char *path = (char *)arg;
	if (my_mixer)
		my_mixer->SetVolumeEx(my_mixer,g_config.ring_volume);
	loop_end = 0;
	while (loop_start){
		playwavfile(path);
		sleep(1);
	}
	if (path)
		free(path);
	loop_end = 1;
	return NULL;
}
void myAudioPlayRing(void)
{
	if (isNeedToPlay() == 0)
		return;
	if (loop_start == 1)
		return;
	while (loop_end == 0) {
		usleep(1000);
	}
	loop_start = 1;
	char *path = (char *) calloc(1,64);
	sprintf(path,"%sring%d.wav",AUDIO_PATH,g_config.ring_num);
	createThread(loopPlay,path);
}
static void* oncePlay(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	char *path = (char *)arg;
	if (my_mixer)
		my_mixer->SetVolumeEx(my_mixer,g_config.ring_volume);
	playwavfile(path);
	if (path)
		free(path);
	return NULL;
}
void myAudioPlayRingOnce(void)
{
	if (isNeedToPlay() == 0)
		return;
	char *path = (char *) calloc(1,64);
	myAudioStopPlay();
	sprintf(path,"%sring%d.wav",AUDIO_PATH,g_config.ring_num);
	createThread(oncePlay,path);
}
void myAudioPlayAlarm(void)
{
	if (my_mixer)
		my_mixer->SetVolumeEx(my_mixer,g_config.alarm_volume);
	char path[64];
	sprintf(path,"%salarm.wav",AUDIO_PATH);
	playwavfile(path);
}
