#ifndef NGL_H
#define NGL_H

#include <stdint.h>

void ngl_init(void);
void ngl_fini(void);
void ngl_prepare(int width, int height);
void ngl_draw_rect(int x, int y, int w, int h, uint32_t color);

#endif
