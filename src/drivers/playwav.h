/*
 * =====================================================================================
 *
 *       Filename:  playwav.h
 *
 *    Description:  .wav文件播放驱动
 *
 *        Version:  1.0
 *        Created:  2015-11-24 14:23:39 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
#ifndef _PLAY_WAV_H
#define _PLAY_WAV_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM     1
#endif

	typedef struct {
		unsigned short Size;
		unsigned short Type;
		char adata[640];
	}SPIWAVE;

	typedef struct {
		unsigned int RIFF;	//="RIFF" 0x46464952
		unsigned int FileSize;	//
		unsigned int WAVE;	//="WAVE" 0x45564157
		unsigned int fmt;	//="fmt " 0x20746D66
	} RIFFFILEHEAD;

	typedef struct { 
		unsigned short  wFormatTag; 
		unsigned short  nChannels; 
		unsigned int nSamplesPerSec; 
		unsigned int nAvgBytesPerSec; 
		unsigned short  nBlockAlign; 
		unsigned short       wBitsPerSample; 
	} MYPCMWAVEFORMAT; 

	typedef struct {
		RIFFFILEHEAD RIFFArea;			//文件标志
		unsigned int WaveFormatSize;	//MYPCMWAVEFORMAT结构大小
		MYPCMWAVEFORMAT WaveFormat;		//音频格式
		unsigned int DataAreaSign;		//="data" 0x61746164
		unsigned int DataAreaSize;		//数据区域大小
	} WAVEFILESTRUCT;


	//播放WAV文件
	int playwavfile(char * lpfilename);
	void playWavStop();

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

