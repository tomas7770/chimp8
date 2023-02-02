#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include "Chip8.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
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

#define MAX_CYCLES_PER_FRAME 100

// SDL keys for CHIP-8 keypad
const SDL_Scancode keymap[KEY_COUNT] = {
	SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
	SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
	SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
	SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
};

uint64_t cycle_rate = 1000;
uint64_t cycle_time;
uint64_t max_cycle_accum;

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

std::shared_ptr<std::fstream> load_config() {
	std::shared_ptr<std::fstream> config = std::make_shared<std::fstream>();

	#ifdef _WIN32
	config->open(CONFIG_NAME, std::ios::in);
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
			std::cout << "Error loading config. Default settings will be used.\n";
			return NULL;
		}
	}

	std::string config_path = config_home + "/" + CONFIG_FOLDER_NAME + "/" + CONFIG_NAME;
	config->open(config_path, std::ios::in);
	#endif

	if (config->fail()) {
		if (errno == ENOENT)
			std::cout << "Config not found. Default settings will be used.\n";
		else
			std::cout << "Error loading config. Default settings will be used.\n";
		return NULL;
	}
	return config;
}

void parse_config(std::shared_ptr<std::fstream> config) {
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
					cycle_rate = std::min(std::stoul(value), 1000000UL);
				} catch (...) {}
			}
			else if (key == "sound" && value == "false") {
				sound_enabled = false;
			}
			else if (key == "sound_buffer") {
				try {
					sound_buffer_size = std::clamp(std::stoi(value), 0, MAX_SOUND_BUFFER);
				} catch (...) {}
			}
		}
	}
	cycle_time = 1000000 / cycle_rate;
	max_cycle_accum = cycle_time * MAX_CYCLES_PER_FRAME;
}

void draw_display(Chip8* vm, SDL_Renderer* renderer) {
	// Clear screen
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(renderer);

	// Draw display
	int x_scale = WINDOW_WIDTH / SCREEN_W;
	int y_scale = WINDOW_HEIGHT / SCREEN_H;
	for (int i = 0; i < SCREEN_SIZE; i++) {
		if (vm->display[i]) {
			// Render white filled quad
			SDL_Rect fill_rect = { (i % SCREEN_W) * x_scale, (i / SCREEN_W) * y_scale, x_scale, y_scale };
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
	// Config
	{
		std::shared_ptr<std::fstream> config = load_config();
		parse_config(config);
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
		beep = Mix_LoadWAV(SOUND_EFFECT);
		if (beep == NULL) {
			std::cout << "Sound effect could not load! SDL_mixer Error: " << Mix_GetError() << std::endl;
			terminate(window, renderer, rom_file, beep, -1);
		}
	}

	// Event handler
	SDL_Event e;
	// VM
	Chip8 vm;
	init_vm(&vm);
	load_rom(&vm, rom_file, &rom_size);
	// Main loop
	uint64_t frame_timestamp = SDL_GetTicks64();
	uint64_t cycle_timer = 0; // in microseconds
	int delay_metatimer = 0;
	int sound_metatimer = 0;
	bool running = true;
	while (running) {
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT)
				running = false;
			else if (e.type == SDL_KEYDOWN) {
				for (int i = 0; i < KEY_COUNT; i++) {
					if (e.key.keysym.scancode == keymap[i]) {
						on_keypress(&vm, i);
						break;
					}
				}
			}
			else if (e.type == SDL_KEYUP) {
				for (int i = 0; i < KEY_COUNT; i++) {
					if (e.key.keysym.scancode == keymap[i]) {
						vm.keys[i] = false;
						break;
					}
				}
			}
		}
		uint64_t delta_time = SDL_GetTicks64() - frame_timestamp;
		cycle_timer += 1000 * delta_time;
		if (cycle_timer > max_cycle_accum)
			cycle_timer = max_cycle_accum;
		delay_metatimer += delta_time;
		sound_metatimer += delta_time;
		frame_timestamp = SDL_GetTicks64();
		while (cycle_timer >= cycle_time) {
			cycle_vm(&vm);
			cycle_timer -= cycle_time;
		}
		cycle_delaytimer(&vm, delay_metatimer);
		uint8_t sound_timer = cycle_soundtimer(&vm, sound_metatimer);
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
