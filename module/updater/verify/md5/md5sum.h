#ifndef _RK_MD5_H_
#define _RK_MD5_H_

#define MD5_LEN 32

class RKMD5
{
public:
    static int md5sum(char *file_name, char *md5_sum);
    static int readmd5sum(char *file_name, char *md5_sum);
    RKMD5();
    ~RKMD5();

};

#endif

