#include "partition.h"
#include "crc32.h"
#ifdef USE_EMMC
//build/setting-emmc-sec-rootfs-abpart.ini
static struct_setting_ini g_setting_ini[8] = {
    {"Fw Header", "",             "/dev/mmcblk0",   "",     0},
    {"UserPart1", "IDBlock",      "/dev/mmcblk0p2", "",     PART_IDBLOCK},
    {"UserPart2", "kernel",       "/dev/mmcblk0p3", "",     PART_KERNEL},
    {"UserPart3", "dtb",          "/dev/mmcblk0p4", "",     PART_DTB},
    {"UserPart4", "userdata",     "/dev/mmcblk0p5", "data", PART_USER},
    {"UserPart5", "root",         "/dev/mmcblk0p6", "root", PART_BOOT},
    {"UserPart6", "kernel_b",     "/dev/mmcblk0p7", "",     PART_KERNEL},
    {"UserPart7", "dtb_b",        "/dev/mmcblk0p8", "",     PART_DTB},
};
#endif

#ifdef USE_NOR
//build/setting-sec-rootfs.ini
static struct_setting_ini g_setting_ini[8] = {
    {"Fw Header", "",          	"/dev/mtdblock8", "",     0},
    {"UserPart1", "IDBlock",   	"/dev/mtdblock1", "",     PART_IDBLOCK},
    {"UserPart2", "dsp",       	"/dev/mtdblock2", "",     PART_USER},
    {"UserPart3", "kernel",    	"/dev/mtdblock3", "",     PART_KERNEL},
    {"UserPart4", "dtb",     	"/dev/mtdblock4", "",     PART_DTB},
    {"UserPart5", "userdata",   "/dev/mtdblock5", "data", PART_USER},
    {"UserPart6", "root",       "/dev/mtdblock6", "root", PART_BOOT},
    {"UserPart7", "face_model", "/dev/mtdblock7", "root/usr/face_model",PART_USER},
};
#endif


#define BUFFER_64K (1024<<6)

unsigned int RKPartition::getFileSize(char* filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return -1;
    fseek(fp, 0L, SEEK_END);
    unsigned int size = ftell(fp);
    fclose(fp);
    return size;
}

int RKPartition::readFile(int fd, void *buffer, int size)
{
    int readcnd = 0;
    int ret = 0;
    unsigned char *p = (unsigned char*)buffer;

    if (NULL == p) {
        printf("%s invalid input params \n", __FUNCTION__);
        return -1;
    }
    readcnd = 0;
    ret = 0;

    while (readcnd < size) {
        ret = read(fd, p + readcnd, size - readcnd);
        if (ret <= 0) {
            if (ret < 0) {
                perror("read");
            }
            return readcnd;
        }
        readcnd += ret;
    }
    return readcnd;

}

int RKPartition::writeFile(int fd, void *buffer, int size)
{
    int writecnd = 0;
    int ret = 0;
    unsigned char *p = (unsigned char*)buffer;

    if (NULL == p) {
        printf("%s invalid input params \n", __FUNCTION__);
        return -1;
    }

    writecnd = 0;
    ret = 0;

    while (writecnd < size) {
        ret = write(fd, p + writecnd, size - writecnd);
        if (ret <= 0) {
            printf("ret = %d\n", ret);
            if (ret < 0) {
                perror("write");
            }
            return -1;
        }
        writecnd += ret;
    }
    return writecnd;

}

int RKPartition::getFwPartInfo(struct_part_info *Info)
{
    int imagefd;

    imagefd = open(imagepath, O_RDONLY);
    if (imagefd < 0) {
        printf("Firmware open %s failed \n", imagepath);
        return -1;
    }

    if (read(imagefd, Info, sizeof(struct_part_info)) <= 0) {
        printf("Firmware read %s failed \n", imagepath);
        return -1;
    }

    if (Info->hdr.uiFwTag != RK_PARTITION_TAG) {
        printf("Firmware Tag error\n");
        return -1;
    }

    close(imagefd);
    return 0;
}

