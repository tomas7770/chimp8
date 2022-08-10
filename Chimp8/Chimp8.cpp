#include <SDL.h>
#include <iostream>
#include "Chip8.h"

#define WINDOW_TITLE "Chimp8 - CHIP-8 Interpreter"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320

void draw_display(Chip8* vm, SDL_Renderer* renderer);

int main(int argc, char* args[]) {
	// Initialize SDL
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
	void* rom_file = SDL_LoadFile("INVADERS.ch8", NULL);
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
	}
	else {
		window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			WINDOW_WIDTH, WINDOW_HEIGHT,
			SDL_WINDOW_SHOWN);
		if (window == NULL) {
			std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		}
		else {
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
			// Insert renderer error handling here

			// Event handler
			SDL_Event e;
			// VM
			Chip8 vm;
			init_vm(&vm);
			load_rom(&vm, rom_file);
			// Main loop
			bool running = true;
			while (running) {
				while (SDL_PollEvent(&e) != 0) {
					if (e.type == SDL_QUIT)
						running = false;
				}
				cycle_vm(&vm);
				draw_display(&vm, renderer);
			}
		}
	}

	// Quit
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_free(rom_file);
	SDL_Quit();
	return 0;
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
			SDL_Rect fill_rect = { (i % SCREEN_W) * x_scale, (i / SCREEN_H) * y_scale, x_scale, y_scale };
			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderFillRect(renderer, &fill_rect);
		}
	}

	// Update renderer
	SDL_RenderPresent(renderer);
}
