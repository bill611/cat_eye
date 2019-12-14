#ifndef CONFIG_H
#define CONFIG_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "iniparser/iniparser.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DEVICE_TYPE	"TC-U9MY-A"
#define DEVICE_SVERSION	"1.0.5"
#define DEVICE_KVERSION	"1.0.0"

#if (defined X86)
#define SDCARD_PATH "./"
#define AUDIO_PATH "./res/wav/"
#define CONFIG_FILE_PATH "./"
#define UPDATE_INI_PATH	"./"
#else
#define SDCARD_PATH "/mnt/sdcard/"
#define AUDIO_PATH "/root/usr/res/wav/"
#define CONFIG_FILE_PATH "/data/"
#define UPDATE_INI_PATH	"/tmp/"
#endif

#define DATABSE_PATH "./"						// 数据库文件路径
#define QRCODE_IMIE "./imei.png"				// 机身码二维码路径
#define QRCODE_APP "./app_url.png"				// app地址二维码路径

#define CONFIG_FILENAME CONFIG_FILE_PATH"config.ini"		// 配置文件，包括机身号等只配置一次的文件
#define CONFIG_PARA_FILENAME CONFIG_FILE_PATH"config_para.ini"	// 配置文件，参数设置,可以删除后恢复

#define CAP_PATH SDCARD_PATH"cap/"				// SD卡抓拍存储目录
#define TALK_PATH SDCARD_PATH"talk/"			// SD卡通话抓拍记录
#define ALARM_PATH SDCARD_PATH"alarm/"          // SD卡报警抓拍记录
#define FACE_PATH SDCARD_PATH"face/"			// SD卡人脸抓拍记录

#define FAST_PIC_PATH "/tmp/cap/"  				// 快速启动抓拍路径，放到内存中

#define IPC_MAIN "/tmp/ipc_main"				// 主进程
#define IPC_UART "/tmp/ipc_uart"				// 串口处理进程
#define IPC_CAMMER "/tmp/ipc_cammer"			// 摄像头处理进程

#define QINIU_URL "http://img.cateye.taichuan.com"  // 七牛云存储地址
#define UPDATE_FILE	"/tmp/Update.cab"	// LAN升级时，升级包存放位置
#define UPDATE_URL	"http://img.cateye.taichuan.com/update.ini" // 七牛云存储升级包配置文件地址
#define UPDATE_INI	UPDATE_INI_PATH"update.ini" // 七牛云存储升级配置文件地址

#define SLEEP_TIMER 20
#define SLEEP_LONG_TIMER 50
#define POWEROFF_TIMER 2