int RKPartition::getStoragePartInfo(struct_part_info *Info)
{
    int flashfd;

    flashfd = open(g_setting_ini[0].storge_path, O_RDONLY);
    if (flashfd < 0) {
        printf("Flash open %s failed \n", g_setting_ini[0].storge_path);
        return -1;
    }

    if (read(flashfd, Info, sizeof(struct_part_info)) <= 0) {
        printf("Flash read %s failed \n", g_setting_ini[0].storge_path);
        return -1;
    }

    if (Info->hdr.uiFwTag != RK_PARTITION_TAG) {
        printf("Flash Tag error\n");
        return -1;
    }

    close(flashfd);

    return 0;
}

int RKPartition::setStoragePartInfo(struct_part_info *Info)
{
    int flashfd;

    flashfd = open(g_setting_ini[0].storge_path, O_RDWR);
    if (flashfd < 0) {
        printf("Flash open %s failed \n", g_setting_ini[0].storge_path);
        return -1;
    }

    if (write(flashfd, Info, sizeof(struct_part_info)) <= 0) {
        printf("Flash write %s failed \n", g_setting_ini[0].storge_path);
        return -1;
    }

    if (Info->hdr.uiFwTag != RK_PARTITION_TAG) {
        printf("Flash Tag error\n");
        return -1;
    }

    fsync(flashfd);
    sync();
    sleep(1);
    fsync(flashfd);
    sync();

    close(flashfd);
    return 0;
}

int RKPartition::setPartBootPriority(struct_part_info *Info, int parttype)
{
    int index[2];

    if (parttype == PART_KERNEL || parttype == PART_DTB) {
        index[0] = getAPartIndex_bytype(*Info, parttype);
        index[1] = getBPartIndex_bytype(*Info, parttype);
        Info->part[index[0]].uiPartProperty = 1;
        Info->part[index[1]].uiPartProperty = 0;
        printf("\told BootPriority index %d > index %d\n", index[0], index[1]);
        printf("\tnew BootPriority index %d > index %d\n", index[1], index[0]);
        setStoragePartInfo(Info);
    }

    return 0;
}

void RKPartition::debugPartsInfos(struct_part_info *info)
{
    printf("PartsInfos:\n");
    printf("\t :%x\n", info->hdr.uiFwTag);

    for (int i = 0; i < info->hdr.uiPartEntryCount; i++) {
        printf("\tszName:%s\n", info->part[i].szName);
        printf("\t\t   tuiPartSize:%x\n", info->part[i].uiPartSize);
        printf("\t\t  uiPartOffset:%x\n", info->part[i].uiPartOffset);
        printf("\t\t  uiDataLength:%x\n", info->part[i].uiDataLength);
        printf("\t\tuiPartProperty:%x\n", info->part[i].uiPartProperty);
    }
}


int RKPartition::checkPartsInfos()
{
    if (mFWPartInfo.hdr.uiPartEntryCount != mPartInfo.hdr.uiPartEntryCount) {
        printf("The number of partitions is inconsistent\n");
        return -1;
    }

    for (int i = 0; i < mPartInfo.hdr.uiPartEntryCount; i++) {
        if (strcmp((const char *)mFWPartInfo.part[i].szName,
                   (const char *)mPartInfo.part[i].szName) != 0) {
            printf("The name of partitions is inconsistent\n");
            return -1;
        }
        if (mFWPartInfo.part[i].uiDataLength > (mPartInfo.part[i].uiPartSize << 9)) {
            printf("The szie of partitions is out of rang\n");
            return -1;
        }
    }

    return 0;
}

