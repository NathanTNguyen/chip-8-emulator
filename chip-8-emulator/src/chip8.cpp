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

    render();
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

void Chip8::render() {
    std::cout << "\nCHIP-8 Display:\n";
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            std::cout << (display[y * 64 + x] ? "#" : " ");  // Print filled or empty space
        }
        std::cout << "\n";
    }
}