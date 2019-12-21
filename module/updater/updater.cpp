#include <unistd.h>
#include <getopt.h>

#include "updater.h"
#include "md5sum.h"

#define SERVER_HTTP_ADDRESS       "http://yourserver.com/"

#define FIREWARM_URL     SERVER_HTTP_ADDRESS"Firmware.img"
#define FIREWARM_MD5_URL SERVER_HTTP_ADDRESS"Firmware.md5"
#define KERNEL_URL       SERVER_HTTP_ADDRESS"kernel.img"
#define KERNEL_MD5_URL   SERVER_HTTP_ADDRESS"kernel.md5"
#define DTB_URL          SERVER_HTTP_ADDRESS"dtb"
#define DTB_MD5_URL      SERVER_HTTP_ADDRESS"dtb.md5"
#define USERDATA_URL     SERVER_HTTP_ADDRESS"userdata.img"
#define USERDATA_MD5_URL SERVER_HTTP_ADDRESS"userdata.md5"
#define BOOT_URL         SERVER_HTTP_ADDRESS"sec-rootfs.img"
#define BOOT_MD5_URL     SERVER_HTTP_ADDRESS"sec-rootfs.md5"

#define DOWNLOAD_PATH           "/mnt/sdcard/"
#define LOCAL_FIREWARM_PATH     DOWNLOAD_PATH"Firmware.img"
#define LOCAL_FIREWARM_MD5_PATH DOWNLOAD_PATH"Firmware.md5"
#define LOCAL_KERNEL_PATH       DOWNLOAD_PATH"kernel.img"
#define LOCAL_KERNEL_MD5_PATH   DOWNLOAD_PATH"kernel.md5"
#define LOCAL_DTB_PATH          DOWNLOAD_PATH"dtb"
#define LOCAL_DTB_MD5_PATH      DOWNLOAD_PATH"dtb.md5"
#define LOCAL_USERDATA_PATH     DOWNLOAD_PATH"userdata.img"
#define LOCAL_USERDATA_MD5_PATH DOWNLOAD_PATH"userdata.md5"

int Updater::showTip(char *tipcap)
{
    struct disp_cap cap;
    if (mdisp != NULL) {
        mdisp->RKDispClean();
        sprintf(cap.str, tipcap);
        cap.str_len = strlen(cap.str) * 8;
        cap.y = 300;
        cap.x = (mdisp->fbinfo.vinfo.xres - cap.str_len) >> 1;
        cap.color = 0xFFFFFFFF;
        mdisp->DrawString(cap);
    }
    return 0;
}

int Updater::prepare()
{
    int ret;

    mdisp = new RKDisplay();
    if (mdisp == NULL) {
        printf("RKDisplay instantiation failed\n");
        return -1;
    }

    mpart = new RKPartition(mdisp);
    if (mpart == NULL) {
        printf("RKPartition instantiation failed\n");
        return -1;
    }
    if (mpart->RKPartition_init() != 0) {
        printf("RKPartition init failed\n");
        return -1;
    }

    return 0;
}

int Updater::download(char* url)
{
    return 0;
}

int Updater::download(int url_type)
{
    switch (url_type) {
    case URL_TYPE_FIRMWARE:
        download((char *) FIREWARM_URL);
        download((char *) FIREWARM_MD5_URL);
        break;
    case URL_TYPE_KERNEL:
        download((char *) KERNEL_URL);
        download((char *) KERNEL_MD5_URL);
        break;
    case URL_TYPE_DTB:
        download((char *) DTB_URL);
        download((char *) DTB_MD5_URL);
        break;
    case URL_TYPE_USERDATA:
        download((char *) USERDATA_URL);
        download((char *) USERDATA_MD5_URL);
    case URL_TYPE_BOOT:
        download((char *) BOOT_URL);
        download((char *) BOOT_MD5_URL);
        break;
    default:
        break;
    }
    return 0;
}

int Updater::checkEnvironment(int url_type)
{
    char md5path[80];
    char imagepath[80];

    char verifymd5sum[MD5_LEN + 1];
    char filemd5sum[MD5_LEN + 1];

    switch (url_type) {
    case URL_TYPE_FIRMWARE:
        strcpy(imagepath, (char *)LOCAL_FIREWARM_PATH);
        strcpy(md5path, (char *)LOCAL_FIREWARM_MD5_PATH);
        break;
    case URL_TYPE_KERNEL:
        strcpy(imagepath, (char *)LOCAL_KERNEL_PATH);
        strcpy(md5path, (char *)LOCAL_KERNEL_MD5_PATH);
        break;
    case URL_TYPE_DTB:
        strcpy(imagepath, (char *)LOCAL_DTB_PATH);
        strcpy(md5path, (char *)LOCAL_DTB_MD5_PATH);
        break;
    case URL_TYPE_USERDATA:
        strcpy(imagepath, (char *)LOCAL_USERDATA_PATH);
        strcpy(md5path, (char *)LOCAL_USERDATA_MD5_PATH);
    default:
        break;
    }

    RKMD5::md5sum(imagepath, verifymd5sum);
    RKMD5::readmd5sum(md5path, filemd5sum);
    if (strcmp(filemd5sum, verifymd5sum) != 0) {
        printf("verify md5sum failed\n");
        return -1;
    }

    mpart->setImagePath(imagepath);
    mpart->setImageType(url_type);
    if (mpart->checkPartitions(url_type) != 0) {
        printf("checkPartitions failed\n");
        return -1;
    }

    return 0;
}

