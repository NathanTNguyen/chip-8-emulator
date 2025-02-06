#include "../includes/chip8.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <algorithm>

Chip8::Chip8() {
    PC = 0x200; //programs start at memory address 0x200
}

void Chip8::reset() {
    //reset the program counter to the start location of most programs
    PC = 0x200;\
    //clear memory
    memory.fill(0);
    //clear the registers
    V.fill(0);
    //reset index
    I = 0;
    //clear the display
    display.fill(0);
    std::cout << "Chip-8 state has been reset" << std::endl;
}

void Chip8::loadROM(const uint8_t* romData, size_t size) {
    if (size > 3584) {
        std::cerr << "ROM is too large!!" << std::endl;
        return;
    }

    for (size_t i = 0; i < size; i++) {
        memory[0x200 + i] = romData[i]; // Load ROM into memory starting at 0x200
    }
    std::cout << "Loaded ROM successfully (" << size << " bytes)" << std::endl;
}

void Chip8::emulateCycle() {
    uint16_t opcode = memory[PC] << 8 | memory[PC + 1];
    std::cout << "Fetched opcode: 0x" << std::hex << std::setw(4) << std::setfill('0') << opcode << std::endl;

    executeOpcode(opcode);
    PC += 2;

    // Add debug output
    std::cout << "First 10 pixels in display buffer: ";
    for (int i = 0; i < 10; i++) {
        std::cout << (int)display[i] << " ";
    }
    std::cout << std::endl;
}

void Chip8::executeOpcode(uint16_t opcode) {
    switch (opcode & 0xF000) { //extracts the first hex (for switch case)
        case 0x0000: //0 instruction
            if (opcode == 0x00E0) {
                display.fill(0); //set all pixels to 0 (off)
                std::cout << "Executed: Clear Screen (0x00E0)" << std::endl;
            }
            break;
        case 0x6000: { //0x6XNN - set VX to NN
            uint8_t X = (opcode & 0x0F00) >> 8; //Extract X
            uint8_t NN = opcode & 0x00FF; //Extract NN
            V[X] = NN;
            std::cout << "Executed: Set V" << std::dec << int(X) << " = 0x" << std::hex << std::setw(2) << std::setfill('0') << int(NN) << std::endl;
            break;
        }
        case 0x7000: { //0x7XNN - Add NN to VX
            uint8_t X = (opcode & 0x0F00) >> 8; //Extract X
            uint8_t NN = opcode & 0x00FF;
            V[X] += NN; //Add NN to VX (no carry flag modification)
            std::cout << "Executed: V" << std::dec << int(X) << " += 0x" << std::hex << std::setw(2) << std::setfill('0') << int(NN) << " (New V" << std::dec << int(X) << " = 0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << int(V[X]) << ")" << std::endl;
            break;
        }
        case 0xA000: { //0xANNN - Set index register I
            uint16_t NNN = opcode & 0x0FFF; //extract NNN (12-bit address)
            I = NNN;
            std::cout << "Executed: Set I = 0x" << std::hex << std::setw(3) << std::setfill('0') << I << std::endl;
            break;
        }
        case 0xD000: { //0xDXYN - Draw sprite at (VX, VY) of N height
            uint8_t X = (opcode & 0x0F00) >> 8; //extract x
            uint8_t Y = (opcode & 0x00F0) >> 4; //extract y
            uint8_t N = opcode & 0x000F;

            uint8_t xPos = V[X] % 64; //ensure we remain within the 64x32 screen
            uint8_t yPos = V[Y] % 32;
            V[0xF] = 0; //clear collision flag

            for (int row = 0; row < N; row++) {  
                uint8_t spriteByte = memory[I + row];  // Get sprite row from memory

                for (int col = 0; col < 8; col++) {  
                    uint8_t pixel = (spriteByte >> (7 - col)) & 1;  // Extract individual pixel

                    int screenIndex = (yPos + row) * 64 + (xPos + col);  // Map (x,y) to 1D array

                    if (pixel == 1) {  // Only XOR if pixel is set
                        if (display[screenIndex] == 1) {
                            V[0xF] = 1;  // Collision detected
                        }
                        display[screenIndex] ^= 1;  // XOR pixel (toggle)
                    }
                }
            }
            std::cout << "Executed: Draw sprite at (V" << std::dec << int(X) << ", V" << int(Y) << ") with height " << int(N) << std::endl;
            break;
        }

        default:
            std::cout << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
    }
}

uint8_t* Chip8::getDisplayBuffer() {
    return display.data(); //return pointer to display array
}