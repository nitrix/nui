#ifndef NGL_H
#define NGL_H

#include "nui.h"
#include <stdint.h>

void ngl_init(void);
void ngl_fini(void);
void ngl_before_render(int width, int height);
void ngl_after_render(void);
void ngl_draw_box(int x, int y, int w, int h, uint32_t fill, uint32_t border, int border_width, int radius);
void ngl_draw_rect(int x, int y, int w, int h, uint32_t color);
void ngl_draw_image(int x, int y, int w, int h, const struct nui_image *image, enum nui_image_mode mode);
void ngl_draw_text(const struct nui_font *font, int x, int y, const char *text, uint32_t color);
void ngl_measure_text(const struct nui_font *font, const char *text, int *width, int *height);
struct nui_image *ngl_load_image_from_file(const char *filename);
struct nui_image *ngl_create_image_rgba(int width, int height, const unsigned char *pixels);
bool ngl_update_image_rgba(struct nui_image *image, const unsigned char *pixels);
void ngl_unload_image(struct nui_image *image);
struct nui_font *ngl_load_font_from_file(const char *filename, float font_size);
void ngl_unload_font(struct nui_font *font);

static struct nui_backend _ngl = {
    .init = ngl_init,
    .fini = ngl_fini,
    .before_render = ngl_before_render,
    .after_render = ngl_after_render,
    .draw_box = ngl_draw_box,
    .draw_rect = ngl_draw_rect,
    .draw_image = ngl_draw_image,
    .draw_text = ngl_draw_text,
    .measure_text = ngl_measure_text,
    .load_image_from_file = ngl_load_image_from_file,
    .create_image_rgba = ngl_create_image_rgba,
    .update_image_rgba = ngl_update_image_rgba,
    .load_font_from_file = ngl_load_font_from_file,
    .unload_image = ngl_unload_image,
    .unload_font = ngl_unload_font,
};

#define NUI_BACKEND_NGL (&_ngl)

#endif
