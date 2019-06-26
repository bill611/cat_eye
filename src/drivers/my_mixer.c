/*
 * =====================================================================================
 *
 *       Filename:  Mixer.c
 *
 *    Description:  混音器接口处理
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
	int Inited;					//是否初始化成功

	int audiofp;				//打开声卡驱动  放音
	int mixer_fd;				//混音器句柄    喇叭

	int audiofp1;				//打开声卡驱动  录音
	int mixer_fd1;				//混音器句柄	咪头

	int CurrSample;				//当前采样率
	int oss_format;				//standard 16bit little endian format, support this format only
	unsigned int MinVolume;		//最小音量
	unsigned int MaxVolume;		//最大音量
	unsigned int MicVolume;		//MIC音量
	unsigned int PlayVolume;	//播放音量
	int bSlience;				//是否静音 1静音 0非静音
}TMixerPriv;

/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
TMixer *my_mixer = NULL;

/* ----------------------------------------------------------------*/
/**
 * @brief mixerOpen 打开放音声卡
 *
 * @param this
 * @param Sample 设置采样率
 * @param ch 设置通道
 *
 * @returns 0失败
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
	//打开声卡放音句柄 确保视频文件停止播放
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
 * @brief mixerClose 关闭放音设备
 *
 * @param this
 * @param Handle 设备句柄
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
 * @brief mixerRead 从声卡读声音，录音
 *
 * @param this
 * @param pBuffer 保存录音数据
 * @param Size 一次读取字节大小
 *
 * @returns 实际读取字节数据
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
 * @brief mixerReadBuffer 从声卡读声音，录音
 *
 * @param this
 * @param AudioBuf 保存录音数据
 * @param NeedSize 一次读取字节大小
 *
 * @returns 实际读取字节数据
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
 * @brief mixerWrite 写声音缓存
 *
 * @param this
 * @param Handle 通道
 * @param pBuffer 数据
 * @param Size 大小
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
 * @brief mixerWriteBuffer 写声音缓存块
 *
 * @param this
 * @param Handle 通道
 * @param pBuffer 数据
 * @param Size 大小
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
 * @brief mixerGetVolume 获得音量
 *
 * @param this
 * @param type 音量类型 0放音 1录音
 *
 * @returns 音量值
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
 * @brief mixerSetVolume 设置音量
 *
 * @param this
 * @param Volume 音量
 * @param type 混音器类型 0放音 1录音
 *
 * @returns 设置结果
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
 * @brief mixerSetVolumeEx 通过外部应用程序设置音量
 *
 * @param this
 * @param Volume 音量
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
 * @brief mixerInitVolume 初始化音量
 *
 * @param this
 * @param Volume 音量
 * @param bSlience 0非静音 1静音
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
 * @brief mixerSetSlience 设置静音
 *
 * @param this
 * @param bSlience 1静音，0非静音
 */
/* ----------------------------------------------------------------*/

static void mixerSetSlience(struct _TMixer *this,int bSlience)
{
	this->Priv->bSlience = bSlience;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerGetSlience 返回静音状态
 *
 * @param this
 *
 * @returns  1静音，0非静音
 */
/* ----------------------------------------------------------------*/
static int mixerGetSlience(struct _TMixer *this)
{
	return this->Priv->bSlience;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerClearRecBuffer 清除声音录音缓冲区
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
 * @brief mixerClearPlayBuffer 清除声音放音缓冲区
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
 * @brief mixerInitPlayAndRec 初始化录音和放音
 *
 * @param this
 * @param handle 返回声卡句柄
 * @param type ENUM_MIXER_CLEAR_PLAY_BUFFER 清除放音句柄
 * 			   ENUM_MIXER_CLEAR_REC_BUFFER 清除录音句柄
 */
/* ----------------------------------------------------------------*/
static void mixerInitPlayAndRec(TMixer *this,int *handle)
{
	int fp;
	fp = this->Open(this,FMT8K,1);			//单声道
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
	fp = this->Open(this,FMT8K,1);			//单声道
	// printf("[%s]fp:%d\n",__FUNCTION__,fp);
	if (fp <= 0) {
		return;
	}
	// anykaCaptureStart(0);
	*handle = fp;
}

/* ----------------------------------------------------------------*/
/**
 * @brief mixerDeInitPlay 清除放音缓存，释放放音设备
 *
 * @param this
 * @param handle 放音设备句柄
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
 * @brief mixerDestroy 销毁混音器，释放资源
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
 * @brief mixerCreate 创建混音器接口
 *
 * @returns 混音器指针
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
