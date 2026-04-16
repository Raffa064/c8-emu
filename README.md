# C8 - Chip8 Emulator

This project features a Chip8 emulator, implemented accordingly to the [technical reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM).

The emulator is designed to be easily portable for different backends, and therefore, it already features these backends:
- X11 
- Raylib

You can download roms on this [repo](https://github.com/kripod/chip8-roms).

To create your own ROMs, you use my [chip8 compiler](https://github.com/Raffa064/c8c)

## How to use

To run Chip8 ROMs, you can use the following command:
```bash
c8 ./path/to/rom.ch8
```

As a command line tool, it's behavior can be changed by command line arguments, so you can always use `-h` option to see available options:
```
c8 [options, ...] <rom_path.ch8>
  -s <scale>  Set canvas scale (df: 10)
  -c <freq>   Change CPU's freq (df: 700)
  -t <freq>   Change sound/delay timer's freq (df: 60)
  -d <freq>   Change display freq (df: 60)
  -b          Set black pixel color (df: 000000)
  -w          Set white pixel color (df: FFFFFF)
  -h          Show this help dialog
```
