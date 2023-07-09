#include <SDL2/SDL_scancode.h> // stupid intellisense breaks without this BEFORE SDL.h!
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <stdexcept>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include "Chip8.h"
#include "Platform.h"
#include "Config.h"

#define WINDOW_TITLE "Chimp8 - CHIP-8 Interpreter"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320
#define SOUND_EFFECT "res/beep.wav"

// SDL keys for CHIP-8 keypad
const SDL_Scancode keymap[key_count] = {
    SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
    SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
    SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
};

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
Mix_Chunk* beep = NULL;

void terminate(int error_code);

void setup_SDL() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        terminate(-1);
    }

    window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        terminate(-1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        terminate(-1);
    }
    SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Initialize SDL_mixer
    if (sound_enabled) {
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 1, sound_buffer_size) < 0) {
            std::cout << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
            terminate(-1);
        }
        beep = Mix_LoadWAV((get_program_path() + "/" + SOUND_EFFECT).c_str());
        if (beep == NULL) {
            std::cout << "Sound effect could not load! SDL_mixer Error: " << Mix_GetError() << std::endl;
            terminate(-1);
        }
    }
}

void load_rom_from_file(char* file_name, Chip8* vm) {
    SDL_RWops* rom_file_rwops = SDL_RWFromFile(file_name, "r+b");
    size_t rom_size;
    void* rom_file = SDL_LoadFile_RW(rom_file_rwops, &rom_size, 1);

    if (rom_file == NULL) {
        std::cout << "ROM could not be loaded! SDL_Error: " << SDL_GetError() << std::endl;
        terminate(-1);
    }

    vm->load_rom(rom_file, rom_size);
    SDL_free(rom_file);
}

void terminate(int error_code) {
    if (window)
        SDL_DestroyWindow(window);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (beep)
        Mix_FreeChunk(beep);
    Mix_Quit();
    SDL_Quit();
    std::exit(error_code);
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

    // Config
    load_config_into_vm(&vm);

    // Initialize SDL
    setup_SDL();

    // Event handler
    SDL_Event e;
    // Load rom from file into VM
    load_rom_from_file(args[1], &vm);
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
        vm.tick(1e6*delta_time);
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
        main_sleep();
    }

    terminate(0);
    return 0;
}
