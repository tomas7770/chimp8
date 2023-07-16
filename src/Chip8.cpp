#include "Chip8.h"
#include <cstdlib>
#include <stdexcept>

constexpr uint64_t cosmac_cycle_rate = 220113;

Chip8::opcode_ptr Chip8::opcode_funcs[] = {
    &Chip8::opcode_00yx, &Chip8::opcode_1NNN, &Chip8::opcode_2NNN, &Chip8::opcode_3XNN,
    &Chip8::opcode_4XNN, &Chip8::opcode_5XY0, &Chip8::opcode_6XNN, &Chip8::opcode_7XNN,
    &Chip8::opcode_8XYx, &Chip8::opcode_9XY0, &Chip8::opcode_ANNN, &Chip8::opcode_BNNN,
    &Chip8::opcode_CXNN, &Chip8::opcode_DXYN, &Chip8::opcode_EXxy, &Chip8::opcode_FXxy
};


Chip8::Chip8() : clock(this) {
    set_timing_mode(TIMING_COSMAC);
    cycles = 0;
    opcode = 0;
    for (int i = 0; i < mem_size; i++)
        memory[i] = 0;
    for (int i = 0; i < fontset_size; i++)
        memory[i + font_address] = chip8_fontset[i];
    for (int i = 0; i < reg_count; i++)
        registers[i] = 0;
    address_reg = 0;
    for (int i = 0; i < stack_depth; i++)
        stack[i] = 0;
    sp = 0;
    pc = 0x200;
    delay_timer = 0;
    sound_timer = 0;
    for (int i = 0; i < key_count; i++)
        keys[i] = 0;
    for (int i = 0; i < screen_size; i++)
        display[i] = 0;
    halted_keypress = false;
    keypress_store_reg = 0;
    exit_opcode_called = false;
    hi_res = false;
    legacy_shift = false;
    legacy_memops = false;
}

void Chip8::load_rom(void* rom_file, size_t rom_size) {
    int i = 0;
    uint8_t* rom_by_byte = (uint8_t*)rom_file;
    while ((i < rom_size) && (i + 0x200 < mem_size)) {
        memory[i + 0x200] = rom_by_byte[i];
        i++;
    }
}

// [SUPER-CHIP] Scroll display N pixels down; in low resolution mode, N/2 pixels
void Chip8::opcode_00CN() {
    uint8_t n = opcode & 0xF;
    for (int i = screen_size-1; i >= 0; i--) {
        int j = i-screen_w*n;
        if (j < 0)
            display[i] = false;
        else
            display[i] = display[j];
    }
}

// Clear display
void Chip8::opcode_00E0() {
    for (int i = 0; i < screen_size; i++)
        display[i] = 0;
    
    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 24; break;
    }
}

// Return from subroutine
void Chip8::opcode_00EE() {
    if (sp <= 0)
        throw std::runtime_error("Interpreter stack underflow");

    pc = stack[--sp];

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 10; break;
    }
}

// [SUPER-CHIP] Scroll right by 4 pixels; in low resolution mode, 2 pixels
void Chip8::opcode_00FB() {
    for (int row = 0; row < screen_h; row++) {
        for (int col = screen_w-1; col >= 0; col--) {
            int dest = col + row*screen_w;
            int src = dest-4;
            if (src < row*screen_w)
                display[dest] = false;
            else
                display[dest] = display[src];
        }
    }
}

// [SUPER-CHIP] Scroll left by 4 pixels; in low resolution mode, 2 pixels
void Chip8::opcode_00FC() {
    for (int row = 0; row < screen_h; row++) {
        for (int col = 0; col < screen_w; col++) {
            int dest = col + row*screen_w;
            int src = dest+4;
            if (src >= (row+1)*screen_w)
                display[dest] = false;
            else
                display[dest] = display[src];
        }
    }
}

// [SUPER-CHIP] Exit interpreter
void Chip8::opcode_00FD() {
    exit_opcode_called = true;
}

// [SUPER-CHIP] Disable extended screen mode
void Chip8::opcode_00FE() {
    hi_res = false;
}

// [SUPER-CHIP] Enable extended screen mode
void Chip8::opcode_00FF() {
    hi_res = true;
}

