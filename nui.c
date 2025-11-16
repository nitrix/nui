#include "nui.h"

void nui_arena_init(struct nui_arena *arena) {
    arena->data = NULL;
    arena->used = 0;
    arena->capacity = 0;
    arena->malloc = malloc;
    arena->free = free;
}

void nui_arena_fini(struct nui_arena *arena) {
    arena->free(arena->data);
}

void nui_context_init(struct nui_context *ctx) {
    nui_arena_init(&ctx->arena);
}

void nui_context_fini(struct nui_context *ctx) {
    nui_arena_fini(&ctx->arena);
}
