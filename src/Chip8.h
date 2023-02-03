#include <cstdint>
#include <cstddef>

#define MEM_SIZE 4096
#define REG_COUNT 16
#define STACK_DEPTH 16
#define KEY_COUNT 16
#define SCREEN_W 64
#define SCREEN_H 32
#define SCREEN_SIZE (SCREEN_W * SCREEN_H)
#define FONT_ADDRESS 0x50

#ifndef CHIP8_H
#define CHIP8_H

typedef struct {
	uint8_t memory[MEM_SIZE];
	uint8_t registers[REG_COUNT];
	// 'I' register
	uint16_t address_reg;
	uint16_t stack[STACK_DEPTH];
	// Stack pointer
	uint16_t sp;
	// Program counter
	uint16_t pc;
	uint8_t delay_timer;
	uint8_t sound_timer;
	bool keys[KEY_COUNT];
	bool display[SCREEN_SIZE];

	// This is for blocking opcode FX0A
	bool halted_keypress;
	// Indicates which register will store the key pressed
	int keypress_store_reg;
} Chip8;

const uint8_t chip8_fontset[80] =
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

void init_vm(Chip8* vm);
void load_rom(Chip8* vm, void* rom_file, size_t* rom_size);
void cycle_vm(Chip8* vm);
void cycle_delaytimer(Chip8* vm, int& delay_metatimer);
uint8_t cycle_soundtimer(Chip8* vm, int& sound_metatimer);
void on_keypress(Chip8*vm, int key);

#endif
