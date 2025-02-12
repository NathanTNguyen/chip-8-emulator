#include "../includes/chip8.h"
#include <fstream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <emscripten.h>
#include <cstdlib>

const uint16_t FONT_START_ADDRESS = 0x50;
const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8()
{
    srand(time(0)); // seed the random number generator
    PC = 0x200;     // programs start at memory address 0x200
    // Load the fontset into memory
    for (size_t i = 0; i < sizeof(chip8_fontset); i++)
    {
        memory[FONT_START_ADDRESS + i] = chip8_fontset[i];
    }
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
    // Reset the stack pointer
    SP = 0;
    // clear the stack
    stack.fill(0);
    // reset timers
    delayTimer = 0;
    soundTimer = 0;
    // reset keys
    keys.fill(0);
    // Reload the fontset after clearing memory
    for (size_t i = 0; i < sizeof(chip8_fontset); i++)
    {
        memory[FONT_START_ADDRESS + i] = chip8_fontset[i];
    }
    // Log the reset action
    //EM_ASM({ appendLog("Chip-8 state has been reset"); });
}

void Chip8::setKeyState(uint8_t key, uint8_t state)
{
    keys[key] = state; // set the state of the key
    //EM_ASM({ appendLog("Key Press Detected: Key " + $0 + " set to " + $1.toString()); }, key, state);
}

void Chip8::loadROM(const uint8_t *romData, size_t size)
{
    if (size > 3584)
    {
        //EM_ASM({appendLog("ERROR: ROM is too large!!");});
        return;
    }

    for (size_t i = 0; i < size; i++)
    {
        memory[0x200 + i] = romData[i]; // Load ROM into memory starting at 0x200
    }
    // Log successful load, passing size as an argument.
    //EM_ASM_({ appendLog("Loaded ROM successfully (" + $0.toString() + " bytes)"); }, size);
}

void Chip8::emulateCycle()
{
    uint16_t opcode = (memory[PC] << 8) | memory[PC + 1];
    // Log fetched opcode. We convert the opcode to hex in JS.
    //EM_ASM_({var hexOpcode = ("0000" + $0.toString(16)).slice(-4);appendLog("Fetched opcode: 0x" + hexOpcode); }, opcode);

    executeOpcode(opcode);

    if (delayTimer > 0)
    {
        delayTimer--;
    }

    if (soundTimer > 0)
    {
        soundTimer--;
    }
}

