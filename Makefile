# OBJS: files to compile as part of the project
OBJS = gb.cpp

# CC: compiler we're using
CC = g++

# COMPILER_FLAGS =

LINKER_FLAGS = -lSDL2

# OBJ_NAME: name of our executable
OBJ_NAME = gb

# This is the target that compiles our executable
all : $(OBS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
