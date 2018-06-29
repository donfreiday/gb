# gb: a Gameboy Emulator
# Author: Don Freiday

# OBJS: files to compile as part of the project
OBJS = common.h joypad.h joypad.cpp mmu.h mmu.cpp gpu.h gpu.cpp cpu.h cpu.cpp gb.h gb.cpp

# CC: compiler we're using
CC = g++

# COMPILER_FLAGS =
COMPILER_FLAGS = -g -Wall

LINKER_FLAGS = -lSDL2 -lGL -lncursesw

# OBJ_NAME: name of our executable
OBJ_NAME = gb

# This is the target that compiles our executable
all : $(OBS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
