#include "Chip8.h"
#include <cstdlib>

void load_fontset(Chip8* vm);

void init_vm(Chip8* vm) {
	for (int i = 0; i < MEM_SIZE; i++)
		vm->memory[i] = 0;
	load_fontset(vm);
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
	for (int i = 0; i < SCREEN_SIZE; i++)
		vm->display[i] = 0;
	vm->halted_keypress = false;
}

void load_fontset(Chip8* vm) {
	for (int i = 0; i < 80; i++)
		vm->memory[i + FONT_ADDRESS] = chip8_fontset[i];
}

void load_rom(Chip8* vm, void* rom_file) {
	int i = 0;
	uint8_t* rom_by_byte = (uint8_t*)rom_file;
	while (rom_by_byte[i] && (i + 0x200 < MEM_SIZE)) {
		vm->memory[i + 0x200] = rom_by_byte[i];
		i++;
	}
}

// Clear display
void opcode_00E0(Chip8* vm) {
	for (int i = 0; i < SCREEN_SIZE; i++)
		vm->display[i] = 0;
}

// Return from subroutine
void opcode_00EE(Chip8* vm) {
	vm->pc = vm->stack[--vm->sp];
}

// Jump
void opcode_1NNN(Chip8* vm, uint16_t opcode) {
	vm->pc = opcode & 0xFFF;
}

// Call subroutine
void opcode_2NNN(Chip8* vm, uint16_t opcode) {
	vm->stack[vm->sp++] = vm->pc;
	vm->pc = opcode & 0xFFF;
}

// Skip next instruction if VX == NN
void opcode_3XNN(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	uint8_t nn = opcode & 0x00FF;
	if (vm->registers[x] == nn)
		vm->pc += 2;
}

// Skip next instruction if VX != NN
void opcode_4XNN(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	uint8_t nn = opcode & 0x00FF;
	if (vm->registers[x] != nn)
		vm->pc += 2;
}

// Skip next instruction if VX == VY
void opcode_5XY0(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	if (vm->registers[x] == vm->registers[y])
		vm->pc += 2;
}

// Set VX to NN
void opcode_6XNN(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	uint8_t nn = opcode & 0x00FF;
	vm->registers[x] = nn;
}

// Add NN to VX (Carry flag is not changed)
void opcode_7XNN(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	uint8_t nn = opcode & 0x00FF;
	vm->registers[x] += nn;
}

// Set VX to the value of VY
void opcode_8XY0(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	vm->registers[x] = vm->registers[y];
}

// Set VX to (VX | VY)
void opcode_8XY1(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	vm->registers[x] |= vm->registers[y];
}

// Set VX to (VX & VY)
void opcode_8XY2(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	vm->registers[x] &= vm->registers[y];
}

// Set VX to (VX xor VY)
void opcode_8XY3(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	vm->registers[x] ^= vm->registers[y];
}

// Add VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not.
void opcode_8XY4(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	if ((uint16_t)vm->registers[x] + (uint16_t)vm->registers[y] > 255)
		vm->registers[0xF] = 1;
	else
		vm->registers[0xF] = 0;
	vm->registers[x] += vm->registers[y];
}

// VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not.
void opcode_8XY5(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	// There's a borrow when a bit of register VX is 0 and the corresponding bit
	// from register VY is 1
	if (~(vm->registers[x]) & vm->registers[y])
		vm->registers[0xF] = 0;
	else
		vm->registers[0xF] = 1;
	vm->registers[x] -= vm->registers[y];
}

// Store the least significant bit of VY in VF, shift VY to the right by 1, and copy the result to VX
void opcode_8XY6(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	vm->registers[0xF] = vm->registers[y] & 0x1;
	vm->registers[x] = vm->registers[y] >>= 1;
}

// Set VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not.
void opcode_8XY7(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	// There's a borrow when a bit of register VY is 0 and the corresponding bit
	// from register VX is 1
	if (vm->registers[x] & ~(vm->registers[y]))
		vm->registers[0xF] = 0;
	else
		vm->registers[0xF] = 1;
	vm->registers[x] = vm->registers[y] - vm->registers[x];
}

