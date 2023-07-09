#include "Config.h"
#include "Platform.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

constexpr int max_sound_buffer = 65536;

// Strings for storing timing mode in config
const std::string timing_mode_strings[] = {
    "fixed",
    "cosmac",
};

ConfigStatus config_status;

uint64_t config_cycle_rate = 500;
bool sound_enabled = true;
int sound_buffer_size = 1024;

std::shared_ptr<std::fstream> load_config(bool write_mode) {
    std::shared_ptr<std::fstream> config = std::make_shared<std::fstream>();
    try {
        config->open(get_config_path(), write_mode ? (std::ios::out | std::ios::trunc) : std::ios::in);
    }
    catch (std::runtime_error) {
        config_status = CONFIG_ERROR;
        return NULL;
    }
    if (config->fail()) {
        if (errno == ENOENT)
            config_status = CONFIG_NOENT;
        else
            config_status = CONFIG_ERROR;
        return NULL;
    }
    config_status = CONFIG_LOADED;
    return config;
}

void parse_config(std::shared_ptr<std::fstream> config, Chip8* vm) {
    if (config) {
        std::string line, key, value;
        while (std::getline(*config, line)) {
            line.erase(std::remove_if(line.begin(), line.end(),
                [](unsigned char x){ return std::isspace(x); }), line.end());

            int eq_pos = line.find("=");
            if (eq_pos == std::string::npos || line.length() <= eq_pos + 1)
                continue;
            key = line.substr(0, eq_pos);
            value = line.substr(eq_pos + 1);
            
            if (key == "cycles") {
                try {
                    config_cycle_rate = std::min(std::stoul(value), 1000000UL);
                } catch (...) {}
            }
            else if (key == "sound" && value == "false") {
                sound_enabled = false;
            }
            else if (key == "sound_buffer") {
                try {
                    sound_buffer_size = std::max(0, std::min(std::stoi(value), max_sound_buffer));
                } catch (...) {}
            }
            else if (key == "legacy_memops" && value == "true") {
                vm->set_legacy_memops(true);
            }
            else if (key == "legacy_shift" && value == "true") {
                vm->set_legacy_shift(true);
            }
            else if (key == "timing" && value == timing_mode_strings[TIMING_FIXED]) {
                vm->set_timing_mode(TIMING_FIXED);
            }
        }
    }
    vm->set_cycle_rate(config_cycle_rate);
}

void write_config_line(std::shared_ptr<std::fstream> config, std::string key, std::string value) {
    std::string line = key + "=" + value + "\n";
    config->write(line.c_str(), line.length());
}

static std::string bool_to_str(bool b) { return b ? "true" : "false";}

void write_config(Chip8* vm) {
    std::shared_ptr<std::fstream> config = load_config(true);
    if (!config) {
        std::cout << "Failed to write config.\n";
        return;
    }
    write_config_line(config, "cycles", std::to_string(config_cycle_rate));
    write_config_line(config, "sound", bool_to_str(sound_enabled));
    write_config_line(config, "sound_buffer", std::to_string(sound_buffer_size));
    write_config_line(config, "legacy_memops", bool_to_str(vm->get_legacy_memops()));
    write_config_line(config, "legacy_shift", bool_to_str(vm->get_legacy_shift()));
    write_config_line(config, "timing", timing_mode_strings[vm->get_timing_mode()]);
}

void load_config_into_vm(Chip8* vm) {
    std::shared_ptr<std::fstream> config = load_config(false);
    switch (config_status) {
        case CONFIG_NOENT:
            std::cout << "Config not found. Default settings will be used, and a config file will be created.\n";
            break;
        case CONFIG_ERROR:
            std::cout << "Error loading config. Default settings will be used.\n";
            break;
    }
    parse_config(config, vm);
    if (config_status != CONFIG_ERROR)
        write_config(vm);
}
