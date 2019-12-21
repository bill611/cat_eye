#ifndef _HTTPD_GET_H_
#define _HTTPD_GET_H_

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "display.h"

enum {
    URL_TYPE_UNKNOW = 0,
    URL_TYPE_FIRMWARE,
    URL_TYPE_KERNEL,
    URL_TYPE_DTB,
    URL_TYPE_USERDATA,
    URL_TYPE_BOOT,
};

struct HTTP_RES_HEADER {
    int status_code;
    char content_type[128];
    long content_length;
};

class Httpd
{
private:
    void parse_url(const char *url, char *host, int *port, char *file_name);
    struct HTTP_RES_HEADER parse_header(char *response);
    void get_ip_addr(char *host_name, char *ip_addr);
    unsigned long get_file_size(const char *filename);
    void progress_bar(long cur_size, long total_size, double speed);
    void download(int client_socket, char *file_name, long content_length);

    char downpath[256];
    char filepath[256 + 40];

    RKDisplay* RKdisp;
public:
    Httpd(RKDisplay *disp = NULL);
    ~Httpd();
    int setDownPath(char* path);
    int get(char *url);

    struct disp_rect cap_rect;

    struct disp_cap cap;

};

#endif
