/* gb: a Gameboy Emulator
   Author: Don Freiday */

#include "cpu.h"
#include "mmu.h"

#define FLAG_CARRY \
  4  // set if a carry occurred from the last arithmetic operation or if
     // register A is the smaller value when executing the CP instruction
#define FLAG_HALF_CARRY \
  5  // set if a carry occurred from the lower nibble in the last math
     // operation.
#define FLAG_SUBTRACT \
  6  // set if a subtraction was performed in the last math instruction.
#define FLAG_ZERO \
  7  // set when the result of an arithmetic operation is zero or two values
     // match when using the CP

CPU::CPU() { reset(); }

void CPU::reset() {
  /* Set all registers to zero before executing boot ROM.
        Real GB hardware behavior is undefined; the boot ROM explicitly
        sets each register as needed.*/
  /*reg.a = 0x01;
  reg.b = 0x00;
  reg.c = 0x13;
  reg.d = 0x00;
  reg.e = 0xD8;
  reg.h = 0x01;
  reg.l = 0x4D;
  reg.f = 0x0D; // todo: check this
  reg.pc = 0x00; // 0x100 is end of 256byte ROM header
  reg.sp = 0xFFFE;*/
  reg.a = 0;
  reg.b = 0;
  reg.c = 0;
  reg.d = 0;
  reg.e = 0;
  reg.h = 0;
  reg.l = 0;
  reg.f = 0;
  reg.pc = 0x000;
  reg.sp = 0;
  cpu_clock_m = 0;
  cpu_clock_t = 0;
  cycles = 0;
  ime = false;
  mmu.reset();
  mmu.write_u8(IF, 0xE1);
  // mmu.write_u8(LCD_CTL, 0x91);
  eiDelay = false;
  debug = false;
  debugVerbose = false;
}

