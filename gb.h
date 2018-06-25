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
  int yMax, xMax; // Terminal dimensions
  std::set<u16> breakpoints;
  bool runToBreak; // Run till breakpoint
  void debug();

  // Disassembler
  struct disassembly {
    u16 pc;
    std::string str;
    u8 operandSize;
    u16 operand;
  };
  std::vector<disassembly> disasm;
  void disassemble(); // Populate disasm
  int getDisasmIndex(u16 pc); // Finds element with given PC

  // Prints disassembly, registers, etc
  void display();
  int cursorPos;   // By disasm index
  bool cursorToPC; // Snap cursor to last executed instruction

 
};

#endif