// Store the most significant bit of VY in VF, shift VY to the left by 1, and copy the result to VX
void opcode_8XYE(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	vm->registers[0xF] = vm->registers[y] & 0x80;
	vm->registers[x] = vm->registers[y] <<= 1;
}

// Skip next instruction if VX != VY
void opcode_9XY0(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	if (vm->registers[x] != vm->registers[y])
		vm->pc += 2;
}

// Set I to the address NNN
void opcode_ANNN(Chip8* vm, uint16_t opcode) {
	uint16_t nnn = opcode & 0x0FFF;
	vm->address_reg = nnn;
}

// Jump to NNN + V0
void opcode_BNNN(Chip8* vm, uint16_t opcode) {
	vm->pc = (opcode & 0xFFF) + vm->registers[0];
}

// Set VX to the result of a bitwise and operation on a random number (0 to 255) and NN
void opcode_CXNN(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	uint8_t nn = opcode & 0xFF;
	uint8_t random_number = rand() % 256;
	vm->registers[x] = random_number & nn;
}

// Draw a sprite at (VX, VY), with width of 8 and height of N
// Each row of 8 pixels is read as a byte starting from memory location I
// VF is set to 1 if any screen pixels are flipped from 1 to 0, and to 0 if not
void opcode_DXYN(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	int y = (opcode & 0x00F0) >> 4;
	uint8_t vx = vm->registers[x];
	uint8_t vy = vm->registers[y];

	int init_screen_coord = vx + vy * SCREEN_W;
	uint8_t n = opcode & 0xF;
	uint16_t I = vm->address_reg;
	uint8_t row;

	vm->registers[0xF] = 0;
	for (int i = 0; i < n; i++) {
		// Draw a row of pixels
		row = vm->memory[I++];
		int mask = 0x80;
		for (int j = 0; j < 8; j++) {
			int screen_coord = init_screen_coord + j + i*8;
			bool pixel = (row & mask) ? 1 : 0;
			if (vm->display[screen_coord] && !pixel)
				vm->registers[0xF] = 1;
			vm->display[screen_coord] = pixel;
			mask >>= 1;
		}
	}
}

// Skip next instruction if the key stored in VX is pressed
void opcode_EX9E(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	uint8_t vx = vm->registers[x];
	if (vm->keys[vx])
		vm->pc += 2;
}

// Skip next instruction if the key stored in VX is not pressed
void opcode_EXA1(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	uint8_t vx = vm->registers[x];
	if (!vm->keys[vx])
		vm->pc += 2;
}

// Set VX to the value of the delay timer
void opcode_FX07(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	vm->registers[x] = vm->delay_timer;
}

// A key press is awaited, and then stored in VX (Blocking Operation)
void opcode_FX0A(Chip8* vm, uint16_t opcode) {
	vm->keypress_store_reg = (opcode & 0x0F00) >> 8;
	vm->halted_keypress = true;
}

// Set the delay timer to VX
void opcode_FX15(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	vm->delay_timer = vm->registers[x];
}

// Set the sound timer to VX
void opcode_FX18(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	vm->sound_timer = vm->registers[x];
}

// Add VX to I
void opcode_FX1E(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	vm->address_reg += vm->registers[x];
}

// Set I to the location of the sprite for the character in VX
void opcode_FX29(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	uint8_t vx = vm->registers[x];
	vm->address_reg = vm->memory[FONT_ADDRESS + vx*5];
}

// Store the binary-coded decimal representation of VX, with the most significant of
// three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2
void opcode_FX33(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	uint8_t vx = vm->registers[x];
	uint16_t I = vm->address_reg;
	vm->memory[I] = vx / 100;
	vm->memory[I + 1] = (vx / 10) % 10;
	vm->memory[I + 2] = vx % 10;
}

// Store from V0 to VX (including VX) in memory, starting at address I. I is increased by 1 for each value written.
void opcode_FX55(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	for (int i = 0; i <= x; i++)
		vm->memory[vm->address_reg++] = vm->registers[i];
}

// Fill from V0 to VX (including VX) with values from memory, starting at address I. I is increased by 1 for each value read.
void opcode_FX65(Chip8* vm, uint16_t opcode) {
	int x = (opcode & 0x0F00) >> 8;
	for (int i = 0; i <= x; i++)
		vm->registers[i] = vm->memory[vm->address_reg++];
}

