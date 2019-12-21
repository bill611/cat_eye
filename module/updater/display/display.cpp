#include "display.h"


typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;

unsigned int RKDisplay::conv24to16(uint8_t r, uint8_t g, uint8_t b)
{
    return ((((r >> 3) & 0x1F) << 11) | (((g >> 2) & 0x2F) << 5) | ((b >> 3) & 0x1F));
}

unsigned int RKDisplay::convColor(unsigned int color)
{
    if (fbinfo.vinfo.bits_per_pixel == 16)
        return ((color >> 3) & 0x1F) | (((color >> 10) & 0x3F) << 5) | (((color >> 19) & 0x1F) << 11);
    return color;
}

void RKDisplay::RKDispClean()
{
    if ((long)(fbinfo.mem_va) != -1) {
        memset(fbinfo.mem_va, 0, fbinfo.size);
    }
}

void RKDisplay::DrawPoint(int x, int y, unsigned int color)
{
    int offset;
    int bytes;

    bytes = fbinfo.vinfo.bits_per_pixel >> 3;
    offset = x * bytes + y * fbinfo.vinfo.xres * bytes;

    if (fbinfo.vinfo.bits_per_pixel == 16) {
        *(fbinfo.mem_va + offset) = color & 0xff;
        *(fbinfo.mem_va + offset + 1) = (color >> 8) & 0xff;
    } else {
        *(fbinfo.mem_va + offset) = color & 0xff;
        *(fbinfo.mem_va + offset + 1) = (color >> 8) & 0xff;
        *(fbinfo.mem_va + offset + 2) = (color >> 16) & 0xff;
        *(fbinfo.mem_va + offset + 3) = 0;
    }

}

void RKDisplay::DrawLine(struct disp_line line, bool shadow)
{
    for (int j = line.y1; j <= line.y2; j++) {
        if (shadow) {
            unsigned char b = 255 * j / (fbinfo.vinfo.xres - 10);
            unsigned char g = 255;
            unsigned char r = 255;
            line.color = conv24to16(r, g, b);
        }
        for (int i = line.x1; i <= line.x2; i++)
            DrawPoint(i, j, line.color);
    }
}

void RKDisplay::DrawRect(struct disp_rect rect, bool fill, bool shadow)
{
    struct disp_line line_top;
    struct disp_line line_bottom;
    struct disp_line line_left;
    struct disp_line line_right;

    line_top.x1 = rect.x;
    line_top.x2 = rect.x + rect.w;
    line_top.y2 = line_top.y1 = rect.y;
    line_top.color = rect.color;

    line_bottom.x1 = rect.x;
    line_bottom.x2 = rect.x + rect.w;
    line_bottom.y2 = line_bottom.y1 = rect.y + rect.h;
    line_bottom.color = rect.color;

    line_left.x1 = line_left.x2 = rect.x;
    line_left.y1 = rect.y;
    line_left.y2 = rect.y + rect.h;
    line_left.color = rect.color;

    line_right.x1 = line_right.x2 = rect.x + rect.w;
    line_right.y1 = rect.y;
    line_right.y2 = rect.y + rect.h;
    line_right.color = rect.color;

    if (fill) {
        line_left.y2--;
        for (int i = rect.x; i <= (rect.w + rect.x); i++) {
            line_left.x1 = line_left.x2 = i;
            DrawLine(line_left, shadow);
        }
    } else {
        DrawLine(line_top);
        DrawLine(line_bottom);
        DrawLine(line_left);
        DrawLine(line_right);
    }

    usleep(10 * 1000);

}

void RKDisplay::DrawEnChar(int x, int y, unsigned char *codes, int color)
{
    int i = 0;

    for (i = 0; i < 16; ++ i) {
        int j = 0;
        x += 8;
        for (j = 0; j < 8; ++j) {
            --x;
            if ((codes[i] >> j) & 0x1)
                DrawPoint(x , y, color);
        }
        ++y;
    }
}

