// gb: a Gameboy Emulator by Don Freiday
// File: disassembler.h
// Description: Disassembler
// NOTE: Assumes HLE bootrom

#ifndef GB_DISASM
#define GB_DISASM

#include "common.h"
#include "cpu.h"

#include <set>
#include <string>

class Disassembler {
 public:
  Disassembler(CPU* Cpu);

  void disassembleFrom(u16 pc);

  char* getDisasm(u16 pc);

 private:
  CPU* cpu;

  std::set<u16> entryPoints;

  // Disassemble from known code entry points, following jumps where possible
  void initDisasm();

  struct Line {
    u16 pc;
    std::string str;

    // Operator overload for set ordering
    bool operator<(const Line& rhs) const { return pc < rhs.pc; }
  };

  std::set<Line> disassembly;
};

#endif