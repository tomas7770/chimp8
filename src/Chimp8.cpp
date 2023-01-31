#include <SDL2/SDL.h>
#include <iostream>
#include <cstdlib>
#include "Chip8.h"

#define WINDOW_TITLE "Chimp8 - CHIP-8 Interpreter"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320

#define CYCLE_RATE 1000
#define MAX_CYCLES_PER_FRAME 100

// SDL key codes for CHIP-8 keypad
const SDL_Keycode keymap[KEY_COUNT] = {
	SDLK_x, SDLK_1, SDLK_2, SDLK_3,
	SDLK_q, SDLK_w, SDLK_e, SDLK_a,
	SDLK_s, SDLK_d, SDLK_z, SDLK_c,
	SDLK_4, SDLK_r, SDLK_f, SDLK_v
};

const uint64_t cycle_time = 1000000 / CYCLE_RATE;
const uint64_t max_cycle_accum = cycle_time * MAX_CYCLES_PER_FRAME;

void terminate(SDL_Window* window, SDL_Renderer* renderer, void* rom_file, int error_code) {
	if (window)
		SDL_DestroyWindow(window);
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (rom_file)
		SDL_free(rom_file);
	SDL_Quit();
	std::exit(error_code);
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
	// Initialize SDL
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_RWops* rom_file_rwops = SDL_RWFromFile(args[1], "r+b");
	size_t rom_size;
	void* rom_file = SDL_LoadFile_RW(rom_file_rwops, &rom_size, 1);

	if (rom_file == NULL) {
		std::cout << "ROM could not be loaded! SDL_Error: " << SDL_GetError() << std::endl;
		terminate(window, renderer, rom_file, -1);
	}
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
		terminate(window, renderer, rom_file, -1);
	}

	window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_SHOWN);
	if (window == NULL) {
		std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		terminate(window, renderer, rom_file, -1);
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		terminate(window, renderer, rom_file, -1);
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
	bool running = true;
	while (running) {
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT)
				running = false;
			else if (e.type == SDL_KEYDOWN) {
				for (int i = 0; i < KEY_COUNT; i++) {
					if (e.key.keysym.sym == keymap[i]) {
						on_keypress(&vm, i);
						break;
					}
				}
			}
			else if (e.type == SDL_KEYUP) {
				for (int i = 0; i < KEY_COUNT; i++) {
					if (e.key.keysym.sym == keymap[i]) {
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
		frame_timestamp = SDL_GetTicks64();
		while (cycle_timer >= cycle_time) {
			cycle_vm(&vm);
			cycle_timer -= cycle_time;
		}
		cycle_delaytimer(&vm, delay_metatimer);
		draw_display(&vm, renderer);
	}

	terminate(window, renderer, rom_file, 0);
	return 0;
}
