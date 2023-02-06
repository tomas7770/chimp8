#ifndef CLOCK_H
#define CLOCK_H

#include <cstdint>
#include "Chip8.h"

// Time units in nanoseconds (except for cycle_rate which is cycles/sec)

class Clock {
public:
	Clock(Chip8* target_vm);
	uint64_t get_cycle_rate();
	void set_cycle_rate(uint64_t new_cycle_rate);
	void tick(uint64_t delta_time);
private:
	uint64_t cycle_timer;
	uint64_t cycle_rate;
	uint64_t cycle_time;
	uint64_t max_cycle_accum;
	Chip8* vm;
};

#endif
