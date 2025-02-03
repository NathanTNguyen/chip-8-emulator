#include <iostream>
#include <cstdint>
#include "../includes/chip8.h"

int main()
{
    std::cout << "CHIP-8 Emulator..." << std::endl;

    Chip8 chip8;
    chip8.loadROM("roms/ibm-logo.ch8");

    for (int i = 0; i < 40; i++) {
        chip8.emulateCycle();
    }

    return 0;
}
