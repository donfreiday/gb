# gb: a Gameboy Emulator
# Author: Don Freiday

# OBJS: files to compile as part of the project
native: OBJS = ./imgui/*cpp joypad.cpp mmu.cpp gpu.cpp cpu.cpp main.cpp
js: OBJS = ./imgui/*cpp joypad.cpp mmu.cpp gpu.cpp cpu.cpp main.cpp

# CC: compiler we're using
native: CC = clang++
js: CC = em++

# COMPILER_FLAGS =
native: COMPILER_FLAGS = -g -Wall `sdl2-config --cflags` -I ./
js: COMPILER_FLAGS = --shell-file emscripten/shell.html --preload-file roms -s USE_SDL=2 --emrun -I ./

native: LINKER_FLAGS = `sdl2-config --libs` -lGL

# OBJ_NAME: name of our executable
native: OBJ_NAME = gb
js: OBJ_NAME = ./emscripten/gb.html

# This is the target that compiles our executable
native : $(OBS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)

js: $(OBS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) -o $(OBJ_NAME)