int RKPartition::checkKernelCRC(char *path, kernel_hdr *hdr)
{
    int fd;
    int readsize = 0;
    struct_part_info info;
    unsigned int kernel_offset;
    unsigned int load_size;
    unsigned char *kernel_data;

    if (strcmp(path, imagepath) == 0) {
        printf("####check Firmware kernel:\n");
        memcpy(&info, &mFWPartInfo, sizeof(struct_part_info));
        kernel_offset = getPartOffset_bytype(info, PART_KERNEL);
    } else {
        printf("\tcheck Storage kernel:\n");
        memcpy(&info, &mPartInfo, sizeof(struct_part_info));
        kernel_offset = 0;
    }
    printf("\t%s read kernel_addr %x \n", path, kernel_offset);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("%s open failed \n", path);
        return -1;
    }

    lseek(fd, kernel_offset << 9, SEEK_SET);
    if (read(fd, hdr, sizeof(kernel_hdr)) <= 0) {
        printf("%s read hdr failed \n", path);
        return -1;
    }

    load_size = hdr->loader_load_size;
    kernel_data = (unsigned char *)malloc(load_size);
    printf("\t%s read hdr->crc32 %x \n", path, hdr->crc32);

    lseek(fd, (kernel_offset + 4) << 9, SEEK_SET);
    readsize = readFile(fd, kernel_data, load_size);
    if (readsize < 0) {
        printf("%s read kernel_data failed \n", path);
        return -1;
    }

    RKCRC* rkcrc = new RKCRC();
    unsigned int crc32 = rkcrc->CRC_32(kernel_data, load_size);
    delete rkcrc;
    rkcrc = NULL;
    printf("\t%s CRC_32 crc32 %x \n", path, crc32);

    close(fd);
    free(kernel_data);

    return 0;
}

int  RKPartition::checkImage(char *partname)
{
    int index;
    unsigned int size;

    index = getPartIndex_byname(mPartInfo, partname);
    if (index < 0) {
        printf("checkimage getPartIndex_byname %s failed\n", partname);
        return -1;
    }

    size = getFileSize(imagepath) >> 9;
    if (size > mPartInfo.part[index].uiPartSize) {
        printf("checkimage %s image size is too large error\n", partname);
        return -1;
    }

    return 0;
}

int  RKPartition::checkImage(int parttype)
{
    int index;
    unsigned int size;

    index = getPartIndex_bytype(mPartInfo, parttype);
    if (index < 0) {
        printf("checkimage getPartIndex_bytype %d failed\n", parttype);
        return -1;
    }

    size = getFileSize(imagepath) >> 9;
    if (size > mPartInfo.part[index].uiPartSize) {
        printf("checkimage type %s image size is too large error\n", parttype);
        return -1;
    }

    return 0;
}

int RKPartition::fwPartitionInit()
{
    if (getFwPartInfo(&mFWPartInfo) != 0) {
        printf("getFwPart failed\n");
        return -1;
    }

    if (checkPartsInfos() != 0) {
        printf("check PartsInfos failed\n");
        return -1;
    }
    if (checkKernelCRC(imagepath, &mFWKernelHdr) != 0) {
        printf("check Firmware Kernel crc failed\n");
        return -1;
    }

    return 0;
}

int RKPartition::partitionInit(char *partname)
{

    if (checkImage(partname) != 0) {
        printf("checkimage failed\n");
        return -1;
    }

    return 0;
}

int RKPartition::partitionInit(int parttype)
{

    if (checkImage(parttype) != 0) {
        printf("checkimage failed\n");
        return -1;
    }

    return 0;
}

int RKPartition::checkPartitions(int url_type)
{
    int ret;

    if (url_type == URL_TYPE_FIRMWARE)
        ret = fwPartitionInit();
    else if (url_type == URL_TYPE_KERNEL)
        ret = partitionInit(PART_KERNEL);
    else if (url_type == URL_TYPE_DTB)
        ret = partitionInit(PART_DTB);
    else if (url_type == URL_TYPE_USERDATA)
        ret = partitionInit(PART_USER);
    else
        return -1;
    return ret;
}

int RKPartition::RKPartition_init()
{
    if (getStoragePartInfo(&mPartInfo) != 0) {
        printf("get Storage Part failed\n");
        return -1;
    }

    return 0;
}

int RKPartition::setImagePath(char *name)
{
    if (name != NULL)
        strcpy(imagepath, name);

    return 0;
}

int RKPartition::setImageType(int type)
{
    imagetype = type;

    return 0;
}

