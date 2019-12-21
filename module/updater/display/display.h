#ifndef _RK_DISPLAY_H_
#define _RK_DISPLAY_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <pthread.h>

#include "font_bitmap.h"

struct disp_rect {
    int x;
    int y;
    int w;
    int h;
    unsigned int color;
};

struct disp_line {
    int x1;
    int y1;
    int x2;
    int y2;
    unsigned int color;
};

struct disp_cap {
    int x;
    int y;
    int str_len;
    char str[40];
    unsigned int color;
};

struct fb_info {
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    int fd;
    char *mem_va;
    unsigned size;
};

class RKDisplay
{
private:

public:
    RKDisplay();
    ~RKDisplay();
    unsigned int conv24to16(uint8_t r, uint8_t g, uint8_t b);
    unsigned int convColor(unsigned int color);
    void RKDispClean();
    void Draw(int length);
    void DrawPoint(int x, int y, unsigned int color);
    void DrawLine(struct disp_line line, bool shadow = 0);
    void DrawRect(struct disp_rect rect, bool fill = 0, bool shadow = 0);
    void DrawEnChar(int x, int y, unsigned char *codes, int color);
    void DrawCnChar(int x, int y, unsigned char *codes, int color);
    void DrawString(struct disp_cap cap);

    struct fb_info fbinfo;

};

#endif

