#include "Clock.h"
#include <cstdint>
#include "Chip8.h"

constexpr uint64_t default_cycle_rate = 1000;
constexpr uint64_t max_cycles_per_frame = 20000;

Clock::Clock(Chip8* target_vm) {
    cycle_timer = 0;
    set_cycle_rate(default_cycle_rate);
    vm = target_vm;
}

void Clock::set_cycle_rate(uint64_t new_cycle_rate) {
    cycle_time = 1e9 / new_cycle_rate;
    max_cycle_accum = cycle_time * max_cycles_per_frame;
}

void Clock::tick(uint64_t delta_time) {
    cycle_timer += delta_time;
    if (cycle_timer > max_cycle_accum)
        cycle_timer = max_cycle_accum;
    while (cycle_timer >= cycle_time) {
        vm->cycle_vm();
        cycle_timer -= cycle_time;
    }
}