// Jump
void Chip8::opcode_1NNN() {
    pc = (opcode & 0xFFF) - 2;

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 12; break;
    }
}

// Call subroutine
void Chip8::opcode_2NNN() {
    if (sp >= stack_depth)
        throw std::runtime_error("Interpreter stack overflow");

    stack[sp++] = pc;
    pc = (opcode & 0xFFF) - 2;

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 26; break;
    }
}

// Skip next instruction if VX == NN
void Chip8::opcode_3XNN() {
    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 10; break;
    }

    int x = (opcode & 0x0F00) >> 8;
    uint8_t nn = opcode & 0x00FF;
    if (registers[x] == nn) {
        pc += 2;
        if (timing_mode == TIMING_COSMAC)
            opcode_cycles += 4;
    }
}

// Skip next instruction if VX != NN
void Chip8::opcode_4XNN() {
    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 10; break;
    }

    int x = (opcode & 0x0F00) >> 8;
    uint8_t nn = opcode & 0x00FF;
    if (registers[x] != nn) {
        pc += 2;
        if (timing_mode == TIMING_COSMAC)
            opcode_cycles += 4;
    }
}

// Skip next instruction if VX == VY
void Chip8::opcode_5XY0() {
    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 14; break;
    }

    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    if (registers[x] == registers[y]) {
        pc += 2;
        if (timing_mode == TIMING_COSMAC)
            opcode_cycles += 4;
    }
}

// Set VX to NN
void Chip8::opcode_6XNN() {
    int x = (opcode & 0x0F00) >> 8;
    uint8_t nn = opcode & 0x00FF;
    registers[x] = nn;

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 6; break;
    }
}

// Add NN to VX (Carry flag is not changed)
void Chip8::opcode_7XNN() {
    int x = (opcode & 0x0F00) >> 8;
    uint8_t nn = opcode & 0x00FF;
    registers[x] += nn;

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 10; break;
    }
}

// Set VX to the value of VY
void Chip8::opcode_8XY0() {
    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    registers[x] = registers[y];

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 12; break;
    }
}

// Set VX to (VX | VY)
void Chip8::opcode_8XY1() {
    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    registers[x] |= registers[y];

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 44; break;
    }
}

// Set VX to (VX & VY)
void Chip8::opcode_8XY2() {
    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    registers[x] &= registers[y];

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 44; break;
    }
}

// Set VX to (VX xor VY)
void Chip8::opcode_8XY3() {
    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    registers[x] ^= registers[y];

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 44; break;
    }
}

// Add VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not.
void Chip8::opcode_8XY4() {
    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    if ((uint16_t)registers[x] + (uint16_t)registers[y] > 255)
        registers[0xF] = 1;
    else
        registers[0xF] = 0;
    registers[x] += registers[y];

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 44; break;
    }
}

// VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not.
void Chip8::opcode_8XY5() {
    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    // There's a borrow when VY > VX
    if (registers[y] > registers[x])
        registers[0xF] = 0;
    else
        registers[0xF] = 1;
    registers[x] -= registers[y];

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 44; break;
    }
}

// Store the least significant bit of VX in VF, shift VX to the right by 1
// Legacy: Store the least significant bit of VY in VF, shift VY to the right by 1, store result in VX
void Chip8::opcode_8XY6() {
    int x = (opcode & 0x0F00) >> 8;
    if (legacy_shift) {
        int y = (opcode & 0x00F0) >> 4;
        registers[0xF] = registers[y] & 0x1;
        registers[y] >>= 1;
        registers[x] = registers[y];
    }
    else {
        registers[0xF] = registers[x] & 0x1;
        registers[x] >>= 1;
    }

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 44; break;
    }
}

// Set VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not.
void Chip8::opcode_8XY7() {
    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    // There's a borrow when VX > VY
    if (registers[x] > registers[y])
        registers[0xF] = 0;
    else
        registers[0xF] = 1;
    registers[x] = registers[y] - registers[x];

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 44; break;
    }
}

