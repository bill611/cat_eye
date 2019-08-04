#ifndef CONFIG_H
#define CONFIG_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif


#define DEVICE_TYPE	"TC-U9MY-A"
#define DEVICE_SVERSION	"V1.0.0"
#define DEVICE_KVERSION	"V1.0.0"

#if (defined X86)
#define SDCARD_PATH "./"
#define AUDIO_PATH "./res/wav/"
#else
#define SDCARD_PATH "/mnt/sdcard/"
#define AUDIO_PATH "/root/usr/res/wav/"
#endif

#define UPDATE_FILE	"/tmp/Update.cab"
#define DATABSE_PATH "./"						// 数据库文件路径
#define QRCODE_IMIE "./imei.png"				// 机身码二维码路径
#define QRCODE_APP "./app_url.png"				// app地址二维码路径

#define CONFIG_FILENAME "./config.ini"			// 配置文件路径

#define TEMP_PIC_PATH "cap/"					// 抓拍存储记录

#define CAP_PATH SDCARD_PATH"cap/"				// SD卡抓拍存储目录
#define TALK_PATH SDCARD_PATH"talk/"			// SD卡通话抓拍记录
#define ALARM_PATH SDCARD_PATH"alarm/"          // SD卡报警抓拍记录
#define FACE_PATH SDCARD_PATH"face/"			// SD卡人脸抓拍记录

#define FAST_PIC_PATH "/tmp/cap/"  				// 快速启动抓拍路径，放到内存中

#define IPC_MAIN "/tmp/ipc_main"				// 主进程
#define IPC_CAMMER "/tmp/ipc_cammer"			// 摄像头处理进程
#define IPC_UART "/tmp/ipc_uart"				// 串口处理进程

#define QINIU_URL "http://img.cateye.taichuan.com"  // 七牛云存储地址

#define AUTO_CLOSE_LCD 11
#define SLEEP_TIMER 10
#define SLEEP_LONG_TIMER 30

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
		char ssid[64];
		char mode[64];
		char security[128];
		char password[64];
		char running[64];

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
		char ssid[64];     // 账号
		uint8_t bssid[16]; // mac地址
		enum AWSS_AUTH_TYPE auth;
		enum AWSS_ENC_TYPE encry;
		uint8_t channel;
		signed  char rssi;
	}TcWifiScan;

	struct wifiLowPower {
		char local_ip[16];		// 本机地址
		char dst_ip[16];		// 目的地址
		char dst_port[8];		// 目的端口
		char dst_mac[20];		// 目的mac地址
	};
	typedef struct _Config {
        char imei[64];         // 太川设备机身码
        char hardcode[64];     // 太川设备硬件码
        char version[16];      // 太川软件版本
        char app_url[128];     // app地址 
		int  timestamp;        // 启动时间戳
		int capture_count;	   // 抓拍图片数量
        TcWifiConfig net_config;  // 网络设置

		struct wifiLowPower wifi_lowpower; // 低功耗wifi参数
	} Config;

	void configLoad(void);
	void ConfigSavePrivate(void);
	void ConfigSavePrivateCallback(configCallback func);

	void createSdcardDirs(void);
	extern Config g_config;

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
