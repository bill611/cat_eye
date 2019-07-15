/*
 * =====================================================================================
 *
 *       Filename:  playwav.c
 *
 *    Description:  .wav�ļ���������
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
#define WAVE_FORMAT_PCM     1

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

typedef struct {
	unsigned short Size;
	unsigned short Type;
	char adata[640];
}SpiWav;

typedef struct {
	unsigned int riff;	//="riff" 0x46464952
	unsigned int file_size;	//
	unsigned int wav;	//="wav" 0x45564157
	unsigned int fmt;	//="fmt " 0x20746D66
} RiffFileHead;

typedef struct {
	unsigned short  format_tag;
	unsigned short  channle;
	unsigned int 	samples_per_sec;
	unsigned int 	avg_bytes_sec;
	unsigned short  block_align;
	unsigned short  bits_per_sample;
} MyPcmWavFormate;

typedef struct {
	RiffFileHead riff_area;			//�ļ���־
	unsigned int wav_formate_size;	//MyPcmWavFormate�ṹ��С
	MyPcmWavFormate wav_format;		//��Ƶ��ʽ
	unsigned int data_area_sign;	//="data" 0x61746164
	unsigned int data_area_size;	//���������С
} WavFileStruct;

/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
static pthread_t thd_play_wav;
static int wav_stop,thread_exit=1;
static int soundfp = -1;


/* ----------------------------------------------------------------*/
/**
 * @brief play_wav_stop ���Ž���������mp3���������
 * ��wav�ļ�δ������Ͳ�����һ���ļ�ʱ����ִ�д˴εĻص�����
 *
 * @param type 0�޻ص����� 1�лص�����
 */
/* ----------------------------------------------------------------*/
void playWavStop()
{
	wav_stop = 1;
	// ���������ĺ����лص�ʱ���ܹرջص�������Ҫ�ص�����ִ����
	while(!thread_exit || (soundfp > 0)) {
		usleep(10000);
	}
	if (my_mixer && soundfp > 0)
		my_mixer->Close(my_mixer, &soundfp);

}

static void playWavThreadEnd(void)
{
}

/* ----------------------------------------------------------------*/
/**
 * @brief threadWav ����.wav�ļ������߳�
 *
 * @param file_name
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static void * threadWav(void *file_name)
{
	FILE *fp;
	int wav_size;
	int read_size;
	char wav_buf[1024] = {0};
	WavFileStruct wav_msg;
	fp = fopen(file_name,"rb");
	if(fp==NULL) {
		printf("can't open %s\n",(char*)file_name);
		goto done2;
	}
	//��ȡ��Ƶ��ʽ
	if(fread(&wav_msg,1,sizeof(WavFileStruct),fp)!=sizeof(WavFileStruct)) {
		printf("%s format error!\n",file_name);
		goto done1;
	}
	if(wav_msg.riff_area.riff != ID_RIFF && wav_msg.data_area_sign != ID_DATA) {
		printf("%s format error or can't support!\n",file_name);
		goto done1;
	}
	if(wav_msg.wav_formate_size != 16) {
		printf("Only support 16bit standard wave format!:%d\n",wav_msg.wav_formate_size);
		goto done1;
	}
	if(wav_msg.wav_format.format_tag != WAVE_FORMAT_PCM) {
		printf("Only support PCM wave format!\n");
		goto done1;
	}
	wav_size = wav_msg.data_area_size;
	if(wav_size<=0) {
		printf("%s format error!\n",file_name);
		goto done1;
	}
	printf("size:%d,sec:%d,channel:%d\n",
			wav_size,
			wav_msg.wav_format.samples_per_sec,
			wav_msg.wav_format.channle);
	if (my_mixer)
		soundfp = my_mixer->Open(my_mixer,
				wav_msg.wav_format.samples_per_sec,
				wav_msg.wav_format.channle);
	if(soundfp <= 0) {
		printf("wav can't open sound card!\n");
		goto done1;
	}

	// printf("stop:%d,wave_size:%d\n",wav_stop,wav_size );
	int wavbuf_size_max = sizeof(wav_buf);
	while(!wav_stop && wav_size) {
		//���ļ�����д��spi�ӿڽ��в���
		read_size = wav_size > wavbuf_size_max ? wavbuf_size_max : wav_size;
		read_size = fread(wav_buf,1,read_size,fp);
		// printf("read_size :%d\n",read_size);
		if(read_size <= 0) {
			printf("Read data size is less zero!\n");
			break;
		}
		if (my_mixer)
			my_mixer->WriteBuffer(my_mixer,soundfp,wav_buf,read_size);
		wav_size-=read_size;
		// usleep(1);
	}
	usleep(100000);
	if (my_mixer) {
		my_mixer->ClearPlayBuffer(my_mixer);
		my_mixer->Close(my_mixer, &soundfp);
	}

done1:
	fclose(fp);
done2:
	free(file_name);
	playWavThreadEnd();
	wav_stop = 1;
	thread_exit = 1;
	pthread_exit(NULL);
	return NULL;
}

/* ----------------------------------------------------------------*/
/**
 * @brief playwavfile ������֧����������16λ��WAV�ļ�
 *
 * @param file_name �����ļ�����
 * @param Proc ������ϻص�����
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
int playwavfile(char * file_name)
{
	char buff[128] = {0};
	printf("play:%s\n", file_name);
	// excuteCmd("aplay",file_name,NULL);
	// return 0;
	int result;
	char *pName;
	pthread_attr_t threadAttr1;				//�߳�����

	playWavStop();
	thread_exit = 0; // ע�⣡�жϽ������߳̿�ʼ��������ֹ�����ٴν��뵼������stop������2���߳�
	pthread_attr_init(&threadAttr1);
	pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);

	pName = (char*)malloc(256);
	strcpy(pName,file_name);
	wav_stop = 0;

	result = pthread_create(&thd_play_wav, &threadAttr1, threadWav, pName); //��ʼ����,��·��ͨ
	if(result) {
		printf("[%s]pthread failt,Error code:%d\n",__FUNCTION__,result);
	}
	pthread_attr_destroy(&threadAttr1);		//�ͷŸ��Ӳ���
	return 0;
}

