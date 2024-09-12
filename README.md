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

Chimp8 uses [CMake](https://cmake.org/) (>= 3.7) and requires the [SDL2](https://www.libsdl.org/) and [SDL2 mixer](https://github.com/libsdl-org/SDL_mixer) libraries.

## Linux, and Windows with MSYS2

1. Install **g++ (or possibly other C++ compiler), CMake, Make/Ninja or equivalent, SDL2 and SDL2 mixer (development libraries)**. The procedure for this depends on your distro.

2. Download/clone this repository and `cd` to its directory.

3. Run CMake:
   ```
   cmake -S. -Bbuild
   cmake --build build
   ```

If compilation succeeds, you should find a binary executable in the `build/` directory, possibly in a subdirectory.
