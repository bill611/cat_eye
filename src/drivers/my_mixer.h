/*
 * =====================================================================================
 *
 *       Filename:  Mixer.h
 *
 *    Description:  �����������ӿ�
 *
 *        Version:  1.0
 *        Created:  2015-12-22 10:32:49 
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
#ifndef _TMIXER_H
#define _TMIXER_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define FMT8K 8000
#define FMT22K 22050
#define FMT44K 44100
#define	UDA_SET_RATE		0x101
#define	UDA_SET_VOLUME		0x102
#define	UDA_START_PLAY		0x103
#define	UDA_START_REC		0x104
#define	UDA_START_RP		0x105
#define	UDA_STOP			0x106
#define UDA_GET_VOLUME		0x107
#define UDA_CTRLCLEARECHO	0x110

	struct _TMixerPriv;

	typedef enum {
		ENUM_MIXER_CLEAR_NONE = 0x00,
		ENUM_MIXER_CLEAR_PLAY_BUFFER= 0x01,
		ENUM_MIXER_CLEAR_REC_BUFFER = 0x02,
		ENUM_MIXER_CLEAR_ALL_BUFFER = 0xff
	}ENUM_MIXER_CLEAR_TYPE;

	typedef struct _TMixer
	{
		struct _TMixerPriv *Priv;
		void (*Destroy)(struct _TMixer *this);
		int (*Open)(struct _TMixer *this,int Sample,int CH);
		int (*Close)(struct _TMixer *this,int *Handle);
		int (*Read)(struct _TMixer *this,void *pBuffer,int Size);
		int (*ReadBuf)(struct _TMixer *this,void *AudioBuf,int NeedSize);
		int (*WriteBuffer)(struct _TMixer *this,int Handle,const void *pBuffer,int Size);
		int (*Write)(struct _TMixer *this,int Handle,const void *pBuffer,int Size);
		void (*InitVolume)(struct _TMixer *this,int Volume,int bSlience);	//��ʼ������
		int (*GetVolume)(struct _TMixer *this,int type);					//��������
		int (*SetVolume)(struct _TMixer *this,int Volume,int type);			//��������
		int (*SetVolumeEx)(struct _TMixer *this,int Volume);				//���ⲿ������������
		void (*SetSlience)(struct _TMixer *this,int bSlience);				//�����Ƿ���
		int (*GetSlience)(struct _TMixer *this);							//�����Ƿ���
		void (*ClearRecBuffer)(struct _TMixer *this);
		void (*ClearPlayBuffer)(struct _TMixer *this);
		void (*InitPlayAndRec)(struct _TMixer *this, int *handle);
		void (*InitPlay8K)(struct _TMixer *this, int *handle);
		void (*DeInitPlay)(struct _TMixer *this, int *handle);
		void (*DeInitPlay8K)(struct _TMixer *this, int *handle);
	} TMixer;

	//����һ��������
	TMixer* mixerCreate(void);

	void myMixerInit(void);
	extern TMixer *my_mixer;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif


