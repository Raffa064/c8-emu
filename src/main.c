#include "core.h"

int main(int argc, char **argv) {
  C8Params params = c8_parse_params(argc, argv);
  
  C8Emu c8 = c8_init(params);
  c8_load_rom(&c8, params.rom_path);
  c8_mem_dump(&c8);
  c8_run(&c8);
  
  return 0;
}
