#include <stdlib.h>

#ifndef RL_BACKEND

#include "../core.h"
#include "./x11.h"

void c8_init_backend(C8Emu *c8) { 
  Display *display = XOpenDisplay(NULL);
  Window window = XCreateSimpleWindow(
    display, 
    XDefaultRootWindow(display), 
    0, 0, 
    c8->params.canvas.width, 
    c8->params.canvas.height,
    0, 0, 0
  );
  
  GC gc = XCreateGC(display, window, 0, NULL);
  
  XSelectInput(display, window, KeyPressMask | KeyReleaseMask);
  XStoreName(display, window, c8->params.rom_path);
  XMapWindow(display, window);

  uint32_t *canvas_data = malloc(c8->params.canvas.buffer_size * sizeof(uint32_t));
  int bytes_per_line = c8->params.canvas.width * sizeof(uint32_t);

  int screen = XDefaultScreen(display);
  XImage *canvas = XCreateImage(
    display, 
    XDefaultVisual(display, screen), 
    XDefaultDepth(display, screen), 
    ZPixmap, 
    0, 
    (void*)canvas_data, 
    c8->params.canvas.width, 
    c8->params.canvas.height, 
    32, 
    bytes_per_line
  );
  
  int bytes_per_pixel = canvas->bits_per_pixel/8;
  canvas->bytes_per_line = c8->params.canvas.width * bytes_per_pixel;

  c8->backend = (X11Backend) {
    .display = display,
    .window = window,
    .gc = gc,
    .canvas = canvas,
    .canvas_data = canvas_data,
    .keymap = { XK_0, XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9, XK_a, XK_b, XK_c, XK_d, XK_e, XK_f }
  };
}

void c8_backend_pool_events(C8Emu *c8) {
  XEvent ev;
  while (XPending(c8->backend.display)) {
    XNextEvent(c8->backend.display, &ev);

    switch (ev.type) {
      case KeyPress: {
        KeySym key = XLookupKeysym(&ev.xkey, 0);
        for (uint8_t  i = 0; i < 16; i++) {
          if (c8->backend.keymap[i] == key) {
            c8->keybord.keys |= 1 << i;
            c8->keybord.last = i;
            break;
          }
        }
      } break;
      case KeyRelease: {
        KeySym key = XLookupKeysym(&ev.xkey, 0);
        for (uint8_t  i = 0; i < 16; i++) {
          if (c8->backend.keymap[i] == key) {
            c8->keybord.keys &= ~(1 << i);
            break;
          }
        }
      } break;
    }
  }
}

void c8_backend_draw(C8Emu *c8) {
  for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
    for (int x = 0; x < DISPLAY_WIDTH; ++x) {
      uint8_t pixel = c8->monitor.display[x + y * DISPLAY_WIDTH];
      uint32_t color = pixel? c8->params.pixel.white : c8->params.pixel.black;
      
      int px_tl_corner = (x + y * c8->params.canvas.width) * c8->params.canvas.scale;
      for (int i = 0; i < c8->params.canvas.scale; i++)
        for (int j = 0; j < c8->params.canvas.scale; j++)
          c8->backend.canvas_data[px_tl_corner + i + j * c8->params.canvas.width] = color;
    }
  }
  
  XPutImage(
    c8->backend.display,
    c8->backend.window,
    c8->backend.gc, 
    c8->backend.canvas,
    0, 0, 
    0, 0,
    c8->params.canvas.width,
    c8->params.canvas.height
  );
}
#endif
