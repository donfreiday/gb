# gb: a Gameboy Emulator
# Author: Don Freiday

# OBJS: files to compile as part of the project
OBJS = gb.cpp cpu.h cpu.cpp mmu.h mmu.cpp gpu.h gpu.cpp joypad.h joypad.cpp

# CC: compiler we're using
CC = g++

# COMPILER_FLAGS =
COMPILER_FLAGS = -g -Wall

LINKER_FLAGS = -lSDL2 -lGL

# OBJ_NAME: name of our executable
OBJ_NAME = gb

# This is the target that compiles our executable
all : $(OBS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
