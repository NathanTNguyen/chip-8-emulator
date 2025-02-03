#include "../includes/chip8.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

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

void Chip8::emulateCycle() {
    //fetch opcode (2 bytes)
    uint16_t opcode = memory[PC] << 8 | memory[PC+1]; //left shift our initial byte of info (by 8 bits) to convert to 16 bit format, then combine both together to form full opcode (2 bytes)

    //print the fetched opcode
    std::cout << "Fetched opcode: 0x" << std::hex << std::setw(4) << std::setfill('0') << opcode << std::endl;

    executeOpcode(opcode);

    //move to next instrucion/opcode
    PC += 2;
}

void Chip8::executeOpcode(uint16_t opcode) {
    switch (opcode & 0xF000) { //extracts the first hex (for switch case)
        case 0x0000: //0 instruction
            if (opcode == 0x00E0) {
                display.fill(0); //set all pixels to 0 (off)
                std::cout << "Executed: Clear Screen (0x00E0)" << std::endl;
            }
            break;
        case 0x6000: { //0x6XNN only has one opcode - set VX to NN
            uint8_t X = (opcode & 0x0F00) >> 8; //Extract X
            uint8_t NN = opcode & 0x00FF;
            V[X] = NN;
            std::cout << "Executed: Set V" << (int)X << " = " << std::hex << (int)NN << std::endl;
            break;

        }
        default:
            std::cout << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
    }
}