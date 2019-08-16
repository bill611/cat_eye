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
#include <errno.h>

#include "hal_mixer.h"
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

	int channle;				// ��������
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

static int MonoToStereo(const void *pSrc,int Size,void *pDst)
{
	const short *p1 = (const short*)pSrc;
	short *p2 = (short *)pDst;
	int i = Size/2;
	do {
		*p2++ = *p1;
		*p2++ = *p1++;
	}while(--i);
	return Size*2;
}
/* ----------------------------------------------------------------*/
/**
 * @brief mixerOpen �򿪷�������
 *
 * @param This
 * @param Sample ���ò�����
 * @param ch ����ͨ��
 *
 * @returns 0ʧ��
 */
/* ----------------------------------------------------------------*/
static int mixerOpen(TMixer *This,int sample,int ch)
{
    pthread_mutex_lock (&This->Priv->mutex);
    int ret = 0;
	static int err_times ;
	if (This->Priv->audiofp > 0) {
        goto mixer_open_end;
	}
	err_times = 0;
	This->Priv->channle = ch;
	//������������� ȷ����Ƶ�ļ�ֹͣ����
	do {
		printf("mixer open sample:%d,ch:%d\n", sample,ch);
		This->Priv->audiofp = rvMixerPlayOpen(sample,ch,16);
		if (This->Priv->audiofp > 0)
			break;
		usleep(100000);
	} while(1);

mixer_open_end:
	This->Priv->Inited = 1;
	ret = This->Priv->audiofp;
    pthread_mutex_unlock (&This->Priv->mutex);
    return ret;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerClose �رշ����豸
 *
 * @param This
 * @param Handle �豸���
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static int mixerClose(TMixer *This,int *Handle)
{
    pthread_mutex_lock (&This->Priv->mutex);
	rvMixerPlayClose();
	This->Priv->Inited = 0;
	*Handle = This->Priv->audiofp = -1;
    pthread_mutex_unlock (&This->Priv->mutex);
	return 0;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerRead ��������������¼��
 *
 * @param This
 * @param pBuffer ����¼������
 * @param Size һ�ζ�ȡ�ֽڴ�С
 *
 * @returns ʵ�ʶ�ȡ�ֽ�����
 */
/* ----------------------------------------------------------------*/
static int mixerRead(TMixer *This,void *pBuffer,int Size)
{
    int ret = 0;
	if (This->Priv->audiofp1 == -1)
		This->Priv->audiofp1 = rvMixerCaptureOpen();
	if (This->Priv->audiofp1 != -1) {
		ret = rvMixerCaptureRead(pBuffer,Size);
	}

	return ret;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerReadBuffer ��������������¼��
 *
 * @param This
 * @param AudioBuf ����¼������
 * @param NeedSize һ�ζ�ȡ�ֽڴ�С
 *
 * @returns ʵ�ʶ�ȡ�ֽ�����
 */
/* ----------------------------------------------------------------*/
static int mixerReadBuffer(TMixer *This, void *AudioBuf, int NeedSize)
{
	int RealSize = This->Read(This,AudioBuf,NeedSize);

	return RealSize;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerWrite д��������
 *
 * @param This
 * @param Handle ͨ��
 * @param pBuffer ����
 * @param Size ��С
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static int mixerWrite(TMixer *This,int Handle,const void *pBuffer,int Size)
{
	// printf("[%s]init:%d,handle:%d,size:%d\n",
			// __FUNCTION__,
			// This->Priv->Inited,Handle,Size);
	int ret = 0;
	if(This->Priv->Inited == 0 ) {
        goto mixer_write_end;
	}

    if(Handle<1) {
        goto mixer_write_end;
    }
    if(Size<10) {
        ret = Size;
		return Size;
	}
	ret = rvMixerPlayWrite((void *)pBuffer,Size);
mixer_write_end:
	return ret;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerWriteBuffer д���������
 *
 * @param This
 * @param Handle ͨ��
 * @param pBuffer ����
 * @param Size ��С
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static int mixerWriteBuffer(TMixer *This,int Handle,const void *pBuffer,int Size)
{
	DBG_P("[%s]\n",__FUNCTION__);
	const char *pBuf = (const char*)pBuffer;
	int LeaveSize = Size;

	while(LeaveSize) {
		int WriteSize = This->Write(This,Handle,pBuf,LeaveSize);
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
 * @param This
 * @param type �������� 0���� 1¼��
 *
 * @returns ����ֵ
 */
/* ----------------------------------------------------------------*/
static int mixerGetVolume(struct _TMixer *This,int type)
{
	if (type) {
		return This->Priv->MicVolume;
	} else {
		return This->Priv->PlayVolume;
	}
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerSetVolume ��������
 *
 * @param This
 * @param Volume ����
 * @param type ���������� 0���� 1¼��
 *
 * @returns ���ý��
 */
/* ----------------------------------------------------------------*/
static int mixerSetVolume(struct _TMixer *This,int type,int Volume)
{
    // pthread_mutex_lock (&This->Priv->mutex);
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
	Value = This->Priv->MinVolume + Volume*(This->Priv->MaxVolume-This->Priv->MinVolume)/100;
	DBG_P("Volume Value:%d\n",Value);

	buf = Value;
	Value <<= 8;
	Value |= buf;

	if (type) {
		if(rvMixerSetPlayVolume(Value) == EXIT_FAILURE) {
			fprintf(stdout,"Set Volume %d ,%s\n", Value,strerror(errno));
            ret = -2;
            goto mixer_set_voluem_end;
		}
		This->Priv->PlayVolume = Volume;
	} else {
		if(rvMixerSetCaptureVolume(Value) == EXIT_FAILURE) {
			fprintf(stdout,"Set Volume %d ,%s\n", Value,strerror(errno));
            ret = -2;
            goto mixer_set_voluem_end;
		}
		This->Priv->MicVolume = Volume;
	}
mixer_set_voluem_end:
    // pthread_mutex_unlock (&This->Priv->mutex);
	return ret;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerSetVolumeEx ͨ���ⲿӦ�ó�����������
 *
 * @param This
 * @param Volume ����
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static int mixerSetVolumeEx(struct _TMixer *This,int Volume)
{
	// PcmOut_setGain(Volume);
	return 0;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerInitVolume ��ʼ������
 *
 * @param This
 * @param Volume ����
 * @param bSlience 0�Ǿ��� 1����
 *
 */
/* ----------------------------------------------------------------*/
static void mixerInitVolume(struct _TMixer *This,int Volume,int bSlience)
{
	This->SetVolume(This,Volume,1);
	This->Priv->bSlience = bSlience;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerSetSlience ���þ���
 *
 * @param This
 * @param bSlience 1������0�Ǿ���
 */
/* ----------------------------------------------------------------*/

static void mixerSetSlience(struct _TMixer *This,int bSlience)
{
	This->Priv->bSlience = bSlience;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerGetSlience ���ؾ���״̬
 *
 * @param This
 *
 * @returns  1������0�Ǿ���
 */
/* ----------------------------------------------------------------*/
static int mixerGetSlience(struct _TMixer *This)
{
	return This->Priv->bSlience;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerClearRecBuffer �������¼��������
 *
 * @param This
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static void mixerClearRecBuffer(TMixer *This)
{
	return;
	int i;
	int buffersize = 1024;
	char *pBuf;
	int ret;
	if (This->Priv->audiofp1 == -1) {
		return;
	}
	pBuf = (char *)malloc(buffersize );
	if(pBuf && buffersize > 0) {
		for(i=0; i<8; i++) {
			memset(pBuf,0,buffersize);
			ret = This->Read(This,pBuf,buffersize);
			printf("ClearRecBuffer[%d]Rec buffersize :%d,real:%d\n", i,buffersize,ret);
		}
	}
	free(pBuf);

}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerClearPlayBuffer �����������������
 *
 * @param This
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
static void mixerClearPlayBuffer(TMixer *This)
{
	// PcmOut_resetAll();
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerInitPlayAndRec ��ʼ��¼���ͷ���
 *
 * @param This
 * @param handle �����������
 * @param type ENUM_MIXER_CLEAR_PLAY_BUFFER ����������
 * 			   ENUM_MIXER_CLEAR_REC_BUFFER ���¼�����
 */
/* ----------------------------------------------------------------*/
static void mixerInitPlayAndRec(TMixer *This,int *handle,int sample,int channle)
{
	int fp;
	fp = This->Open(This,FMT8K,channle);
	if (fp <= 0) {
		return;
	}
	// anykaCaptureStart(1);
	This->ClearRecBuffer(This);
	// This->ClearPlayBuffer(This);
	*handle = fp;
}

static void mixerInitPlay8K(TMixer *This,int *handle)
{
	int fp;
	fp = This->Open(This,FMT8K,1);
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
 * @param This
 * @param handle �����豸���
 */
/* ----------------------------------------------------------------*/
static void mixerDeInitPlay(TMixer *This,int *handle)
{
	if (This->Priv->audiofp1 < 0)
		return;
	rvMixerCaptureClose();
	This->Priv->audiofp1 = -1;
	This->ClearPlayBuffer(This);
	This->Close(This, handle);
}
static void mixerDeInitPlay8K(TMixer *This,int *handle)
{
	This->ClearPlayBuffer(This);
	This->Close(This, handle);
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerDestroy ���ٻ��������ͷ���Դ
 *
 * @param This
 */
/* ----------------------------------------------------------------*/
static void mixerDestroy(TMixer *This)
{
	if(This->Priv->audiofp!=-1)
		close(This->Priv->audiofp);
	if(This->Priv->mixer_fd!=-1)
		close(This->Priv->mixer_fd);
	if(This->Priv->audiofp1!=-1)
		close(This->Priv->audiofp1);
	if(This->Priv->mixer_fd1!=-1)
		close(This->Priv->mixer_fd1);
	free(This->Priv);
	free(This);
	This = NULL;
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
	TMixer *This = (TMixer *)malloc(sizeof(TMixer));
	memset(This,0,sizeof(TMixer));
	This->Priv = (TMixerPriv *)malloc(sizeof(TMixerPriv));
	memset(This->Priv,0,sizeof(TMixerPriv));

    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&This->Priv->mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

	rvMixerPlayInit();
	rvMixerCaptureInit();

	This->Priv->Inited = 0;
	This->Priv->audiofp = -1;
	This->Priv->audiofp1 = -1;

	This->Destroy = mixerDestroy;
	This->Open = mixerOpen;
	This->Close = mixerClose;
	This->Read = mixerRead;
	This->ReadBuf =  mixerReadBuffer;
	This->Write = mixerWrite;
	This->WriteBuffer = mixerWriteBuffer;
	This->InitVolume = mixerInitVolume;
	This->GetVolume  = mixerGetVolume;
	This->SetVolume  = mixerSetVolume;
	This->SetVolumeEx  = mixerSetVolumeEx;
	This->SetSlience = mixerSetSlience;
	This->GetSlience = mixerGetSlience;
	This->ClearRecBuffer = mixerClearRecBuffer;
	This->ClearPlayBuffer = mixerClearPlayBuffer;
	This->InitPlayAndRec = mixerInitPlayAndRec;
	This->InitPlay8K = mixerInitPlay8K;
	This->DeInitPlay = mixerDeInitPlay;
	This->DeInitPlay8K = mixerDeInitPlay8K;
	return This;
}

void myMixerInit(void)
{
	my_mixer = mixerCreate();
}
