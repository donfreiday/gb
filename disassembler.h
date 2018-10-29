// gb: a Gameboy Emulator by Don Freiday
// File: disassembler.h
// Description: Disassembler
// NOTE: Assumes HLE bootrom

#ifndef GB_DISASM
#define GB_DISASM

#include "common.h"
#include "cpu.h"

#include <string>

struct disassembly {
  std::string str;
  u16 operand;
};

class Disassembler {
 public:
  Disassembler(CPU* Cpu);

  disassembly disassemble(u16& pc);

 private:
  CPU* cpu;
};

#endif