int Updater::runCmd(char* cmd)
{
    char buffer[BUFSIZ];
    FILE* read_fp;
    int chars_read;
    int ret;

    memset(buffer, 0, BUFSIZ);
    read_fp = popen(cmd, "r");
    if (read_fp != NULL) {
        chars_read = fread(buffer, sizeof(char), BUFSIZ - 1, read_fp);
        if (chars_read > 0) {
            ret = 1;
        } else {
            ret = -1;
        }
        pclose(read_fp);
    } else {
        ret = -1;
    }

    return ret;
}

int Updater::waitAppEixt(char *app_name)
{

    char cmd[128];
    char buf[512];
    char compare_str[20];
    static int try_times = 0;

    sprintf(cmd, "busybox killall %s", app_name);
    runCmd(cmd);
    usleep(100 * 1000);

    sprintf(cmd, "ps | busybox grep %s", app_name);
    sprintf(compare_str, "busybox grep %s", app_name);

    FILE * fp = popen(cmd, "r");
    if (!fp) {
        perror("popen ps | grep wlan_setting fail");
        return -1;
    }
    while (fgets(buf, sizeof(buf), fp)) {
        try_times++;
        if (try_times > 15) {
            printf("wait_app_killer %s can not be killed! \n", app_name);
            break;
        }
        if ( !strstr(buf, compare_str) && strstr(buf, app_name) && !strstr(buf, " Z ") ) {
            puts(buf);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int Updater::doUpdate(int type)
{
    bool rwkernel = false;
    bool rwdtb = false;
    bool rwuserdata = false;
    bool rwboot = false;

    switch (type) {
    case UPDATE_KERNEL:
        rwkernel = true;
        break;
    case UPDATE_DTB:
        rwdtb = true;
        break;
    case UPDATE_USERDATA:
        rwuserdata = true;
        break;
    case UPDATE_BOOT:
        rwboot = true;
        break;
    case UPDATE_ALL:
        rwkernel = true;
        rwuserdata = true;
        rwdtb = true;
        rwboot = true;
    default:
        break;
    }

    if (rwdtb) {
        if (mpart->update(PART_DTB) != 0) {
            printf("update_kernel failed\n");
            return -1;
        }
    }

    if (rwkernel) {
        if (mpart->update(PART_KERNEL) != 0) {
            printf("update_kernel failed\n");
            return -1;
        }
    }

    if (rwuserdata) {
        if (mpart->update(PART_USER) != 0) {
            printf("update_userdata failed\n");
            return -1;
        }
    }

    if (rwboot) {
        if (mpart->update(PART_BOOT) != 0) {
            printf("update_userdata failed\n");
            return -1;
        }
    }
}

Updater::Updater()
    : mpart(NULL)
    , mdisp(NULL)
{

}

Updater::~Updater()
{
    if (mpart != NULL) {
        delete mpart;
        mpart = NULL;
    }

    if (mdisp != NULL) {
        delete mdisp;
        mdisp = NULL;
    }

}

int main(int argc, char* argv[])
{
    int opt;
    int url_type = URL_TYPE_FIRMWARE;
    int update_type = UPDATE_ALL;

    while (1) {
        static struct option opts[] = {
            {"image type",  required_argument, 0, 'i'},  //all/kernel/dtb/user/boot
            {"update part", required_argument, 0, 'p'},  //all/kernel/dtb/user/boot
            {"help", no_argument, 0, 'h'},
        };

        int i = 0;
        opt = getopt_long(argc, argv, "i:p:", opts, &i);
        if (opt == -1)
            break;

        switch (opt) {
        case 'i':
            if (!strcmp(optarg, "all"))
                url_type = URL_TYPE_FIRMWARE;
            else if (!strcmp(optarg, "kernel"))
                url_type = URL_TYPE_KERNEL;
            else if (!strcmp(optarg, "dtb"))
                url_type = URL_TYPE_DTB;
            else if (!strcmp(optarg, "user"))
                url_type = URL_TYPE_USERDATA;
            else if (!strcmp(optarg, "boot"))
                url_type = URL_TYPE_BOOT;
            break;
        case 'p':
            if (!strcmp(optarg, "all"))
                update_type = UPDATE_ALL;
            else if (!strcmp(optarg, "kernel"))
                update_type = UPDATE_KERNEL;
            else if (!strcmp(optarg, "dtb"))
                update_type = UPDATE_DTB;
            else if (!strcmp(optarg, "user"))
                update_type = UPDATE_USERDATA;
            else if (!strcmp(optarg, "boot"))
                update_type = UPDATE_BOOT;
            break;
        case 'h':
            printf("updater -i [all/kernel/dtb/user/boot] -p [all/kernel/dtb/user/boot]\n");
            printf("\t-i image type, default all\n");
            printf("\t-p update part, default all\n");
            return 0;
        }
    }

    Updater* updater = new Updater();
    if (updater->prepare() != 0) {
        printf("updater prepare failed\n");
        return -1;
    }

    updater->showTip((char *)"wait for other app exit!");
    printf("####to waitAppEixt:\n");
    updater->waitAppEixt((char *)"cat_eye");
    printf("####waitAppEixt done:\n");
    printf("\n");


    // updater->showTip((char *)"prepare for donwload image!");
    // if (updater->download(url_type) != 0) {
      // printf("updater checkEnvironment failed\n");
      // return -1;
    // }

    updater->showTip((char *)"check image...");
    if (updater->checkEnvironment(url_type) != 0) {
        printf("updater checkEnvironment failed\n");
        return -1;
    }

    if (updater->doUpdate(update_type) != 0) {
        printf("updater failed\n");
        updater->showTip((char *)"updater failed!");
        return -1;
    }

    updater->showTip((char *)"updater success!");

    delete updater;
    updater = NULL;

	// system("busybox reboot");
    return 0;
}





