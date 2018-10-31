// gb: a Gameboy Emulator by Don Freiday
// File: disassembler.cpp
// Description: Disassembler

#include "disassembler.h"

Disassembler::Disassembler(CPU* Cpu) { cpu = Cpu; }

// This is a hack
disassembly Disassembler::disassemble(u16& pc) {
  std::string str;
  u8 opcode = cpu->mmu.memory[pc];
  u8 operandSize = 0;
  u16 operand = 0;

  if (opcode == 0xCB) {
    operandSize = 1;
    operand = cpu->mmu.memory[++pc];
    str += cpu->instructions_CB[operand].disassembly;
  } else if (cpu->instructions[opcode].operandLength == 1) {
    operandSize = 1;
    operand = cpu->mmu.memory[++pc];
    str = cpu->instructions[opcode].disassembly;
  } else if (cpu->instructions[opcode].operandLength == 2) {
    operandSize = 2;
    operand = cpu->mmu.read_u16(++pc);
    str += cpu->instructions[opcode].disassembly;
    pc++;
  } else {
    ++pc;
    operandSize = 0;
    str += cpu->instructions[opcode].disassembly;
  }

  pc += operandSize;
  return { str, operand };
}