#include <bits/time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>

#include "core.h"

// Read 16bit from memory address (big endian)
uint16_t read_16(Memory mem, uint16_t addr) { 
  uint16_t word = mem[addr];
  word <<= 8;
  word |= mem[addr + 1];

  return word;
}

// Load buffer into memory
void ldbuf(Memory mem, uint16_t addr, const char *buf, size_t length) {
  for (size_t i = 0; i < length; ++i)
    mem[addr + i] = buf[i];

  printf("ldbuf from %d to %zu\n", addr, addr + length);
}

uint16_t c8_inst(C8Emu *c8) {
  return read_16(c8->mem, c8->cpu.PC);
}

void c8_invalid_ins(C8Emu *c8) {
  fprintf(stderr, "Error: Invalid instruction at %d: %04x\n", c8->cpu.PC, c8_inst(c8));
  exit(1);
}

bool c8_key_pressed(C8Emu *c8, uint8_t key) {
  return (c8->keybord.keys >> key) & 1;
}

void c8_cycle(C8Emu *c8) {
  /*
    0nnn    nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
    000n    n or nibble - A 4-bit value, the lowest 4 bits of the instruction
    0x00    x - A 4-bit value, the lower 4 bits of the high byte of the instruction
    00y0    y - A 4-bit value, the upper 4 bits of the low byte of the instruction
    00kk    kk or byte - An 8-bit value, the lowest 8 bits of the instruction
  */

  uint16_t inst = c8_inst(c8);
  uint16_t nnn = inst & 0xFFF;
  uint8_t n = inst & 0xF;
  uint8_t x = (inst >> 8) & 0xF;
  uint8_t y = (inst >> 4) & 0xF;
  uint8_t kk = inst & 0xFF;
  uint8_t opcode = (inst >> 12) & 0xF;

  uint16_t next_PC = c8->cpu.PC + 2;
  switch (opcode) {
    case 0: {
      switch (kk) {
        case 0xE0: // CLS
          memset(c8->monitor.display, 0, DISPLAY_BUFFER_SIZE);
          break;
        case 0xEE: // RET
          next_PC = c8->cpu.stack[--c8->cpu.SP];
          break;
        default: c8_invalid_ins(c8);
      }
    } break;
    case 1: // JP addr
      next_PC = nnn;
      break;
    case 2: // CALL addr
      c8->cpu.stack[c8->cpu.SP++] = c8->cpu.PC + 2;
      next_PC = nnn;
      break;
    case 3: // SE Vx, byte
      if (c8->cpu.V[x] == kk)
        next_PC = c8->cpu.PC + 4;
      break;
    case 4: // SNE Vx, byte
      if (c8->cpu.V[x] != kk)
        next_PC = c8->cpu.PC + 4;
      break; 
    case 5: // SE Vx, Vy
      if (c8->cpu.V[x] == c8->cpu.V[y])
        next_PC = c8->cpu.PC + 4;
      break; 
    case 6: // LD Vx, byte
      c8->cpu.V[x] = kk;
      break; 
    case 7: // ADD Vx, byte
      c8->cpu.V[x] += kk;
      break; 
    case 8: { 
      switch (n) {
        case 0:  // LD Vx, Vy
          c8->cpu.V[x] = c8->cpu.V[y];
          break;
        case 1: // OR Vx, Vy
          c8->cpu.V[x] |= c8->cpu.V[y];
          break;
        case 2: // AND Vx, Vy
          c8->cpu.V[x] &= c8->cpu.V[y];
          break;
        case 3: // XOR Vx, Vy
          c8->cpu.V[x] ^= c8->cpu.V[y];
          break;
        case 4: { // ADD Vx, Vy
          uint8_t result = c8->cpu.V[x] + c8->cpu.V[y];
          c8->cpu.VF = result < c8->cpu.V[x]; // carry (overflow flag)
          c8->cpu.V[x] = result;
        } break;
        case 5: { // SUB Vx, Vy
          c8->cpu.VF = c8->cpu.V[x] > c8->cpu.V[y]; // not borrow
          c8->cpu.V[x] -= c8->cpu.V[y];
        } break;
        case 6: { // SHR Vx{, Vy}
          c8->cpu.VF = c8->cpu.V[x] & 1;
          c8->cpu.V[x] >>= 1;
        } break;
        case 7: { // SUBN Vx, Vy
          c8->cpu.VF = c8->cpu.V[y] > c8->cpu.V[x]; // borrow flag
          c8->cpu.V[x] = c8->cpu.V[y] - c8->cpu.V[x];
        } break;
        case 0xE: { // SHL Vx, Vy
          c8->cpu.VF = (c8->cpu.V[x] >> 7) & 1;
          c8->cpu.V[x] <<= 1;
        } break;
        default: c8_invalid_ins(c8);
      }
    } break;
    case 9: // SNE Vx, Vy
      if (c8->cpu.V[x] != c8->cpu.V[y])
        next_PC = c8->cpu.PC + 4;
      break;
    case 0xA: // LD I, addr
      c8->cpu.I = nnn;
      break;
    case 0xB: // JP V0, addr
      next_PC = nnn + c8->cpu.V[0];
      break;
    case 0xC: // RND Vx, byte
      c8->cpu.V[x] = (rand() % 256) & kk;
      break;
    case 0xD: { // DRW Vx, Vy, nibble
      // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
      // The interpreter reads n bytes from memory, starting at the address stored in I. 
      // These bytes are then displayed as sprites on screen at coordinates (Vx, Vy).
      // Sprites are XORed onto the existing screen. If this causes any pixels to be erased, VF is set to 1, 
      // otherwise it is set to 0. If the sprite is positioned so part of it is outside the coordinates of the display,
      // it wraps around to the opposite side of the screen. See instruction 8xy3 for more information on XOR, 
      // and section 2.4, Display, for more information on the Chip-8 screen and sprites.
    
      c8->cpu.VF = 0;
      int Vx = c8->cpu.V[x];
      int Vy = c8->cpu.V[y];
      
      for (int line = 0; line < n; ++line) {
        uint8_t spr = c8->mem[c8->cpu.I + line];
        for (int col = 0; col < 8; ++col) {
          uint8_t shift = 7 - col;
          uint8_t bit = (spr >> shift) & 1;

          if (bit) {
            uint8_t px = (Vx + col) % DISPLAY_WIDTH;
            uint8_t py = (Vy + line) % DISPLAY_HEIGHT;
            uint8_t *pixel = &c8->monitor.display[px + py * DISPLAY_WIDTH];
            uint8_t flipped = (*pixel) ^ bit;

            if (*pixel && !flipped)
              c8->cpu.VF = 1; // Erased/Collide flag

            *pixel = flipped;
          }
        }
      }
    } break;
    case 0xE: {
      switch (kk) {
        case 0x9E: // SKP Vx
          if (c8_key_pressed(c8, c8->cpu.V[x]))
            next_PC = c8->cpu.PC + 4;
        break;
        case 0xA1: // SKNP Vx
          if (!c8_key_pressed(c8, c8->cpu.V[x]))
            next_PC = c8->cpu.PC + 4;
        break;
        default: c8_invalid_ins(c8);
      }
    } break;
    case 0xF: {
      switch (kk) {
        case 0x7: // LD Vx, DT
          c8->cpu.V[x] = c8->cpu.DT;
          break;
        case 0xA: // LD Vx, K
          if (!c8->keybord.keys)
            next_PC = c8->cpu.PC;
          else
            c8->cpu.V[x] = c8->keybord.last;
          break;
        case 0x15: // LD DT, Vx
          c8->cpu.DT = c8->cpu.V[x];
          break;
        case 0x18: // LD ST, Vx
          c8->cpu.ST = c8->cpu.V[x];
          break;
        case 0x1E: // ADD I, Vx
          c8->cpu.I += c8->cpu.V[x];
          break;
        case 0x29: // LD F, Vx
          c8->cpu.I = SPRITES_LOCATION_ADDR + c8->cpu.V[x] * SPRITE_SIZE; 
          break;
        case 0x33: { // LD B, Vx
          uint8_t Vx = c8->cpu.V[x];
          c8->mem[c8->cpu.I + 0] = Vx / 100;
          c8->mem[c8->cpu.I + 1] = (Vx / 10) % 10;
          c8->mem[c8->cpu.I + 2] = Vx % 10;
        } break;
        case 0x55: { // LD [i], Vx
          for (int i = 0; i <= x; i++)
            c8->mem[c8->cpu.I + i] = c8->cpu.V[i];
        } break;
        case 0x65: { // LD Vx, [i]
          for (int i = 0; i <= x; i++)
            c8->cpu.V[i] = c8->mem[c8->cpu.I + i]; 
        } break;
        default: c8_invalid_ins(c8);
      };
    } break;
    default: c8_invalid_ins(c8);
  }

  c8->cpu.PC = next_PC;
}

