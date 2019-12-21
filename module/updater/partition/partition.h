#ifndef _PARTTITION_H_
#define _PARTTITION_H_

#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <time.h>

#include "httpd.h"
#include "display.h"

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

#define RK_PARTITION_TAG (0x50464B52)

typedef enum {
    PART_VENDOR = 1 << 0,
    PART_IDBLOCK = 1 << 1,
    PART_KERNEL = 1 << 2,
    PART_BOOT = 1 << 3,
    PART_RECOVERY = 1 << 4,
    PART_DTB = 1 << 5,
    PART_UBOOT = 1 << 6,
    PART_USER = 1 << 31
} enum_parttion_type;

typedef struct {
    uint16  year;
    uint8   month;
    uint8   day;
    uint8   hour;
    uint8   min;
    uint8   sec;
    uint8   reserve;
} struct_datetime, *pstruct_datetime;

typedef struct {
    uint32  uiFwTag; //"RKFP"
    struct_datetime dtReleaseDataTime;
    uint32  uiFwVer;
    uint32  uiSize;//size of sturct,unit of uint8
    uint32  uiPartEntryOffset;//unit of sector
    uint32  uiBackupPartEntryOffset;
    uint32  uiPartEntrySize;//unit of uint8
    uint32  uiPartEntryCount;
    uint32  uiFwSize;//unit of uint8
    uint8   reserved[464];
    uint32  uiPartEntryCrc;
    uint32  uiHeaderCrc;
} struct_fw_header, *pstruct_fw_header;

typedef struct {
    uint8   szName[32];
    enum_parttion_type emPartType;
    uint32  uiPartOffset;//unit of sector
    uint32  uiPartSize;//unit of sector
    uint32  uiDataLength;//unit of uint8
    uint32  uiPartProperty;
    uint8   reserved[76];
} struct_part_entry, *pstruct_part_entry;

typedef struct {
    struct_fw_header hdr;     //0.5KB
    struct_part_entry part[12]; //1.5KB
} struct_part_info, *pstruct_part_info;


typedef struct tag_kernel_hdr {
    unsigned char magic[16];  // "KERNEL"

    unsigned int loader_load_addr;           /* physical load addr ,default is 0x60000000*/
    unsigned int loader_load_size;           /* size in bytes */
    unsigned int crc32;                      /* crc32 */
    unsigned int hash_len;                   /* 20 or 32 , 0 is no hash*/
    unsigned char hash[32];     /* sha */

    unsigned int js_hash;
    unsigned char reserved[1024 - 32 - 32 - 4];
    uint32 signTag; //0x4E474953, 'N' 'G' 'I' 'S'
    uint32 signlen; //256
    unsigned char rsaHash[256];
    unsigned char reserved2[2048 - 1024 - 256 - 8];
} kernel_hdr;   //Size:2K


typedef struct Setting_ini_info {
    char UserPart[40];
    char szname[40];
    char storge_path[40];
    char mount_path[40];
    int type;
} struct_setting_ini;

class RKPartition
{
private:
    unsigned int getFileSize(char* filename);
    int readFile(int fd, void *buffer, int size);
    int writeFile(int fd, void *buffer, int size);
    int getFwPartInfo(struct_part_info *Info);
    int getStoragePartInfo(struct_part_info *Info);
    int setStoragePartInfo(struct_part_info *Info);
    int setPartBootPriority(struct_part_info *Info, int parttype);
    void debugPartsInfos(struct_part_info *info);
    int checkPartsInfos();
    int checkKernelCRC(char *path, kernel_hdr *hdr);
    int checkImage(char *partname);
    int checkImage(int parttype);
    unsigned int getPartOffset_bytype(struct_part_info info, uint32 type);
    int getPartIndex_bytype(struct_part_info info, uint32 type);
    int getAPartIndex_bytype(struct_part_info info, uint32 type);
    int getBPartIndex_bytype(struct_part_info info, uint32 type);
    unsigned int getPartOffset_byname(struct_part_info info, char *name);
    int getPartIndex_byname(struct_part_info info, char *name);
    int updatePart(struct_setting_ini info);
    int fwPartitionInit();
    int partitionInit(char *partname);
    int partitionInit(int parttype);

public:
    int setImagePath(char *name);
    int setImageType(int type);
    int checkPartitions(int parttype);
    int RKPartition_init();
    int update(int parttype);
    RKPartition(RKDisplay *disp = NULL);
    ~RKPartition();

    struct_part_info mPartInfo;
    struct_part_info mFWPartInfo;

    kernel_hdr mKernelHdr;
    kernel_hdr mFWKernelHdr;

    char imagepath[40];
    int  imagetype;

    RKDisplay* RKdisp;
    struct disp_rect box_rect;
    struct disp_rect fill_rect;
    struct disp_cap cap;
};


#endif

