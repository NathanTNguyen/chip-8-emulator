EMCC=emcc
SRC=src/main.cpp src/chip8.cpp
OUT=chip8.js
CXXFLAGS=-s EXPORTED_FUNCTIONS='["_loadROM", "_emulateCycle", "_getDisplay", "_malloc", "_free"]' -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "getValue", "setValue"]' -s USE_SDL=2 --preload-file roms

all:
	$(EMCC) $(SRC) -o $(OUT) $(CXXFLAGS)

clean:
	del /Q chip8.js chip8.wasm chip8.data 2>nul || exit 0
