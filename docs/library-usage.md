# Library usage

## Lifetime

* The library must be initialized be calling `nui_init()` and finalized by calling `nui_fini()`. Depending on the backend used, this can come with additional requirements like an already set up and valid OpenGL context.

* The size of viewport needs to be maintained proper by calling `nui_viewport()` with the dimensions of the framebuffer when the window is created and when it gets resized.

## Main loop

* Every frame, before performing any call to nui, you must invoke `nui_frame()`, which, amongst other things, will reset the internal arena.

* After declaring your UI, `nui_update()` and `nui_render()` have to be called to calculate various properties of the elements and to render them.

## Backends

The backends are plug-and-play, some of them even coming with the library. `ngl` is an OpenGL renderer while `sw` is a software renderer to PNG images for testing.

You want to call `nui_init()` with the backend of your choice, like `NUI_BACKEND_NGL` or `NUI_BACKEND_SW`.

## Custom memory allocators

Even though the library uses an area, you can control how that arena aquires and releases memory with `malloc` and `free` by providing replacements with `nui_custom_memory()`.

## Syntaxic sugar

The macro NUI() isn't magical. It simply invokes `nui_element_begin()` before your code and `nui_element_end()` after your code. The body is simultanously for configuring the element and to provide the children.

So this below:
```c
NUI {
    // Configuration here.
    nui_fixed(100, 100);
    
    // Children also here.
    NUI {
        nui_fixed(50, 50);
    }
}
```

is the exact same as:

```c
struct nui_element parent = {0};
nui_element_begin(&parent);
{
    // Configuration here.
    nui_fixed(100, 100);

    // Children also here.
    struct nui_element child = {0};
    nui_element_begin(&child);
    nui_fixed(50, 50);
    nui_element_end(&child);
}
nui_element_end(&parent);
```