// Store the most significant bit of VX in VF, shift VX to the left by 1
// Legacy: Store the most significant bit of VY in VF, shift VY to the left by 1, store result in VX
void Chip8::opcode_8XYE() {
    int x = (opcode & 0x0F00) >> 8;
    if (legacy_shift) {
        int y = (opcode & 0x00F0) >> 4;
        registers[0xF] = (registers[y] & 0x80) >> 7;
        registers[y] <<= 1;
        registers[x] = registers[y];
    }
    else {
        registers[0xF] = (registers[x] & 0x80) >> 7;
        registers[x] <<= 1;
    }

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 44; break;
    }
}

// Skip next instruction if VX != VY
void Chip8::opcode_9XY0() {
    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 14; break;
    }

    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    if (registers[x] != registers[y]) {
        pc += 2;
        if (timing_mode == TIMING_COSMAC)
            opcode_cycles += 4;
    }
}

// Set I to the address NNN
void Chip8::opcode_ANNN() {
    uint16_t nnn = opcode & 0x0FFF;
    address_reg = nnn;

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 12; break;
    }
}

// Jump to NNN + V0
void Chip8::opcode_BNNN() {
    pc = (opcode & 0xFFF) + registers[0] - 2;

    switch (timing_mode) {
        case TIMING_COSMAC:
            opcode_cycles = 22;
            if ((opcode & 0xFF) + registers[0] >= 0x100) {
                // Page boundary crossed
                opcode_cycles += 2;
            }
            break;
    }
}

// Set VX to the result of a bitwise and operation on a random number (0 to 255) and NN
void Chip8::opcode_CXNN() {
    int x = (opcode & 0x0F00) >> 8;
    uint8_t nn = opcode & 0xFF;
    uint8_t random_number = rand() % 256;
    registers[x] = random_number & nn;

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 36; break;
    }
}

// Draw a sprite at (VX, VY), with width of 8 and height of N.
// Sprites are XORed onto the existing screen.
// Each row of 8 pixels is read as a byte starting from memory location I
// VF is set to 1 if any screen pixels are flipped from 1 to 0, and to 0 if not
//
// [SUPER-CHIP] If N=0 and extended mode, draw 16x16 sprite.
// In extended mode, sets VF to the number of rows that either collide with
// another sprite or are clipped by the bottom of the screen.
void Chip8::opcode_DXYN() {
    int x = (opcode & 0x0F00) >> 8;
    int y = (opcode & 0x00F0) >> 4;
    uint8_t vx = (registers[x]) % screen_w;
    uint8_t vy = (registers[y]) % screen_h;

    uint8_t n = opcode & 0xF;
    int columns = 8;
    uint16_t I = address_reg;
    uint8_t row;
    int collision_count = 0;
    if (n == 0 && hi_res) {
        n = 16;
        columns = 16;
    }

    registers[0xF] = 0;
    for (int i = 0; i < n; i++) {
        // Draw a row of pixels
        int mask;
        for (int j = 0; j < columns; j++) {
            if (j % 8 == 0) {
                row = memory[I++];
                mask = 0x80;
            }
            bool pixel = (row & mask) ? 1 : 0;
            draw_display_pixel(vx + j, vy + i, pixel, &collision_count);
            mask >>= 1;
        }
    }

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 3072 + n*(94 + collision_count*8); break; // Oversimplified
    }
}

// Skip next instruction if the key stored in VX is pressed
void Chip8::opcode_EX9E() {
    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 14; break;
    }

    int x = (opcode & 0x0F00) >> 8;
    uint8_t vx = registers[x];
    if (keys[vx]) {
        pc += 2;
        if (timing_mode == TIMING_COSMAC)
            opcode_cycles += 4;
    }
}

// Skip next instruction if the key stored in VX is not pressed
void Chip8::opcode_EXA1() {
    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 14; break;
    }

    int x = (opcode & 0x0F00) >> 8;
    uint8_t vx = registers[x];
    if (!keys[vx]) {
        pc += 2;
        if (timing_mode == TIMING_COSMAC)
            opcode_cycles += 4;
    }
}

// Set VX to the value of the delay timer
void Chip8::opcode_FX07() {
    int x = (opcode & 0x0F00) >> 8;
    registers[x] = delay_timer;

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 10; break;
    }
}

// A key press is awaited, and then stored in VX (Blocking Operation)
void Chip8::opcode_FX0A() {
    keypress_store_reg = (opcode & 0x0F00) >> 8;
    halted_keypress = true;

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 2; break; // Oversimplified
    }
}

