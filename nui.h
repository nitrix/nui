#ifndef NUI_H
#define NUI_H

#include <stdlib.h>

struct nui_arena {
    char *data;
    size_t used;
    size_t capacity;
    void *(*malloc)(size_t size);
    void (*free)(void *ptr);
};

void nui_arena_init(struct nui_arena *arena);
void nui_arena_fini(struct nui_arena *arena);

struct nui_context {
    struct nui_arena arena;
};

void nui_context_init(struct nui_context *ctx);
void nui_context_fini(struct nui_context *ctx);

#endif
