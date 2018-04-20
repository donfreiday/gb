#include "cpu.h"
#include "mmu.h"

CPU::CPU() {
  reset();
}

//
void CPU::reset() {
  reg.a = 0x01;
  reg.b = 0x00;
  reg.c = 0x13;
  reg.d = 0x00;
  reg.e = 0xD8;
  reg.h = 0x01;
  reg.l = 0x4D;
  reg.f = 0xB0;
  reg.pc = 0x100;
  reg.sp = 0xFFFE;
  clock_m = 0;
  clock_t = 0;
  cycles = 0;
}

void CPU::execute(u8 op) {
  switch(op) {
    // NOP
    case 0x00:
    printf("NOP\n");
    cycles += 4;
    break;

    // JP nn
    case 0xC3:
    printf("JP %04X\n", mmu.read_word(reg.pc));
    reg.pc = mmu.read_word(reg.pc);
    reg.pc += 2;
    cycles += 16;
    break;



    default:
    printf("Unimplemented opcode: %02X\n", op);
  }
}
