#ifndef C8_CORE_H
#define C8_CORE_H

#include <stdbool.h>
#include <stdint.h>
#include "./backends.h"

#define SPRITES_LOCATION_ADDR 0
#define SPRITE_SIZE 5

static const uint8_t SPRITES[16 * 5] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, //0
  0x20, 0x60, 0x20, 0x20, 0x70, //1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
  0x90, 0x90, 0xF0, 0x10, 0x10, //4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
  0xF0, 0x10, 0x20, 0x40, 0x40, //7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
  0xF0, 0x90, 0xF0, 0x90, 0x90, //A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
  0xF0, 0x80, 0x80, 0x80, 0xF0, //C
  0xE0, 0x90, 0x90, 0x90, 0xE0, //D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
  0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

#define PROGRAM_START_ADDR 0x200 // PC register whould start at 0x200 (512)
#define MEMORY_BUFFER_SIZE 4096
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define DISPLAY_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT)

typedef uint8_t Memory[MEMORY_BUFFER_SIZE];
typedef uint8_t R8;
typedef uint16_t R16;

typedef struct {
  uint8_t display[DISPLAY_BUFFER_SIZE];
} Monitor;

typedef struct {
  uint8_t last;
  uint16_t keys;
} Keyboard;

typedef struct {
  R16 PC;
  R16 stack[16]; // stores up to 16 addresses of 16bit each
  R8 SP;
  union {
    R8 V[16];
    struct { R8 _[15]; R8 VF; }; // VF or V[16] is a reserved flag register that should not be used by programs
  };
  R16 I;
  R8 DT, ST; // Dalay and sound timers
} CPU;

typedef struct {
  long start;
  short freq; // Hz
  bool tick;
} Clock;

typedef struct {
  struct {
    int scale, width, height, buffer_size;
  } canvas;
  short cpu_freq, timers_freq, display_freq;
  const char *rom_path;
  struct {
    uint32_t black, white;
  } pixel;
} C8Params;

typedef struct C8Emu {
  Backend backend;
  C8Params params;
  Monitor monitor;
  Keyboard keybord;
  Memory mem;
  CPU cpu;
  struct {
    Clock cpu; 
    Clock timers;
    Clock display;
  } clock;
  bool should_close;
} C8Emu;

uint16_t c8_inst(C8Emu *c8);

void c8_invalid_ins(C8Emu *c8);

bool c8_key_pressed(C8Emu *c8, uint8_t key);

void c8_cycle(C8Emu *c8);

void c8_update_timers(C8Emu *c8);

void c8_mem_dump(C8Emu *c8);

C8Emu c8_init(C8Params params);

void c8_load_rom(C8Emu *c8, const char *rom_path);

void c8_tick(long now, Clock *clock);

void c8_update_clocks(C8Emu *c8);

void c8_print_usage();

C8Params c8_parse_params(int argc, char **argv);

void c8_run(C8Emu *c8);

#endif
