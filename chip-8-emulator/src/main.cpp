#include <iostream>
#include <cstdint>
#include "../includes/chip8.h"
#include <emscripten.h>

Chip8 chip8;


extern "C" { // Expose to JS
    EMSCRIPTEN_KEEPALIVE void loadROM(uint8_t* data, size_t size) {
        chip8.loadROM(data, size);
    }

    EMSCRIPTEN_KEEPALIVE void emulateCycle() {
        chip8.emulateCycle();
    }

    EMSCRIPTEN_KEEPALIVE uint8_t* getDisplay() {
        return chip8.getDisplayBuffer();
    }

    EMSCRIPTEN_KEEPALIVE void reset() {
        chip8.reset();
    }
}
