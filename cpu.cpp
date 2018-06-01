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
  reg.pc = 0x00; // 0x100 is end of 256byte ROM header
  reg.sp = 0xFFFE;
  cpu_clock_m = 0;
  cpu_clock_t = 0;
  cycles = 0;
  interrupt = true;
}

void CPU::decrement_reg(u8 &reg1) {
  reg1--;
  if ((reg.f & 0x10) == 1) {
    reg.f |= 0x10; // carry flag
  }
  reg.f = 0;
  if (reg1==0) {
    reg.f |= 0x80; // zero flag
  }
  reg.f |= 0x40; // add/sub flag

  if((reg1 & 0xF) == 0xF) {
    reg.f |= 0x20;
  }
}

// Returns false for unimplemented opcodes
bool CPU::execute(u8 op) {
  switch(op) {
    // NOP
    case 0x00:
    printf("NOP\n");
    cpu_clock_m = 1;
    cpu_clock_t = 4;
    cycles += 4;
    break;

    // DEC b
    case 0x05:
    printf("DEC b\n");
    decrement_reg(reg.b);
    cpu_clock_m = 1;
    cpu_clock_t = 4;
    cycles += 4;
    break;

    //LD b n
    case 0x06:
    printf("LD b %02X\n", mmu.read_u8(reg.pc));
    reg.b = mmu.read_u8(reg.pc);
    reg.pc += 1;
    cpu_clock_m = 2;
    cpu_clock_t = 8;
    cycles += 8;
    break;

    // DEC c
    case 0x0D:
    printf("DEC c\n");
    decrement_reg(reg.c);
    cpu_clock_m = 1;
    cpu_clock_t = 4;
    cycles+=4;
    break;

    // LD c n
    case 0x0E:
    printf("LD c %02X\n", mmu.read_u8(reg.pc));
    reg.c = mmu.read_u8(reg.pc);
    reg.pc += 1;
    cpu_clock_m = 2;
    cpu_clock_t = 8;
    cycles += 8;
    break;

    // RRC A
    // Performs a RRC A faster and modifies the flags differently.
    case 0x0F:
    printf("RRC a\n");
    reg.f = 0;
    if (reg.a & 1) {
      reg.f |= 0x10; // Set carry flag
    }
    reg.a >>= 1; // Shift A right, high bit becomes 0
    reg.a += (reg.f & 0x10 << 7); // Carry flag becomes high bit of A
    if (reg.a == 0) {
      reg.f |= 0x80;
    }
    cpu_clock_m = 2;
    cpu_clock_t = 8;
    cycles += 8;
    return false;
    break;

    // JR nz n
    // Relative jump by signed immediate if last result was not zero (zero flag = 0)
    // todo: figure this out
    case 0x20:
    printf("JR nz %02X\n", mmu.read_u8(reg.pc));
    if (!(reg.f & 0x80)) {
      reg.pc+=(s8)(mmu.read_u8(reg.pc));
      cpu_clock_m = 3;
      cpu_clock_t = 12;
      cycles+=12;
    }
    else {
      cpu_clock_m = 2;
      cpu_clock_t = 8;
      cycles+=8;
    }
    reg.pc++;
    break;

    // LD l n
    case 0x2E:
    printf("LD l %02X\n", mmu.read_u8(reg.pc));
    reg.l = mmu.read_u8(reg.pc);
    reg.pc += 1;
    cpu_clock_m = 2;
    cpu_clock_t = 8;
    cycles += 8;
    break;

    // LD hl nn
    case 0x21:
    printf("LD hl %04X\n", mmu.read_u16(reg.pc));
    reg.hl = mmu.read_u16(reg.pc);
    reg.pc += 2;
    cpu_clock_m = 3;
    cpu_clock_t = 12;
    cycles += 12;
    break;

    // LD SP, nnnn
    case 0x31:
    printf("LD SP %04X\n", mmu.read_u16(reg.pc));
    reg.sp = mmu.read_u16(reg.pc);
    reg.pc += 2;
    break;

    // LDD (hl) a
    // Save a to address pointed to by hl and decrement hl
    case 0x32:
    printf("LDD (hl) a\n");
    mmu.write_u8(reg.hl--, reg.a);
    cpu_clock_m = 2;
    cpu_clock_t = 8;
    cycles += 8;
    break;

    // LD a n
    case 0x3E:
    printf("LD a %02X\n", mmu.read_u8(reg.pc));
    reg.a = mmu.read_u8(reg.pc++);
    cpu_clock_m = 2;
    cpu_clock_t = 8;
    cycles+=8;
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
    cpu_clock_m = 1;
    cpu_clock_t = 4;
    cycles += 4;
    break;

    // JP nn
    case 0xC3:
    printf("JP %04X\n", mmu.read_u16(reg.pc));
    reg.pc = mmu.read_u16(reg.pc);
    cpu_clock_m = 4;
    cpu_clock_t = 16;
    cycles += 16;
    break;

    // LDH n a
    // Write value in reg.a at address pointed to by 0xFF00+n
    case 0xE0:
    printf("LDH 0xFF00+%02X a\n",mmu.read_u8(reg.pc));
    mmu.write_u8(0xFF00+mmu.read_u8(reg.pc), reg.a);
    reg.pc++;
    cpu_clock_m = 3;
    cpu_clock_t = 12;
    cycles += 12;
    break;

    // LDH a n
    // Store value at 0xFF00+n in reg.a
    case 0xF0:
    printf("LDH %04X a\n", mmu.read_u8(0xFF00+mmu.read_u8(reg.pc)));
    reg.a = mmu.read_u8(mmu.read_u8(0xFF00+mmu.read_u8(reg.pc++)));
    cpu_clock_m = 3;
    cpu_clock_t = 12;
    cycles += 12;
    break;

    // DI
    // Disable interrupts
    case 0xF3:
    printf("DI\n");
    interrupt = false;
    cpu_clock_m = 3;
    cpu_clock_t = 12;
    cycles += 4;
    break;

    // CP a
    // CP is a subtraction from A that doesn't update A, only the flags it would have set/reset if it really was subtracted.
    case 0xFE:
    break;

    default:
    printf("Unimplemented opcode: %02X\n", op);
    return false;
    break;
  }
  return true;
}
