#ifndef CHIMP8CFG_H
#define CHIMP8CFG_H

#include <cstdint>
#include <fstream>
#include <string>
#include <memory>
#include "Chip8.h"

enum ConfigStatus {
    CONFIG_LOADED,
    CONFIG_NOENT,
    CONFIG_ERROR,
};

extern ConfigStatus config_status;
extern uint64_t config_cycle_rate;
extern bool sound_enabled;
extern int sound_buffer_size;

std::shared_ptr<std::fstream> load_config(bool write_mode);
void parse_config(std::shared_ptr<std::fstream> config, Chip8* vm);
void write_config_line(std::shared_ptr<std::fstream> config, std::string key, std::string value);
void write_config(Chip8* vm);
void load_config_into_vm(Chip8* vm);

#endif