#define MAX_RINGS_NUM 5

	typedef struct _EtcValueChar {
		const char* section;	
		const char* key;
		char *value;
		unsigned int leng;
		const char *default_char;
	} EtcValueChar;

	typedef struct _EtcValueInt {
		const char* section;	
		const char* key;
		int *value;
		int default_int;
	} EtcValueInt;

	struct DevConfig{
		char device_name[64];
		char product_key[64];
		char device_secret[64];
	};

	typedef struct {
        // ethernet
		int enable;

		// station 
		char ssid[64 + 1];
		char mode[64 + 1];
		char security[128 + 1];
		char password[64 + 1];
		char running[64 + 1];

	}TcWifiConfig;

	enum {
		TC_SET_STATION,
		TC_SET_AP,
	};

	typedef void (*configCallback)(void);
	/* encry type */
	enum AWSS_ENC_TYPE {
		AWSS_ENC_TYPE_NONE,
		AWSS_ENC_TYPE_WEP,
		AWSS_ENC_TYPE_TKIP,
		AWSS_ENC_TYPE_AES,
		AWSS_ENC_TYPE_TKIPAES,
		AWSS_ENC_TYPE_MAX = AWSS_ENC_TYPE_TKIPAES,
		AWSS_ENC_TYPE_INVALID = 0xff,
	};
	/* auth type */
	enum AWSS_AUTH_TYPE {
		AWSS_AUTH_TYPE_OPEN,
		AWSS_AUTH_TYPE_SHARED,
		AWSS_AUTH_TYPE_WPAPSK,
		AWSS_AUTH_TYPE_WPA8021X,
		AWSS_AUTH_TYPE_WPA2PSK,
		AWSS_AUTH_TYPE_WPA28021X,
		AWSS_AUTH_TYPE_WPAPSKWPA2PSK,
		AWSS_AUTH_TYPE_MAX = AWSS_AUTH_TYPE_WPAPSKWPA2PSK,
		AWSS_AUTH_TYPE_INVALID = 0xff,
	};
	enum TC_AUTH_TYPE {
		TC_AUTH_TYPE_OPEN = 0,
		TC_AUTH_TYPE_SHARED = (1 << 0),
		TC_AUTH_TYPE_WPAPSK = (1 << 1),
		TC_AUTH_TYPE_WPA8021X = (1 << 2),
		TC_AUTH_TYPE_WPA2PSK = (1 << 3),
		TC_AUTH_TYPE_WPA28021X = (1 << 4),
		TC_AUTH_TYPE_WPAPSKWPA2PSK = (1 << 5),
		TC_AUTH_TYPE_MAX = TC_AUTH_TYPE_WPAPSKWPA2PSK,
		TC_AUTH_TYPE_INVALID = 0xff,
	};
	typedef struct _TcWifiScan{
		char ssid[64 + 1];     // 账号
		uint8_t bssid[16 + 1]; // mac地址
		enum AWSS_AUTH_TYPE auth;
		enum AWSS_ENC_TYPE encry;
		uint8_t channel;
		signed  char rssi;
	}TcWifiScan;

	struct wifiLowPower {
		char local_ip[16 + 1];		// 本机地址
		char dst_ip[16 + 1];		// 目的地址
		char dst_port[8 + 1];		// 目的端口
		char dst_mac[20 + 1];		// 目的mac地址
	};

	struct CapType {
		int type;	// 0 图片 1 录像
		int count; // 抓拍图片数量
		int timer; // 录像时间长短
	};
	struct Mute {
		int state;      // 0关闭免扰 1打开免扰
		int start_time; // 免扰开始时间
		int end_time;   // 免扰结束时间
	};
	typedef struct _Config {
        char imei[64 + 1];         // 设备机身码
        char hardcode[64 + 1];     // 设备硬件码
        char version[16 + 1];      // 软件版本
        char s_version[2 + 1];    // 单片机版本
        char app_url[128 + 1];     // app地址 
		int  timestamp;        // 启动时间戳
        TcWifiConfig net_config;  // 网络设置
        char f_license[128 + 1];     // 阅面人脸
		int  ring_num;        // 铃声编号
		int  ring_volume;     // 门铃音量
		int  alarm_volume;    // 报警音量
		int  talk_volume;     // 对讲音量
		int  pir_active_timer;     // PIR触发多少秒后报警
		int  pir_alarm;     // PIR触发是否发出报警音
		int  pir_strength;   // PIR触发强度,0近，1中，2远
		int  screensaver_time;   // 息屏时间
		int  brightness;   // 屏幕亮度
		int  record_time;  // 主界面录像倒计时时间
		struct Mute  mute;  // 免扰模式参数
		int  auto_sync_time;  // 是否自动同步时间

		struct wifiLowPower wifi_lowpower; // 低功耗wifi参数
		struct CapType cap_doorbell;// 按门铃抓拍或录像
		struct CapType cap_alarm;	// 报警时抓拍或录像
		struct CapType cap_talk;	// 通话时抓拍或录像
	} Config;

	void configLoad(void);
	void ConfigSavePrivate(void);
	void ConfigSavePrivateCallback(configCallback func);
	void ConfigSavePublic(void);
	void ConfigSavePublicCallback(configCallback func);

	void createSdcardDirs(void);
	void configLoadEtcChar(dictionary *cfg_ini, EtcValueChar *etc_file,
			unsigned int length);
	extern Config g_config;

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
