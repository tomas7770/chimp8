#include <cstdint>

#define MEM_SIZE 4096
#define REG_COUNT 16
#define STACK_DEPTH 16
#define KEY_COUNT 16
#define SCREEN_W 64
#define SCREEN_H 32

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
	bool display[SCREEN_W * SCREEN_H];
} Chip8;

void init_vm(Chip8* vm);
void cycle_vm(Chip8* vm);

#endif
