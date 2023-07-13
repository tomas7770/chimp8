#include "Chimp8App.h"
#include "Platform.h"
#include "Config.h"
#include <iostream>

Chimp8App::Chimp8App() {
    load_config_into_vm(&vm);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        terminate(-1);
    }

    window_sdl = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        window_width, window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window_sdl == NULL) {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        terminate(-1);
    }

    renderer_sdl = SDL_CreateRenderer(window_sdl, -1, SDL_RENDERER_ACCELERATED);
    if (renderer_sdl == NULL) {
        std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        terminate(-1);
    }
    SDL_RenderSetLogicalSize(renderer_sdl, window_width, window_height);

    // Initialize SDL_mixer
    if (sound_enabled) {
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 1, sound_buffer_size) < 0) {
            std::cout << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
            terminate(-1);
        }
        beep = Mix_LoadWAV((get_program_path() + "/" + sound_effect).c_str());
        if (beep == NULL) {
            std::cout << "Sound effect could not load! SDL_mixer Error: " << Mix_GetError() << std::endl;
            terminate(-1);
        }
    }
}

void Chimp8App::load_rom_from_file(char* file_name) {
    SDL_RWops* rom_file_rwops = SDL_RWFromFile(file_name, "r+b");
    size_t rom_size;
    void* rom_file = SDL_LoadFile_RW(rom_file_rwops, &rom_size, 1);

    if (rom_file == NULL) {
        std::cout << "ROM could not be loaded! SDL_Error: " << SDL_GetError() << std::endl;
        terminate(-1);
    }

    vm.load_rom(rom_file, rom_size);
    SDL_free(rom_file);
}

void Chimp8App::draw_display() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer_sdl, 0, 0, 0, 0xFF);
    SDL_RenderClear(renderer_sdl);

    // Draw display
    int x_scale = window_width / screen_w;
    int y_scale = window_height / screen_h;
    for (int i = 0; i < screen_size; i++) {
        if (vm.get_display_pixel(i)) {
            // Render white filled quad
            SDL_Rect fill_rect = { (i % screen_w) * x_scale, (i / screen_w) * y_scale, x_scale, y_scale };
            SDL_SetRenderDrawColor(renderer_sdl, 0xFF, 0xFF, 0xFF, 0xFF);
            SDL_RenderFillRect(renderer_sdl, &fill_rect);
        }
    }

    // Update renderer
    SDL_RenderPresent(renderer_sdl);
}

void Chimp8App::main_loop() {
    uint64_t frame_timestamp = SDL_GetTicks64();
    int delay_metatimer = 0;
    int sound_metatimer = 0;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event_sdl) != 0) {
            if (event_sdl.type == SDL_QUIT)
                running = false;
            else if (event_sdl.type == SDL_KEYDOWN) {
                for (int i = 0; i < key_count; i++) {
                    if (event_sdl.key.keysym.scancode == keymap[i]) {
                        vm.on_keypress(i);
                        break;
                    }
                }
            }
            else if (event_sdl.type == SDL_KEYUP) {
                for (int i = 0; i < key_count; i++) {
                    if (event_sdl.key.keysym.scancode == keymap[i]) {
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
        draw_display();
        main_sleep();
    }

    terminate(0);
}

void Chimp8App::terminate(int error_code) {
    if (window_sdl)
        SDL_DestroyWindow(window_sdl);
    if (renderer_sdl)
        SDL_DestroyRenderer(renderer_sdl);
    if (beep)
        Mix_FreeChunk(beep);
    Mix_Quit();
    SDL_Quit();
    std::exit(error_code);
}
