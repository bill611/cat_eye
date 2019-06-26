/*
 * =====================================================================================
 *
 *       Filename:  Mixer.c
 *
 *    Description:  �������ӿڴ���
 *
 *        Version:  1.0
 *        Created:  2015-12-21 15:28:18
 *       Revision:  1.0
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
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include <sys/types.h>
#include <sys/poll.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include "tinyplay.h"
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
#if DBG_MYMIXER > 0
	#define DBG_P( x... ) printf( x )
#else
	#define DBG_P( x... )
#endif

#define MAXMIXER 3
typedef struct _TMixerPriv
{
    pthread_mutex_t mutex;
	void *aec_buf;
	int Inited;					//�Ƿ��ʼ���ɹ�

	int audiofp;				//����������  ����
	int mixer_fd;				//���������    ����

	int audiofp1;				//����������  ¼��
	int mixer_fd1;				//���������	��ͷ

	int CurrSample;				//��ǰ������
	int oss_format;				//standard 16bit little endian format, support this format only
	unsigned int MinVolume;		//��С����
	unsigned int MaxVolume;		//�������
	unsigned int MicVolume;		//MIC����
	unsigned int PlayVolume;	//��������
	int bSlience;				//�Ƿ��� 1���� 0�Ǿ���
}TMixerPriv;

/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
TMixer *my_mixer = NULL;

/* ----------------------------------------------------------------*/
/**
 * @brief mixerOpen �򿪷�������
 *
 * @param this
 * @param Sample ���ò�����
 * @param ch ����ͨ��
 *
 * @returns 0ʧ��
 */