void cycle_vm(Chip8* vm) {
	if (vm->halted_keypress)
		return;

	// Opcode is 16 bits, big-endian
	uint16_t opcode = (vm->memory[vm->pc] << 8) | vm->memory[vm->pc + 1];
	uint16_t opcode_firstdigit = opcode & 0xF000;
	// Horrible "if/else if" chain; is there a better way to do this?
	// Switch looks like it could get messy to tell apart the different types of opcode
	if (opcode == 0x00E0)
		opcode_00E0(vm);
	else if (opcode == 0x00EE) {
		opcode_00EE(vm);
		return;
	}
	else if (opcode_firstdigit == 0x1000) {
		opcode_1NNN(vm, opcode);
		return;
	}
	else if (opcode_firstdigit == 0x2000) {
		opcode_2NNN(vm, opcode);
		return;
	}
	else if (opcode_firstdigit == 0x3000)
		opcode_3XNN(vm, opcode);
	else if (opcode_firstdigit == 0x4000)
		opcode_4XNN(vm, opcode);
	else if (opcode_firstdigit == 0x5000)
		opcode_5XY0(vm, opcode);
	else if (opcode_firstdigit == 0x6000)
		opcode_6XNN(vm, opcode);
	else if (opcode_firstdigit == 0x7000)
		opcode_7XNN(vm, opcode);
	else if ((opcode & 0xF00F) == 0x8000)
		opcode_8XY0(vm, opcode);
	else if ((opcode & 0xF00F) == 0x8001)
		opcode_8XY1(vm, opcode);
	else if ((opcode & 0xF00F) == 0x8002)
		opcode_8XY2(vm, opcode);
	else if ((opcode & 0xF00F) == 0x8003)
		opcode_8XY3(vm, opcode);
	else if ((opcode & 0xF00F) == 0x8004)
		opcode_8XY4(vm, opcode);
	else if ((opcode & 0xF00F) == 0x8005)
		opcode_8XY5(vm, opcode);
	else if ((opcode & 0xF00F) == 0x8006)
		opcode_8XY6(vm, opcode);
	else if ((opcode & 0xF00F) == 0x8007)
		opcode_8XY7(vm, opcode);
	else if ((opcode & 0xF00F) == 0x800E)
		opcode_8XYE(vm, opcode);
	else if (opcode_firstdigit == 0x9000)
		opcode_9XY0(vm, opcode);
	else if (opcode_firstdigit == 0xA000)
		opcode_ANNN(vm, opcode);
	else if (opcode_firstdigit == 0xB000)
		opcode_BNNN(vm, opcode);
	else if (opcode_firstdigit == 0xC000)
		opcode_CXNN(vm, opcode);
	else if (opcode_firstdigit == 0xD000)
		opcode_DXYN(vm, opcode);
	else if ((opcode & 0xF00F) == 0xE00E)
		opcode_EX9E(vm, opcode);
	else if ((opcode & 0xF00F) == 0xE001)
		opcode_EXA1(vm, opcode);
	else if ((opcode & 0xF0FF) == 0xF007)
		opcode_FX07(vm, opcode);
	else if ((opcode & 0xF0FF) == 0xF00A)
		opcode_FX0A(vm, opcode);
	else if ((opcode & 0xF0FF) == 0xF015)
		opcode_FX15(vm, opcode);
	else if ((opcode & 0xF0FF) == 0xF018)
		opcode_FX18(vm, opcode);
	else if ((opcode & 0xF0FF) == 0xF01E)
		opcode_FX1E(vm, opcode);
	else if ((opcode & 0xF0FF) == 0xF029)
		opcode_FX29(vm, opcode);
	else if ((opcode & 0xF0FF) == 0xF033)
		opcode_FX33(vm, opcode);
	else if ((opcode & 0xF0FF) == 0xF055)
		opcode_FX55(vm, opcode);
	else if ((opcode & 0xF0FF) == 0xF065)
		opcode_FX65(vm, opcode);

	vm->pc += 2;
}
