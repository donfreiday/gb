// gb: a Gameboy Emulator by Don Freiday
// File: disassembler.cpp
// Description: Disassembler

#include "disassembler.h"

Disassembler::Disassembler(CPU* Cpu) { cpu = Cpu; }

// This is a hack
std::string Disassembler::disassemble(u16& pc) {
  std::string result;
  u8 opcode = cpu->mmu.memory[pc];
  u8 operandSize = 0;
  u16 operand;

  if (opcode == 0xCB) {
    operandSize = 1;
    operand = cpu->mmu.memory[++pc];
    result = cpu->instructions_CB[operand].disassembly;
  } else if (cpu->instructions[opcode].operandLength == 1) {
    operandSize = 1;
    operand = cpu->mmu.memory[++pc];
    result = cpu->instructions[opcode].disassembly;
  } else if (cpu->instructions[opcode].operandLength == 2) {
    operandSize = 2;
    operand = cpu->mmu.read_u16(++pc);
    result = cpu->instructions[opcode].disassembly;
    pc++;
  } else {
    ++pc;
    operandSize = 0;
    result = cpu->instructions[opcode].disassembly;
  }

  pc += operandSize;
  return result;
}