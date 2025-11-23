#ifndef SW_H
#define SW_H

#include "nui.h"
#include <stdint.h>

void sw_init(void);
void sq_fini(void);
void sw_before_render(int width, int height);
void sw_draw_rect(int x, int y, int width, int height, uint32_t color);
void sw_export(const char *filename);

static struct nui_backend _sw = {
    .init = sw_init,
    .fini = sq_fini,
    .before_render = sw_before_render,
    .draw_rect = sw_draw_rect,
};

#define NUI_BACKEND_SW (&_sw)

#endif
