/************************************************************************************
文件名称：
文件功能：
函数列表：
修改日期：
*************************************************************************************/

/*------------------------------------------- 头文件包含 ------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "my_echo.h"
#include "RK_VOICE_ProInterface.h"


void RK_VOICE_SetPara(short int *pshwPara, short int  shwLen)
{
	short int shwCnt;

	for (shwCnt=0; shwCnt<shwLen; shwCnt++)
	{
		pshwPara[shwCnt] = 0;
	}
#if 1
	/*------ 0.总模块参数设置 ------*/
	pshwPara[0] = 16000;                           /* INT16 Q0  采样率设置；当前仅支持8000,16000 */
	pshwPara[1] = 320;                             /* INT16 Q0  处理帧长设置；8k:160；16k:320 */

	/*------ 1.AEC 参数设置 ------*/
	pshwPara[10] = 1;                              /*           使能标志：回声消除使能 */
	pshwPara[11] = 320;                            /* INT16 Q0  处理帧长设置；8k:160；16k:320 */
	pshwPara[12] = 1;                              /* INT16 Q0  默认延时设置(单位样点数),0到3200 */
	pshwPara[13] = 0;                              /*           使能标志：延时估计使能 */
	pshwPara[14] = 160;                            /* INT16 Q0  延时估计处理帧长；8k:80；16k:160 */
	pshwPara[15] = 16;                             /* INT16 Q0  最大delay帧数设置，一帧10ms；当前固定值 */
	pshwPara[16] = 0;                              /* INT16 Q0  反向超前帧数，一帧10ms；当前固定值 0 */
	pshwPara[17] = 320;                            /* INT16 Q0  MDF处理帧长设置；当前固定值 8k:160；16k:320 */
	pshwPara[18] = 640;                            /* INT16 Q0  MDF拖尾长度设置，即滤波长度；当前固定值 8k:320；16k:640 */
	pshwPara[19] = 16000;                          /* INT16 Q0  MDF采样率设置；当前固定值 8k,16k */
	pshwPara[20] = 320;                            /* INT16 Q0  NLP处理帧长设置；当前固定值 8k:160；16k:320 */
	pshwPara[21] = 32;                             /* INT16 Q0  NLP处理子带设置；当前固定值 8k:24；16k:32 */
	pshwPara[22] = 1;                              /*           非线性回声消除COH方法使能标志 */
    pshwPara[23] = 1;                              /* INT16 Q0  COH人工回声增益次方施加 */
	pshwPara[24] = 0;                              /*           非线性回声消除SER方法使能标志 */
	pshwPara[25] = 1;                              /* INT16 Q0  SER人工回声增益次方施加 */
	pshwPara[26] = 320;                            /* INT16 Q0  DTD判决处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[27] = 32;                             /* INT16 Q0  DTD判决子带个数；当前固定值 8k:24；16k:32 */
	pshwPara[28] = 200;                            /* INT16 Q0  PASS态判断：远端信号幅度阈值 */
	pshwPara[29] = 5;                              /* INT16 Q0  PASS态判断：进入PASS态平滑帧数 */
	pshwPara[30] = 10;                             /* INT16 Q0  子带PASS态判断：子带能量阈值 */
	pshwPara[31] = 1;                              /*           使能标志：子带DTD的0,1判断 */
	pshwPara[32] = 10;                              /* INT16 Q0  远端信号大于该阈值即做子带DTD的0,1判断 */
	pshwPara[33] = 20;                             /* INT16 Q0  残差低于此阈值则做子带DTD的0判断 */
	pshwPara[34] = (short int)(0.3f*(1<<15));      /* INT16 Q15 残差与近端能量比低于此阈值则做子带DTD的0判断 */
	pshwPara[35] = 1;                              /* INT16 Q0  残差平滑DTD0,1判断时对非线性程度倍数扩充 1至5 */
	pshwPara[36] = 32;                             /* INT16 Q0  高于该子带号的子带实行高频子带强压制为0 */
	pshwPara[37] = 5;                              /* INT16 Q0  实行高频压制远端能量阈值 */
	pshwPara[38] = 1;                              /* INT16 Q0  DTD整帧判决:使用子带起始索引 1-24 */
	pshwPara[39] = 10;                              /* INT16 Q0  DTD整帧判决:使用子带终止索引 1-24 */
	pshwPara[40] = 3;                              /* INT16 Q0  DTD整帧判决:高于Gain阈值子带数高于该值判DT */
	pshwPara[41] = 3;                              /* INT16 Q0  DTD整帧判决:连续出现该值个数的ST才最终判为ST */
	pshwPara[42] = (short int)(0.30f*(1<<15));     /* INT16 Q15 DTD整帧判决:Gain阈值初始值 */
	pshwPara[43] = (short int)(0.04f*(1<<15));     /* INT16 Q15 DTD整帧判决:Gain阈值调整步长 */
	pshwPara[44] = (short int)(0.40f*(1<<15));     /* INT16 Q15 DTD整帧判决:Gain阈值最大值 */
	pshwPara[45] = (short int)(0.25f*(1<<15));     /* INT16 Q15 DTD整帧判决:Gain阈值最小值 */
	pshwPara[46] = (short int)(0.0313f*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号0 */
	pshwPara[47] = (short int)(0.0625*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号1 */
	pshwPara[48] = (short int)(0.0938*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号2 */
	pshwPara[49] = (short int)(0.1250*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号3 */
    pshwPara[50] = (short int)(0.1563*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号4 */
    pshwPara[51] = (short int)(0.1875*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号5 */
    pshwPara[52] = (short int)(0.2188*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号6 */
    pshwPara[53] = (short int)(0.2500*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号7 */
    pshwPara[54] = (short int)(0.2813*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号8 */
    pshwPara[55] = (short int)(0.3125*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号9 */
    pshwPara[56] = (short int)(0.3438*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号10 */
    pshwPara[57] = (short int)(0.3750*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号11 */
    pshwPara[58] = (short int)(0.4063*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号12 */
	pshwPara[59] = (short int)(0.4375*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号13 */
	pshwPara[60] = (short int)(0.4688*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号14 */
	pshwPara[61] = (short int)(0.5000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号15 */
	pshwPara[62] = (short int)(0.5313*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号16 */
	pshwPara[63] = (short int)(0.5625*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号17 */
	pshwPara[64] = (short int)(0.5938*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号18 */
	pshwPara[65] = (short int)(0.6250*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号19 */
	pshwPara[66] = (short int)(0.6563*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号20 */
	pshwPara[67] = (short int)(0.6875*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号21 */
	pshwPara[68] = (short int)(0.7188*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号22 */  
	pshwPara[69] = (short int)(0.7500*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号23 */
	pshwPara[70] = (short int)(0.7813*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号24 */
	pshwPara[71] = (short int)(0.8125*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号25 */
	pshwPara[72] = (short int)(0.8438*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号26 */
	pshwPara[73] = (short int)(0.8750*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号27 */
	pshwPara[74] = (short int)(0.9063*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号28 */
	pshwPara[75] = (short int)(0.9375*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号29 */
	pshwPara[76] = (short int)(0.9688*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号30 */
	pshwPara[77] = (short int)(1.0000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号31 */

	/*------ 2.ANR 参数设置 ------*/
	pshwPara[90] = 1;                              /*           使能标志：上行噪声抑制使能 */
	pshwPara[91] = 16000;                          /* INT16 Q0  ANR采样率设置，当前固定值 8k,16k */
	pshwPara[92] = 320;                            /* INT16 Q0  ANR帧长设置，当前固定值 8k:160；16k:320 */
	pshwPara[93] = 32;                             /* INT16 Q0  ANR子带个数，当前固定值 8k:24；16k:32 */
	pshwPara[94] = 2;                              /* INT16 Q0  ANR噪声估计时间片，0到10 */

	pshwPara[110] = 1;                             /*           使能标志：下行噪声抑制使能 */
	pshwPara[111] = 16000;                         /* INT16 Q0  ANR采样率设置，当前固定值 8k,16k */
	pshwPara[112] = 320;                           /* INT16 Q0  ANR帧长设置，当前固定值 8k:160；16k:320 */
	pshwPara[113] = 32;                            /* INT16 Q0  ANR子带个数，当前固定值 8k:24；16k:32 */
	pshwPara[114] = 2;                            /* INT16 Q0  ANR噪声估计时间片，0到10 */

	/*------ 3.AGC 参数配置 ------*/
	pshwPara[130] = 1;                              /*           使能标志：上行AGC消除使能 */
	pshwPara[131] = 16000;                          /* INT16 Q0  采样率配置；当前固定值 8k,16k */
	pshwPara[132] = 320;                            /* INT16 Q0  处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[133] = (short int)(6.0f*(1<<5));       /* INT16 Q5  线性段提升dB数 */
	pshwPara[134] = (short int)(-55.0f*(1<<5));     /* INT16 Q5  扩张段起始能量dB阈值 */
	pshwPara[135] = (short int)(-46.0f*(1<<5));     /* INT16 Q5  线性段起始能量dB阈值 */
	pshwPara[136] = (short int)(-24.0f*(1<<5));     /* INT16 Q5  压缩段起始能量dB阈值 */
	pshwPara[137] = (short int)(1.2f*(1<<12));      /* INT16 Q12 扩张段斜率 */ 
	pshwPara[138] = (short int)(0.8f*(1<<12));      /* INT16 Q12 线性段斜率 */  
	pshwPara[139] = (short int)(0.4f*(1<<12));      /* INT16 Q12 压缩段斜率 */ 
	pshwPara[140] = 40;                            /* INT16 Q0  扩张段时域平滑点数 */
	pshwPara[141] = 80;                            /* INT16 Q0  线性段时域平滑点数 */
	pshwPara[142] = 80;                            /* INT16 Q0  压缩段时域平滑点数 */

	pshwPara[150] = 0;                             /*           使能标志：上行AGC消除使能 */
	pshwPara[151] = 16000;                         /* INT16 Q0  采样率配置；当前固定值 8k,16k */
	pshwPara[152] = 320;                           /* INT16 Q0  处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[153] = (short int)(6.0f*(1<<5));      /* INT16 Q5  线性段提升dB数 */
	pshwPara[154] = (short int)(-55.0f*(1<<5));    /* INT16 Q5  扩张段起始能量dB阈值 */
	pshwPara[155] = (short int)(-46.0f*(1<<5));    /* INT16 Q5  线性段起始能量dB阈值 */
	pshwPara[156] = (short int)(-24.0f*(1<<5));    /* INT16 Q5  压缩段起始能量dB阈值 */
	pshwPara[157] = (short int)(1.2f*(1<<12));     /* INT16 Q12 扩张段斜率 */ 
	pshwPara[158] = (short int)(0.8f*(1<<12));     /* INT16 Q12 线性段斜率 */ 
	pshwPara[159] = (short int)(0.4f*(1<<12));     /* INT16 Q12 压缩段斜率 */
	pshwPara[160] = 40;                            /* INT16 Q0  扩张段时域平滑点数 */
	pshwPara[161] = 80;                            /* INT16 Q0  线性段时域平滑点数 */
	pshwPara[162] = 80;                            /* INT16 Q0  压缩段时域平滑点数 */

	/*------ 4.EQ 参数设置 ------*/
	pshwPara[170] = 0;                             /*           使能标志：上行EQ使能标志 */
	pshwPara[171] = 320;                           /* INT16 Q0  上行EQ处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[172] = 1;                             /* INT16 Q0  上行EQ滤波器长度 */
	pshwPara[173] = (short int)(1.0f*(1<<15));     /* INT16 Q15 上行EQ滤波器系数数组 */

	pshwPara[330] = 0;                             /*           使能标志：上行EQ使能标志 */
	pshwPara[331] = 320;                           /* INT16 Q0  上行EQ处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[332] = 1;                             /* INT16 Q0  上行EQ滤波器长度 */
	pshwPara[333] = (short int)(1.0f*(1<<15));     /* INT16 Q15 上行EQ滤波器系数数组 */

	/*------ 5.CNG 参数设置 ------*/
	pshwPara[490] = 1;                             /*           使能标志：CNG使能标志 */
	pshwPara[491] = 16000;                         /* INT16 Q0  CNG处理采样率；当前固定值 8k,16k */
	pshwPara[492] = 320;                           /* INT16 Q0  CNG处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[493] = 2;                             /* INT16 Q0 施加舒适噪声幅度比例 */
	pshwPara[494] = 10;                             /* INT16 Q0 白噪随机数生成幅度 */
	pshwPara[495] = (short int)(0.92f*(1<<15));    /* INT16 Q15 施加舒适噪声平滑度 */
	pshwPara[496] = (short int)(0.3f*(1<<15));     /* INT16 Q15 施加舒适噪声语音纹理模拟程度 */
#else
	/*------ 0.总模块参数设置 ------*/
	pshwPara[0] = 8000;                           /* INT16 Q0  采样率设置；当前仅支持8000,16000 */
	pshwPara[1] = 160;                             /* INT16 Q0  处理帧长设置；8k:160；16k:320 */

	/*------ 1.AEC 参数设置 ------*/
	pshwPara[10] = 1;                              /*           使能标志：回声消除使能 */
	pshwPara[11] = 160;                            /* INT16 Q0  处理帧长设置；8k:160；16k:320 */
	pshwPara[12] = 1;                              /* INT16 Q0  默认延时设置(单位样点数),0到3200 */
	pshwPara[13] = 0;                              /*           使能标志：延时估计使能 */
	pshwPara[14] = 80;                            /* INT16 Q0  延时估计处理帧长；8k:80；16k:160 */
	pshwPara[15] = 16;                             /* INT16 Q0  最大delay帧数设置，一帧10ms；当前固定值 */
	pshwPara[16] = 0;                              /* INT16 Q0  反向超前帧数，一帧10ms；当前固定值 0 */
	pshwPara[17] = 160;                            /* INT16 Q0  MDF处理帧长设置；当前固定值 8k:160；16k:320 */
	pshwPara[18] = 320;                            /* INT16 Q0  MDF拖尾长度设置，即滤波长度；当前固定值 8k:320；16k:640 */
	pshwPara[19] = 8000;                          /* INT16 Q0  MDF采样率设置；当前固定值 8k,16k */
	pshwPara[20] = 160;                            /* INT16 Q0  NLP处理帧长设置；当前固定值 8k:160；16k:320 */
	pshwPara[21] = 24;                             /* INT16 Q0  NLP处理子带设置；当前固定值 8k:24；16k:32 */
	pshwPara[22] = 1;                              /*           非线性回声消除COH方法使能标志 */
    pshwPara[23] = 1;                              /* INT16 Q0  COH人工回声增益次方施加 */
	pshwPara[24] = 0;                              /*           非线性回声消除SER方法使能标志 */
	pshwPara[25] = 1;                              /* INT16 Q0  SER人工回声增益次方施加 */
	pshwPara[26] = 160;                            /* INT16 Q0  DTD判决处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[27] = 24;                             /* INT16 Q0  DTD判决子带个数；当前固定值 8k:24；16k:32 */
	pshwPara[28] = 200;                            /* INT16 Q0  PASS态判断：远端信号幅度阈值 */
	pshwPara[29] = 5;                              /* INT16 Q0  PASS态判断：进入PASS态平滑帧数 */
	pshwPara[30] = 20;                             /* INT16 Q0  子带PASS态判断：子带能量阈值 */
	pshwPara[31] = 1;                              /*           使能标志：子带DTD的0,1判断 */
	pshwPara[32] = 0;                              /* INT16 Q0  远端信号大于该阈值即做子带DTD的0,1判断 */
	pshwPara[33] = 50;                             /* INT16 Q0  残差低于此阈值则做子带DTD的0判断 */
	pshwPara[34] = (short int)(0.1f*(1<<15));      /* INT16 Q15 残差与近端能量比低于此阈值则做子带DTD的0判断 */
	pshwPara[35] = 1;                              /* INT16 Q0  残差平滑DTD0,1判断时对非线性程度倍数扩充 1至5 */
	pshwPara[36] = 24;                             /* INT16 Q0  高于该子带号的子带实行高频子带强压制为0 */
	pshwPara[37] = 5;                              /* INT16 Q0  实行高频压制远端能量阈值 */
	pshwPara[38] = 1;                              /* INT16 Q0  DTD整帧判决:使用子带起始索引 1-24 */
	pshwPara[39] = 5;                              /* INT16 Q0  DTD整帧判决:使用子带终止索引 1-24 */
	pshwPara[40] = 1;                              /* INT16 Q0  DTD整帧判决:高于Gain阈值子带数高于该值判DT */
	pshwPara[41] = 3;                              /* INT16 Q0  DTD整帧判决:连续出现该值个数的ST才最终判为ST */
	pshwPara[42] = (short int)(0.05f*(1<<15));     /* INT16 Q15 DTD整帧判决:Gain阈值初始值 */
	pshwPara[43] = (short int)(0.05f*(1<<15));     /* INT16 Q15 DTD整帧判决:Gain阈值调整步长 */
	pshwPara[44] = (short int)(0.05f*(1<<15));     /* INT16 Q15 DTD整帧判决:Gain阈值最大值 */
	pshwPara[45] = (short int)(0.05f*(1<<15));     /* INT16 Q15 DTD整帧判决:Gain阈值最小值 */                         
	pshwPara[46] = (short int)(0.0417*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号0 */
	pshwPara[47] = (short int)(0.0833*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号1 */
	pshwPara[48] = (short int)(0.1250*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号2 */
	pshwPara[49] = (short int)(0.1667*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号3 */
    pshwPara[50] = (short int)(0.2083*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号4 */
    pshwPara[51] = (short int)(0.2500*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号5 */
    pshwPara[52] = (short int)(0.2917*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号6 */
    pshwPara[53] = (short int)(0.3333*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号7 */
    pshwPara[54] = (short int)(0.3750*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号8 */
    pshwPara[55] = (short int)(0.4167*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号9 */
    pshwPara[56] = (short int)(0.4583*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号10 */
    pshwPara[57] = (short int)(0.5000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号11 */
    pshwPara[58] = (short int)(0.5417*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号12 */
	pshwPara[59] = (short int)(0.5833*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号13 */
	pshwPara[60] = (short int)(0.6250*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号14 */
	pshwPara[61] = (short int)(0.6667*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号15 */
	pshwPara[62] = (short int)(0.7083*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号16 */
	pshwPara[63] = (short int)(0.7500*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号17 */
	pshwPara[64] = (short int)(0.7917*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号18 */
	pshwPara[65] = (short int)(0.8333*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号19 */
	pshwPara[66] = (short int)(0.8750*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号20 */
	pshwPara[67] = (short int)(0.9167*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号21 */
	pshwPara[68] = (short int)(0.9583*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号22 */  
	pshwPara[69] = (short int)(1.0000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号23 */
	pshwPara[70] = (short int)(1.0000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号24 */
	pshwPara[71] = (short int)(1.0000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号25 */
	pshwPara[72] = (short int)(1.0000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号26 */
	pshwPara[73] = (short int)(1.0000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号27 */
	pshwPara[74] = (short int)(1.0000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号28 */
	pshwPara[75] = (short int)(1.0000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号29 */
	pshwPara[76] = (short int)(1.0000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号30 */
	pshwPara[77] = (short int)(1.0000*(1<<15));     /* INT16 Q15 子带非线性程度配置，子带号31 */

	/*------ 2.ANR 参数设置 ------*/
	pshwPara[90] = 1;                              /*           使能标志：上行噪声抑制使能 */
	pshwPara[91] = 8000;                          /* INT16 Q0  ANR采样率设置，当前固定值 8k,16k */
	pshwPara[92] = 160;                            /* INT16 Q0  ANR帧长设置，当前固定值 8k:160；16k:320 */
	pshwPara[93] = 24;                             /* INT16 Q0  ANR子带个数，当前固定值 8k:24；16k:32 */
	pshwPara[94] = 5;                             /* INT16 Q0  ANR噪声估计时间片，0到10 */

	pshwPara[110] = 0;                             /*           使能标志：下行噪声抑制使能 */
	pshwPara[111] = 8000;                         /* INT16 Q0  ANR采样率设置，当前固定值 8k,16k */
	pshwPara[112] = 160;                           /* INT16 Q0  ANR帧长设置，当前固定值 8k:160；16k:320 */
	pshwPara[113] = 24;                            /* INT16 Q0  ANR子带个数，当前固定值 8k:24；16k:32 */
	pshwPara[114] = 5;                            /* INT16 Q0  ANR噪声估计时间片，0到10 */

	/*------ 3.AGC 参数配置 ------*/
	pshwPara[130] = 1;                              /*           使能标志：上行AGC消除使能 */
	pshwPara[131] = 8000;                          /* INT16 Q0  采样率配置；当前固定值 8k,16k */
	pshwPara[132] = 160;                            /* INT16 Q0  处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[133] = (short int)(6.0f*(1<<5));       /* INT16 Q5  线性段提升dB数 */
	pshwPara[134] = (short int)(-55.0f*(1<<5));     /* INT16 Q5  扩张段起始能量dB阈值 */
	pshwPara[135] = (short int)(-46.0f*(1<<5));     /* INT16 Q5  线性段起始能量dB阈值 */
	pshwPara[136] = (short int)(-24.0f*(1<<5));     /* INT16 Q5  压缩段起始能量dB阈值 */
	pshwPara[137] = (short int)(1.2f*(1<<12));      /* INT16 Q12 扩张段斜率 */ 
	pshwPara[138] = (short int)(0.8f*(1<<12));      /* INT16 Q12 线性段斜率 */  
	pshwPara[139] = (short int)(0.4f*(1<<12));      /* INT16 Q12 压缩段斜率 */ 
	pshwPara[140] = 20;                            /* INT16 Q0  扩张段时域平滑点数 */
	pshwPara[141] = 40;                            /* INT16 Q0  线性段时域平滑点数 */
	pshwPara[142] = 40;                            /* INT16 Q0  压缩段时域平滑点数 */

	pshwPara[150] = 0;                             /*           使能标志：上行AGC消除使能 */
	pshwPara[151] = 8000;                         /* INT16 Q0  采样率配置；当前固定值 8k,16k */
	pshwPara[152] = 160;                           /* INT16 Q0  处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[153] = (short int)(6.0f*(1<<5));      /* INT16 Q5  线性段提升dB数 */
	pshwPara[154] = (short int)(-55.0f*(1<<5));    /* INT16 Q5  扩张段起始能量dB阈值 */
	pshwPara[155] = (short int)(-46.0f*(1<<5));    /* INT16 Q5  线性段起始能量dB阈值 */
	pshwPara[156] = (short int)(-24.0f*(1<<5));    /* INT16 Q5  压缩段起始能量dB阈值 */
	pshwPara[157] = (short int)(1.2f*(1<<12));     /* INT16 Q12 扩张段斜率 */ 
	pshwPara[158] = (short int)(0.8f*(1<<12));     /* INT16 Q12 线性段斜率 */ 
	pshwPara[159] = (short int)(0.2f*(1<<12));     /* INT16 Q12 压缩段斜率 */
	pshwPara[160] = 20;                            /* INT16 Q0  扩张段时域平滑点数 */
	pshwPara[161] = 40;                            /* INT16 Q0  线性段时域平滑点数 */
	pshwPara[162] = 40;                            /* INT16 Q0  压缩段时域平滑点数 */

	/*------ 4.EQ 参数设置 ------*/
	pshwPara[170] = 0;                             /*           使能标志：上行EQ使能标志 */
	pshwPara[171] = 160;                           /* INT16 Q0  上行EQ处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[172] = 1;                             /* INT16 Q0  上行EQ滤波器长度 */
	pshwPara[173] = (short int)(1.0f*(1<<15));     /* INT16 Q15 上行EQ滤波器系数数组 */

	pshwPara[330] = 0;                             /*           使能标志：上行EQ使能标志 */
	pshwPara[331] = 160;                           /* INT16 Q0  上行EQ处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[332] = 1;                             /* INT16 Q0  上行EQ滤波器长度 */
	pshwPara[333] = (short int)(1.0f*(1<<15));     /* INT16 Q15 上行EQ滤波器系数数组 */

	/*------ 5.CNG 参数设置 ------*/
	pshwPara[490] = 1;                             /*           使能标志：CNG使能标志 */
	pshwPara[491] = 8000;                         /* INT16 Q0  CNG处理采样率；当前固定值 8k,16k */
	pshwPara[492] = 160;                           /* INT16 Q0  CNG处理帧长；当前固定值 8k:160；16k:320 */
	pshwPara[493] = 2;                             /* INT16 Q0 施加舒适噪声幅度比例 */
	pshwPara[494] = 10;                             /* INT16 Q0 白噪随机数生成幅度 */
	pshwPara[495] = (short int)(0.92f*(1<<15));    /* INT16 Q15 施加舒适噪声平滑度 */
	pshwPara[496] = (short int)(0.3f*(1<<15));     /* INT16 Q15 施加舒适噪声语音纹理模拟程度 */
#endif
}

void rkEchoInit(void)
{
	// short int ashwPara[500] = {0};

	// RK_VOICE_SetPara(ashwPara, 500);

	// [> 初始化 <]
	// VOICE_Init(ashwPara);
}
void rkEchoUnInit(void)
{
	// VOICE_Destory();
}

void rkEchoTx(short int *pshwIn,
	    short int *pshwRef,
		short int *pshwOut,
		int swFrmLen)
{
	// VOICE_ProcessTx(pshwIn, pshwRef, pshwOut, swFrmLen);
}

void rkEchoRx(short int *pshwIn, 
		short int *pshwOut, 
		int swFrmLen)
{
	// VOICE_ProcessRx(pshwIn, pshwOut, swFrmLen);
}

int test()
{
	// int swCnt, swFrmLen;
	// int swFrmNum;

	// FILE *fp_in  = fopen("../test/TEST_ALL_Case1/in_411.pcm", "rb");
	// FILE *fp_rx  = fopen("../test/TEST_ALL_Case1/ref_411.pcm", "rb");
	// FILE *fp_out = fopen("../test/TEST_ALL_Case1/rk_411_out_31.pcm", "wb");
	

	// short int ashwTxIn[320]  = {0};
	// short int ashwTxOut[320] = {0};
	// short int ashwRxIn[320]  = {0};
	// short int ashwRxOut[320] = {0};
	
	// short int ashwPara[500] = {0};

	// RK_VOICE_SetPara(ashwPara, 500);

	// [> 初始化 <]
	// VOICE_Init(ashwPara);

	// swFrmNum = 0;
	// swFrmLen = 320;

	// [> 2.主体功能函数测试<]
	// while (!feof(fp_in) && !feof(fp_rx))
	// {
		// fread(ashwTxIn, sizeof(short), swFrmLen, fp_in);
		// fread(ashwRxIn, sizeof(short), swFrmLen, fp_rx);

		// swFrmNum++;
		// if (16 == swFrmNum)
		// {
			// swFrmNum = swFrmNum;
		// }

		// [> 调用单帧处理函数 <]
		// VOICE_ProcessRx(ashwRxIn, ashwRxOut, swFrmLen);
		// VOICE_ProcessTx(ashwTxIn, ashwRxIn, ashwTxOut, swFrmLen);
		

		// fwrite(ashwTxOut, sizeof(short), swFrmLen, fp_out);
	// }

	// [> 3.测试环境清理 <]
	// VOICE_Destory();

	// fclose(fp_in);
	// fclose(fp_rx);
	// fclose(fp_out);

}






