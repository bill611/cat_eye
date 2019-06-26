/*
 * =====================================================================================
 *
 *       Filename:  playwav.c
 *
 *    Description:  .wav文件播放驱动
 *
 *        Version:  1.0
 *        Created:  2015-11-24 14:22:41
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
/* ----------------------------------------------------------------*
 *                      include head files
 *-----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include "pthread.h"
#include "playwav.h"
#include "my_mixer.h"

/* ----------------------------------------------------------------*
 *                  extern variables declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                  internal functions declare
 *-----------------------------------------------------------------*/
/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
static pthread_t thd_play_wav;
static int wav_stop,thread_exit=1;
static int soundfp = -1;


/* ----------------------------------------------------------------*/
/**
 * @brief play_wav_stop 播放结束，若有mp3则继续播放
 * 若wav文件未播放完就播放下一个文件时，不执行此次的回调函数
 *
 * @param type 0无回调函数 1有回调函数
 */
/* ----------------------------------------------------------------*/
void playWavStop()
{
	wav_stop = 1;
	// 播放语音的函数有回调时不能关闭回调，必须要回调函数执行完
	while(!thread_exit || (soundfp > 0)) {
		usleep(10000);
	}
	if (soundfp > 0)
		my_mixer->Close(my_mixer, &soundfp);

}

static void playWavThreadEnd(void)
{
}

/* ----------------------------------------------------------------*/
/**
 * @brief thread_wav 创建.wav文件播放线程
 *
 * @param lpfilename
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static void * thread_wav(void *lpfilename)
{
	FILE *fp;
	int WavSize;
	int ReadSize;
	char cWavBuf[1024];
	WAVEFILESTRUCT WavMsg;
	fp = fopen(lpfilename,"rb");
	if(fp==NULL) {
		printf("can't open %s\n",(char*)lpfilename);
		goto done2;
	}
	//获取音频格式
	if(fread(&WavMsg,1,sizeof(WAVEFILESTRUCT),fp)!=sizeof(WAVEFILESTRUCT)) {
		printf("%s format error!\n",lpfilename);
		goto done1;
	}
	if(WavMsg.RIFFArea.RIFF!=0x46464952 && WavMsg.DataAreaSign!=0x61746164) {
		printf("%s format error or can't support!\n",lpfilename);
		goto done1;
	}
	if(WavMsg.WaveFormatSize != 16) {
		printf("Only support 16bit standard wave format!\n");
		goto done1;
	}
	if(WavMsg.WaveFormat.wFormatTag != WAVE_FORMAT_PCM) {
		printf("Only support PCM wave format!\n");
		goto done1;
	}
	WavSize = WavMsg.DataAreaSize;
	if(WavSize<=0) {
		printf("%s format error!\n",lpfilename);
		goto done1;
	}
	printf("size:%d,sec:%d,channel:%d\n",WavSize,WavMsg.WaveFormat.nSamplesPerSec,WavMsg.WaveFormat.nChannels);
	soundfp = my_mixer->Open(my_mixer,WavMsg.WaveFormat.nSamplesPerSec,
		WavMsg.WaveFormat.nChannels);
	if(soundfp <= 0) {
		printf("wav can't open sound card!\n");
		goto done1;
	}

	// printf("stop:%d,wave_size:%d\n",wav_stop,WavSize );
	while(!wav_stop && WavSize) {
		//读文件内容写入spi接口进行播放
		ReadSize = WavSize > sizeof(cWavBuf) ? sizeof(cWavBuf) : WavSize;
		ReadSize = fread(cWavBuf,1,ReadSize,fp);
		// printf("ReadSize :%d\n",ReadSize);
		if(ReadSize<=0) {
			printf("Read data size is less zero!\n");
			break;
		}

		my_mixer->WriteBuffer(my_mixer,soundfp,cWavBuf,ReadSize);
		WavSize-=ReadSize;
		// usleep(1);
	}
	my_mixer->ClearPlayBuffer(my_mixer);
	my_mixer->Close(my_mixer, &soundfp);

done1:
	fclose(fp);
done2:
	free(lpfilename);
	playWavThreadEnd();
	wav_stop = 1;
	thread_exit = 1;
	pthread_exit(NULL);
	return NULL;
}

/* ----------------------------------------------------------------*/
/**
 * @brief playwavfile 驱动仅支持两声道，16位的WAV文件
 *
 * @param lpfilename 播放文件名称
 * @param Proc 播放完毕回调函数
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
int playwavfile(char * lpfilename)
{
	int result;
	char *pName;
	pthread_attr_t threadAttr1;				//线程属性

	printf("play:%s\n", lpfilename);
	playWavStop();
	thread_exit = 0; // 注意！判断结束后即线程开始创建，防止马上再次进入导致两次stop而创建2个线程
	pthread_attr_init(&threadAttr1);		//附加参数
	pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);	//设置线程为自动销毁

	pName = (char*)malloc(256);
	strcpy(pName,lpfilename);
	wav_stop = 0;

	result = pthread_create(&thd_play_wav, &threadAttr1, thread_wav, pName); //开始振铃,线路连通
	if(result) {
		printf("[%s]pthread failt,Error code:%d\n",__FUNCTION__,result);
	}
	pthread_attr_destroy(&threadAttr1);		//释放附加参数
	return 0;
}

