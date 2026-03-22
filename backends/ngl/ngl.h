#ifndef NGL_H
#define NGL_H

#include "nui.h"
#include <stdint.h>

void ngl_init(void);
void ngl_fini(void);
void ngl_before_render(int width, int height);
void ngl_after_render(void);
void ngl_draw_rect(int x, int y, int w, int h, uint32_t color);
void ngl_draw_image(int x, int y, int w, int h, const struct nui_image *image);
void ngl_draw_text(const struct nui_font *font, int x, int y, const char *text, uint32_t color);
struct nui_image *ngl_load_image_from_file(const char *filename);
void ngl_unload_image(struct nui_image *image);
struct nui_font *ngl_load_font_from_file(const char *filename, float font_size);
void ngl_unload_font(struct nui_font *font);

static struct nui_backend _ngl = {
    .init = ngl_init,
    .fini = ngl_fini,
    .before_render = ngl_before_render,
    .after_render = ngl_after_render,
    .draw_rect = ngl_draw_rect,
    .draw_image = ngl_draw_image,
    .draw_text = ngl_draw_text,
    .load_image_from_file = ngl_load_image_from_file,
    .load_font_from_file = ngl_load_font_from_file,
    .unload_image = ngl_unload_image,
    .unload_font = ngl_unload_font,
};

#define NUI_BACKEND_NGL (&_ngl)

#endif
