# Library usage

## Lifetime

* The library must be initialized be calling `nui_init()` and finalized by calling `nui_fini()`. Depending on the backend used, this can come with additional requirements like an already set up and valid OpenGL context.

* The size of viewport needs to be maintained proper by calling `nui_viewport()` with the dimensions of the framebuffer when the window is created and when it gets resized.

* Images are expected to be manually preloaded with `nui_load_image_from_file()` which produces a handle for later uses and then cleaned up with `nui_unload_image()`.

## Main loop

* Every frame, before performing any call to nui, you must invoke `nui_frame()`, which, amongst other things, will reset the internal arena.

* After declaring your UI, `nui_update()` and `nui_render()` have to be called to calculate various properties of the elements and to render them.

## Input

`nui` owns platform-agnostic input state, but it does not own a platform layer.
Applications translate their own windowing library events into calls like:

```c
nui_input_mouse_move(x, y);
nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, down);
nui_input_key(NUI_KEY_BACKSPACE, down, modifiers);
nui_input_text_utf8(text);
```

Transient input such as `pressed`, `released`, scroll, mouse delta, and text
input is cleared by `nui_frame()`. A typical frame therefore looks like this:

```c
nui_frame();
poll_platform_events_and_feed_nui_input();

// Declare UI here.

nui_update();
nui_render();
```

Persistent state such as whether a mouse button or key is down remains available
across frames through `nui_get_input()`.

## Widgets

The optional `nuiw` layer builds immediate-mode controls on top of regular
`nui` elements. It consumes the core input snapshot and keeps only interaction
state such as hot, active, focused, selected, and open combo IDs.

For widgets, call `nuiw_begin_frame()` after feeding input, then call
`nuiw_end_frame()` after `nui_update()` so widgets can harvest computed element
rectangles for next-frame hit testing:

```c
nui_frame();
poll_platform_events_and_feed_nui_input();
nuiw_begin_frame();

NUI {
    nuiw_panel_begin();
        if (nuiw_button("Apply")) {
            // Respond to click.
        }
    nuiw_panel_end();
}

nui_update();
nuiw_end_frame();
nui_render();
```

When labels are not unique, push an ID scope around repeated widgets:

```c
for (uint64_t i = 0; i < count; i++) {
    nuiw_push_id_u64(i);
    nuiw_checkbox("Visible", &items[i].visible);
    nuiw_pop_id();
}
```

`nuiw_theme_high_contrast()` is the first built-in widget theme. It follows the
High Contrast look: very dark panels, bright text, orange active/selected
states, blue focus, tight padding, small radii, and crisp borders.

## Backends

The backends are plug-and-play, some of them even coming with the library. `ngl` is an OpenGL renderer while `sw` is a software renderer to PNG images for testing.

You want to call `nui_init()` with the backend of your choice, like `NUI_BACKEND_NGL` or `NUI_BACKEND_SW`.

## Custom memory allocators

Even though the library uses an area, you can control how that arena aquires and releases memory with `malloc` and `free` by providing replacements with `nui_custom_memory()`.

## Syntaxic sugar

The macro `NUI()` isn't magical. It simply invokes `nui_element_begin()` before your code and `nui_element_end()` after your code. The body is simultanously for configuring the element and to provide the children.

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
nui_element_begin();
{
    // Configuration here.
    nui_fixed(100, 100);

    // Children also here.
    nui_element_begin();
    {
        nui_fixed(50, 50);
    }
    nui_element_end();
}
nui_element_end();
```

By convention, we place all the element's attributes at the top prior to the children.

Images and text are configured on regular elements:

```c
NUI {
    nui_image(image);
}
```

An image element fits to the image's native size by default, just like a text
element fits to its measured text. If the element becomes larger than the image,
the image repeats by default. Use `nui_image_mode()` to stretch it instead:

```c
NUI {
    nui_grow_width();
    nui_image(image);
    nui_image_mode(NUI_IMAGE_MODE_STRETCH);
}
```