#ifndef C8_BACKEND_RL_H
#define C8_BACKEND_RL_H

#include <raylib.h>

typedef struct {
  AudioStream stream;
  short *buffer;
  float time;
  KeyboardKey keymap[16];
} RLBackend;

#endif
