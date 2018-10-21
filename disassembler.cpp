// gb: a Gameboy Emulator by Don Freiday
// File: disassembler.cpp
// Description: Disassembler
// NOTE: Assumes HLE bootrom

#include "disassembler.h"

Disassembler::Disassembler(CPU* Cpu) {
  cpu = Cpu;

  // Code entry points, including interrupt (todo: and maybe restart?) vectors
  entryPoints = {0x40, 0x48, 0x50, 0x58, 0x60, 0x100};
  initDisasm();
}

// This is a hack
void Disassembler::initDisasm() {
  disassembly.clear();
  disassembleFrom(0);
}

// This is a hack
void Disassembler::disassembleFrom(u16 pc) {
  while (pc < cpu->mmu.memory.size()) {
    Line line;
    line.pc = pc;
    u8 operandSize = 0;
    line.opcode = cpu->mmu.memory[pc];
    if (line.opcode == 0xCB) {
      operandSize = 0;
      line.operand = cpu->mmu.memory[++pc];
      line.str = cpu->instructions_CB[line.operand].disassembly;
    } else if (cpu->instructions[line.opcode].operandLength == 1) {
      operandSize = 1;
      line.operand = cpu->mmu.memory[++pc];
      line.str = cpu->instructions[line.opcode].disassembly;
    } else if (cpu->instructions[line.opcode].operandLength == 2) {
      operandSize = 2;
      line.operand = cpu->mmu.read_u16(++pc);
      line.str = cpu->instructions[line.opcode].disassembly;
      pc++;
    } else {
      operandSize = 0;
      line.str = cpu->instructions[line.opcode].disassembly;
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