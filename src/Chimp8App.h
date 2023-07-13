#ifndef CHIMP8APP_H
#define CHIMP8APP_H

#include <SDL2/SDL_scancode.h> // stupid intellisense breaks without this BEFORE SDL.h!
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "Chip8.h"

class Chimp8App {
public:
    Chimp8App();
    void load_rom_from_file(char* file_name);
    void main_loop();
private:
    constexpr const static char* window_title = "Chimp8 - CHIP-8 Interpreter";
    constexpr static int window_width = 640;
    constexpr static int window_height = 320;
    constexpr const static char* sound_effect = "res/beep.wav";
    // SDL keys for CHIP-8 keypad
    constexpr static SDL_Scancode keymap[key_count] = {
        SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
        SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
        SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
        SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
    };
    //
    SDL_Window* window_sdl = NULL;
    SDL_Renderer* renderer_sdl = NULL;
    Mix_Chunk* beep = NULL;
    SDL_Event event_sdl;
    Chip8 vm;

    void draw_display();
    void terminate(int error_code);
};

#endif
