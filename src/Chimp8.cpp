#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include "Chip8.h"
#include "Clock.h"

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#else
#include <unistd.h>
#include <linux/limits.h>
#endif

#define WINDOW_TITLE "Chimp8 - CHIP-8 Interpreter"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320
#define SOUND_EFFECT "res/beep.wav"
#define MAX_SOUND_BUFFER 65536

#define CONFIG_FOLDER_NAME "Chimp8"
#define CONFIG_NAME "Chimp8.ini"

#define IDLE_SLEEP 1000
#define WIN_IDLE_SLEEP 1

enum ConfigStatus {
	CONFIG_LOADED,
	CONFIG_NOENT,
	CONFIG_ERROR,
};

ConfigStatus config_status;

// SDL keys for CHIP-8 keypad
const SDL_Scancode keymap[key_count] = {
	SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
	SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
	SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
	SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
};

bool sound_enabled = true;
int sound_buffer_size = 1024;

void terminate(SDL_Window* window, SDL_Renderer* renderer, void* rom_file, Mix_Chunk* beep, int error_code) {
	if (window)
		SDL_DestroyWindow(window);
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (rom_file)
		SDL_free(rom_file);
	if (beep)
		Mix_FreeChunk(beep);
	Mix_Quit();
	SDL_Quit();
	std::exit(error_code);
}

std::string get_program_path() {
#ifdef _WIN32
	WCHAR program_path[MAX_PATH];
	GetModuleFileNameW(NULL, program_path, MAX_PATH);
	PathRemoveFileSpecW(program_path);

	char program_path_conv[MAX_PATH];
	WideCharToMultiByte(CP_UTF8, 0, program_path, -1, program_path_conv, MAX_PATH, NULL, NULL);

	return std::string(program_path_conv);
#else
	char program_path[PATH_MAX];
	if (readlink("/proc/self/exe", program_path, PATH_MAX) == -1) {
		throw std::runtime_error("Couldn't find program path");
	}

	std::string program_path_str(program_path);
	return program_path_str.substr(0, program_path_str.find_last_of("/"));
#endif
}

std::string get_config_path() {
#ifdef _WIN32
	std::string config_path = get_program_path() + "\\" + CONFIG_NAME;
	return config_path;
#else
	std::string config_home;
	
	if (const char* config_home_raw = std::getenv("XDG_CONFIG_HOME")) {
		config_home = std::string(config_home_raw);
	}
	else {
		if (const char* user_home = std::getenv("HOME")) {
			config_home = std::string(user_home) + "/.config";
		}
		else {
			throw std::runtime_error("Couldn't find appropriate location to load config");
		}
	}

	std::string config_path = config_home + "/" + CONFIG_FOLDER_NAME + "/" + CONFIG_NAME;
	return config_path;
#endif
}

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

void parse_config(std::shared_ptr<std::fstream> config, Chip8* vm, Clock* clock) {
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
					clock->set_cycle_rate(std::min(std::stoul(value), 1000000UL));
				} catch (...) {}
			}
			else if (key == "sound" && value == "false") {
				sound_enabled = false;
			}
			else if (key == "sound_buffer") {
				try {
					sound_buffer_size = std::max(0, std::min(std::stoi(value), MAX_SOUND_BUFFER));
				} catch (...) {}
			}
			else if (key == "legacy_memops" && value == "true") {
				vm->set_legacy_memops(true);
			}
			else if (key == "legacy_shift" && value == "true") {
				vm->set_legacy_shift(true);
			}
		}
	}
}

std::string bool_to_str(bool b) { return b ? "true" : "false";}

void write_config_line(std::shared_ptr<std::fstream> config, std::string key, std::string value) {
	std::string line = key + "=" + value + "\n";
	config->write(line.c_str(), line.length());
}

void write_config(Chip8* vm, Clock* clock) {
	std::shared_ptr<std::fstream> config = load_config(true);
	if (!config) {
		std::cout << "Failed to write config.\n";
		return;
	}
	write_config_line(config, "cycles", std::to_string(clock->get_cycle_rate()));
	write_config_line(config, "sound", bool_to_str(sound_enabled));
	write_config_line(config, "sound_buffer", std::to_string(sound_buffer_size));
	write_config_line(config, "legacy_memops", bool_to_str(vm->get_legacy_memops()));
	write_config_line(config, "legacy_shift", bool_to_str(vm->get_legacy_shift()));
}

