#ifndef CHIP8_H
#define CHIP8_H

#include <array>
#include <cstdint>

class Chip8 {
    public:
        Chip8(); //the constructor
        void loadROM(const char* filename); //loads ROM into memory
        void emulateCycle(); //fetch, decode and execute an opcode/instruction

    private:
        std::array<uint8_t, 4096> memory{}; //4kb RAM
        uint16_t PC; //program counter
        uint16_t I; //index register (for storing memory addresses)
        std::array<uint8_t, 64 * 32> display{};
        std::array<uint8_t, 16> V{}; //chip-8 has 16 registers (V0 through to VF)
        void executeOpcode(uint16_t opcode);
        void render();
};

#endif