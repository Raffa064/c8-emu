#ifndef C8_BACKEND_X11_H
#define C8_BACKEND_X11_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdint.h>

typedef struct {
  Display *display;
  Window window;
  GC gc;
  XImage *canvas;
  uint32_t *canvas_data;
  KeySym keymap[16];
} X11Backend;

#endif