// Set the delay timer to VX
void Chip8::opcode_FX15() {
    int x = (opcode & 0x0F00) >> 8;
    delay_timer = registers[x];

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 10; break;
    }
}

// Set the sound timer to VX
void Chip8::opcode_FX18() {
    int x = (opcode & 0x0F00) >> 8;
    sound_timer = registers[x];

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 10; break;
    }
}

// Add VX to I
void Chip8::opcode_FX1E() {
    int x = (opcode & 0x0F00) >> 8;
    uint16_t prev_I = address_reg;
    address_reg += registers[x];

    switch (timing_mode) {
        case TIMING_COSMAC:
            opcode_cycles = 16;
            if ((prev_I & 0xFF) + registers[x] >= 0x100) {
                // Page boundary crossed
                opcode_cycles += 6;
            }
            break;
    }
}

// Set I to the location of the sprite for the character in VX
void Chip8::opcode_FX29() {
    int x = (opcode & 0x0F00) >> 8;
    uint8_t vx = registers[x];
    address_reg = font_address + vx*5;

    switch (timing_mode) {
        case TIMING_COSMAC: opcode_cycles = 20; break;
    }
}

// Store the binary-coded decimal representation of VX, with the most significant of
// three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2
void Chip8::opcode_FX33() {
    int x = (opcode & 0x0F00) >> 8;
    uint8_t vx = registers[x];
    uint16_t I = address_reg;
    memory[I] = vx / 100;
    memory[I + 1] = (vx / 10) % 10;
    memory[I + 2] = vx % 10;

    switch (timing_mode) {
        case TIMING_COSMAC:
            opcode_cycles = 84 + (memory[I] + memory[I + 1] + memory[I + 2])*16;
            break;
    }
}

// Store from V0 to VX (including VX) in memory, starting at address I and increasing by 1 for each value written.
void Chip8::opcode_FX55() {
    int x = (opcode & 0x0F00) >> 8;
    for (int i = 0; i <= x; i++) {
        uint16_t address = legacy_memops ? address_reg++ : address_reg+i;
        memory[address] = registers[i];
    }

    switch (timing_mode) {
        case TIMING_COSMAC:
            opcode_cycles = 18 + (x+1)*14;
            break;
    }
}

// Fill from V0 to VX (including VX) with values from memory, starting at address I and increasing by 1 for each value read.
void Chip8::opcode_FX65() {
    int x = (opcode & 0x0F00) >> 8;
    for (int i = 0; i <= x; i++) {
        uint16_t address = legacy_memops ? address_reg++ : address_reg+i;
        registers[i] = memory[address];
    }

    switch (timing_mode) {
        case TIMING_COSMAC:
            opcode_cycles = 18 + (x+1)*14;
            break;
    }
}


void Chip8::opcode_00yx() {
    if (opcode & 0x00F0 == 0x00C0) {
        opcode_00CN();
        return;
    }
    switch (opcode) {
        case 0x00E0:
            opcode_00E0();
            break;
        case 0x00EE:
            opcode_00EE();
            break;
        case 0x00FB:
            opcode_00FB();
            break;
        case 0x00FC:
            opcode_00FC();
            break;
        case 0x00FD:
            opcode_00FD();
            break;
        case 0x00FE:
            opcode_00FE();
            break;
        case 0x00FF:
            opcode_00FF();
            break;
    }
}

void Chip8::opcode_8XYx() {
    switch (opcode & 0xF) {
        case 0x0:
            opcode_8XY0();
            break;
        case 0x1:
            opcode_8XY1();
            break;
        case 0x2:
            opcode_8XY2();
            break;
        case 0x3:
            opcode_8XY3();
            break;
        case 0x4:
            opcode_8XY4();
            break;
        case 0x5:
            opcode_8XY5();
            break;
        case 0x6:
            opcode_8XY6();
            break;
        case 0x7:
            opcode_8XY7();
            break;
        case 0xE:
            opcode_8XYE();
            break;
    }
}

void Chip8::opcode_EXxy() {
    switch (opcode & 0xF) {
        case 0xE:
            opcode_EX9E();
            break;
        case 0x1:
            opcode_EXA1();
            break;
    }
}

