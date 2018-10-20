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

  void disassembleFrom(u16 pc, bool cleanStart);

  struct Line {
    u16 pc;
    u8 opcode;
    u16 operand;
    std::string str;

    // Operator overload for set ordering
    bool operator<(const Line& rhs) const { return pc < rhs.pc; }
  };

  std::set<Line> disassembly;

 private:
  CPU* cpu;

  std::set<u16> entryPoints;

  // Disassemble from known code entry points, following jumps where possible
  void initDisasm();
};

#endif