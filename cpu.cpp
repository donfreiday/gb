#include "cpu.h"
#include "mmu.h"

CPU::CPU() {
  reset();
}

void CPU::reset() {
  reg.a = 0x00;
  reg.b = 0x00;
  reg.c = 0x00;
  reg.d = 0x00;
  reg.e = 0x00;
  reg.h = 0x00;
  reg.l = 0x00;
  reg.f = 0x00;
  reg.pc = 0x100;
  reg.sp = 0x00;
  reg.m = 0x00;
  reg.t = 0x00;
}

void CPU::execute(u8 op) {
  switch(op) {
    // nop
    case 0x00:
    printf("NOP\n");
    break;

    // jp nn
    case 0xC3:
    printf("jp %02X%02X\n", memory[reg.pc+1], memory[reg.pc]);
    reg.pc = memory[reg.pc];
    printf("pc:%04X\n", reg.pc);
    break;

    default:
    printf("Unimplemented opcode: %02X\n", op);
  }
}