void RKDisplay::DrawCnChar(int x, int y, unsigned char *codes, int color)
{

}

void RKDisplay::DrawString(struct disp_cap cap)
{
    int pos = 0;
    int x = cap.x;
    int y = cap.y;
    unsigned char *ptr;
    unsigned int ch;
    unsigned int cl;
    unsigned int offset;


    while (*(cap.str + pos)) {
        ch = (unsigned int)cap.str[pos];
        cl = (unsigned int)cap.str[pos + 1];

        if (( ch >= 0xa1) && (ch < 0xf8) && (cl >= 0xa1) && (cl < 0xff)) {
            offset = ((ch - 0xa1) * 94 + (cl - 0xal)) * 32;
            ptr = __ASCII8X16__ + offset;
            //DrawCnChar(x, y, ptr, cap.color);
            x += 16;
            pos += 2;
        } else {
            ptr = __ASCII8X16__ + 16 * ch;
            DrawEnChar(x, y + 4, ptr, cap.color);
            x += 8;
            pos += 1;
        }
    }


}

RKDisplay::RKDisplay()
{
    fbinfo.fd = open("/dev/fb0", O_RDWR);
    if (!fbinfo.fd) {
        printf("Error: cannot open framebuffer device.\n");
        return;
    }

    /* Get fixed fi information */
    if (ioctl(fbinfo.fd, FBIOGET_FSCREENINFO, &fbinfo.finfo)) {
        printf("Error reading fixed information.\n");
        return;
    }

    /* Get variable fi information */
    if (ioctl(fbinfo.fd, FBIOGET_VSCREENINFO, &fbinfo.vinfo)) {
        printf("Error reading variable information.\n");
        return;
    }

    /* Figure out the size of */
    fbinfo.size = fbinfo.vinfo.xres * fbinfo.vinfo.yres * fbinfo.vinfo.bits_per_pixel / 8;

    /* Map the device to memory */
    fbinfo.mem_va = (char *)mmap(0, fbinfo.size,
                                 PROT_READ | PROT_WRITE, MAP_SHARED, fbinfo.fd, 0);
    if ((long)(fbinfo.mem_va) == -1) {
        printf("Error: failed to map framebuffer device to memory.\n");
        return;
    }
}

RKDisplay::~RKDisplay()
{
    munmap(fbinfo.mem_va, fbinfo.size);
    close(fbinfo.fd);
    fbinfo.fd = -1;
}


#if 0
int main(int argc, char const *argv[])
{
    int pos = 0;
    RKDisplay* RKdisp = new RKDisplay();


    struct disp_rect box_rect;
    box_rect.x = 100;
    box_rect.y = 270;
    box_rect.w = 280;
    box_rect.h = 60;
    box_rect.color = RKdisp->convColor(0xFFFFFFFF);


    struct disp_rect fill_rect;
    fill_rect.x = box_rect.x;
    fill_rect.y = box_rect.y;
    fill_rect.w = box_rect.w / 10;
    fill_rect.h = box_rect.h;
    fill_rect.color = RKdisp->convColor(0xFFFFFFFF);

    struct disp_cap cap;
    memcpy(cap.str, "update kernel", sizeof("update kernel"));
    cap.str_len = strlen(cap.str) * 8;
    cap.x = (RKdisp->fbinfo.vinfo.xres - cap.str_len) >> 1;
    cap.y = 200;
    cap.color = 0xFFFFFFFF;

    while (1) {
        if (!(pos % 10)) {
            RKdisp->RKDispClean();
            RKdisp->DrawRect(box_rect);
            RKdisp->DrawString(cap);
            pos = 0;
        }
        fill_rect.x = box_rect.x + pos * fill_rect.w;
        RKdisp->DrawRect(fill_rect, 1);
        usleep(1000 * 1000);
        pos++;
    }

    delete RKdisp;
    RKdisp = NULL;
    return 0;
}
#endif