void c8_update_timers(C8Emu *c8) {
  if (c8->cpu.DT > 0)
    c8->cpu.DT--;

  if (c8->cpu.ST > 0)
    c8->cpu.ST--;
}

void c8_mem_dump(C8Emu *c8) {
  uint16_t last = 256;
  size_t count = 0;
  for (size_t i = 0; i < MEMORY_BUFFER_SIZE; ++i) {
    if (c8->mem[i] == last)
      count++;
    else {
      if (count > 1)
        printf(".. ");

      printf("%02x ", c8->mem[i]);
      count = 1;
    }

    last = c8->mem[i];
  }

  if (count > 1)
    printf("...");

  printf("\n");
}

C8Emu c8_init(C8Params params) {
  C8Emu c8 = { .params = params };
  c8_init_backend(&c8);

  // Reset CPU state
  srand(time(NULL));
  c8.cpu = (CPU) {0};
  c8.cpu.PC = PROGRAM_START_ADDR;

  c8.clock.cpu.freq = params.cpu_freq;
  c8.clock.timers.freq = params.timers_freq;
  c8.clock.display.freq = params.display_freq;
  
  // Load sprites into memory
  ldbuf(c8.mem, SPRITES_LOCATION_ADDR, SPRITES, sizeof(SPRITES));

  return c8;
};