unsigned int RKPartition::getPartOffset_bytype(struct_part_info info, uint32 type)
{
    uint32 i;

    if (info.hdr.uiFwTag == RK_PARTITION_TAG)
        for (i = 0; i < info.hdr.uiPartEntryCount; i++)
            if (info.part[i].emPartType == type) {
                if (info.part[i].uiPartProperty == 0)
                    return info.part[i].uiPartOffset;
            }
    return 0;
}

int RKPartition::getPartIndex_bytype(struct_part_info info, uint32 type)
{
    int i;

    if (info.hdr.uiFwTag == RK_PARTITION_TAG)
        for (i = 0; i < info.hdr.uiPartEntryCount; i++)
            if (info.part[i].emPartType == type) {
                if (info.part[i].uiPartProperty == 0)
                    return i;
            }
    return 0;
}

int RKPartition::getAPartIndex_bytype(struct_part_info info, uint32 type)
{
    int i, cnt = 0;
    int index[4] = { -1, -1, -1, -1};

    if (info.hdr.uiFwTag == RK_PARTITION_TAG)
        for (i = 0; i < info.hdr.uiPartEntryCount; i++)
            if (info.part[i].emPartType == type) {
                index[cnt++] = i;
            }

    if (index[1] < 0)
        return index[0];

    if (info.part[index[0]].uiPartProperty == 0)
        return index[0];
    else
        return index[1];
}

int RKPartition::getBPartIndex_bytype(struct_part_info info, uint32 type)
{
    int i, cnt = 0;
    int index[4] = { -1, -1, -1, -1};

    if (info.hdr.uiFwTag == RK_PARTITION_TAG)
        for (i = 0; i < info.hdr.uiPartEntryCount; i++)
            if (info.part[i].emPartType == type) {
                index[cnt++] = i;
            }

    if (index[1] < 0)
        return index[0];

    if (info.part[index[0]].uiPartProperty == 0)
        return index[1];

    if (info.part[index[0]].uiPartProperty != 0)
        return index[0];
}

unsigned int RKPartition::getPartOffset_byname(struct_part_info info, char *name)
{
    uint32 i;

    if (info.hdr.uiFwTag == RK_PARTITION_TAG)
        for (i = 0; i < info.hdr.uiPartEntryCount; i++)
            if (strcmp((const char *)info.part[i].szName, (const char *)name) == 0)
                return info.part[i].uiPartOffset;
    return 0;
}

int RKPartition::getPartIndex_byname(struct_part_info info, char *name)
{
    int i;

    if (info.hdr.uiFwTag == RK_PARTITION_TAG)
        for (i = 0; i < info.hdr.uiPartEntryCount; i++)
            if (strcmp((const char *)info.part[i].szName, (const char *)name) == 0)
                return i;
    return -1;
}