void Chip8::executeOpcode(uint16_t opcode)
{
    switch (opcode & 0xF000)
    {            // Extracts the first hex nibble (for switch case)
    case 0x0000: // 0 instructions
        if (opcode == 0x00E0)
        {
            display.fill(0); // Set all pixels to 0 (off)
            //EM_ASM({appendLog("Executed: Clear Screen (0x00E0)");});
        }
        else if (opcode == 0x00EE)
        { // 0x00EE - return from subroutine
            if (SP == 0)
            {
                //EM_ASM({appendLog("ERROR: Stack underflow on 0x00EE!!!")});
                return;
            }
            else
            {
                PC = stack[SP - 1]; // move the pointer
                SP--;               // decrement stack pointer
                //EM_ASM_({var hexAddr = ("000" + $0.toString(16)).slice(-3);
                //appendLog("Executed: Return from subroutine (0x00EE), jumping to 0x" + hexAddr); }, PC);
                // PC explicity set, exiting early to avoid PC increment
                return;
            }
        }
        PC += 2;
        break;
    case 0x1000:
    { // 0x1NNN - Set the program counter to NNN (Jump)
        uint16_t NNN = opcode & 0x0FFF;
        PC = NNN;
        //EM_ASM_({
                //var hexAddr = ("000" + $0.toString(16)).slice(-3);
                //appendLog("Executed: Jump to address 0x" + hexAddr); }, NNN);
    }
    break;
    case 0x2000:
    {                                   // 0x2NNN - Call subroutine at NNN
        uint16_t NNN = opcode & 0x0FFF; // extract NNN from opcode
        if (SP >= stack.size())
        {
            //EM_ASM({appendLog("ERROR: Stack overflow on 0x2NNN!!!")});
        }
        else
        {
            stack[SP] = PC + 2; // push return address to the stack
            SP++;               // increment stack pointer
            PC = NNN;           // set the PC to NNN to jump to the subroutine
            //EM_ASM_({var hexAddr = ("000" + $0.toString(16)).slice(-3);
            //appendLog("Executed: Call subroutine at 0x" + hexAddr); }, NNN);
            break;
        }
        return; // return early since PC is explictly set
    }
    case 0x3000:
    {                                       // 0x3XNN - Skip next instruction if V[X] == NN
        uint8_t X = (opcode & 0x0F00) >> 8; // extract X
        uint8_t NN = opcode & 0x00FF;       // extract NN

        if (V[X] == NN)
        {
            PC += 4; // skip the next instruction (2 bytes for current instruction + 2 for next)
            //EM_ASM_({ appendLog("Executed: Skip next instruction because V" + $0.toString() + " equals 0x" + ($1.toString(16).toUpperCase().padStart(2, "0"))); }, X, NN);
        }
        else
        {
            PC += 2; // otherwise proceed as normal
            //EM_ASM_({ appendLog("Executed: No skip because V" + $0.toString() + " does not equal 0x" + ($1.toString(16).toUpperCase().padStart(2, "0"))); }, X, NN);
        }
        break;
    }
    case 0x4000:
    { // 0x4XNN - Skip next instruction if V[X] != NN
        uint8_t X = (opcode & 0x0F00) >> 8;
        uint8_t NN = opcode & 0x00FF;

        if (V[X] != NN)
        {
            PC += 4; // skip the next instruction (2 bytes for current and 2 bytes for next)
            //EM_ASM_({ appendLog("Executed: Skip next instruction because V" + $0.toString() + " not equals 0x" + ($1.toString(16).toUpperCase().padStart(2, "0"))); }, X, NN);
        }
        else
        {
            PC += 2; // proceed as normal
            //EM_ASM_({ appendLog("Executed: No skip because V" + $0.toString() + " equals 0x" + ($1.toString(16).toUpperCase().padStart(2, "0"))); }, X, NN);
        }
        break;
    }
    case 0x5000:
    {                                       // 0x5XY0 - Skip next instruction if V[X] == V[Y] and opcode ends in 0
        uint8_t X = (opcode & 0x0F00) >> 8; // extract X
        uint8_t Y = (opcode & 0x00F0) >> 4; // extract Y
        uint8_t END = opcode & 0x0000F;

        if ((V[X] == V[Y]) && END == 0)
        {
            PC += 4; // skip next instruction (2 bytes for current + 2 for next)
            //EM_ASM_({ appendLog("Executed: Skip because V[" + $0.toString() + "]" + " equals V[" + $1.toString() + "]" + " and opcode ended in " + $2.toString()); }, X, Y, END);
        }
        else
        {
            PC += 2; // proceed as normal
            //EM_ASM_({ appendLog("Executed: No skip because V[" + $0.toString() + "]" + " does not equal V[" + $1.toString() + "]" + " or opcode did not end in " + $2.toString()); }, X, Y, END);
        }
        break;
    }
    case 0x6000:
    {                                       // 0x6XNN - Set VX to NN
        uint8_t X = (opcode & 0x0F00) >> 8; // Extract X
        uint8_t NN = opcode & 0x00FF;       // Extract NN
        V[X] = NN;
        //EM_ASM_({ appendLog("Executed: Set V" + $0.toString() + " = 0x" + ($1.toString(16).toUpperCase().padStart(2, "0"))); }, X, NN);
        PC += 2;
        break;
    }
    case 0x7000:
    {                                       // 0x7XNN - Add NN to VX
        uint8_t X = (opcode & 0x0F00) >> 8; // Extract X
        uint8_t NN = opcode & 0x00FF;
        V[X] += NN; // Add NN to VX (no carry flag modification)
        //EM_ASM_({ appendLog("Executed: V" + $0.toString() + " += 0x" + ($1.toString(16).toUpperCase().padStart(2, "0")) +
                            //" (New V" + $0.toString() + " = 0x" + ($2.toString(16).toUpperCase().padStart(2, "0")) + ")"); }, X, NN, V[X]);
        PC += 2;
        break;
    }
    case 0x8000:
    {
        switch (opcode & 0x000F) // switching on the 0x8XY_ opcodes (distinguished by last nibble)
        {
        case 0x0000: // 0x8XY0 - Set V[X] = V[Y]
        {
            uint8_t X = (opcode & 0x0F00) >> 8; // extract X
            uint8_t Y = (opcode & 0x00F0) >> 4; // extract Y
            V[X] = V[Y];                        // setting the value in register Y to register X
            //EM_ASM_({ appendLog("Executed: V[" + $0.toString() + "] = V[" + $1.toString() + "] (" + "0x" + $2.toString(16).toUpperCase().padStart(2, "0") + " = " + "0x" + $3.toString(16).toUpperCase().padStart(2, "0") + ")"); }, X, Y, V[X], V[Y]);
            PC += 2;
            break;
        }
        case 0x0001: // 0x8XY1 - Set V[X] = V[X] | V[Y]
        {
            uint8_t X = (opcode & 0x0F00) >> 8; // extract X
            uint8_t Y = (opcode & 0x00F0) >> 4; // extract Y
            uint8_t oldVX = V[X];               // store original V[X] for logging
            V[X] = V[X] | V[Y];                 // perform bitwise OR (combine the bits)
            //EM_ASM_({ appendLog("Executed: V[" + $0.toString() + "] |= V[" + $1.toString() +
                               //"] (0x" + $2.toString(16).toUpperCase().padStart(2, "0") +
                                //" |= 0x" + $3.toString(16).toUpperCase().padStart(2, "0") +
                                //" => 0x" + $4.toString(16).toUpperCase().padStart(2, "0") + ")"); }, X, Y, oldVX, V[Y], V[X]);
            PC += 2;
            break;
        }
        case 0x0002: // 0x8XY2 - Set V[X] = V[X] & V[Y]
        {
            uint8_t X = (opcode & 0x0F00) >> 8; // extract X
            uint8_t Y = (opcode & 0x00F0) >> 4; // extract Y
            uint8_t oldVX = V[X];
            V[X] = V[X] & V[Y];
            //EM_ASM_({ appendLog("Executed: V[" + $0.toString() + "] &= V[" + $1.toString() +
                                //"] (0x" + $2.toString(16).toUpperCase().padStart(2, "0") +
                                //" &= 0x" + $3.toString(16).toUpperCase().padStart(2, "0") +
                                //" => 0x" + $4.toString(16).toUpperCase().padStart(2, "0") + ")"); }, X, Y, oldVX, V[Y], V[X]);
            PC += 2;
            break;
        }
        case 0x0003: // 0x8XY3 - Set V[X] = V[X] ^ V[Y]
        {
            uint8_t X = (opcode & 0x0F00) >> 8; // extract X
            uint8_t Y = (opcode & 0x00F0) >> 4; // extract Y
            uint8_t oldVX = V[X];
            V[X] = V[X] ^ V[Y];
            //EM_ASM_({ appendLog("Executed: V[" + $0.toString() + "] ^= V[" + $1.toString() + "] (0x" +
                                //$2.toString(16).toUpperCase().padStart(2, '0') + " ^= 0x" +
                                //$3.toString(16).toUpperCase().padStart(2, '0') + " => 0x" +
                                //$4.toString(16).toUpperCase().padStart(2, '0') + ")"); }, X, Y, oldVX, V[Y], V[X]);
            PC += 2;
            break;
        }
        case 0x0004: // 0x8XY4 - Perform V[X] = V[X] + V[Y]
        {
            uint8_t X = (opcode & 0x0F00) >> 8; // extract X
            uint8_t Y = (opcode & 0x00F0) >> 4; // extract Y
            uint8_t oldVX = V[X];               // store old V[X] for logging

            uint16_t sum = V[X] + V[Y];

            if (sum > 0xFF)
            {
                V[0xF] = 0x1; // if sum is over 0xFF (255) we store 1 in the carry flag V[0xF]
            }
            else
            {
                V[0xF] = 0x0; // else we simply store 0
            }
            V[X] = (sum & 0xFF); // store the lower 8 bits in V[X]
            //EM_ASM_({ appendLog("Executed: V[" + $0.toString() + "] = V[" + $0.toString() + "] + V[" + $1.toString() +
                                //"] (0x" + $2.toString(16).toUpperCase().padStart(2, "0") +
                                //" + 0x" + $3.toString(16).toUpperCase().padStart(2, "0") +
                                //" = 0x" + $4.toString(16).toUpperCase().padStart(3, "0") +
                                //", carry=" + $5.toString() +
                                //") => new V[" + $0.toString() + "] = 0x" +
                                //$6.toString(16).toUpperCase().padStart(2, "0")); }, X, Y, oldVX, V[Y], sum, V[0xF], V[X]);
            PC += 2;
            break;
        }
        case 0x0005: // 0x8XY5 - Perform V[X] = V[X] - V[Y]
        {
            uint8_t X = (opcode & 0x0F00) >> 8; // extract X
            uint8_t Y = (opcode & 0x00F0) >> 4; // extract Y
            uint8_t oldVX = V[X];               // store old V[X] for logging

            if (V[X] >= V[Y]) // if V[X] is >= V[Y] set carry flag (V[F]) to 1 else 0
            {
                V[0xF] = 0x1;
            }
            else
            {
                V[0xF] = 0x0;
            }
            V[X] = V[X] - V[Y]; // perform the subtraction
            //EM_ASM_({ appendLog("Executed: V[" + $0.toString() + "] -= V[" + $1.toString() +
                                //"] (0x" + $2.toString(16).toUpperCase().padStart(2, '0') +
                                //" - 0x" + $3.toString(16).toUpperCase().padStart(2, '0') +
                                //" = 0x" + $4.toString(16).toUpperCase().padStart(2, '0') +
                                //", VF=" + $5.toString() + ")"); }, X, Y, oldVX, V[Y], V[X], V[0xF]);
            PC += 2;
            break;
        }
        case 0x0006: // store LSB in V[F] and shift V[X] right by one
        {
            uint8_t X = (opcode & 0x0F00) >> 8;
            uint8_t oldVX = V[X];
            V[0xF] = (V[X] & 0x01); // store least significant bit in V[F]
            V[X] = V[X] >> 1;       // shift V[X] right by 1
            //EM_ASM_({ appendLog("Executed: V[" + $0.toString() + "] >> 1 (0x" +
                                //$1.toString(16).toUpperCase().padStart(2, '0') + " >> 1 = 0x" +
                                //$2.toString(16).toUpperCase().padStart(2, '0') +
                                //", LSB = " + $3.toString() + ")"); }, X, oldVX, V[X], V[0xF]);
            PC += 2;
            break;
        }
        case 0x0007: // perform V[X] = V[Y] - V[X], setting V[F] accordingly
        {
            uint8_t X = (opcode & 0x0F00) >> 8;
            uint8_t Y = (opcode & 0x00F0) >> 4;
            uint8_t oldVX = V[X]; // store old V[X] and V[Y] for logging
            uint8_t oldVY = V[Y];
            if (V[Y] >= V[X])
            {
                V[0xF] = 0x1; // no borrow occured, set V[F] to 1
            }
            else
            {
                V[0xF] = 0x0; // borrow occured, set V[F] to 0
            }
            V[X] = V[Y] - V[X]; // perform the substraction
            //EM_ASM_({ appendLog("Executed: V[" + $0.toString() + "] = V[" + $1.toString() +
                                //"] - V[" + $0.toString() + "] (0x" +
                                //$2.toString(16).toUpperCase().padStart(2, '0') +
                                //" - 0x" + $3.toString(16).toUpperCase().padStart(2, '0') +
                                //" = 0x" + $4.toString(16).toUpperCase().padStart(2, '0') +
                                //"), VF = " + $5.toString()); }, X, Y, oldVY, oldVX, V[X], V[0xF]);
            PC += 2;
            break;
        }
        case 0x000E: // store MSB in V[F] and left shift V[X]
        {
            uint8_t X = (opcode & 0x0F00) >> 8;
            uint8_t oldVX = V[X];
            V[0xF] = (V[X] & 0x80) >> 7; // in a 8 bit (e.g. 10000000) value the MSB is in the 0x80 position (i.e. performing this results in 00000001 if V[X] was 10000000)
            V[X] = V[X] << 1;            // left shift V[X] by 1
            //EM_ASM_({ appendLog("Executed: V[" + $0.toString() + "] << 1 (0x" +
                                //$1.toString(16).toUpperCase().padStart(2, '0') +
                                //" << 1 = 0x" + $2.toString(16).toUpperCase().padStart(2, '0') +
                                //", MSB = " + $3.toString() + ")"); }, X, oldVX, V[X], V[0xF]);
            PC += 2;
            break;
        }
        default:
        {
            PC += 2;
        }
        }
        break;
    }
    case 0x9000: // skip next instruction if V[X] != V[Y]
    {
        uint8_t X = (opcode & 0x0F00) >> 8;
        uint8_t Y = (opcode & 0x00F0) >> 4;
        if (V[X] != V[Y])
        {
            PC += 4;
            //EM_ASM_({ appendLog("Executed: Skip next instruction because V[" + $0.toString() + "] (0x" +
                                //$1.toString(16).toUpperCase().padStart(2, "0") + ") != V[" + $2.toString() +
                                //"] (0x" + $3.toString(16).toUpperCase().padStart(2, "0") + ")"); }, X, V[X], Y, V[Y]);
        }
        else
        {
            //EM_ASM_({ appendLog("Executed: No skip because V[" + $0.toString() + "] (0x" +
                                //$1.toString(16).toUpperCase().padStart(2, "0") + ") == V[" + $2.toString() +
                                //"] (0x" + $3.toString(16).toUpperCase().padStart(2, "0") + ")"); }, X, V[X], Y, V[Y]);
            PC += 2;
        }
        break;
    }
    case 0xA000:
    {                                   // 0xANNN - Set index register I
        uint16_t NNN = opcode & 0x0FFF; // Extract NNN (12-bit address)
        I = NNN;
        //EM_ASM_({
                //var hexI = ("000" + $0.toString(16)).slice(-3);
                //appendLog("Executed: Set I = 0x" + hexI); }, NNN);
        PC += 2;
        break;
    }
    case 0xB000:
    {                                      // 0xBNNN - set PC to NNN + V[0]
        uint16_t NNN = opcode & 0x0FFF;    // extract NNN
        uint16_t jumpAddress = NNN + V[0]; // Calculate jump address
        //EM_ASM_({
        //var hexAddr = ("000" + $0.toString(16)).slice(-3);
        //appendLog("Executed: Jump to address 0x" + hexAddr + " (NNN + V[0])"); }, jumpAddress);
        PC = jumpAddress; // Set PC to NNN + V[0]
        break;
    }
    case 0xC000:
    { // 0xCXNN - Set V[X] = rand() & NN
        uint8_t X = (opcode & 0x0F00) >> 8;
        uint8_t NN = opcode & 0x00FF;
        uint8_t rnd = rand() % 256; // generate random 8-bit value (0 to 255)
        V[X] = rnd & NN;            // performn bitwise AND with NN and store in V[X]
        //EM_ASM_({ appendLog("Executed: V[" + $0.toString() + "] = Random(0x" +
                            //$1.toString(16).toUpperCase().padStart(2, '0') +
                            //") & 0x" + $2.toString(16).toUpperCase().padStart(2, '0') +
                            //" = 0x" + $3.toString(16).toUpperCase().padStart(2, '0')); }, X, rnd, NN, V[X]);
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
                uint8_t pixel = (spriteByte >> (7 - col)) & 1; // Extract individual pixel
                int drawX = (xPos + col) % 64;
                int drawY = (yPos + row) % 32;
                int screenIndex = drawY * 64 + drawX;
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
        //EM_ASM_({ appendLog("Executed: Draw sprite at (V" + $0.toString() + ", V" + $1.toString() + ") with height " + $2.toString()); }, X, Y, N);
        PC += 2;
        break;
    }
    case 0xE000:
    {
        uint8_t X = (opcode & 0x0F00) >> 8;
        switch (opcode & 0x00FF)
        {
        case 0x009E:
        { // 0xEX9E - Skip next instruction if V[X] is pressed
            if (keys[V[X]] != 0)
            {
                //EM_ASM_({ appendLog("Executed: Skip next instruction because key for V[" + $0.toString() +
                                    //"] (key value: 0x" + $1.toString(16).toUpperCase().padStart(1, '0') + ") is pressed."); }, X, V[X]);
                PC += 4;
            }
            else
            {
                //EM_ASM_({ appendLog("Executed: No skip because key for V[" + $0.toString() +
                                    //"] (key value: 0x" + $1.toString(16).toUpperCase().padStart(1, '0') + ") is not pressed."); }, X, V[X]);
                PC += 2;
            }
            break;
        }
        case 0x00A1:
        { // 0x8XA1 - Skip next instruction if V[X] is not pressed
            if (keys[V[X]] == 0)
            {
                //EM_ASM_({ appendLog("Executed: Skip next instruction because key for V[" + $0.toString() +
                                    //"] (key value: 0x" + $1.toString(16).toUpperCase().padStart(1, '0') +
                                    //") is not pressed."); }, X, V[X]);

                PC += 4;
            }
            else
            {
                //EM_ASM_({ appendLog("Executed: No skip because key for V[" + $0.toString() +
                                    //"] (key value: 0x" + $1.toString(16).toUpperCase().padStart(1, '0') +
                                    //") is pressed."); }, X, V[X]);
                PC += 2;
            }
            break;
        }
        default:
        {
            PC += 2;
            break;
        }
        }
        break;
    }
    case 0xF000:
    {
        switch (opcode & 0x00FF)
        {
        case 0x0007:
        { // 0xFX07 - Set V[X] to current value of delay timer
            uint8_t X = (opcode & 0x0F00) >> 8;
            V[X] = delayTimer;
            //EM_ASM_({ appendLog("Executed: V[" + $0.toString() + "] = delayTimer (0x" +
                                //$1.toString(16).toUpperCase().padStart(2, '0') + ")"); }, X, delayTimer);
            PC += 2;
            break;
        }
        case 0x000A:
        { // 0xFX0A - Wait for key press, store the key in V[X]
            bool keyPressed = false;
            uint8_t X = (opcode & 0x0F00) >> 8;
            for (uint8_t i = 0; i < keys.size(); i++)
            {
                if (keys[i] != 0)
                {
                    V[X] = i; // key press detected, storing key index in V[X]
                    keyPressed = true;
                    //EM_ASM_({ appendLog("Executed: Key press detected - key 0x" +
                                        //$0.toString(16).toUpperCase() +
                                        //" stored in V[" + $1.toString() + "]"); }, i, X);
                    break;
                }
            }
            if (!keyPressed)
            {
                return; // don't increment PC, causing a retry of this opcode
            }
            PC += 2;
            break;
        }
        case 0x0015:
        { // 0xFX15 - Set delay timer to V[X]
            uint8_t X = (opcode & 0x0F00) >> 8;
            delayTimer = V[X];
            //EM_ASM_({ appendLog("Executed: delayTimer = V[" + $0.toString() + "] (0x" +
                                //$1.toString(16).toUpperCase().padStart(2, '0') + ")"); }, X, V[X]);
            PC += 2;
            break;
        }
        case 0x0018:
        { // 0xFX18 - Set sound timer to V[X]
            uint8_t X = (opcode & 0x0F00) >> 8;
            soundTimer = V[X];
            //EM_ASM_({ appendLog("Executed: soundTimer = V[" + $0.toString() + "] (0x" +
                                //$1.toString(16).toUpperCase().padStart(2, '0') + ")"); }, X, V[X]);
            PC += 2;
            break;
        }
        case 0x001E:
        { // 0xFX1E - Add V[X] to I
            uint8_t X = (opcode & 0x0F00) >> 8;
            uint16_t oldI = I;
            I += V[X]; // perform addition
            //EM_ASM_({ appendLog("Executed: I = I + V[" + $0.toString() +
                                //"] (0x" + $1.toString(16).toUpperCase().padStart(3, '0') +
                                //" + 0x" + $2.toString(16).toUpperCase().padStart(2, '0') +
                                //" = 0x" + $3.toString(16).toUpperCase().padStart(3, '0') + ")"); }, X, oldI, V[X], I);
            PC += 2;
            break;
        }
        case 0x0029:
        { // 0xFX29 - Set I to the location of the sprite for the digit in V[X]
            uint8_t X = (opcode & 0x0F00) >> 8;
            uint8_t digit = V[X];
            I = FONT_START_ADDRESS + (digit * 5); // each sprite is 5 bytes
            //EM_ASM_({ appendLog("Executed: I = FONT_START_ADDRESS + (V[" + $0.toString() +
                                //"] * 5) (0x" + $1.toString(16).toUpperCase().padStart(3, '0') +
                                //" + (0x" + $2.toString(16).toUpperCase().padStart(2, '0') +
                                //" * 5) = 0x" + $3.toString(16).toUpperCase().padStart(3, '0') + ")"); }, X, FONT_START_ADDRESS, digit, I);
            PC += 2;
            break;
        }
        case 0x0033:
        { // 0xFX33 - Store BCD of V[X] in memory at I, I + 1, I + 2
            uint8_t X = (opcode & 0x0F00) >> 8;
            uint8_t value = V[X];
            memory[I] = value / 100;           // hundred digit
            memory[I + 1] = (value / 10) % 10; // tens digit
            memory[I + 2] = value % 10;        // ones digit
            //EM_ASM_({ appendLog("Executed: BCD of V[" + $0.toString() +
                                //"] (0x" + $1.toString(16).toUpperCase().padStart(2, '0') +
                                //") stored at memory[I..I+2] as: hundreds=0x" +
                                //$2.toString(16).toUpperCase().padStart(2, '0') +
                                //", tens=0x" + $3.toString(16).toUpperCase().padStart(2, '0') +
                                //", ones=0x" + $4.toString(16).toUpperCase().padStart(2, '0')); }, X, value, memory[I], memory[I + 1], memory[I + 2]);
            PC += 2;
            break;
        }
        case 0x0055:
        { // 0xFX55 - Store registers V0 to VX in memory
            uint8_t X = (opcode & 0x0F00) >> 8;
            for (uint8_t i = 0; i <= X; i++)
            {
                memory[I + i] = V[i];
            }
            //EM_ASM_({ appendLog("Executed: Loaded registers V0 to V[" + $0.toString() + "] from memory starting at I (0x" +
                                //$1.toString(16).toUpperCase().padStart(3, '0') + ")"); }, X, I);
            PC += 2;
            break;
        }
        case 0x0065:
        { // 0xFX65 - Load registers V0 to VX from memory
            uint8_t X = (opcode & 0x0F00) >> 8;
            for (uint8_t i = 0; i <= X; i++)
            {
                V[i] = memory[I + i];
            }
            //EM_ASM_({ appendLog("Executed: Loaded registers V0 to V[" + $0.toString() + "] from memory starting at I (0x" +
                                //$1.toString(16).toUpperCase().padStart(3, '0') + ")"); }, X, I);
            PC += 2;
            break;
        }
        default:
        {
            PC += 2;
            break;
        }
        }
        break;
    }
    default:
        //EM_ASM({
            //appendLog("Executed: Unknown opcode encountered.");
        //});
        PC += 2;
        break;
    }
}

uint8_t *Chip8::getDisplayBuffer()
{
    return display.data(); // Return pointer to display array
}