template <typename t>
void CPU::decrementReg(t &reg1) {
  reg1--;
  bool carry = bitTest(reg.f, FLAG_CARRY);
  reg.f = 0;
  bitSet(reg.f, FLAG_SUBTRACT);
  if (carry) {
    bitSet(reg.f, FLAG_CARRY);
  }
  if (reg1 == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
  if ((reg1 & 0xF) == 0xF) {
    bitSet(reg.f, FLAG_HALF_CARRY);
  }
}

template <typename t>
void CPU::incrementReg(t &reg1) {
  reg1++;
  bool carry = bitTest(reg.f, FLAG_CARRY);
  reg.f = 0;
  if (carry) {
    bitSet(reg.f, FLAG_CARRY);
  }
  if (reg1 == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
  if ((reg1 & 0xF) == 0xF) {
    bitSet(reg.f, FLAG_HALF_CARRY);
  }
}

template <typename t>
void CPU::rotateRightCarry(t &reg1) {
  reg.f = 0;
  // Low bit of register is shifted into carry flag
  if (bitTest(reg1, 1)) {
    bitSet(reg.f, FLAG_CARRY);
  }
  reg1 >>= 1;  // Shift register right, high bit becomes 0
  if (bitTest(reg.f, FLAG_CARRY)) {
    bitSet(reg1, 7);  // Carry flag shifts into high bit of register
  }
  if (reg1 == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

template <typename t>
void CPU::rotateRight(t &reg1) {
  bool prevCarry = bitTest(reg.f, FLAG_CARRY);
  reg.f = 0;
  bool carry = bitTest(reg1, 0);
  reg1 >>= 1;
  reg1 += (prevCarry << 7);
  if (carry) {
    bitSet(reg.f, FLAG_CARRY);
  }
  if (reg1 == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

template <typename t>
void CPU::rotateLeftCarry(t &reg1) {
  reg.f = 0;
  // High bit is shifted into carry flag
  if (bitTest(reg1, 7)) {
    bitSet(reg.f, FLAG_CARRY);
  }
  reg.a <<= 1;  // Shift register left, low bit becomes 0
  if (bitTest(reg.f, FLAG_CARRY)) {
    reg1++;
  }
  if (reg1 == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

template <typename t>
void CPU::rotateLeft(t &reg1) {
  bool prevCarry = bitTest(reg.f, FLAG_CARRY);
  reg.f = 0;
  bool carry = bitTest(reg1, 7);
  reg1 <<= 1;
  reg1 += prevCarry;
  if (carry) {
    bitSet(reg.f, FLAG_CARRY);
  }
  if (reg1 == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

template <typename t>
void CPU::subtract(t num) {
  reg.f = 0;
  if (reg.a < num) {
    bitSet(reg.f, FLAG_CARRY);
  }
  if ((reg.a & 0xF) < (num & 0xF)) {
    bitSet(reg.f, FLAG_HALF_CARRY);
  }
  bitSet(reg.f, FLAG_SUBTRACT);
  reg.a -= num;
  if (reg.a == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

// Add n to A.
template <typename t>
void CPU::add(t n) {
  reg.f = 0;
  if ((reg.a + n) > 0xFF) {
    bitSet(reg.f, FLAG_CARRY);
  }
  if ((reg.a & 0xF) + (n & 0xF) > 0xF) {
    bitSet(reg.f, FLAG_HALF_CARRY);
  }
  bitClear(reg.f, FLAG_SUBTRACT);
  reg.a += n;
  if (reg.a == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

// Add n to reg1.
template <typename t>
void CPU::add(t &reg1, t n) {
  reg.f = 0;
  if ((reg1 + n) > 0xFF) {
    bitSet(reg.f, FLAG_CARRY);
  }
  if ((reg1 & 0xF) + (n & 0xF) > 0xF) {
    bitSet(reg.f, FLAG_HALF_CARRY);
  }
  bitClear(reg.f, FLAG_SUBTRACT);
  reg1 += n;
  if (reg1 == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

// Compare A with n. This is basically an A - n subtraction instruction but the
// results are thrown away.
template <typename t>
void CPU::compare(t num) {
  reg.f = 0;
  if (reg.a < num) {
    bitSet(reg.f, FLAG_CARRY);
  }
  if ((reg.a & 0xF) < (num & 0xF)) {
    bitSet(reg.f, FLAG_HALF_CARRY);
  }
  bitSet(reg.f, FLAG_SUBTRACT);
  if (reg.a - num == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

void CPU::bitTestReg(u8 reg1, u8 pos) {
  bool carry = bitTest(reg.f, FLAG_CARRY);
  reg.f = 0;
  if (carry) {
    bitSet(reg.f, FLAG_CARRY);
  }
  bitSet(reg.f, FLAG_HALF_CARRY);
  bitClear(reg.f, FLAG_SUBTRACT);
  if (!bitTest(reg1, pos)) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

// Logical OR n with register A, result in A.
template <typename t>
void CPU::orReg(t reg1) {
  reg.a |= reg1;
  reg.f = 0;
  if (reg.a == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

// Logically AND n with A, result in A.
template <typename t>
void CPU::andReg(t reg1) {
  reg.a &= reg1;
  reg.f = 0;
  if (reg.a == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
  bitSet(reg.f, FLAG_HALF_CARRY);
}

// Swap upper & lower nibles of n.
template <typename t>
void CPU::swapReg(t &reg1) {
  t lowerNibble = reg1 >> 4;
  reg1 <<= 4;
  reg1 |= lowerNibble;
  reg.f = 0;
  if (reg1 == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

// Logical exclusive OR n with register A, result in A.
template <typename t>
void CPU::xorReg(t reg1) {
  reg.a ^= reg1;
  reg.f = 0;
  if (reg.a == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

bool CPU::execute() {
  // Fetch the opcode from MMU and increment PC
  u8 op = mmu.read_u8(reg.pc++);
  // Parse operand and print disassembly of instruction
  u16 operand;
  if (instructions[op].operandLength == 1) {
    operand = mmu.read_u8(reg.pc);
  } else if (instructions[op].operandLength == 2) {
    operand = mmu.read_u16(reg.pc);
  }

  // Adjust PC
  reg.pc += instructions[op].operandLength;

  // Update cpu clock
  cpu_clock_t = instructions[op].cycles;

  // Go go go!
  switch (op) {
    // NOP
    case 0x00:
      break;

    // LD BC, nnnn
    case 0x01:
      reg.bc = operand;
      break;

    // LD (BC), A
    case 0x02:
      mmu.write_u8(reg.bc, reg.a);
      break;

    // INC BC
    case 0x03:
      incrementReg(reg.bc);
      break;

    // INC B
    case 0x04:
      incrementReg(reg.b);
      break;

    // DEC B
    case 0x05:
      decrementReg(reg.b);
      break;

    // LD B, nn
    case 0x06:
      reg.b = operand;
      break;

    // RLCA
    case 0x07:
      rotateLeftCarry(operand);
      break;

    // LD nnnn, SP
    case 0x08:
      mmu.write_u16(operand, reg.sp);
      break;

    // ADD HL, BC
    case 0x09:
      add(reg.hl, reg.bc);
      break;

    // LD A, (BC)
    case 0x0A:
      reg.a = mmu.read_u8(reg.bc);
      break;

    // DEC BC
    case 0x0B:
      decrementReg(reg.bc);
      break;

    // INC C
    case 0x0C:
      incrementReg(reg.c);
      break;

    // DEC C
    case 0x0D:
      decrementReg(reg.c);
      break;

    // LD C, nn
    case 0x0E:
      reg.c = operand;
      break;

    // RRC A
    // Performs a RRC A faster and modifies the flags differently.
    case 0x0F:
      rotateRightCarry(reg.a);
      bitClear(reg.f, FLAG_ZERO);
      break;

    // STOP
    case 0x10:
      // todo: GBC speed modes
      break;

    // LD DE, nnnn
    case 0x11:
      reg.de = operand;
      break;

    // LD (DE), A
    case 0x12:
      mmu.write_u8(reg.de, reg.a);
      break;

    // INC DE
    case 0x13:
      incrementReg(reg.de);
      break;

    // INC D
    case 0x14:
      incrementReg(reg.d);
      break;

    // DEC D
    case 0x15:
      decrementReg(reg.d);
      break;

    // LD D, nn
    case 0x16:
      reg.d = operand;
      break;

    // RL A
    case 0x17:
      rotateLeft(reg.a);
      break;

    // JR nn
    case 0x18:
      reg.pc += (s8)operand;
      break;

    // ADD HL, DE
    case 0x19:
      add(reg.hl, reg.de);
      break;

    // LD A, (DE)
    case 0x1A:
      reg.a = mmu.read_u8(reg.de);
      break;

    // DEC DE
    case 0x1B:
      decrementReg(reg.de);
      break;

    // INC E
    case 0x1C:
      incrementReg(reg.e);
      break;

    // DEC E
    case 0x1D:
      decrementReg(reg.e);
      break;

    // LD E, nn
    case 0x1E:
      reg.e = operand;
      break;

    // RRA
    case 0x1F:
      rotateRight(reg.a);
      break;

    // JR nz nn
    case 0x20:
      if (!bitTest(reg.f, FLAG_ZERO)) {
        reg.pc += (s8)(operand);
        cpu_clock_t += 4;
      }
      break;

    // LD hl, nnnn
    case 0x21:
      reg.hl = operand;
      break;

    // LDI (HL), A
    case 0x22:
      mmu.write_u8(reg.hl++, reg.a);
      break;

    // INC HL
    case 0x23:
      incrementReg(reg.hl);
      break;

    // INC H
    case 0x24:
      incrementReg(reg.h);
      break;

    // DEC H
    case 0x25:
      decrementReg(reg.h);
      break;

    // LD H, 0x%02X
    case 0x26:
      reg.h = operand;
      break;

    // DAA
    // BCD: todo: restudy
    case 0x27:
      printw("Unimplemented instruction: 0x27: DAA\n");
      return false;
      break;

    // JR Z, nn
    case 0x28:
      if (bitTest(reg.f, FLAG_ZERO)) {
        reg.pc += (s8)operand;
        cpu_clock_t += 4;
      }
      break;

    // ADD HL, HL
    case 0x29:
      add(reg.hl, reg.hl);
      break;

    // LDI A, (HL)
    case 0x2A:
      reg.a = mmu.read_u8(reg.hl++);
      break;

    // DEC HL
    case 0x2B:

      break;

    // INC L
    case 0x2C:
      incrementReg(reg.l);
      break;

    // DEC L
    case 0x2D:

      break;

    // LD L, nn
    case 0x2E:
      reg.l = operand;
      break;

    // CPL
    // Complement A register. (Flip all bits.)
    case 0x2F:
      reg.a = ~reg.a;
      bitSet(reg.f, FLAG_SUBTRACT);
      bitSet(reg.f, FLAG_HALF_CARRY);
      break;

    // JR NC, 0x%02X
    case 0x30:

      break;

    // LD SP, nnnn
    case 0x31:
      reg.sp = operand;
      break;

    // LDD (hl--), a
    case 0x32:
      mmu.write_u8(reg.hl--, reg.a);
      break;

    // INC SP
    case 0x33:

      break;

    // INC (HL)
    case 0x34: {
      u8 byte = mmu.read_u8(reg.hl);
      incrementReg(byte);
      mmu.write_u8(reg.hl, byte);
    } break;

    // DEC (HL)
    case 0x35: {
      u8 val = mmu.read_u8(reg.hl);
      decrementReg(val);
      mmu.write_u8(reg.hl, val);
    } break;

    // LD (HL), nn
    case 0x36:
      mmu.write_u8(reg.hl, operand);
      break;

    // SCF
    case 0x37:

      break;

    // JR C, 0x%02X
    case 0x38:

      break;

    // ADD HL, SP
    case 0x39:

      break;

    // LDD A, (HL)
    case 0x3A:

      break;

    // DEC SP
    case 0x3B:

      break;

    // INC A
    case 0x3C:
      incrementReg(reg.a);
      break;

    // DEC A
    case 0x3D:
      decrementReg(reg.a);
      break;

    // LD A, nn
    case 0x3E:
      reg.a = operand;
      break;

    // CCF
    case 0x3F:

      break;

    // LD B, B
    case 0x40:

      break;

    // LD B, C
    case 0x41:

      break;

    // LD B, D
    case 0x42:

      break;

    // LD B, E
    case 0x43:

      break;

    // LD B, H
    case 0x44:

      break;

    // LD B, L
    case 0x45:

      break;

    // LD B, (HL)
    case 0x46:

      break;

    // LD B, A
    case 0x47:
      reg.b = reg.a;
      break;

      // LD C, B
    case 0x48:
      reg.c = reg.b;
      break;

    // LD C, C
    case 0x49:
      // reg.c = reg.c
      break;

    // LD C, D
    case 0x4A:
      reg.c = reg.d;
      break;

    // LD C, E
    case 0x4B:
      reg.c = reg.e;
      break;

    // LD C, H
    case 0x4C:
      reg.c = reg.h;
      break;

    // LD C, L
    case 0x4D:
      reg.c = reg.l;
      break;

    // LD C, (HL)
    case 0x4E:
      reg.c = mmu.read_u8(reg.hl);
      break;

    // LD C, A
    case 0x4F:
      reg.c = reg.a;
      break;

    // LD D, B
    case 0x50:
      reg.d = reg.b;
      break;

    // LD D, C
    case 0x51:
      reg.d = reg.c;
      break;

    // LD D, D
    case 0x52:
      // reg.d = reg.d;
      break;

    // LD D, E
    case 0x53:
      reg.d = reg.e;
      break;

    // LD D, H
    case 0x54:
      reg.d = reg.h;
      break;

    // LD D, L
    case 0x55:
      reg.d = reg.l;
      break;

    // LD D, (HL)
    case 0x56:
      reg.d = mmu.read_u8(reg.hl);
      break;

    // LD D, A
    case 0x57:
      reg.d = reg.a;
      break;

    // LD E, B
    case 0x58:
      reg.e = reg.b;
      break;

    // LD E, C
    case 0x59:
      reg.e = reg.c;
      break;

    // LD E, D
    case 0x5A:
      reg.e = reg.d;
      break;

    // LD E, E
    case 0x5B:
      // reg.e = reg.e;
      break;

    // LD E, H
    case 0x5C:
      reg.e = reg.h;
      break;

    // LD E, L
    case 0x5D:
      reg.e = reg.l;
      break;

    // LD E, (HL)
    case 0x5E:
      reg.e = mmu.read_u8(reg.hl);
      break;

    // LD E, A
    case 0x5F:
      reg.e = reg.a;
      break;

    // LD H, B
    case 0x60:
      reg.h = reg.b;
      break;

    // LD H, C
    case 0x61:
      reg.h = reg.c;
      break;

    // LD H, D
    case 0x62:
      reg.h = reg.d;
      break;

    // LD H, E
    case 0x63:
      reg.h = reg.e;
      break;

    // LD H, H
    case 0x64:
      // reg.h = reg.h;
      break;

    // LD H, L
    case 0x65:
      reg.h = reg.l;
      break;

    // LD H, (HL)
    case 0x66:
      reg.h = mmu.read_u8(reg.hl);
      break;

    // LD H, A
    case 0x67:
      reg.h = reg.a;
      break;

    // LD L, B
    case 0x68:
      reg.l = reg.b;
      break;

    // LD L, C
    case 0x69:
      reg.l = reg.c;
      break;

    // LD L, D
    case 0x6A:
      reg.l = reg.d;
      break;

    // LD L, E
    case 0x6B:
      reg.l = reg.e;
      break;

    // LD L, H
    case 0x6C:
      reg.l = reg.h;
      break;

    // LD L, L
    case 0x6D:
      // reg.l = reg.l;
      break;

    // LD L, (HL)
    case 0x6E:
      reg.l = mmu.read_u8(reg.hl);
      break;

    // LD L, A
    case 0x6F:
      reg.l = reg.a;
      break;

    // LD (HL), B
    case 0x70:
      mmu.write_u8(reg.hl, reg.b);
      break;

    // LD (HL), C
    case 0x71:
      mmu.write_u8(reg.hl, reg.c);
      break;

    // LD (HL), D
    case 0x72:
      mmu.write_u8(reg.hl, reg.d);
      break;

    // LD (HL), E
    case 0x73:
      mmu.write_u8(reg.hl, reg.e);
      break;

    // LD (HL), H
    case 0x74:
      mmu.write_u8(reg.hl, reg.h);
      break;

    // LD (HL), L
    case 0x75:
      mmu.write_u8(reg.hl, reg.l);
      break;

    // HALT
    case 0x76:
      printw("Todo: 0x76 Halt\n");
      return false;
      break;

    // LD (HL), A
    case 0x77:
      mmu.write_u8(reg.hl, reg.a);
      break;

    // LD A, B
    case 0x78:
      reg.a = reg.b;
      break;

    // LD A, C
    case 0x79:
      reg.a = reg.c;
      break;

    // LD A, D
    case 0x7A:
      reg.a = reg.d;
      break;

    // LD A, E
    case 0x7B:
      reg.a = reg.e;
      break;

    // LD A, H
    case 0x7C:
      reg.a = reg.h;
      break;

    // LD A, L
    case 0x7D:
      reg.a = reg.l;
      break;

    // LD A, (HL)
    case 0x7E:
      reg.a = mmu.read_u8(reg.hl);
      break;

    // LD A, A
    case 0x7F:
      // reg.a = reg.a;
      break;

    // ADD A, B
    case 0x80:
      add(reg.b);
      break;

    // ADD A, C
    case 0x81:
      add(reg.c);
      break;

    // ADD A, D
    case 0x82:
      add(reg.d);
      break;

    // ADD A, E
    case 0x83:
      add(reg.e);
      break;

    // ADD A, H
    case 0x84:
      add(reg.h);
      break;

    // ADD A, L
    case 0x85:
      add(reg.l);
      break;

    // ADD A, (HL)
    case 0x86:
      add(mmu.read_u8(reg.hl));
      break;

    // ADD A
    case 0x87:
      add(reg.a);
      break;

    // ADC B
    case 0x88:
      
      break;

    // ADC C
    case 0x89:

      break;

    // ADC D
    case 0x8A:

      break;

    // ADC E
    case 0x8B:

      break;

    // ADC H
    case 0x8C:

      break;

    // ADC L
    case 0x8D:

      break;

    // ADC (HL)
    case 0x8E:

      break;

    // ADC A
    case 0x8F:

      break;

    // SUB B
    case 0x90:
      subtract(reg.b);
      break;

    // SUB C
    case 0x91:

      break;

    // SUB D
    case 0x92:

      break;

    // SUB E
    case 0x93:

      break;

    // SUB H
    case 0x94:

      break;

    // SUB L
    case 0x95:

      break;

    // SUB (HL)
    case 0x96:

      break;

    // SUB A
    case 0x97:

      break;

    // SBC B
    case 0x98:

      break;

    // SBC C
    case 0x99:

      break;

    // SBC D
    case 0x9A:

      break;

    // SBC E
    case 0x9B:

      break;

    // SBC H
    case 0x9C:

      break;

    // SBC L
    case 0x9D:

      break;

    // SBC (HL)
    case 0x9E:

      break;

    // SBC A
    case 0x9F:

      break;

    // AND B
    case 0xA0:

      break;

    // AND C
    case 0xA1:
      andReg(reg.c);
      break;

    // AND D
    case 0xA2:

      break;

    // AND E
    case 0xA3:

      break;

    // AND H
    case 0xA4:

      break;

    // AND L
    case 0xA5:

      break;

    // AND (HL)
    case 0xA6:

      break;

    // AND A
    case 0xA7:
      andReg(reg.a);
      break;

    // XOR B
    case 0xA8:

      break;

    // XOR C
    case 0xA9:
      xorReg(reg.c);
      break;

    // XOR D
    case 0xAA:

      break;

    // XOR E
    case 0xAB:

      break;

    // XOR H
    case 0xAC:

      break;

    // XOR L
    case 0xAD:

      break;

    // XOR (HL)
    case 0xAE:

      break;

    // XOR A
    /* Compares each bit of its first operand to the corresponding bit of its
    second operand. If one bit is 0 and the other bit is 1, the corresponding
    result bit is set to 1. Otherwise, the corresponding result bit is set to
    0.*/
    case 0xAF:
      xorReg(reg.a);
      break;

    // OR B
    case 0xB0:
      orReg(reg.b);
      break;

    // OR C
    case 0xB1:
      orReg(reg.c);
      break;

    // OR D
    case 0xB2:

      break;

    // OR E
    case 0xB3:

      break;

    // OR H
    case 0xB4:

      break;

    // OR L
    case 0xB5:

      break;

    // OR (HL)
    case 0xB6:

      break;

    // OR A
    case 0xB7:

      break;

    // CP B
    case 0xB8:

      break;

    // CP C
    case 0xB9:

      break;

    // CP D
    case 0xBA:

      break;

    // CP E
    case 0xBB:

      break;

    // CP H
    case 0xBC:

      break;

    // CP L
    case 0xBD:

      break;

    // CP (HL)
    case 0xBE:
      compare(mmu.read_u8(reg.hl));
      break;

    // CP A
    case 0xBF:

      break;

    // RET NZ
    case 0xC0:
      if (!bitTest(reg.f, FLAG_ZERO)) {
        reg.pc = mmu.read_u16(reg.sp);
        reg.sp += 2;
        cpu_clock_t += 12;
      }
      break;

    // POP BC
    case 0xC1:
      reg.bc = mmu.read_u16(reg.sp);
      reg.sp += 2;
      break;

    // JP NZ, 0x%04X
    case 0xC2:

      break;

    // JP nnnn
    case 0xC3:
      reg.pc = operand;
      break;

    // CALL NZ, 0x%04X
    case 0xC4:
      if (!bitTest(reg.f, FLAG_ZERO)) {
        cpu_clock_t += 12;
        reg.sp -= 2;
        mmu.write_u16(reg.sp, reg.pc);
        reg.pc = operand;
      }
      break;

    // PUSH BC
    case 0xC5:
      reg.sp -= 2;
      mmu.write_u16(reg.sp, reg.bc);
      break;

    // ADD A, 0x%02X
    case 0xC6:

      break;

    // RST 0x00
    case 0xC7:

      break;
    // RET Z
    case 0xC8:
      if (bitTest(reg.f, FLAG_ZERO)) {
        reg.pc = mmu.read_u16(reg.sp);
        reg.sp += 2;
        cpu_clock_t += 12;
      }
      break;

    // RET
    case 0xC9:
      reg.pc = mmu.read_u16(reg.sp);
      reg.sp += 2;
      break;

    // JP Z, 0x%04X
    case 0xCA:
      if (bitTest(reg.f, FLAG_ZERO)) {
        reg.pc = operand;
        cpu_clock_t += 4;
      }
      break;

    // CB is a prefix
    case 0xCB:
      if (!execute_CB(operand)) {
        clear();
        printw("^^^ Unimplemented instruction: %04X: %02X ^^^\n", --reg.pc, op);
        printw("Code stub:\n\n// %s\ncase 0x%02X:\n\nbreak;\n\n",
               instructions_CB[operand].disassembly, operand);
        refresh();
        return false;
      }
      cpu_clock_t = instructions_CB[operand].cycles;
      break;

    // CALL Z, 0x%04X
    case 0xCC:

      break;

    // CALL nnnn
    case 0xCD:
      reg.sp -= 2;
      mmu.write_u16(reg.sp, reg.pc);
      reg.pc = operand;
      break;

    // ADC 0x%02X
    case 0xCE:

      break;

    // RST 0x08
    case 0xCF:

      break;

    // RET NC
    case 0xD0:

      break;

    // POP DE
    case 0xD1:
      reg.de = mmu.read_u16(reg.sp);
      reg.sp += 2;
      break;

    // JP NC, 0x%04X
    case 0xD2:

      break;

    // UNKNOWN
    case 0xD3:

      break;

    // CALL NC, 0x%04X
    case 0xD4:

      break;

    // PUSH DE
    case 0xD5:
      reg.sp -= 2;
      mmu.write_u16(reg.sp, reg.de);
      break;

    // SUB 0x%02X
    case 0xD6:

      break;

    // RST 0x10
    case 0xD7:

      break;

    // RET C
    case 0xD8:

      break;

    // RETI
    case 0xD9:
      reg.pc = mmu.read_u16(reg.sp);
      reg.sp += 2;
      ime = true;
      break;

    // JP C, 0x%04X
    case 0xDA:

      break;

    // UNKNOWN
    case 0xDB:

      break;

    // CALL C, 0x%04X
    case 0xDC:

      break;

    // UNKNOWN
    case 0xDD:

      break;

    // SBC 0x%02X
    case 0xDE:

      break;

    // RST 0x18
    case 0xDF:

      break;

    // LDH (0xFF00 + nn), A
    case 0xE0:
      mmu.write_u8(0xFF00 + operand, reg.a);
      break;

    // POP HL
    case 0xE1:
      reg.hl = mmu.read_u16(reg.sp);
      reg.sp += 2;
      break;

    // LD (0xFF00 + C), A
    case 0xE2:
      mmu.write_u8((0xFF00 + reg.c), reg.a);
      break;

    // UNKNOWN
    case 0xE3:

      break;

    // UNKNOWN
    case 0xE4:

      break;

    // PUSH HL
    case 0xE5:
      reg.sp -= 2;
      mmu.write_u16(reg.sp, reg.hl);
      break;

    // AND nn
    case 0xE6:
      andReg(operand);
      break;

    // RST 0x20
    case 0xE7:

      break;

    // ADD SP,0x%02X
    case 0xE8:

      break;

    // JP HL
    case 0xE9:
      reg.pc = reg.hl;
      break;

    // LD (nnnn), A
    case 0xEA:
      mmu.write_u8(operand, reg.a);
      break;

    // UNKNOWN
    case 0xEB:

      break;

    // UNKNOWN
    case 0xEC:

      break;

    // UNKNOWN
    case 0xED:

      break;

    // XOR 0x%02X
    case 0xEE:

      break;

    // RST 0x28
    case 0xEF:
      reg.sp -= 2;
      mmu.write_u16(reg.sp, reg.pc);
      reg.pc = 0x28;
      break;

    // LDH A, (0xFF00 + nn)
    case 0xF0:
      reg.a = mmu.read_u8(0xFF00 + operand);
      break;

    // POP AF
    case 0xF1:
      reg.af = mmu.read_u16(reg.sp);
      reg.sp += 2;
      break;

    // LD A, (0xFF00 + C)
    case 0xF2:

      break;

    // DI
    case 0xF3:
      ime = false;
      break;

    // UNKNOWN
    case 0xF4:

      break;

    // PUSH AF
    case 0xF5:
      reg.sp -= 2;
      mmu.write_u16(reg.sp, reg.af);
      break;

    // OR 0x%02X
    case 0xF6:

      break;

    // RST 0x30
    case 0xF7:

      break;

    // LD HL, SP+0x%02X
    case 0xF8:

      break;

    // LD SP, HL
    case 0xF9:

      break;

    // LD A, (nnnn)
    case 0xFA:
      reg.a = mmu.read_u8(operand);
      break;

    // EI
    case 0xFB:
      ime = true;
      eiDelay = true;
      break;

    // UNKNOWN
    case 0xFC:

      break;

    // UNKNOWN
    case 0xFD:

      break;

    // CP nn
    // Implied subtraction (A - nn) and set flags
    case 0xFE:
      reg.f = 0;
      if (reg.a < operand) {
        bitSet(reg.f, FLAG_CARRY);
      }
      if ((reg.a & 0xF) < (operand & 0xF)) {
        bitSet(reg.f, FLAG_HALF_CARRY);
      }
      bitSet(reg.f, FLAG_SUBTRACT);
      if ((reg.a - operand) == 0) {
        bitSet(reg.f, FLAG_ZERO);
      }
      break;

    // RST 0x38
    case 0xFF:

      break;

    default:
      clear();
      printw("^^^ Unimplemented instruction: %04X: 0x%02X ^^^\n\n", --reg.pc,
             op);
      printw("Code stub:\n\n// %s\ncase 0x%02X:\n\nbreak;\n\n",
             instructions[op].disassembly, op);
      refresh();
      return false;
      break;
  }

  cpu_clock_m = cpu_clock_t / 4;
  cycles += cpu_clock_t;

  return true;
}

// extended instruction set via 0xCB prefix
bool CPU::execute_CB(u8 op) {
  switch (op) {
    // RLC B
    case 0x00:

      break;

    // RLC C
    case 0x01:

      break;

    // RLC D
    case 0x02:

      break;

    // RLC E
    case 0x03:

      break;

    // RLC H
    case 0x04:

      break;

    // RLC L
    case 0x05:

      break;

    // RLC (HL)
    case 0x06:

      break;

    // RLC A
    case 0x07:

      break;

    // RRC B
    case 0x08:

      break;

    // RRC C
    case 0x09:

      break;

    // RRC D
    case 0x0A:

      break;

    // RRC E
    case 0x0B:

      break;

    // RRC H
    case 0x0C:

      break;

    // RRC L
    case 0x0D:

      break;

    // RRC (HL)
    case 0x0E:

      break;

    // RRC A
    case 0x0F:

      break;

    // RL B
    case 0x10:

      break;

    // RL C
    case 0x11:
      rotateLeft(reg.c);
      break;

    // RL D
    case 0x12:

      break;

    // RL E
    case 0x13:

      break;

    // RL H
    case 0x14:

      break;

    // RL L
    case 0x15:

      break;

    // RL (HL)
    case 0x16:

      break;

    // RL A
    case 0x17:

      break;

    // RR B
    case 0x18:

      break;

    // RR C
    case 0x19:

      break;

    // RR D
    case 0x1A:

      break;

    // RR E
    case 0x1B:

      break;

    // RR H
    case 0x1C:

      break;

    // RR L
    case 0x1D:

      break;

    // RR (HL)
    case 0x1E:

      break;

    // RR A
    case 0x1F:

      break;

    // SLA B
    case 0x20:

      break;

    // SLA C
    case 0x21:

      break;

    // SLA D
    case 0x22:

      break;

    // SLA E
    case 0x23:

      break;

    // SLA H
    case 0x24:

      break;

    // SLA L
    case 0x25:

      break;

    // SLA (HL)
    case 0x26:

      break;

    // SLA A
    case 0x27:

      break;

    // SRA B
    case 0x28:

      break;

    // SRA C
    case 0x29:

      break;

    // SRA D
    case 0x2A:

      break;

    // SRA E
    case 0x2B:

      break;

    // SRA H
    case 0x2C:

      break;

    // SRA L
    case 0x2D:

      break;

    // SRA (HL)
    case 0x2E:

      break;

    // SRA A
    case 0x2F:

      break;

    // SWAP B
    case 0x30:

      break;

    // SWAP C
    case 0x31:

      break;

    // SWAP D
    case 0x32:

      break;

    // SWAP E
    case 0x33:

      break;

    // SWAP H
    case 0x34:

      break;

    // SWAP L
    case 0x35:

      break;

    // SWAP (HL)
    case 0x36:

      break;

    // SWAP A
    case 0x37:
      swapReg(reg.a);
      break;

    // SRL B
    case 0x38:

      break;

    // SRL C
    case 0x39:

      break;

    // SRL D
    case 0x3A:

      break;

    // SRL E
    case 0x3B:

      break;

    // SRL H
    case 0x3C:

      break;

    // SRL L
    case 0x3D:

      break;

    // SRL (HL)
    case 0x3E:

      break;

    // SRL A
    case 0x3F:

      break;

    // BIT 0, B
    case 0x40:

      break;

    // BIT 0, C
    case 0x41:

      break;

    // BIT 0, D
    case 0x42:

      break;

    // BIT 0, E
    case 0x43:

      break;

    // BIT 0, H
    case 0x44:

      break;

    // BIT 0, L
    case 0x45:

      break;

    // BIT 0, (HL)
    case 0x46:

      break;

    // BIT 0, A
    case 0x47:

      break;

    // BIT 1, B
    case 0x48:

      break;

    // BIT 1, C
    case 0x49:

      break;

    // BIT 1, D
    case 0x4A:

      break;

    // BIT 1, E
    case 0x4B:

      break;

    // BIT 1, H
    case 0x4C:

      break;

    // BIT 1, L
    case 0x4D:

      break;

    // BIT 1, (HL)
    case 0x4E:

      break;

    // BIT 1, A
    case 0x4F:

      break;

    // BIT 2, B
    case 0x50:

      break;

    // BIT 2, C
    case 0x51:

      break;

    // BIT 2, D
    case 0x52:

      break;

    // BIT 2, E
    case 0x53:

      break;

    // BIT 2, H
    case 0x54:

      break;

    // BIT 2, L
    case 0x55:

      break;

    // BIT 2, (HL)
    case 0x56:

      break;

    // BIT 2, A
    case 0x57:

      break;

    // BIT 3, B
    case 0x58:

      break;

    // BIT 3, C
    case 0x59:

      break;

    // BIT 3, D
    case 0x5A:

      break;

    // BIT 3, E
    case 0x5B:

      break;

    // BIT 3, H
    case 0x5C:

      break;

    // BIT 3, L
    case 0x5D:

      break;

    // BIT 3, (HL)
    case 0x5E:

      break;

    // BIT 3, A
    case 0x5F:

      break;

    // BIT 4, B
    case 0x60:

      break;

    // BIT 4, C
    case 0x61:

      break;

    // BIT 4, D
    case 0x62:

      break;

    // BIT 4, E
    case 0x63:

      break;

    // BIT 4, H
    case 0x64:

      break;

    // BIT 4, L
    case 0x65:

      break;

    // BIT 4, (HL)
    case 0x66:

      break;

    // BIT 4, A
    case 0x67:

      break;

    // BIT 5, B
    case 0x68:

      break;

    // BIT 5, C
    case 0x69:

      break;

    // BIT 5, D
    case 0x6A:

      break;

    // BIT 5, E
    case 0x6B:

      break;

    // BIT 5, H
    case 0x6C:

      break;

    // BIT 5, L
    case 0x6D:

      break;

    // BIT 5, (HL)
    case 0x6E:

      break;

    // BIT 5, A
    case 0x6F:

      break;

    // BIT 6, B
    case 0x70:

      break;

    // BIT 6, C
    case 0x71:

      break;

    // BIT 6, D
    case 0x72:

      break;

    // BIT 6, E
    case 0x73:

      break;

    // BIT 6, H
    case 0x74:

      break;

    // BIT 6, L
    case 0x75:

      break;

    // BIT 6, (HL)
    case 0x76:

      break;

    // BIT 6, A
    case 0x77:

      break;

    // BIT 7, B
    case 0x78:

      break;

    // BIT 7, C
    case 0x79:

      break;

    // BIT 7, D
    case 0x7A:

      break;

    // BIT 7, E
    case 0x7B:

      break;

    // BIT 7, H
    case 0x7C:
      bitTestReg(reg.h, 7);
      break;

    // BIT 7, L
    case 0x7D:

      break;

    // BIT 7, (HL)
    case 0x7E:

      break;

    // BIT 7, A
    case 0x7F:

      break;

    // RES 0, B
    case 0x80:

      break;

    // RES 0, C
    case 0x81:

      break;

    // RES 0, D
    case 0x82:

      break;

    // RES 0, E
    case 0x83:

      break;

    // RES 0, H
    case 0x84:

      break;

    // RES 0, L
    case 0x85:

      break;

    // RES 0, (HL)
    case 0x86:

      break;

    // RES 0, A
    case 0x87:
      bitClear(reg.a, 0);
      break;

    // RES 1, B
    case 0x88:

      break;

    // RES 1, C
    case 0x89:

      break;

    // RES 1, D
    case 0x8A:

      break;

    // RES 1, E
    case 0x8B:

      break;

    // RES 1, H
    case 0x8C:

      break;

    // RES 1, L
    case 0x8D:

      break;

    // RES 1, (HL)
    case 0x8E:

      break;

    // RES 1, A
    case 0x8F:

      break;

    // RES 2, B
    case 0x90:

      break;

    // RES 2, C
    case 0x91:

      break;

    // RES 2, D
    case 0x92:

      break;

    // RES 2, E
    case 0x93:

      break;

    // RES 2, H
    case 0x94:

      break;

    // RES 2, L
    case 0x95:

      break;

    // RES 2, (HL)
    case 0x96:

      break;

    // RES 2, A
    case 0x97:

      break;

    // RES 3, B
    case 0x98:

      break;

    // RES 3, C
    case 0x99:

      break;

    // RES 3, D
    case 0x9A:

      break;

    // RES 3, E
    case 0x9B:

      break;

    // RES 3, H
    case 0x9C:

      break;

    // RES 3, L
    case 0x9D:

      break;

    // RES 3, (HL)
    case 0x9E:

      break;

    // RES 3, A
    case 0x9F:

      break;

    // RES 4, B
    case 0xA0:

      break;

    // RES 4, C
    case 0xA1:

      break;

    // RES 4, D
    case 0xA2:

      break;

    // RES 4, E
    case 0xA3:

      break;

    // RES 4, H
    case 0xA4:

      break;

    // RES 4, L
    case 0xA5:

      break;

    // RES 4, (HL)
    case 0xA6:

      break;

    // RES 4, A
    case 0xA7:

      break;

    // RES 5, B
    case 0xA8:

      break;

    // RES 5, C
    case 0xA9:

      break;

    // RES 5, D
    case 0xAA:

      break;

    // RES 5, E
    case 0xAB:

      break;

    // RES 5, H
    case 0xAC:

      break;

    // RES 5, L
    case 0xAD:

      break;

    // RES 5, (HL)
    case 0xAE:

      break;

    // RES 5, A
    case 0xAF:

      break;

    // RES 6, B
    case 0xB0:

      break;

    // RES 6, C
    case 0xB1:

      break;

    // RES 6, D
    case 0xB2:

      break;

    // RES 6, E
    case 0xB3:

      break;

    // RES 6, H
    case 0xB4:

      break;

    // RES 6, L
    case 0xB5:

      break;

    // RES 6, (HL)
    case 0xB6:

      break;

    // RES 6, A
    case 0xB7:

      break;

    // RES 7, B
    case 0xB8:

      break;

    // RES 7, C
    case 0xB9:

      break;

    // RES 7, D
    case 0xBA:

      break;

    // RES 7, E
    case 0xBB:

      break;

    // RES 7, H
    case 0xBC:

      break;

    // RES 7, L
    case 0xBD:

      break;

    // RES 7, (HL)
    case 0xBE:

      break;

    // RES 7, A
    case 0xBF:

      break;

    // SET 0, B
    case 0xC0:

      break;

    // SET 0, C
    case 0xC1:

      break;

    // SET 0, D
    case 0xC2:

      break;

    // SET 0, E
    case 0xC3:

      break;

    // SET 0, H
    case 0xC4:

      break;

    // SET 0, L
    case 0xC5:

      break;

    // SET 0, (HL)
    case 0xC6:

      break;

    // SET 0, A
    case 0xC7:

      break;

    // SET 1, B
    case 0xC8:

      break;

    // SET 1, C
    case 0xC9:

      break;

    // SET 1, D
    case 0xCA:

      break;

    // SET 1, E
    case 0xCB:

      break;

    // SET 1, H
    case 0xCC:

      break;

    // SET 1, L
    case 0xCD:

      break;

    // SET 1, (HL)
    case 0xCE:

      break;

    // SET 1, A
    case 0xCF:

      break;

    // SET 2, B
    case 0xD0:

      break;

    // SET 2, C
    case 0xD1:

      break;

    // SET 2, D
    case 0xD2:

      break;

    // SET 2, E
    case 0xD3:

      break;

    // SET 2, H
    case 0xD4:

      break;

    // SET 2, L
    case 0xD5:

      break;

    // SET 2, (HL)
    case 0xD6:

      break;

    // SET 2, A
    case 0xD7:

      break;

    // SET 3, B
    case 0xD8:

      break;

    // SET 3, C
    case 0xD9:

      break;

    // SET 3, D
    case 0xDA:

      break;

    // SET 3, E
    case 0xDB:

      break;

    // SET 3, H
    case 0xDC:

      break;

    // SET 3, L
    case 0xDD:

      break;

    // SET 3, (HL)
    case 0xDE:

      break;

    // SET 3, A
    case 0xDF:

      break;

    // SET 4, B
    case 0xE0:

      break;

    // SET 4, C
    case 0xE1:

      break;

    // SET 4, D
    case 0xE2:

      break;

    // SET 4, E
    case 0xE3:

      break;

    // SET 4, H
    case 0xE4:

      break;

    // SET 4, L
    case 0xE5:

      break;

    // SET 4, (HL)
    case 0xE6:

      break;

    // SET 4, A
    case 0xE7:

      break;

    // SET 5, B
    case 0xE8:

      break;

    // SET 5, C
    case 0xE9:

      break;

    // SET 5, D
    case 0xEA:

      break;

    // SET 5, E
    case 0xEB:

      break;

    // SET 5, H
    case 0xEC:

      break;

    // SET 5, L
    case 0xED:

      break;

    // SET 5, (HL)
    case 0xEE:

      break;

    // SET 5, A
    case 0xEF:

      break;

    // SET 6, B
    case 0xF0:

      break;

    // SET 6, C
    case 0xF1:

      break;

    // SET 6, D
    case 0xF2:

      break;

    // SET 6, E
    case 0xF3:

      break;

    // SET 6, H
    case 0xF4:

      break;

    // SET 6, L
    case 0xF5:

      break;

    // SET 6, (HL)
    case 0xF6:

      break;

    // SET 6, A
    case 0xF7:

      break;

    // SET 7, B
    case 0xF8:

      break;

    // SET 7, C
    case 0xF9:

      break;

    // SET 7, D
    case 0xFA:

      break;

    // SET 7, E
    case 0xFB:

      break;

    // SET 7, H
    case 0xFC:

      break;

    // SET 7, L
    case 0xFD:

      break;

    // SET 7, (HL)
    case 0xFE:

      break;

    // SET 7, A
    case 0xFF:

      break;

    default:
      return false;
      break;
  }

  return true;
}

/* Check interrupts
Bit 0: V-Blank Interupt
Bit 1: LCD Interupt
Bit 2: Timer Interupt
Bit 4: Joypad Interupt
Interrupt request register: 0xFF0F
Interrupt enable register: 0xFFFF allows disabling/enabling specific registers
*/
void CPU::checkInterrupts() {
  if (!ime || eiDelay) {  // IME disabled or the last instruction was EI
    eiDelay = false;
    return;
  }
  u8 requested = mmu.read_u8(IF);
  if (requested > 0) {
    u8 enabled = mmu.read_u8(IE);
    for (u8 i = 0; i < 5; i++) {
      if (bitTest(requested, i)) {
        if (bitTest(enabled, i)) {
          doInterrupt(i);
        }
      }
    }
  }
}

void CPU::doInterrupt(u8 interrupt) {
  ime = false;  // IME = disabled

  // Reset bit in Interrupt Request Register
  u8 flags = mmu.read_u8(IF);
  bitClear(flags, interrupt);
  mmu.write_u8(IF, interrupt);

  reg.sp -= 2;
  mmu.write_u16(reg.sp, reg.pc);  // Push PC to the stack

  // Interrupts have specific service routines at defined memory locations
  switch (interrupt) {
    case 0:
      reg.pc = 0x40;
      break;

    case 1:
      reg.pc = 0x48;
      break;

    case 2:
      reg.pc = 0x50;
      break;

    case 4:
      reg.pc = 0x60;
      break;

    default:
      break;
  }
}
