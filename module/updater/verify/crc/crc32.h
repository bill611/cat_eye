#ifndef _RK_CRC32_H_
#define _RK_CRC32_H_

class RKCRC
{
public:
    static unsigned int gTable_Crc32[256];
    static unsigned int CRC_32( unsigned char * aData, unsigned int aSize );
    RKCRC();
    ~RKCRC();

};

#endif
