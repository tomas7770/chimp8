#include <iostream>
#include "Chimp8App.h"

int main(int argc, char* args[]) {
    if (argc < 2) {
        std::cout << "Usage: Chimp8 <rom file>" << std::endl;
        return 0;
    }

    Chimp8App app;
    app.load_rom_from_file(args[1]);
    app.main_loop();

    return 0;
}