void draw_display(Chip8* vm, SDL_Renderer* renderer) {
	// Clear screen
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(renderer);

	// Draw display
	int x_scale = WINDOW_WIDTH / screen_w;
	int y_scale = WINDOW_HEIGHT / screen_h;
	for (int i = 0; i < screen_size; i++) {
		if (vm->get_display_pixel(i)) {
			// Render white filled quad
			SDL_Rect fill_rect = { (i % screen_w) * x_scale, (i / screen_w) * y_scale, x_scale, y_scale };
			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderFillRect(renderer, &fill_rect);
		}
	}

	// Update renderer
	SDL_RenderPresent(renderer);
}

int main(int argc, char* args[]) {
	if (argc < 2) {
		std::cout << "Usage: Chimp8 <rom file>" << std::endl;
		return 0;
	}

	// Create VM
	Chip8 vm;
	Clock clock(&vm);

	// Config
	{
		std::shared_ptr<std::fstream> config = load_config(false);
		switch (config_status) {
			case CONFIG_NOENT:
				std::cout << "Config not found. Default settings will be used, and a config file will be created.\n";
				break;
			case CONFIG_ERROR:
				std::cout << "Error loading config. Default settings will be used.\n";
				break;
		}
		parse_config(config, &vm, &clock);
		if (config_status != CONFIG_ERROR)
			write_config(&vm, &clock);
	}

	// Initialize SDL
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_RWops* rom_file_rwops = SDL_RWFromFile(args[1], "r+b");
	size_t rom_size;
	void* rom_file = SDL_LoadFile_RW(rom_file_rwops, &rom_size, 1);

	if (rom_file == NULL) {
		std::cout << "ROM could not be loaded! SDL_Error: " << SDL_GetError() << std::endl;
		terminate(window, renderer, rom_file, NULL, -1);
	}
	
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
		terminate(window, renderer, rom_file, NULL, -1);
	}

	window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		terminate(window, renderer, rom_file, NULL, -1);
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		terminate(window, renderer, rom_file, NULL, -1);
	}
	SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

	// Initialize SDL_mixer
	Mix_Chunk* beep = NULL;
	if (sound_enabled) {
		if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 1, sound_buffer_size) < 0) {
			std::cout << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
			terminate(window, renderer, rom_file, beep, -1);
		}
		beep = Mix_LoadWAV((get_program_path() + "/" + SOUND_EFFECT).c_str());
		if (beep == NULL) {
			std::cout << "Sound effect could not load! SDL_mixer Error: " << Mix_GetError() << std::endl;
			terminate(window, renderer, rom_file, beep, -1);
		}
	}

	// Event handler
	SDL_Event e;
	// Load rom from file into VM
	vm.load_rom(rom_file, rom_size);
	// Main loop
	uint64_t frame_timestamp = SDL_GetTicks64();
	int delay_metatimer = 0;
	int sound_metatimer = 0;
	bool running = true;
	while (running) {
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT)
				running = false;
			else if (e.type == SDL_KEYDOWN) {
				for (int i = 0; i < key_count; i++) {
					if (e.key.keysym.scancode == keymap[i]) {
						vm.on_keypress(i);
						break;
					}
				}
			}
			else if (e.type == SDL_KEYUP) {
				for (int i = 0; i < key_count; i++) {
					if (e.key.keysym.scancode == keymap[i]) {
						vm.on_keyrelease(i);
						break;
					}
				}
			}
		}
		uint64_t delta_time = SDL_GetTicks64() - frame_timestamp;
		frame_timestamp = SDL_GetTicks64();
		clock.tick(1e6*delta_time);
		delay_metatimer += delta_time;
		sound_metatimer += delta_time;
		vm.cycle_delaytimer(delay_metatimer);
		uint8_t sound_timer = vm.cycle_soundtimer(sound_metatimer);
		if (sound_enabled) {
			if (sound_timer > 0 && !Mix_Playing(0)) {
				Mix_PlayChannel(0, beep, -1);
			}
			else if (sound_timer == 0 && Mix_Playing(0)) {
				Mix_HaltChannel(0);
			}
		}
		draw_display(&vm, renderer);
		#ifdef _WIN32
		Sleep(WIN_IDLE_SLEEP);
		#else
		usleep(IDLE_SLEEP);
		#endif
	}

	terminate(window, renderer, rom_file, beep, 0);
	return 0;
}
