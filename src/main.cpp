#include "chip8.hpp"

int main(int argc, char** argv) {

    if (argc < 2) {
        return 1;
    }

    c8::chip8 system;
    system.load_rom(argv[1]);
    system.run();

    return 0;
}
