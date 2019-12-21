#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "md5sum.h"

#define STR_VALUE(val) #val
#define STR(name) STR_VALUE(name)

#define PATH_LEN 256

int RKMD5::md5sum(char *file_name, char *md5_sum)
{
#define MD5SUM_CMD_FMT "busybox md5sum %." STR(PATH_LEN) "s 2>/dev/null"
    char cmd[PATH_LEN + sizeof (MD5SUM_CMD_FMT)];
    sprintf(cmd, MD5SUM_CMD_FMT, file_name);
#undef MD5SUM_CMD_FMT
    int i, ch;

    FILE *p = popen(cmd, "r");
    if (p == NULL)
        return -1;

    for (i = 0; i < MD5_LEN && isxdigit(ch = fgetc(p)); i++) {
        *md5_sum++ = ch;
    }
    *md5_sum = '\0';
    pclose(p);

    if (i != MD5_LEN) {
        puts("Error occured!");
        return -1;
    } else {
        return 0;
    }
}

int RKMD5::readmd5sum(char *file_name, char *md5_sum)
{

    int fd, ret;

    fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        printf("readmd5sum open %s failed \n", file_name);
        return -1;
    }
    ret = read(fd, md5_sum, MD5_LEN + 1);
    if (ret != MD5_LEN + 1) {
        printf("readmd5sum read %s failed \n", file_name);
        return -1;
    }

    md5_sum[MD5_LEN] = '\0';

    close(fd);

    return 0;
}


RKMD5::RKMD5()
{
    printf("RKMD5()\n");

}

RKMD5::~RKMD5()
{
    printf("~RKMD5()\n");

}

#if 0
int main(int argc, char *argv[])
{
    char md5_sum[MD5_LEN + 1];

    RKMD5::md5sum((char *)"./Firmware.img", md5_sum);
    printf("md5_sum is: %s\n", md5_sum);


}
#endif
