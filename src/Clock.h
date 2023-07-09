#ifndef CLOCK_H
#define CLOCK_H

#include <cstdint>

class Chip8;

// Time units in nanoseconds

class Clock {
public:
    Clock(Chip8* target_vm);
    void set_cycle_rate(uint64_t new_cycle_rate);
    void tick(uint64_t delta_time);
private:
    uint64_t cycle_timer;
    uint64_t cycle_time;
    uint64_t max_cycle_accum;
    Chip8* vm;
};

#endif
