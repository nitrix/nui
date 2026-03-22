#ifndef SW_H
#define SW_H

#include "nui.h"
#include <stdint.h>

void sw_init(void);
void sw_fini(void);
void sw_before_render(int width, int height);
void sw_after_render(void);
void sw_draw_rect(int x, int y, int width, int height, uint32_t color);
void sw_draw_image(int x, int y, int width, int height, const struct nui_image *image);
void sw_draw_text(const struct nui_font *font, int x, int y, const char *text, uint32_t color);
void sw_export(const char *filename);
struct nui_image *sw_load_image_from_file(const char *filename);
void sw_unload_image(struct nui_image *image);
struct nui_font *sw_load_font_from_file(const char *filename, float font_size);
void sw_unload_font(struct nui_font *font);

static struct nui_backend _sw = {
    .init = sw_init,
    .fini = sw_fini,
    .load_image_from_file = sw_load_image_from_file,
    .unload_image = sw_unload_image,
    .before_render = sw_before_render,
    .after_render = sw_after_render,
    .draw_rect = sw_draw_rect,
    .draw_image = sw_draw_image,
    .draw_text = sw_draw_text,
    .load_font_from_file = sw_load_font_from_file,
    .unload_font = sw_unload_font,
};

#define NUI_BACKEND_SW (&_sw)

#endif
