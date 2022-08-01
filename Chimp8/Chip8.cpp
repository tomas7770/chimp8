#include "Chip8.h"

void init_vm(Chip8* vm) {
	for (int i = 0; i < MEM_SIZE; i++)
		vm->memory[i] = 0;
	for (int i = 0; i < REG_COUNT; i++)
		vm->registers[i] = 0;
	vm->address_reg = 0;
	for (int i = 0; i < STACK_DEPTH; i++)
		vm->stack[i] = 0;
	vm->sp = 0;
	vm->pc = 0x200;
	vm->delay_timer = 0;
	vm->sound_timer = 0;
	for (int i = 0; i < KEY_COUNT; i++)
		vm->keys[i] = 0;
	for (int i = 0; i < (SCREEN_W * SCREEN_H); i++)
		vm->display[i] = 0;
}

void cycle_vm(Chip8* vm) {
	// Opcode is 16 bytes, big-endian
	uint16_t opcode = (vm->memory[vm->pc] << 8) | vm->memory[vm->pc + 1];
	switch (opcode) {

	}
	vm->pc += 2;
}