/* ----------------------------------------------------------------*/
static int mixerOpen(TMixer *this,int sample,int ch)
{
    pthread_mutex_lock (&this->Priv->mutex);
    int ret = 0;
	static int err_times ;
	if (this->Priv->audiofp > 0) {
        goto mixer_open_end;
	}
	err_times = 0;
	//������������� ȷ����Ƶ�ļ�ֹͣ����
	do {
		printf("mixer open sample:%d,ch:%d\n", sample,ch);
		this->Priv->audiofp = rvMixerOpen(sample,ch,16);
		if (this->Priv->audiofp > 0)
			break;
		usleep(100000);
	} while(1);

mixer_open_end:
	this->Priv->Inited = 1;
	ret = this->Priv->audiofp;
    pthread_mutex_unlock (&this->Priv->mutex);
    return ret;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerClose �رշ����豸
 *
 * @param this
 * @param Handle �豸���
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static int mixerClose(TMixer *this,int *Handle)
{
    pthread_mutex_lock (&this->Priv->mutex);
	rvMixerClose();
	this->Priv->CurrSample = 0;
	this->Priv->Inited = 0;
	*Handle = this->Priv->audiofp = -1;
    pthread_mutex_unlock (&this->Priv->mutex);
	return 0;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerRead ��������������¼��
 *
 * @param this
 * @param pBuffer ����¼������
 * @param Size һ�ζ�ȡ�ֽڴ�С
 *
 * @returns ʵ�ʶ�ȡ�ֽ�����
 */
/* ----------------------------------------------------------------*/
static int mixerRead(TMixer *this,void *pBuffer,int Size)
{
    int ret = 0;
	if (this->Priv->audiofp1 != -1) {
		ret = read(this->Priv->audiofp1,pBuffer,Size);
	}

	return ret;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerReadBuffer ��������������¼��
 *
 * @param this
 * @param AudioBuf ����¼������
 * @param NeedSize һ�ζ�ȡ�ֽڴ�С
 *
 * @returns ʵ�ʶ�ȡ�ֽ�����
 */
/* ----------------------------------------------------------------*/
static int mixerReadBuffer(TMixer *this, void *AudioBuf, int NeedSize)
{
	int RealSize = 0;
	if (this->Priv->audiofp1 == -1) {
		goto mixer_read_end;
	}
	RealSize = this->Read(this,AudioBuf,NeedSize);

mixer_read_end:
	return RealSize;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerWrite д��������
 *
 * @param this
 * @param Handle ͨ��
 * @param pBuffer ����
 * @param Size ��С
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static int mixerWrite(TMixer *this,int Handle,const void *pBuffer,int Size)
{
	// printf("[%s]init:%d,handle:%d,size:%d\n",
			// __FUNCTION__,
			// this->Priv->Inited,Handle,Size);
	int ret = 0;
	if(this->Priv->Inited == 0 ) {
        goto mixer_write_end;
	}

    if(Handle<1) {
        goto mixer_write_end;
    }
    if(Size<10) {
        ret = Size;
		return Size;
	}
	ret = rvMixerWrite((void *)pBuffer,Size);
mixer_write_end:
	return ret;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerWriteBuffer д���������
 *
 * @param this
 * @param Handle ͨ��
 * @param pBuffer ����
 * @param Size ��С
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static int mixerWriteBuffer(TMixer *this,int Handle,const void *pBuffer,int Size)
{
	DBG_P("[%s]\n",__FUNCTION__);
	const char *pBuf = (const char*)pBuffer;
	int LeaveSize = Size;

	while(LeaveSize) {
		int WriteSize = this->Write(this,Handle,pBuf,LeaveSize);
		if(WriteSize <= 0) {
			break;
		}
		pBuf += WriteSize;
		LeaveSize -= WriteSize;
	}
	return Size;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerGetVolume �������
 *
 * @param this
 * @param type �������� 0���� 1¼��
 *
 * @returns ����ֵ
 */
/* ----------------------------------------------------------------*/
static int mixerGetVolume(struct _TMixer *this,int type)
{
	if (type) {
		return this->Priv->MicVolume;
	} else {
		return this->Priv->PlayVolume;
	}
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerSetVolume ��������
 *
 * @param this
 * @param Volume ����
 * @param type ���������� 0���� 1¼��
 *
 * @returns ���ý��
 */
/* ----------------------------------------------------------------*/
static int mixerSetVolume(struct _TMixer *this,int type,int Volume)
{
    // pthread_mutex_lock (&this->Priv->mutex);
	int Value,buf;
    int ret = 0;

	if(Volume<0) {
		Volume = 0;
		DBG_P("Set volume value %d wrong,set zero\n",Volume);
	}
	if(Volume>100) {
		Volume = 100;
		DBG_P("Set volume value %d wrong,set 100\n",Volume);
	}
	Value = this->Priv->MinVolume + Volume*(this->Priv->MaxVolume-this->Priv->MinVolume)/100;
	DBG_P("Volume Value:%d\n",Value);

	buf = Value;
	Value <<= 8;
	Value |= buf;

	if (type) {
		if( -1 == ioctl(this->Priv->mixer_fd1 , MIXER_WRITE(SOUND_MIXER_PCM), &Value)) {
			fprintf(stdout,"Set Volume %d ,%s\n", Value,strerror(errno));
            ret = -2;
            goto mixer_set_voluem_end;
		}
		this->Priv->PlayVolume = Volume;
	} else {
		if( -1 == ioctl(this->Priv->mixer_fd , MIXER_WRITE(SOUND_MIXER_PCM), &Value)) {
			fprintf(stdout,"Set Volume %d ,%s\n", Value,strerror(errno));
            ret = -2;
            goto mixer_set_voluem_end;
		}
		this->Priv->MicVolume = Volume;
	}
mixer_set_voluem_end:
    // pthread_mutex_unlock (&this->Priv->mutex);
	return ret;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerSetVolumeEx ͨ���ⲿӦ�ó�����������
 *
 * @param this
 * @param Volume ����
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static int mixerSetVolumeEx(struct _TMixer *this,int Volume)
{
	// PcmOut_setGain(Volume);
	return 0;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerInitVolume ��ʼ������
 *
 * @param this
 * @param Volume ����
 * @param bSlience 0�Ǿ��� 1����
 *
 */
/* ----------------------------------------------------------------*/
static void mixerInitVolume(struct _TMixer *this,int Volume,int bSlience)
{
	this->SetVolume(this,Volume,1);
	this->Priv->bSlience = bSlience;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerSetSlience ���þ���
 *
 * @param this
 * @param bSlience 1������0�Ǿ���
 */
/* ----------------------------------------------------------------*/

static void mixerSetSlience(struct _TMixer *this,int bSlience)
{
	this->Priv->bSlience = bSlience;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerGetSlience ���ؾ���״̬
 *
 * @param this
 *
 * @returns  1������0�Ǿ���
 */
/* ----------------------------------------------------------------*/
static int mixerGetSlience(struct _TMixer *this)
{
	return this->Priv->bSlience;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerClearRecBuffer �������¼��������
 *
 * @param this
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static void mixerClearRecBuffer(TMixer *this)
{
	return;
	int i;
	int buffersize = 1024;
	char *pBuf;
	int ret;
	if (this->Priv->audiofp1 == -1) {
		return;
	}
	pBuf = (char *)malloc(buffersize );
	if(pBuf && buffersize > 0) {
		for(i=0; i<8; i++) {
			memset(pBuf,0,buffersize);
			ret = this->Read(this,pBuf,buffersize);
			printf("ClearRecBuffer[%d]Rec buffersize :%d,real:%d\n", i,buffersize,ret);
		}
	}
	free(pBuf);

}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerClearPlayBuffer �����������������
 *
 * @param this
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static void mixerClearPlayBuffer(TMixer *this)
{
	// PcmOut_resetAll();
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerInitPlayAndRec ��ʼ��¼���ͷ���
 *
 * @param this
 * @param handle �����������
 * @param type ENUM_MIXER_CLEAR_PLAY_BUFFER ����������
 * 			   ENUM_MIXER_CLEAR_REC_BUFFER ���¼�����
 */
/* ----------------------------------------------------------------*/
static void mixerInitPlayAndRec(TMixer *this,int *handle)
{
	int fp;
	fp = this->Open(this,FMT8K,1);			//������
	if (fp <= 0) {
		return;
	}
	// anykaCaptureStart(1);
	this->ClearRecBuffer(this);
	// this->ClearPlayBuffer(this);
	*handle = fp;
}

static void mixerInitPlay8K(TMixer *this,int *handle)
{
	int fp;
	fp = this->Open(this,FMT8K,1);			//������
	// printf("[%s]fp:%d\n",__FUNCTION__,fp);
	if (fp <= 0) {
		return;
	}
	// anykaCaptureStart(0);
	*handle = fp;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerDeInitPlay ����������棬�ͷŷ����豸
 *
 * @param this
 * @param handle �����豸���
 */
/* ----------------------------------------------------------------*/
static void mixerDeInitPlay(TMixer *this,int *handle)
{
	// anykaCaptureStop();
	this->ClearPlayBuffer(this);
	this->Close(this, handle);
}
static void mixerDeInitPlay8K(TMixer *this,int *handle)
{
	this->ClearPlayBuffer(this);
	this->Close(this, handle);
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerDestroy ���ٻ��������ͷ���Դ
 *
 * @param this
 */
/* ----------------------------------------------------------------*/
static void mixerDestroy(TMixer *this)
{
	if(this->Priv->audiofp!=-1)
		close(this->Priv->audiofp);
	if(this->Priv->mixer_fd!=-1)
		close(this->Priv->mixer_fd);
	if(this->Priv->audiofp1!=-1)
		close(this->Priv->audiofp1);
	if(this->Priv->mixer_fd1!=-1)
		close(this->Priv->mixer_fd1);
	free(this->Priv);
	free(this);
	this = NULL;
}


/* ----------------------------------------------------------------*/
/**
 * @brief mixerCreate �����������ӿ�
 *
 * @returns ������ָ��
 */
/* ----------------------------------------------------------------*/
TMixer* mixerCreate(void)
{
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	TMixer *this = (TMixer *)malloc(sizeof(TMixer));
	memset(this,0,sizeof(TMixer));
	this->Priv = (TMixerPriv *)malloc(sizeof(TMixerPriv));
	memset(this->Priv,0,sizeof(TMixerPriv));

	this->Priv->oss_format = AFMT_S16_LE;

    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&this->Priv->mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

	rvMixerInit();

	this->Priv->Inited = 0;
	this->Priv->audiofp = -1;
    // this->Priv->audiofp1 = prepare_capture();

	this->Destroy = mixerDestroy;
	this->Open = mixerOpen;
	this->Close = mixerClose;
	this->Read = mixerRead;
	this->ReadBuf =  mixerReadBuffer;
	this->Write = mixerWrite;
	this->WriteBuffer = mixerWriteBuffer;
	this->InitVolume = mixerInitVolume;
	this->GetVolume  = mixerGetVolume;
	this->SetVolume  = mixerSetVolume;
	this->SetVolumeEx  = mixerSetVolumeEx;
	this->SetSlience = mixerSetSlience;
	this->GetSlience = mixerGetSlience;
	this->ClearRecBuffer = mixerClearRecBuffer;
	this->ClearPlayBuffer = mixerClearPlayBuffer;
	this->InitPlayAndRec = mixerInitPlayAndRec;
	this->InitPlay8K = mixerInitPlay8K;
	this->DeInitPlay = mixerDeInitPlay;
	this->DeInitPlay8K = mixerDeInitPlay8K;
	return this;
}

void myMixerInit(void)
{
	my_mixer = mixerCreate();
}
