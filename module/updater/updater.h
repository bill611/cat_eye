#ifndef _RKUPDATER_H_
#define _RKUPDATER_H_

#include "httpd.h"
#include "partition.h"

enum {
    UPDATE_UNKONW = 0,
    UPDATE_KERNEL,
    UPDATE_DTB,
    UPDATE_USERDATA,
    UPDATE_BOOT,
    UPDATE_ALL,
};

class Updater
{
public:
    int showTip(char *tipcap);
    int prepare();
    int download(char* url);
    int download(int url_type);
    int checkEnvironment(char *path,int url_type);
    int runCmd(char* cmd);
    int waitAppEixt(char *app_name);
    int doUpdate(int type);
    Updater();
    ~Updater();

    RKPartition* mpart;
    RKDisplay *mdisp;

    char *updateimg[256];
    char *updateimgmd5[256];
};

#endif