void Chip8::opcode_FXxy() {
    switch (opcode & 0xFF) {
        case 0x07:
            opcode_FX07();
            break;
        case 0x0A:
            opcode_FX0A();
            break;
        case 0x15:
            opcode_FX15();
            break;
        case 0x18:
            opcode_FX18();
            break;
        case 0x1E:
            opcode_FX1E();
            break;
        case 0x29:
            opcode_FX29();
            break;
        case 0x33:
            opcode_FX33();
            break;
        case 0x55:
            opcode_FX55();
            break;
        case 0x65:
            opcode_FX65();
            break;
    }
}

void Chip8::cycle_vm() {
    if (halted_keypress)
        return;
    
    if (cycles > 0) {
        cycles--;
        return;
    }

    // Opcode is 16 bits, big-endian
    opcode = (memory[pc] << 8) | memory[pc + 1];
    (this->*opcode_funcs[(opcode & 0xF000) >> 12])();

    pc += 2;
    
    cycles += opcode_cycles - 1;
}

void Chip8::cycle_delaytimer(int& delay_metatimer) {
    // 17 ms ~ 60 Hz
    while (delay_metatimer >= 17) {
        if (delay_timer != 0)
            delay_timer -= 1;
        delay_metatimer -= 17;
    }
}

uint8_t Chip8::cycle_soundtimer(int& sound_metatimer) {
    // 17 ms ~ 60 Hz
    while (sound_metatimer >= 17) {
        if (sound_timer != 0)
            sound_timer -= 1;
        sound_metatimer -= 17;
    }
    return sound_timer;
}

void Chip8::on_keypress(int key) {
    keys[key] = true;
    if (halted_keypress) {
        halted_keypress = false;
        registers[keypress_store_reg] = (uint8_t) key;
    }
}

void Chip8::on_keyrelease(int key) {
    keys[key] = false;
}

void Chip8::tick(uint64_t delta_time) {
    clock.tick(delta_time);
}

void Chip8::set_cycle_rate(uint64_t new_cycle_rate) {
    if (timing_mode == TIMING_FIXED)
        clock.set_cycle_rate(new_cycle_rate);
}

TimingMode Chip8::get_timing_mode() {
    return timing_mode;
}

void Chip8::set_timing_mode(TimingMode new_timing_mode) {
    timing_mode = new_timing_mode;
    switch (new_timing_mode) {
        case TIMING_FIXED:
            opcode_cycles = 1;
            break;
        case TIMING_COSMAC:
            clock.set_cycle_rate(cosmac_cycle_rate);
            // Define opcode_cycles for sanity even though it should be replaced
            opcode_cycles = 1;
            break;
    }
    cycles = 0;
}

bool Chip8::get_display_pixel(int i) {
    return display[i];
}

bool Chip8::was_exit_opcode_called() {
    return exit_opcode_called;
}

bool Chip8::get_legacy_shift() {
    return legacy_shift;
}

bool Chip8::get_legacy_memops() {
    return legacy_memops;
}

void Chip8::set_legacy_shift(bool enabled) {
    legacy_shift = enabled;
}

void Chip8::set_legacy_memops(bool enabled) {
    legacy_memops = enabled;
}

// Draw display pixel, adjusted for lo/hi-res (draw 2x2 pixel in lo-res)
void Chip8::draw_display_pixel(int x, int y, bool pixel, int* collision_count) {
    for (int ny = hi_res ? y : y*2; ny <= (hi_res ? y : y*2+1); ny++) {
        bool row_collided = false;
        for (int nx = hi_res ? x : x*2; nx <= (hi_res ? x : x*2+1); nx++) {
            int screen_coord = nx % screen_w + (ny % screen_h)*screen_w;
            if (ny >= screen_h && hi_res && !row_collided) {
                row_collided = true;
                registers[0xF]++;
            }
            if (display[screen_coord] && !(display[screen_coord] ^ pixel) && !row_collided) {
                row_collided = true;
                if (hi_res)
                    registers[0xF]++;
                else
                    registers[0xF] = 1;
            }
            if (display[screen_coord] && pixel)
                *(collision_count)++;
            display[screen_coord] ^= pixel;
        }
    }
}