int RKPartition::updatePart(struct_setting_ini info)
{
    int imagefd;
    int storagefd;
    int part_index = 0;
    uint32 part_offset = 0;
    uint32 part_datalen = 0;
    kernel_hdr hdr;
    uint32 rlen, wlen;
    long rwremain;
    int percent = 0, old_percent = 0;

    unsigned char* ubuf = (unsigned char*)malloc(BUFFER_64K);

    printf("####update part:%s dev:%s image:%s:\n",
           info.szname, info.storge_path, imagepath);

    if (imagetype == URL_TYPE_FIRMWARE) {
        printf("\tupdate image get from firmware\n");
        part_index = getPartIndex_bytype(mFWPartInfo, info.type);
        if (part_index < 0) {
            printf("update getPartIndex_bytype %s failed\n", info.szname);
            return -1;
        }
        part_offset  = mFWPartInfo.part[part_index].uiPartOffset;
        part_datalen = mFWPartInfo.part[part_index].uiDataLength;
    } else {
        printf("\tupdate image get from part image\n");
        part_offset = 0;
        part_datalen = getFileSize(imagepath);
    }

    imagefd = open(imagepath, O_RDONLY);
    if (imagefd < 0) {
        printf("update open %s failed \n", imagepath);
        return -1;
    }
    lseek(imagefd, part_offset << 9, SEEK_SET);

    if ((storagefd = open(info.storge_path, O_RDWR)) < 0)
        if (imagefd < 0) {
            printf("update open %s failed \n", info.storge_path);
            close(imagefd);
            return -1;
        }

    if (RKdisp != NULL) {
        RKdisp->RKDispClean();
        RKdisp->DrawRect(box_rect);

        sprintf(cap.str, "update %s...", info.szname);
        cap.str_len = strlen(cap.str) * 8;
        cap.y = 200;
        cap.x = (RKdisp->fbinfo.vinfo.xres - cap.str_len) >> 1;
        cap.color = 0xFFFFFFFF;
        RKdisp->DrawString(cap);
    }

    rwremain = part_datalen;
    while (rwremain > 0) {

        if (RKdisp != NULL) {
            percent = ((part_datalen - rwremain) * 100) / part_datalen;
            for (int i = old_percent; i <= percent; i++) {
                fill_rect.x = box_rect.x + (i / 10) * fill_rect.w;
                RKdisp->DrawRect(fill_rect, 1, 1);
            }
            old_percent = percent;
        }

        //printf("\tupdate %s remain 0x%x byte\n", part_name, rwremain, percent);
        //read
        rlen = readFile(imagefd, ubuf, BUFFER_64K);
        if (rlen < 0) {
            printf("update read %s data failed \n", info.szname);
            close(storagefd);
            close(imagefd);
            return -1;
        }

        //write
        wlen = writeFile(storagefd, ubuf, rlen);
        if (wlen != rlen) {
            printf("update write %s data failed \n", info.storge_path);
            close(storagefd);
            close(imagefd);
            return -1;
        }

        rwremain -= wlen;
        fsync(storagefd);
        sync();
    }

    if (RKdisp != NULL) {
        RKdisp->RKDispClean();
        sprintf(cap.str, "wait update %s sync", info.szname);
        cap.str_len = strlen(cap.str) * 8;
        cap.x = (RKdisp->fbinfo.vinfo.xres - cap.str_len) >> 1;
        cap.y = 300;
        RKdisp->DrawString(cap);
    }

    sleep(3);
    fsync(storagefd);
    sync();
    close(storagefd);
    close(imagefd);
    free(ubuf);

    if (info.type == PART_KERNEL) {
        if (checkKernelCRC(info.storge_path, &hdr) != 0) {
            printf("update %s crc32 failed \n", info.storge_path);
            return -1;
        }
    }

#ifdef USE_EMMC
    setPartBootPriority(&mPartInfo, info.type);
#endif

    return 0;
}


RKPartition::RKPartition(      RKDisplay *disp)
    : RKdisp(disp)
{
    if (RKdisp != NULL) {
        box_rect.x = 100;
        box_rect.y = 270;
        box_rect.w = 280;
        box_rect.h = 60;
        box_rect.color = RKdisp->convColor(0xFFFFFFFF);

        fill_rect.x = box_rect.x;
        fill_rect.y = box_rect.y;
        fill_rect.w = box_rect.w / 10;
        fill_rect.h = box_rect.h;
        fill_rect.color = RKdisp->convColor(0xFFFFFFFF);
    }
}

RKPartition::~RKPartition()
{

}

int RKPartition::update(int parttype)
{
    int index;

    index = getBPartIndex_bytype(mPartInfo, parttype);

    if (parttype == PART_USER || PART_BOOT == parttype) {
        if (umount2(g_setting_ini[index].mount_path, MNT_FORCE | MNT_DETACH) < 0) {
            printf("ERROR: umount2 %s fail\n", g_setting_ini[index].mount_path);
        }
    }

    updatePart(g_setting_ini[index]);

    return 0;
}

#if 0
int main(int argc, char const *argv[])
{
    RKPartition* RKpart = new RKPartition();

    //read partinfos and check fw
    if (RKpart->RKPartition_init() != 0) {
        printf("RKPartition_init failed\n");
        return -1;
    }
    if (RKpart->update_kernel() != 0) {
        printf("update_kernel failed\n");
        return -1;
    }

    if (RKpart->update_userdata() != 0) {
        printf("update_userdata failed\n");
        return -1;
    }

    delete RKpart;
    RKpart = NULL;
    return 0;
}
#endif

