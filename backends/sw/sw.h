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
void sw_export(const char *filename);
struct nui_image *sw_load_image_from_file(const char *filename);
void sw_unload_image(struct nui_image *image);

static struct nui_backend _sw = {
    .init = sw_init,
    .fini = sw_fini,
    .load_image_from_file = sw_load_image_from_file,
    .unload_image = sw_unload_image,
    .before_render = sw_before_render,
    .after_render = sw_after_render,
    .draw_rect = sw_draw_rect,
    .draw_image = sw_draw_image,
};

#define NUI_BACKEND_SW (&_sw)

#endif
