#ifndef GB_CORE
#define GB_CORE

#include <SDL2/SDL.h>
#include <ncurses.h>
#include <stdio.h>
#include <algorithm>
#include <set>
#include <vector>
#include "common.h"
#include "cpu.h"
#include "gpu.h"
#include "joypad.h"
#include "debugger.h"

class gb {
 public:
  gb();
  ~gb();

  bool loadROM(char* filename);
  void run();

  bool debugEnabled;

 private:
  void handleSDLKeydown(SDL_Keycode key);
  void handleSDLKeyup(SDL_Keycode key);
  void step();

  CPU cpu;
  GPU gpu;
  Joypad joypad;
  Debugger debugger;
};

#endif
