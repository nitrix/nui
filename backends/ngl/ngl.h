#ifndef NGL_H
#define NGL_H

#include "nui.h"
#include <stdint.h>

void ngl_init(void);
void ngl_fini(void);
void ngl_before_render(int width, int height);
void ngl_draw_rect(int x, int y, int w, int h, uint32_t color);

static struct nui_backend _ngl = {
    .init = ngl_init,
    .fini = ngl_fini,
    .before_render = ngl_before_render,
    .draw_rect = ngl_draw_rect,
};

#define NUI_BACKEND_NGL (&_ngl)

#endif
