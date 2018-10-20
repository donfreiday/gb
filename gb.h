// gb: a Gameboy Emulator by Don Freiday
// File: gb.h
// Description: Emulator core
//
// Links the CPU, GPU, MMU, APU, Joypad, and Debugger
// Handles SDL input events and operates emulator components

#ifndef GB_CORE
#define GB_CORE

#include <SDL2/SDL.h>
#include <stdio.h>

#include "common.h"
#include "cpu.h"
#include "gpu.h"
#include "joypad.h"

class gb {
 public:
  gb();

  bool loadROM(char* filename);

  void step();

  CPU cpu;
  GPU gpu;
  Joypad joypad;

  void handleSDLKeydown(SDL_Keycode key);
  void handleSDLKeyup(SDL_Keycode key);
};

#endif
