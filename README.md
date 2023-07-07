# Chimp8 - CHIP-8 Interpreter

A [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) interpreter, written in C++ and using SDL2.

Runs on **Windows** and **Linux**. Make sure to run it from CLI (or drag and drop a rom file in Windows).

# Features

- All CHIP-8 opcodes implemented

- Sound support with replaceable .wav file

- Configurable speed

- COSMAC VIP timing emulation mode (inaccurate, a bit too fast but works well enough in most games), based on [Laurence Scotford's analysis of the respective CHIP-8 interpreter](https://laurencescotford.com/chip-8-on-the-cosmac-vip-instruction-index/).

- Toggleable 8XY6/8XYE (bit shift) and FX55/FX65 (memory store/fill) behavior between CHIP-8 and SUPER-CHIP

## Planned features

- SUPER-CHIP support

- Configurable keymap

- Configuration at runtime (i.e. without having to restart the interpreter)

- GUI for loading roms and configuring the interpreter

- CMake support (for building)

# Default keymap

COSMAC VIP layout mapped to a 4x4 rectangular array of keys, 1 through V in QWERTY. Shown below.

(Real key in a QWERTY layout: emulated key)

1: 1, 2: 2, 3: 3, 4: C,



Q: 4, W: 5, E: 6, R: D,



A: 7, S: 8, D: 9, F: E,



Z: A, X: 0, C: B, V: F

# Configuration

Run the interpreter with any rom file. A default config will be created in the interpreter's directory **(Windows)** or `$XDG_CONFIG_HOME` **(Linux)**.

`cycles`: How many opcodes per second to execute. **Only works in fixed timing mode.**

`sound`: Set to `true` to enable sound, and to `false` to disable.

`sound_buffer`: Sound buffer size

`legacy_memops`: Set to `false` to use SUPER-CHIP's FX55/FX65 (memory store/fill) behavior, and to `true` to use CHIP-8's.

`legacy_shift`: Set to `false` to use SUPER-CHIP's 8XY6/8XYE (bit shift) behavior, and to `true` to use CHIP-8's.

`timing`: Set to `cosmac` to emulate COSMAC VIP timing (inaccurate). Set to `fixed` to run at a specific speed in opcodes per second.

# Build instructions

## Linux

1. Install **g++, Make, SDL2 and SDL2 mixer (development libraries)**. The procedure for this depends on your distro.

2. Download/clone this repository and `cd` to its directory.

3. Build it (dynamically linked):
   `make`

4. Test it (optional):
   `./Chimp8`

5. Strip debug symbols to reduce executable size:
   `make strip`

## Windows

1. Install [MSYS2](https://www.msys2.org/) and start with the **UCRT64** environment.

2. Update the packages:
   `pacman -Syu`
   You may need to run this step **more than once** to make sure that all packages are up-to-date.

3. Install build tools and dependencies:
   `pacman -S mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_mixer`

4. Download/clone this repository and `cd` to its directory.

5. Build it (dynamically linked):
   `mingw32-make`
   Alternatively, build a statically linked executable:
   `mingw32-make STATIC=1`

6. Test it (optional):
   `./Chimp8.exe`

7. Strip debug symbols to reduce executable size:
   `mingw32-make strip`

8. If you built a dynamically linked executable, you'll need to copy DLLs from `/ucrt64/bin` for the app to start outside MSYS2.
