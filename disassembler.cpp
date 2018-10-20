// gb: a Gameboy Emulator by Don Freiday
// File: disassembler.cpp
// Description: Disassembler
// NOTE: Assumes HLE bootrom

#include "disassembler.h"

Disassembler::Disassembler(CPU* Cpu) {
  cpu = Cpu;

  // Code entry points, including interrupts
  entryPoints = {0x100, 0x40, 0x48, 0x50, 0x58, 0x60};
  initDisasm();
}

// This is a hack, it blindly disassembles all of memory
void Disassembler::initDisasm() {
  disassembly.clear();
  u16 pc = 0;
  while (pc < cpu->mmu.memory.size()) {  // hack: disassemble entire memory map
    Line line;
    line.pc = pc;
    u8 operandSize = 0;
    u16 operand;
    u8 op = cpu->mmu.memory[pc];
    if (op == 0xCB) {
      operandSize = 0;
      operand = cpu->mmu.memory[++pc];
      line.str = cpu->instructions_CB[operand].disassembly;
    } else if (cpu->instructions[op].operandLength == 1) {
      operandSize = 1;
      operand = cpu->mmu.memory[++pc];
      line.str = cpu->instructions[op].disassembly;
    } else if (cpu->instructions[op].operandLength == 2) {
      operandSize = 2;
      operand = cpu->mmu.read_u16(++pc);
      line.str = cpu->instructions[op].disassembly;
      pc++;
    } else {
      operandSize = 0;
      line.str = cpu->instructions[op].disassembly;
    }
    pc++;
    disassembly.insert(line);
  }
}

/*void Debugger::step() {
  cpu->checkInterrupts();
  cpu->execute();
  gpu->step(cpu->cpu_clock_t);
  if (DUMP_TO_FILE) {
    fout << setfill('0') << setw(4) << hex << cpu->reg.pc+1 << ": "
         << setfill('0') << setw(4) << " af=" << cpu->reg.af << setfill('0')
         << setw(4) << " bc=" << cpu->reg.bc << setfill('0') << setw(4)
         << " de=" << cpu->reg.de << setfill('0') << setw(4)
         << " hl=" << cpu->reg.hl << setfill('0') << setw(4)
         << " sp=" << cpu->reg.sp << endl;
  }
}*/