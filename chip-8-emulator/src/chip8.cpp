#include "../includes/chip8.h"
#include <iostream>
#include <fstream>
#include <vector>

Chip8::Chip8() {
    PC = 0x200; //programs start at memory address 0x200
}

void Chip8::loadROM(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate); //open file

    if (!file) {
        std::cerr << "Failed to open ROM file: " << filename << std::endl;
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > 3584) {
        std::cerr << "ROM is too large!!" << std::endl;
        return;
    }

    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        for (size_t i = 0; i < buffer.size(); i++) {
            memory[0x200 + i] = buffer[i];
        }
        std::cout << "Loaded ROM " << filename << " successfully" << std::endl;
    }
}