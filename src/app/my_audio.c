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

void myAudioPlayRecognizer(char *usr_name)
{
	char path[64];
	sprintf(path,"%s%s.wav",AUDIO_PATH,usr_name);	
	playwavfile(path);		
}
static void* loopPlay(void *arg)
{
	char *path = (char *)arg;
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
	if (loop_start == 1)
		return;
	while (loop_end == 0) {
		usleep(1000);
	}
	loop_start = 1;
	char *path = (char *) calloc(1,64);
	sprintf(path,"%sring.wav",AUDIO_PATH);	
	createThread(loopPlay,path);
}
void myAudioStopPlay(void)
{
	loop_start = 0;
	// excuteCmd("busybox","killall","aplay",NULL);
}
void myAudioPlayDingdong(void)
{
	char *path = (char *) calloc(1,64);
	sprintf(path,"%sdingdong.wav",AUDIO_PATH);	
	playwavfile(path);		
}
