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
  int yMax, xMax;  // Terminal dimensions
  std::set<u16> breakpoints;
  bool runToBreak;  // Run till breakpoint
  void debug();

  // Disassembler
  struct disassembly {
    u16 pc;
    std::string str;
    u8 operandSize;
    u16 operand;
    bool operator==(const u16& rhs) const { return pc == rhs; }
  };
  std::vector<disassembly> disasm; // todo: use a better suited data structure
  void disassemble();          // Populate disasm
  int getDisasmIndex(u16 pc);  // Finds element with given PC

  // Prints disassembly, registers, etc
  void display();
  int cursorPos;  // By disasm index
  void cursorMove(int distance);
};

#endif
