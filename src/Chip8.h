#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <cstddef>
#include "Clock.h"

constexpr int mem_size = 4096;
constexpr int reg_count = 16;
constexpr int stack_depth = 16;
constexpr int key_count = 16;
constexpr int screen_w = 64;
constexpr int screen_h = 32;
constexpr int screen_size = screen_w * screen_h;
constexpr int font_address = 0x50;
constexpr int fontset_size = 80;

constexpr uint8_t chip8_fontset[fontset_size] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

enum TimingMode {
    TIMING_FIXED,
    TIMING_COSMAC,
};

class Chip8 {
public:
    Chip8();

    void load_rom(void* rom_file, size_t rom_size);
    void cycle_vm();
    void cycle_delaytimer(int& delay_metatimer);
    uint8_t cycle_soundtimer(int& sound_metatimer);
    void on_keypress(int key);
    void on_keyrelease(int key);

    void tick(uint64_t delta_time);
    void set_cycle_rate(uint64_t new_cycle_rate);
    TimingMode get_timing_mode();
    void set_timing_mode(TimingMode new_timing_mode);

    bool get_display_pixel(int i);
    bool get_legacy_shift();
    bool get_legacy_memops();
    void set_legacy_shift(bool enabled);
    void set_legacy_memops(bool enabled);

    typedef void(Chip8::*opcode_ptr)();
private:
    Clock clock;
    int opcode_cycles;
    TimingMode timing_mode;
    int cycles = 0;

    uint16_t opcode;
    uint8_t memory[mem_size];
    uint8_t registers[reg_count];
    // 'I' register
    uint16_t address_reg;
    uint16_t stack[stack_depth];
    // Stack pointer
    uint16_t sp;
    // Program counter
    uint16_t pc;
    uint8_t delay_timer;
    uint8_t sound_timer;
    bool keys[key_count];
    bool display[screen_size];

    // This is for blocking opcode FX0A
    bool halted_keypress;
    // Indicates which register will store the key pressed
    int keypress_store_reg;

    // Flag for original CHIP-8 8XY6 and 8XYE opcode behavior (if false, use SCHIP behavior)
    bool legacy_shift;
    // Flag for original CHIP-8 FX55 and FX65 opcode behavior (if false, use SCHIP behavior)
    bool legacy_memops;

    // Opcodes
    void opcode_00E0();
    void opcode_00EE();
    void opcode_1NNN();
    void opcode_2NNN();
    void opcode_3XNN();
    void opcode_4XNN();
    void opcode_5XY0();
    void opcode_6XNN();
    void opcode_7XNN();
    void opcode_8XY0();
    void opcode_8XY1();
    void opcode_8XY2();
    void opcode_8XY3();
    void opcode_8XY4();
    void opcode_8XY5();
    void opcode_8XY6();
    void opcode_8XY7();
    void opcode_8XYE();
    void opcode_9XY0();
    void opcode_ANNN();
    void opcode_BNNN();
    void opcode_CXNN();
    void opcode_DXYN();
    void opcode_EX9E();
    void opcode_EXA1();
    void opcode_FX07();
    void opcode_FX0A();
    void opcode_FX15();
    void opcode_FX18();
    void opcode_FX1E();
    void opcode_FX29();
    void opcode_FX33();
    void opcode_FX55();
    void opcode_FX65();

    // Opcode function pointers
    void opcode_00Ex();
    void opcode_8XYx();
    void opcode_EXxy();
    void opcode_FXxy();
    static opcode_ptr opcode_funcs[16];
};

#endif
