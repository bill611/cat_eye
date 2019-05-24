#include "rk_fb/rk_fb.h"

#define COLOR_KEY_R 0x0
#define COLOR_KEY_G 0x0
#define COLOR_KEY_B 0x1

void display_clean_uiwin(void)
{
    struct win * ui_win;
    struct color_key color_key;
    unsigned short rgb565_data;
    unsigned short *ui_buff;
    int i;
    int w, h;

    ui_win = rk_fb_getuiwin();
    ui_buff = (unsigned short *)ui_win->buffer;

    /* enable and set color key */
    color_key.enable = 1;
    color_key.red = (COLOR_KEY_R & 0x1f) << 3;
    color_key.green = (COLOR_KEY_G & 0x3f) << 2;
    color_key.blue = (COLOR_KEY_B & 0x1f) << 3;
    rk_fb_set_color_key(color_key);

    rk_fb_get_out_device(&w, &h);

    /* set ui win color key */
    rgb565_data = (COLOR_KEY_R & 0x1f) << 11 | ((COLOR_KEY_G & 0x3f) << 5) | (COLOR_KEY_B & 0x1f);
    for (i = 0; i < w * h; i ++) {
        ui_buff[i] = rgb565_data;
    }
}

int display_init()
{
	rk_fb_init(FB_FORMAT_RGB_565);
	rk_fb_set_yuv_range(CSC_BT601F);
	display_clean_uiwin();
}


