#ifndef _MGUI_RKFB_H
#define _MGUI_RKFB_H

#include "rk_fb/rk_fb.h"

MG_EXPORT int GUIAPI rkfb_video_disp(struct win *winmsg);
MG_EXPORT struct win * GUIAPI rkfb_video_getwin(void);
MG_EXPORT int rkfb_video_getwins(struct win **win_write, struct win **win_disp);
MG_EXPORT int GUIAPI rkfb_video_close(void);
MG_EXPORT int GUIAPI rkfb_set_color_key(struct color_key *key);
MG_EXPORT int GUIAPI rkfb_video_disp_ex(struct win *winmsg,int width,int height);
MG_EXPORT void GUIAPI rkfb_get_resolution(int *width,int *height);
MG_EXPORT int GUIAPI rkfb_get_screen_state(void);
MG_EXPORT void GUIAPI rkfb_screen_on(void);
MG_EXPORT void GUIAPI rkfb_screen_off(void);
MG_EXPORT int GUIAPI rkfb_get_pixel_format(void);
#endif
