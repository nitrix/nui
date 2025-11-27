#ifndef NGL_H
#define NGL_H

#include "nui.h"
#include <stdint.h>

void ngl_init(void);
void ngl_fini(void);
void ngl_before_render(int width, int height);
void ngl_after_render(void);
void ngl_draw_rect(int x, int y, int w, int h, uint32_t color);
void ngl_draw_image(int x, int y, int w, int h, struct nui_image *image);
struct nui_image *ngl_load_image_from_file(const char *filename);
void ngl_unload_image(struct nui_image *image);

static struct nui_backend _ngl = {
    .init = ngl_init,
    .fini = ngl_fini,
    .before_render = ngl_before_render,
    .after_render = ngl_after_render,
    .draw_rect = ngl_draw_rect,
    .draw_image = ngl_draw_image,
    .load_image_from_file = ngl_load_image_from_file,
    .unload_image = ngl_unload_image,
};

#define NUI_BACKEND_NGL (&_ngl)

#endif
