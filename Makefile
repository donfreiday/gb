# gb: a Gameboy Emulator
# Author: Don Freiday

# OBJS: files to compile as part of the project
OBJS = joypad.cpp mmu.cpp gpu.cpp cpu.cpp gb.cpp debugger.cpp

# CC: compiler we're using
CC = clang++

# COMPILER_FLAGS =
COMPILER_FLAGS = -g -Wall

LINKER_FLAGS = -lSDL2 -lGL -lncursesw

# OBJ_NAME: name of our executable
OBJ_NAME = gb

# This is the target that compiles our executable
all : $(OBS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
