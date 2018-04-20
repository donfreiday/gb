#include "cpu.h"
#include "mmu.h"

CPU::CPU() {
  reset();
}

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

    //LD b n
    case 0x06:
    printf("LD b %02X\n", mmu.read_byte(reg.pc));
    reg.b = mmu.read_byte(reg.pc);
    reg.pc += 1;
    cycles += 8;
    break;


    // LD c n
    case 0x0E:
    printf("LD c %02X\n", mmu.read_byte(reg.pc));
    reg.c = mmu.read_byte(reg.pc);
    reg.pc += 1;
    cycles += 8;
    break;

    // LD l n
    case 0x2E:
    printf("LD l %02X", mmu.read_byte(reg.pc));
    reg.l = mmu.read_byte(reg.pc);
    reg.pc += 1;
    cycles += 8;
    break;

    // LD hl nn
    case 0x21:
    printf("LD hl %04X\n", mmu.read_word(reg.pc));
    reg.hl = mmu.read_word(reg.pc);
    reg.pc += 2;
    cycles += 12;
    break;

    // LDD (hl) a
    // Save a to address pointed to by hl and decrement hl
    case 0x32:
    printf("LDD (hl) a\n");
    mmu.write_byte(reg.hl--, reg.a);
    printf("%02X\n", mmu.read_byte(++reg.hl));
    cycles += 8;
    break;

    // XOR a
    /* Compares each bit of its first operand to the corresponding bit of its second operand.
    If one bit is 0 and the other bit is 1, the corresponding result bit is set to 1.
    Otherwise, the corresponding result bit is set to 0.*/
    case 0xAF:
    printf("XOR a\n");
    reg.a^=reg.a;
    reg.f = 0;
    if(reg.a==0) {
      reg.f |= 0x80;
    }
    cycles+=4;
    break;

    // JP nn
    case 0xC3:
    printf("JP %04X\n", mmu.read_word(reg.pc));
    reg.pc = mmu.read_word(reg.pc);
    cycles += 16;
    break;

    default:
    printf("Unimplemented opcode: %02X\n", op);
  }
}
