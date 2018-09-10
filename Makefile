# gb: a Gameboy Emulator
# Author: Don Freiday

# OBJS: files to compile as part of the project
native: OBJS = joypad.cpp mmu.cpp gpu.cpp cpu.cpp gb.cpp debugger.cpp main.cpp
js: OBJS = joypad.cpp mmu.cpp gpu.cpp cpu.cpp gb.cpp main.cpp

# CC: compiler we're using
native: CC = clang++
js: CC = emcc

# COMPILER_FLAGS =
native: COMPILER_FLAGS = -g -Wall
js: COMPILER_FLAGS = --preload-file roms -s USE_SDL=2 --emrun

LINKER_FLAGS = -lSDL2 -lGL -lncursesw

# OBJ_NAME: name of our executable
native: OBJ_NAME = gb
js: OBJ_NAME = ./emscripten/gb.html

# This is the target that compiles our executable
native : $(OBS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)

js: $(OBS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) -o $(OBJ_NAME)