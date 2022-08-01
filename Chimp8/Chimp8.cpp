#include <SDL.h>
#include <iostream>
#include "Chip8.h"

#define WINDOW_TITLE "Chimp8 - CHIP-8 Interpreter"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320

int main(int argc, char* args[]) {
	// Initialize SDL
	SDL_Window* window = NULL;
	SDL_Surface* screen_surface = NULL;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
	}
	else {
		window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			WINDOW_WIDTH, WINDOW_HEIGHT,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
		if (window == NULL) {
			std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		}
		else {
			screen_surface = SDL_GetWindowSurface(window);

			// Event handler
			SDL_Event e;
			// Main loop
			bool running = true;
			while (running) {
				while (SDL_PollEvent(&e) != 0) {
					if (e.type == SDL_QUIT)
						running = false;
				}
			}
		}
	}

	// Quit
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
