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

In your distro of choice, install **g++, Make, SDL2 and SDL mixer (development libraries, not runtime)**. `cd` to the repository's directory and run `make`.

Alternatively, compile the **src/** files using another C++ compiler of your choice. Make sure to link with SDL2 and SDL mixer.

## Windows

Install **MinGW-w64** (either on a Linux distro or on Windows). Download **SDL2 and SDL mixer development libraries for MinGW**. `cd` to the repository's directory and run something like the following:

```
x86_64-w64-mingw32-g++ src/*.cpp -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -L ~/SDL2/x86_64-w64-mingw32/lib/ -L ~/SDL2_mixer/x86_64-w64-mingw32/lib/ -lshlwapi -I ~/SDL2/x86_64-w64-mingw32/include/ -I ~/SDL2/x86_64-w64-mingw32/include/SDL2/ -I ~/SDL2_mixer/x86_64-w64-mingw32/include/
```

**This command may vary depending on your setup.** It assumes that you extracted SDL2 and SDL mixer in folders named `SDL2` and `SDL2_mixer` in your `home` directory, and that `x86_64-w64-mingw32-g++` is the MinGW g++ compiler for 64-bit Windows. **32-bit versions have not been tested. Try at your own risk.**

Might also compile with Visual Studio/MSVC if you know how to configure it.