void c8_load_rom(C8Emu *c8, const char *rom_path) {
  FILE *f = fopen(rom_path, "rb");

  if (!f) {
    fprintf(stderr, "Can't load rom file from '%s'\n", rom_path);
    exit(1);
  }

  fseek(f, 0, SEEK_END);
  size_t file_size = ftell(f);
  rewind(f);

  char *buf = malloc(file_size);
  
  size_t total = 0;
  while (total < file_size) {
    size_t bytes = fread(buf, 1, file_size - total, f);

    if (bytes == 0)
      break;

    total += bytes;
  }
  
  ldbuf(c8->mem, PROGRAM_START_ADDR, buf, total);
  free(buf);
  fclose(f);
}

long getns(struct timespec ts) {
  return ts.tv_sec * 1e9 + ts.tv_nsec;
}

void c8_tick(long now, Clock *clock) {
  long elapsed = now - clock->start;
  long target = 1e9 / clock->freq;

  clock->tick = 0;
  if (elapsed >= target) {
    clock->tick = 1;
    clock->start = now;
  }
}

void c8_update_clocks(C8Emu *c8) {
  struct timespec now_spec = {0};
  clock_gettime(CLOCK_MONOTONIC, &now_spec);
  
  long now = getns(now_spec);
  c8_tick(now, &c8->clock.cpu);
  c8_tick(now, &c8->clock.timers);
  c8_tick(now, &c8->clock.display);
}

bool match_opt(const char *arg, const char *option) {
  return strcmp(arg, option) == 0;
}

void c8_print_usage() {
  printf(
    "c8 [options, ...] <rom_path.ch8>\n"
    "  -s <scale>  Set canvas scale (df: 10)\n"
    "  -c <freq>   Change CPU's freq (df: 700)\n"
    "  -t <freq>   Change sound/delay timer's freq (df: 60)\n"
    "  -d <freq>   Change display freq (df: 60)\n"
    "  -b          Set black pixel color (df: 000000)\n"
    "  -w          Set white pixel color (df: FFFFFF)\n"
    "  -h          Show this help dialog\n"
  );
}

uint32_t strtocolor(const char *s) {
  if (strlen(s) != 6 || s[strspn(s, "0123456789abcdefABCDEF")] != '\0') {
    fprintf(stderr, "Error: Malformed color argument, expected 6-digits hexadecimal . Ex: C0FFee, FF0000, 181818\n");
    exit(1);
  }
  
  return strtol(s, NULL, 16);
}

C8Params c8_parse_params(int argc, char **argv) {
  if (argc < 2) {
      c8_print_usage();
      fprintf(stderr, "Error: no arguments\n");
      exit(1);
  }

  C8Params params = {
    .canvas.scale = 10,
    .cpu_freq = 700,
    .timers_freq = 60,
    .display_freq = 60,
    .pixel.black = 0x000000,
    .pixel.white = 0xFFFFFFF,
  };

  // c8 [params, ...] rom.ch8
  for (int i = 1; i < argc; ++i) {
    char *opt = argv[i];

    if (opt[0] == '-') {
      if (match_opt(opt, "-s")) {
        params.canvas.scale = strtol(argv[++i], NULL, 10);
      } else if (match_opt(opt, "-c")) {
        params.cpu_freq = strtol(argv[++i], NULL, 10);
      } else if (match_opt(opt, "-t")) {
        params.timers_freq = strtol(argv[++i], NULL, 10);
      } else if (match_opt(opt, "-d")) {
        params.display_freq = strtol(argv[++i], NULL, 10);
      } else if (match_opt(opt, "-b")) {
        params.pixel.black = strtocolor(argv[++i]);
      } else if (match_opt(opt, "-w")) {
        params.pixel.white = strtocolor(argv[++i]);
      } else if (match_opt(opt, "-h")) {
        c8_print_usage();
        exit(0);
      } else {
        c8_print_usage();
        fprintf(stderr, "Error: Invalid option: %s\n", opt);
        exit(1);
      }
    } else {
      params.rom_path = opt;
    }
  }

  if (!params.rom_path) {
    printf("No selected rom file\n");
    exit(1);
  }

  params.canvas.width  = DISPLAY_WIDTH  * params.canvas.scale;
  params.canvas.height = DISPLAY_HEIGHT * params.canvas.scale;
  params.canvas.buffer_size = params.canvas.width * params.canvas.height;

  return params;
}

void c8_run(C8Emu *c8) {
  while (!c8->should_close) {
    c8_update_clocks(c8);
    c8_backend_pool_events(c8);
    
    if (c8->clock.cpu.tick)
      c8_cycle(c8);

    if (c8->clock.timers.tick) 
      c8_update_timers(c8);
   
    if (c8->clock.display.tick)
      c8_backend_draw(c8);
  }
}
