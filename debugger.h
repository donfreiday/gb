// gb: a Gameboy Emulator by Don Freiday
// File: debugger.h
// Description: Interactive debugger implemented using ncurses

#ifndef GB_DEBUG
#define GB_DEBUG

#include "common.h"
#include "cpu.h"
#include "gpu.h"
#include "mmu.h"

#include "imgui/imgui.h"
#include <SDL2/SDL.h>


#include <set>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iomanip>

class Debugger {
 public:
  Debugger();
  ~Debugger();
  void init(CPU* cpuPtr, GPU* gpuPtr);
  void run();
  bool runToBreak;

 private:
  std::ofstream fout;

  CPU* cpu;
  GPU* gpu;

  std::set<u16> breakpoints;

  void display();

  // Disassembler
  struct disassembly {
    u16 pc;
    std::string str;
    u8 operandSize;
    u16 operand;
    bool operator==(const u16& rhs) const { return pc == rhs; }
  };

  std::vector<disassembly> disasm;  // todo: use a better suited data structure
  void disassemble(u16 initPC);     // Populate disasm
  int getDisasmIndex(u16 pc);

  void step();
};

#endif