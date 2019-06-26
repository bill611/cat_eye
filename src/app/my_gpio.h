/*
 * =====================================================================================
 *
 *       Filename:  MyGpioCtr.h
 *
 *    Description:  创建GPIO对象
 *
 *        Version:  1.0
 *        Created:  2015-12-24 09:26:29
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
#ifndef _MY_GPIO_H
#define _MY_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define FLASH_FOREVER	0x7FFFFFFF

	typedef enum {
		ENUM_GPIO_MICKEY,   // MIC: H外L内
		ENUM_GPIO_SPKL,		// SPK:机内 高开低关
		ENUM_GPIO_SPKR,     // SPK:机外 高开低关
		ENUM_GPIO_KEYLED1,  // 红灯 高开低关
		ENUM_GPIO_KEYLED2,  // 白灯 高开低关
		ENUM_GPIO_IRLEDEN,  // 红外灯 高开低关
		ENUM_GPIO_ASNKEY,   // 消回音 H外L内
		ENUM_GPIO_MICEN,    // 模拟电子开关电源使能脚 高关低开
		ENUM_GPIO_SDCTRL,   // TF卡2.0标准时拉高  TF卡3.0标准时拉低
		ENUM_GPIO_ICRAIN,   // 1.	A高B低 为白天彩色模式
		ENUM_GPIO_ICBAIN,   // 2.	A低B高为晚上黑白模式
	}GPIO_TBL;

	typedef enum {//50ms为周期
		FLASH_STOP = 0,
		FLASH_FAST = 4,	//200ms
		FLASH_SLOW = 20,	//1s
	}STRUCT_SPEED;

	enum {
		IO_NO_EXIST = -1,	//当IO不存在时，用于个别型号不存在该IO口
		IO_INPUT = 0,		// 输入
		IO_ACTIVE ,		// 有效 (输出)
		IO_INACTIVE,	// 无效 (输出)
	};

	struct GpioArgs{
		struct _MyGpio *gpio;
		int port;
	};

	struct _MyGpioPriv;
	typedef struct _MyGpio {
		struct _MyGpioPriv *priv;
		int io_num;		//GPIO数量
		//  设置GPIO闪烁，并执行
		void (*FlashStart)(struct _MyGpio *This,int port,int freq,int times);
		// GPIO停止闪烁
		void (*FlashStop)(struct _MyGpio *This,int port);
		// GPIO口输出赋值，不立即执行
		int (*SetValue)(struct _MyGpio *This,int port,int Value);
		// GPIO口输出赋值，并立即执行
		void (*SetValueNow)(struct _MyGpio *This,int port,int Value);
		//  读取输入IO值，并判断是否IO有效
		int (*IsActive)(struct _MyGpio *This,int port);
	    //  设置输入IO的输入有效电平时间
		void (*setActiveTime)(struct _MyGpio *This,int port,int value);
	    //  检测输入IO电平 1为有效 0为无效
		int (*inputHandle)(struct _MyGpio *This,int port);
		// 创建输出线程
		void (*addInputThread)(struct _MyGpio *This,
				struct GpioArgs *args,
			   	void *(* thread)(void *));

		void (*Destroy)(struct _MyGpio *This);
	}MyGpio;

	void gpioInit(void);
	extern MyGpio* gpio;
	extern void gpioInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
