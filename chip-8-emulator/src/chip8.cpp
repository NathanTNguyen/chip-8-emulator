#include "../includes/chip8.h"
#include <fstream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <emscripten.h>

Chip8::Chip8()
{
    PC = 0x200; // programs start at memory address 0x200
}

void Chip8::reset()
{
    // Reset the program counter to the start location of most programs
    PC = 0x200;
    // Clear memory
    memory.fill(0);
    // Clear the registers
    V.fill(0);
    // Reset index
    I = 0;
    // Clear the display
    display.fill(0);
    // Log the reset action
    EM_ASM({
        appendLog("Chip-8 state has been reset");
    });
}

void Chip8::loadROM(const uint8_t *romData, size_t size)
{
    if (size > 3584)
    {
        EM_ASM({
            appendLog("ERROR: ROM is too large!!");
        });
        return;
    }

    for (size_t i = 0; i < size; i++)
    {
        memory[0x200 + i] = romData[i]; // Load ROM into memory starting at 0x200
    }
    // Log successful load, passing size as an argument.
    EM_ASM_({ appendLog("Loaded ROM successfully (" + $0.toString() + " bytes)"); }, size);
}

void Chip8::emulateCycle()
{
    uint16_t opcode = (memory[PC] << 8) | memory[PC + 1];
    // Log fetched opcode. We convert the opcode to hex in JS.
    EM_ASM_({
        // Format opcode as a 4-digit hexadecimal string.
        var hexOpcode = ("0000" + $0.toString(16)).slice(-4);
        appendLog("Fetched opcode: 0x" + hexOpcode); }, opcode);

    executeOpcode(opcode);
}

void Chip8::executeOpcode(uint16_t opcode)
{
    switch (opcode & 0xF000)
    {            // Extracts the first hex nibble (for switch case)
    case 0x0000: // 0 instructions
        if (opcode == 0x00E0)
        {
            display.fill(0); // Set all pixels to 0 (off)
            EM_ASM({
                appendLog("Executed: Clear Screen (0x00E0)");
            });
        }
        PC += 2;
        break;
    case 0x1000:
    { // 0x1NNN - Set the program counter to NNN (Jump)
        uint16_t NNN = opcode & 0x0FFF;
        PC = NNN;
        EM_ASM_({
                var hexAddr = ("000" + $0.toString(16)).slice(-3);
                appendLog("Executed: Jump to address 0x" + hexAddr); }, NNN);
    }
    break;
    case 0x6000:
    {                                       // 0x6XNN - Set VX to NN
        uint8_t X = (opcode & 0x0F00) >> 8; // Extract X
        uint8_t NN = opcode & 0x00FF;       // Extract NN
        V[X] = NN;
        EM_ASM_({ appendLog("Executed: Set V" + $0.toString() + " = 0x" + ($1.toString(16).toUpperCase().padStart(2, "0"))); }, X, NN);
        PC += 2;
        break;
    }
    case 0x7000:
    {                                       // 0x7XNN - Add NN to VX
        uint8_t X = (opcode & 0x0F00) >> 8; // Extract X
        uint8_t NN = opcode & 0x00FF;
        V[X] += NN; // Add NN to VX (no carry flag modification)
        EM_ASM_({ appendLog("Executed: V" + $0.toString() + " += 0x" + ($1.toString(16).toUpperCase().padStart(2, "0")) +
                            " (New V" + $0.toString() + " = 0x" + ($2.toString(16).toUpperCase().padStart(2, "0")) + ")"); }, X, NN, V[X]);
        PC += 2;
        break;
    }
    case 0xA000:
    {                                   // 0xANNN - Set index register I
        uint16_t NNN = opcode & 0x0FFF; // Extract NNN (12-bit address)
        I = NNN;
        EM_ASM_({
                var hexI = ("000" + $0.toString(16)).slice(-3);
                appendLog("Executed: Set I = 0x" + hexI); }, NNN);
        PC += 2;
        break;
    }
    case 0xD000:
    {                                       // 0xDXYN - Draw sprite at (VX, VY) with height N
        uint8_t X = (opcode & 0x0F00) >> 8; // Extract X
        uint8_t Y = (opcode & 0x00F0) >> 4; // Extract Y
        uint8_t N = opcode & 0x000F;
        uint8_t xPos = V[X] % 64; // Ensure within 64x32 screen
        uint8_t yPos = V[Y] % 32;
        V[0xF] = 0; // Clear collision flag

        for (int row = 0; row < N; row++)
        {
            uint8_t spriteByte = memory[I + row]; // Get sprite row from memory
            for (int col = 0; col < 8; col++)
            {
                uint8_t pixel = (spriteByte >> (7 - col)) & 1;      // Extract individual pixel
                int screenIndex = (yPos + row) * 64 + (xPos + col); // Map (x,y) to 1D array
                if (pixel == 1)
                { // Only XOR if pixel is set
                    if (display[screenIndex] == 1)
                    {
                        V[0xF] = 1; // Collision detected
                    }
                    display[screenIndex] ^= 1; // XOR pixel (toggle)
                }
            }
        }
        EM_ASM_({ appendLog("Executed: Draw sprite at (V" + $0.toString() + ", V" + $1.toString() + ") with height " + $2.toString()); }, X, Y, N);
        PC += 2;
        break;
    }
    default:
        EM_ASM({
            appendLog("Executed: Unknown opcode encountered.");
        });
        PC += 2;
        break;
    }
}

uint8_t *Chip8::getDisplayBuffer()
{
    return display.data(); // Return pointer to display array
}
