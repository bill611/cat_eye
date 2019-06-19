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
#define UPDATE_FILE	"/tmp/Update.cab"
#define CFG_PUBLIC_DRIVE "./"
#define CFG_PRIVATE_DRIVE "./"
#define DATABSE_PATH "./"
#define QRCODE_IMIE CFG_PRIVATE_DRIVE"imei.png"
#define QRCODE_APP CFG_PRIVATE_DRIVE"app_url.png"
#else
#define UPDATE_FILE	"/tmp/Update.cab"
#define CFG_PUBLIC_DRIVE "./"
#define CFG_PRIVATE_DRIVE "./"
#define DATABSE_PATH "./"
#define QRCODE_IMIE CFG_PRIVATE_DRIVE"imei.png"
#define QRCODE_APP CFG_PRIVATE_DRIVE"app_url.png"
#endif


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

	typedef struct _Config {
        char imei[64];         // 太川设备机身码
        char hardcode[64];     // 太川设备硬件码
        char version[16];      // 太川软件版本
        char app_url[128];     // appd地址 
		int  timestamp;        // 启动时间戳
		int capture_count;	   // 抓拍图片数量
        TcWifiConfig net_config;  // 网络设置
	} Config;

	void configLoad(void);
	void ConfigSave(configCallback func);
	void ConfigSavePrivate(void);
	void ConfigSavePrivateCallback(configCallback func);
	void configSync(void);

	void tcSetNetwork(int type);
	extern Config g_config;

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
