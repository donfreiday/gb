#ifndef GB_CORE
#define GB_CORE

#include <SDL2/SDL.h>
#include <stdio.h>
#include <set>
#include "common.h"
#include "cpu.h"
#include "gpu.h"
#include "joypad.h"
#include <ncurses.h>
#include <vector>

class gb {
 public:
  gb();
  ~gb();

  bool loadROM();
  void run();
  
  
  bool debugEnabled;

 private:
  void handleSDLKeydown(SDL_Keycode key);
  void step();

  CPU cpu;
  GPU gpu;
  Joypad joypad;

  // Debugger
  std::set<u16> breakpoints;
  int rows, cols; // Cursor position and terminal size
  u16 disasmStartAddr;
  void debug();
  void disassemble();
  void display();
};

#endif
