#ifdef RL_BACKEND

#include <math.h>
#include <stdlib.h>
#include "../core.h"
#include <raylib.h>
#include "./rl.h"
#include "string.h"

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 4096
#define AUDIO_FREQUENCY   440

// Audio callback function to generate sine wave
void audio_callback(void *bufferData, unsigned int frames) {
  static float time = 0.0f;
  float frequency = 440.0f; // A4 note
  float sampleRate = 44100.0f;
  short *d = (short *)bufferData;

  for (unsigned int i = 0; i < frames; i++) {
      d[i] = (short)(sinf(2.0f * PI * frequency * time) * 32000.0f);
      time += 1.0f / sampleRate;
  }
}

void c8_init_backend(C8Emu *c8) { 
  InitWindow(c8->params.canvas.width, c8->params.canvas.height, c8->params.rom_path);
  InitAudioDevice();

  c8->backend = (RLBackend) {
    .stream = LoadAudioStream(AUDIO_SAMPLE_RATE, 16, 1),
    .buffer = malloc(AUDIO_BUFFER_SIZE * sizeof(short)),
    .keymap = { KEY_ZERO, KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F }
  };

  SetAudioStreamCallback(c8->backend.stream, audio_callback);
}

void c8_backend_pool_events(C8Emu *c8) {
  if (WindowShouldClose()) {
    c8->should_close = 1;
    return;
  }

  for (int i = 0; i < 16; ++i) {
    KeyboardKey key = c8->backend.keymap[i];
    if (IsKeyPressed(key)) {
      c8->keybord.keys |= 1 << i;
      c8->keybord.last = i;
    }

    if (IsKeyReleased(key)) {
      c8->keybord.keys &= ~(1 << i);
    }
  }

  if (c8->cpu.ST > 0)
    PlayAudioStream(c8->backend.stream);
  else
    PauseAudioStream(c8->backend.stream);
}

void c8_backend_draw(C8Emu *c8) {
  ClearBackground(BLACK);
  BeginDrawing();
  int scale = c8->params.canvas.scale;
  for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
    for (int x = 0; x < DISPLAY_WIDTH; ++x) {
      uint8_t pixel = c8->monitor.display[x + y * DISPLAY_WIDTH];
      uint32_t argb = pixel? c8->params.pixel.white : c8->params.pixel.black;
      uint32_t color = argb << 8 | 0xFF; // Conert to Raylib pixel format (RGBA)

      DrawRectangle(
        x * scale, 
        y * scale, 
        scale, 
        scale, 
        GetColor(color)
      );
    }
  }
  EndDrawing();  
}

#endif
