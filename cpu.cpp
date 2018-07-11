/* gb: a Gameboy Emulator
   Author: Don Freiday */

#include "cpu.h"
#include "mmu.h"

u16 prevPC[999];
u16 prevPcIndex = 0;

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

// Test bit b in register r
template <typename t>
void CPU::bit(u8 bit, t reg1) {
  u8 prevCarry = bitTest(reg.f, FLAG_CARRY);
  reg.f = 0;
  if (!bitTest(reg1, bit)) {
    bitSet(reg.f, FLAG_ZERO);
  }
  bitClear(reg.f, FLAG_SUBTRACT);
  bitSet(reg.f, FLAG_HALF_CARRY);
  if (prevCarry) {
    bitSet(reg.f, FLAG_CARRY);
  }
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
  if ((reg1 & 0xF) == 0) {
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

// Shift n left into Carry. LSB of n set to 0.
template <typename t>
void CPU::sla(t &reg1) {
  reg.f = 0;
  if (bitTest(reg1, 7)) {
    bitSet(reg.f, FLAG_CARRY);
  }
  reg1 <<= 1;
  if (reg1 == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

// Shift n right into Carry. MSB doesn't change.
template <typename t>
void CPU::sra(t &reg1) {
  reg.f = 0;
  u8 msb = bitTest(reg1, 7);
  if (bitTest(reg1, 0)) {
    bitSet(reg.f, FLAG_CARRY);
  }
  reg1 >>= 1;
  reg1 += (msb << 7);
  if (reg1 == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

// Shift n right into Carry. MSB set to 0.
template <typename t>
void CPU::srl(t &reg1) {
  reg.f = 0;
  if (bitTest(reg1, 0)) {
    bitSet(reg.f, FLAG_CARRY);
  }
  reg1 >>= 1;
  if (reg1 == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

template <typename t>
void CPU::subtract(t n) {
  reg.f = 0;
  if (reg.a < n) {
    bitSet(reg.f, FLAG_CARRY);
  }
  if ((reg.a & 0xF) < (n & 0xF)) {
    bitSet(reg.f, FLAG_HALF_CARRY);
  }
  bitSet(reg.f, FLAG_SUBTRACT);
  reg.a -= n;
  if (reg.a == 0) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

// Subtract n + Carry flag from A.
// todo: verify this
template <typename t>
void CPU::subtractCarry(t n) {
  u8 prevCarry = bitTest(reg.f, FLAG_CARRY);
  if (reg.a < n + prevCarry) {
    bitSet(reg.f, FLAG_CARRY);
  }
  if ((reg.a & 0x0F) < ((n + prevCarry) & 0x0F)) {
    bitSet(reg.f, FLAG_HALF_CARRY);
  }
  bitSet(reg.f, FLAG_SUBTRACT);
  reg.a -= (n + prevCarry);
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

// Add n to reg1.
template <typename t>
void CPU::add16(t &reg1, t n) {
  bool zero = bitTest(reg.f, FLAG_ZERO);
  reg.f = 0;
  if ((reg1 + n) > 0xFFFF) {
    bitSet(reg.f, FLAG_CARRY);
  }
  if ((reg1 & 0x0FFF) + (n & 0x0FFF) > 0x0FFF) {
    bitSet(reg.f, FLAG_HALF_CARRY);
  }
  bitClear(reg.f, FLAG_SUBTRACT);
  reg1 += n;
  if (zero) {
    bitSet(reg.f, FLAG_ZERO);
  }
}

// Add n + carry flag to A
// todo: is this correct? verify
template <typename t>
void CPU::addCarry(t n) {
  u8 prevCarry = bitTest(reg.f, FLAG_CARRY);
  reg.f = 0;

  if (reg.a + n + prevCarry > 0xFF) {
    bitSet(reg.f, FLAG_CARRY);
  }

  if ((reg.a & 0x0F) + (n & 0x0F) + prevCarry > 0x0F) {
    bitSet(reg.f, FLAG_HALF_CARRY);
  }

  reg.a += n + prevCarry;

  if (reg.a == 0) {
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
  if (prevPcIndex > 998) {
    prevPcIndex = 0;
  }
  prevPC[prevPcIndex++] = reg.pc;

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
    // Flags are not affected by 16bit increment
    case 0x03:
      reg.bc++;
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
      add16(reg.hl, reg.bc);
      break;

    // LD A, (BC)
    case 0x0A:
      reg.a = mmu.read_u8(reg.bc);
      break;

    // DEC BC
    // Flags are not affected by 16bit decrement
    case 0x0B:
      reg.bc--;
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
    // Flags are not affected by 16bit increment
    case 0x13:
      reg.de++;
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
      add16(reg.hl, reg.de);
      break;

    // LD A, (DE)
    case 0x1A:
      reg.a = mmu.read_u8(reg.de);
      break;

    // DEC DE
    // Flags are not affected by 16bit decrement
    case 0x1B:
      reg.de--;
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
    // Flags are not affected by 16bit increment
    case 0x23:
      reg.hl++;
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
    // This instruction adjusts register A so that the correct representation of
    // Binary Coded Decimal (BCD) is obtained.
    // Adapted from http://forums.nesdev.com/viewtopic.php?t=9088
    case 0x27: {
      int a = reg.a;

      if (!bitTest(reg.f, FLAG_SUBTRACT)) {
        if (bitTest(reg.f, FLAG_HALF_CARRY) || (a & 0xF) > 9) {
          a += 0x06;
        }
        if(bitTest(reg.f, FLAG_CARRY || a > 0x9F)) {
          a += 0x60;
        }
      } else {
        if (bitTest(reg.f, FLAG_HALF_CARRY)) {
          a = (a - 6) & 0xFF;
        }
        if(bitTest(reg.f, FLAG_CARRY)) {
          a -= 0x60;
        }
      }
      bitClear(reg.f, FLAG_HALF_CARRY);
      bitClear(reg.f, FLAG_ZERO);


      if ((a & 0x100) == 0x100) {
        bitSet(reg.f, FLAG_CARRY);
      }

      a &= 0xFF;

      if (a == 0) {
        bitSet(reg.f, FLAG_ZERO);
      }

      reg.a = (u8)a;
    }

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
      add16(reg.hl, reg.hl);
      break;

    // LDI A, (HL)
    case 0x2A:
      reg.a = mmu.read_u8(reg.hl++);
      break;

    // DEC HL
    // Flags are not affected by 16bit decrement
    case 0x2B:
      reg.hl--;
      break;

    // INC L
    case 0x2C:
      incrementReg(reg.l);
      break;

    // DEC L
    case 0x2D:
      decrementReg(reg.l);
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
      if (!bitTest(reg.f, FLAG_CARRY)) {
        cycles += 4;
        reg.pc += (s8)operand;
      }
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
    // Flags are not affected by 16bit increment
    case 0x33:
      reg.sp++;
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
      bitClear(reg.f, FLAG_SUBTRACT);
      bitClear(reg.f, FLAG_HALF_CARRY);
      bitSet(reg.f, FLAG_CARRY);
      break;

    // JR C, 0x%02X
    case 0x38:
      if (bitTest(reg.f, FLAG_CARRY)) {
        cycles += 4;
        reg.pc += (s8)operand;
      }
      break;

    // ADD HL, SP
    case 0x39:
      add16(reg.hl, reg.sp);
      break;

    // LDD A, (HL)
    case 0x3A:
      reg.a = mmu.read_u8(reg.hl--);
      break;

    // DEC SP
    // Flags are not affected by 16bit decrement
    case 0x3B:
      reg.sp--;
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
    // Complement carry flag
    case 0x3F:
      bitClear(reg.f, FLAG_SUBTRACT);
      bitClear(reg.f, FLAG_HALF_CARRY);
      if (bitTest(reg.f, FLAG_CARRY)) {
        bitClear(reg.f, FLAG_CARRY);
      } else {
        bitSet(reg.f, FLAG_CARRY);
      }
      break;

    // LD B, B
    case 0x40:
      // reg.b = reg.b;
      break;

    // LD B, C
    case 0x41:
      reg.b = reg.c;
      break;

    // LD B, D
    case 0x42:
      reg.b = reg.d;
      break;

    // LD B, E
    case 0x43:
      reg.b = reg.e;
      break;

    // LD B, H
    case 0x44:
      reg.b = reg.h;
      break;

    // LD B, L
    case 0x45:
      reg.b = reg.l;
      break;

    // LD B, (HL)
    case 0x46:
      reg.b = mmu.read_u8(reg.hl);
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
      refresh();
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
      addCarry(reg.b);
      break;

    // ADC C
    case 0x89:
      addCarry(reg.c);
      break;

    // ADC D
    case 0x8A:
      addCarry(reg.d);
      break;

    // ADC E
    case 0x8B:
      addCarry(reg.e);
      break;

    // ADC H
    case 0x8C:
      addCarry(reg.h);
      break;

    // ADC L
    case 0x8D:
      addCarry(reg.l);
      break;

    // ADC (HL)
    case 0x8E:
      addCarry(mmu.read_u8(reg.hl));
      break;

    // ADC A
    case 0x8F:
      addCarry(reg.a);
      break;

    // SUB B
    case 0x90:
      subtract(reg.b);
      break;

    // SUB C
    case 0x91:
      subtract(reg.c);
      break;

    // SUB D
    case 0x92:
      subtract(reg.d);
      break;

    // SUB E
    case 0x93:
      subtract(reg.e);
      break;

    // SUB H
    case 0x94:
      subtract(reg.h);
      break;

    // SUB L
    case 0x95:
      subtract(reg.l);
      break;

    // SUB (HL)
    case 0x96:
      subtract(mmu.read_u8(reg.hl));
      break;

    // SUB A
    case 0x97:
      subtract(reg.a);
      break;

    // SBC B
    case 0x98:
      subtractCarry(reg.b);
      break;

    // SBC C
    case 0x99:
      subtractCarry(reg.c);
      break;

    // SBC D
    case 0x9A:
      subtractCarry(reg.d);
      break;

    // SBC E
    case 0x9B:
      subtractCarry(reg.e);
      break;

    // SBC H
    case 0x9C:
      subtractCarry(reg.h);
      break;

    // SBC L
    case 0x9D:
      subtractCarry(reg.l);
      break;

    // SBC (HL)
    case 0x9E:
      subtractCarry(mmu.read_u8(reg.hl));
      break;

    // SBC A
    case 0x9F:
      subtractCarry(reg.a);
      break;

    // AND B
    case 0xA0:
      andReg(reg.b);
      break;

    // AND C
    case 0xA1:
      andReg(reg.c);
      break;

    // AND D
    case 0xA2:
      andReg(reg.d);
      break;

    // AND E
    case 0xA3:
      andReg(reg.e);
      break;

    // AND H
    case 0xA4:
      andReg(reg.h);
      break;

    // AND L
    case 0xA5:
      andReg(reg.l);
      break;

    // AND (HL)
    case 0xA6:
      andReg(mmu.read_u8(reg.hl));
      break;

    // AND A
    case 0xA7:
      andReg(reg.a);
      break;

    // XOR B
    case 0xA8:
      xorReg(reg.b);
      break;

    // XOR C
    case 0xA9:
      xorReg(reg.c);
      break;

    // XOR D
    case 0xAA:
      xorReg(reg.d);
      break;

    // XOR E
    case 0xAB:
      xorReg(reg.e);
      break;

    // XOR H
    case 0xAC:
      xorReg(reg.h);
      break;

    // XOR L
    case 0xAD:
      xorReg(reg.l);
      break;

    // XOR (HL)
    case 0xAE:
      xorReg(mmu.read_u8(reg.hl));
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
      orReg(reg.d);
      break;

    // OR E
    case 0xB3:
      orReg(reg.e);
      break;

    // OR H
    case 0xB4:
      orReg(reg.h);
      break;

    // OR L
    case 0xB5:
      orReg(reg.l);
      break;

    // OR (HL)
    case 0xB6:
      orReg(mmu.read_u8(reg.hl));
      break;

    // OR A
    case 0xB7:
      orReg(reg.a);
      break;

    // CP B
    case 0xB8:
      compare(reg.b);
      break;

    // CP C
    case 0xB9:
      compare(reg.c);
      break;

    // CP D
    case 0xBA:
      compare(reg.d);
      break;

    // CP E
    case 0xBB:
      compare(reg.e);
      break;

    // CP H
    case 0xBC:
      compare(reg.h);
      break;

    // CP L
    case 0xBD:
      compare(reg.l);
      break;

    // CP (HL)
    case 0xBE:
      compare(mmu.read_u8(reg.hl));
      break;

    // CP A
    case 0xBF:
      compare(reg.a);
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
      if (!bitTest(reg.f, FLAG_ZERO)) {
        reg.pc = operand;
        cpu_clock_t += 4;
      }
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
      add(operand);
      break;

    // RST 0x00
    case 0xC7:
      reg.pc = 0x00;
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
      if (bitTest(reg.f, FLAG_ZERO)) {
        cpu_clock_t += 12;
        reg.sp -= 2;
        mmu.write_u16(reg.sp, reg.pc);
        reg.pc = operand;
      }
      break;

    // CALL nnnn
    case 0xCD:
      reg.sp -= 2;
      mmu.write_u16(reg.sp, reg.pc);
      reg.pc = operand;
      break;

    // ADC 0x%02X
    case 0xCE:
      addCarry(operand);
      break;

    // RST 0x08
    case 0xCF:
      reg.pc = 0x08;
      break;

    // RET NC
    case 0xD0:
      if (!bitTest(reg.f, FLAG_CARRY)) {
        cpu_clock_t += 12;
        reg.pc = mmu.read_u16(reg.sp);
        reg.sp += 2;
      }
      break;

    // POP DE
    case 0xD1:
      reg.de = mmu.read_u16(reg.sp);
      reg.sp += 2;
      break;

    // JP NC, 0x%04X
    case 0xD2:
      if (!bitTest(reg.f, FLAG_CARRY)) {
        cpu_clock_t += 4;
        reg.pc = operand;
      }
      break;

    // UNKNOWN
    case 0xD3:
      printw("CPU: 0xD3 UNKNOWN\n");
      refresh();
      return false;
      break;

    // CALL NC, 0x%04X
    case 0xD4:
      if (!bitTest(reg.f, FLAG_CARRY)) {
        cpu_clock_t += 12;
        reg.sp -= 2;
        mmu.write_u16(reg.sp, reg.pc);
        reg.pc = operand;
      }
      break;

    // PUSH DE
    case 0xD5:
      reg.sp -= 2;
      mmu.write_u16(reg.sp, reg.de);
      break;

    // SUB 0x%02X
    case 0xD6:
      subtract(operand);
      break;

    // RST 0x10
    case 0xD7:
      reg.pc = 0x10;
      break;

    // RET C
    case 0xD8:
      if (bitTest(reg.f, FLAG_CARRY)) {
        cpu_clock_t += 12;
        reg.pc = mmu.read_u16(reg.sp);
        reg.sp += 2;
      }
      break;

    // RETI
    case 0xD9:
      reg.pc = mmu.read_u16(reg.sp);
      reg.sp += 2;
      ime = true;
      break;

    // JP C, 0x%04X
    case 0xDA:
      if (bitTest(reg.f, FLAG_CARRY)) {
        cpu_clock_t += 4;
        reg.pc = operand;
      }
      break;

    // UNKNOWN
    case 0xDB:
      printw("CPU: 0xDB UNKNOWN\n");
      refresh();
      return false;
      break;

    // CALL C, 0x%04X
    case 0xDC:
      if (bitTest(reg.f, FLAG_CARRY)) {
        cpu_clock_t += 12;
        reg.sp -= 2;
        mmu.write_u16(reg.sp, reg.pc);
        reg.pc = operand;
      }
      break;

    // UNKNOWN
    case 0xDD:
      printw("CPU: 0xDD UNKNOWN\n");
      refresh();
      return false;
      break;

    // SBC 0x%02X
    case 0xDE:
      subtractCarry(operand);
      break;

    // RST 0x18
    case 0xDF:
      reg.pc = 0x18;
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
      printw("CPU: 0xE3 UNKNOWN\n");
      refresh();
      return false;
      break;

    // UNKNOWN
    case 0xE4:
      printw("CPU: 0xE4 UNKNOWN\n");
      refresh();
      return false;
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
      reg.pc = 0x20;
      break;

    // ADD SP,0x%02X
    case 0xE8:
      bitClear(reg.f, FLAG_ZERO);
      bitClear(reg.f, FLAG_SUBTRACT);
      if ((reg.sp + (s8)operand) > 0xFF) {
        bitSet(reg.f, FLAG_CARRY);
      }
      if ((reg.sp & 0xF) + ((s8)operand & 0xF) > 0xF) {
        bitSet(reg.f, FLAG_HALF_CARRY);
      }
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
      printw("CPU: 0xEB UNKNOWN\n");
      refresh();
      return false;
      break;

    // UNKNOWN
    case 0xEC:
      printw("CPU: 0xEC UNKNOWN\n");
      refresh();
      return false;
      break;

    // UNKNOWN
    case 0xED:
      printw("CPU: 0xED UNKNOWN\n");
      refresh();
      return false;
      break;

    // XOR 0x%02X
    case 0xEE:
      xorReg(operand);
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
      reg.a = mmu.read_u8(0xFF00 + reg.c);
      break;

    // DI
    case 0xF3:
      ime = false;
      break;

    // UNKNOWN
    case 0xF4:
      printw("CPU: 0xF4 UNKNOWN\n");
      refresh();
      return false;
      break;

    // PUSH AF
    case 0xF5:
      reg.sp -= 2;
      mmu.write_u16(reg.sp, reg.af);
      break;

    // OR 0x%02X
    case 0xF6:
      orReg(operand);
      break;

    // RST 0x30
    case 0xF7:
      reg.pc = 0x30;
      break;

    // LD HL, SP+0x%02X
    case 0xF8:
      reg.hl = reg.sp + operand;
      break;

    // LD SP, HL
    case 0xF9:
      reg.sp = reg.hl;
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
      printw("CPU: 0xFC UNKNOWN\n");
      refresh();
      return false;
      break;

    // UNKNOWN
    case 0xFD:
      printw("CPU: 0xFD UNKNOWN\n");
      refresh();
      return false;
      break;

    // CP nn
    // Implied subtraction (A - nn) and set flags
    case 0xFE:
      compare(operand);
      break;

    // RST 0x38
    case 0xFF:
      reg.pc = 0x38;
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
      rotateLeftCarry(reg.b);
      break;

    // RLC C
    case 0x01:
      rotateLeftCarry(reg.c);
      break;

    // RLC D
    case 0x02:
      rotateLeftCarry(reg.d);
      break;

    // RLC E
    case 0x03:
      rotateLeftCarry(reg.e);
      break;

    // RLC H
    case 0x04:
      rotateLeftCarry(reg.h);
      break;

    // RLC L
    case 0x05:
      rotateLeftCarry(reg.l);
      break;

    // RLC (HL)
    case 0x06: {
      u8 val = mmu.read_u8(reg.hl);
      rotateLeftCarry(val);
      mmu.write_u8(reg.hl, val);
    } break;

    // RLC A
    case 0x07:
      rotateLeftCarry(reg.a);
      break;

    // RRC B
    case 0x08:
      rotateRightCarry(reg.b);
      break;

    // RRC C
    case 0x09:
      rotateRightCarry(reg.c);
      break;

    // RRC D
    case 0x0A:
      rotateRightCarry(reg.d);
      break;

    // RRC E
    case 0x0B:
      rotateRightCarry(reg.e);
      break;

    // RRC H
    case 0x0C:
      rotateRightCarry(reg.h);
      break;

    // RRC L
    case 0x0D:
      rotateRightCarry(reg.l);
      break;

    // RRC (HL)
    case 0x0E: {
      u8 val = mmu.read_u8(reg.hl);
      rotateRightCarry(val);
      mmu.write_u8(reg.hl, val);
    } break;

    // RRC A
    case 0x0F:
      rotateRightCarry(reg.a);
      break;

    // RL B
    case 0x10:
      rotateLeft(reg.b);
      break;

    // RL C
    case 0x11:
      rotateLeft(reg.c);
      break;

    // RL D
    case 0x12:
      rotateLeft(reg.d);
      break;

    // RL E
    case 0x13:
      rotateLeft(reg.e);
      break;

    // RL H
    case 0x14:
      rotateLeft(reg.h);
      break;

    // RL L
    case 0x15:
      rotateLeft(reg.l);
      break;

    // RL (HL)
    case 0x16: {
      u8 val = mmu.read_u8(reg.hl);
      rotateLeft(val);
      mmu.write_u8(reg.hl, val);
    } break;

    // RL A
    case 0x17:
      rotateLeft(reg.a);
      break;

    // RR B
    case 0x18:
      rotateRight(reg.b);
      break;

    // RR C
    case 0x19:
      rotateRight(reg.c);
      break;

    // RR D
    case 0x1A:
      rotateRight(reg.d);
      break;

    // RR E
    case 0x1B:
      rotateRight(reg.e);
      break;

    // RR H
    case 0x1C:
      rotateRight(reg.h);
      break;

    // RR L
    case 0x1D:
      rotateRight(reg.l);
      break;

    // RR (HL)
    case 0x1E: {
      u8 val = mmu.read_u8(reg.hl);
      rotateRight(val);
      mmu.write_u8(reg.hl, val);
    } break;

    // RR A
    case 0x1F:
      rotateRight(reg.a);
      break;

    // SLA B
    case 0x20:
      sla(reg.b);
      break;

    // SLA C
    case 0x21:
      sla(reg.c);
      break;

    // SLA D
    case 0x22:
      sla(reg.d);
      break;

    // SLA E
    case 0x23:
      sla(reg.e);
      break;

    // SLA H
    case 0x24:
      sla(reg.h);
      break;

    // SLA L
    case 0x25:
      sla(reg.l);
      break;

    // SLA (HL)
    case 0x26: {
      u8 val = mmu.read_u8(reg.hl);
      sla(val);
      mmu.write_u8(reg.hl, val);
    } break;

    // SLA A
    case 0x27:
      sla(reg.a);
      break;

    // SRA B
    case 0x28:
      sra(reg.b);
      break;

    // SRA C
    case 0x29:
      sra(reg.c);
      break;

    // SRA D
    case 0x2A:
      sra(reg.d);
      break;

    // SRA E
    case 0x2B:
      sra(reg.e);
      break;

    // SRA H
    case 0x2C:
      sra(reg.h);
      break;

    // SRA L
    case 0x2D:
      sra(reg.l);
      break;

    // SRA (HL)
    case 0x2E: {
      u8 val = mmu.read_u8(reg.hl);
      sra(val);
      mmu.write_u8(reg.hl, val);
    } break;

    // SRA A
    case 0x2F:
      sra(reg.b);
      break;

    // SWAP B
    case 0x30:
      swapReg(reg.b);
      break;

    // SWAP C
    case 0x31:
      swapReg(reg.c);
      break;

    // SWAP D
    case 0x32:
      swapReg(reg.d);
      break;

    // SWAP E
    case 0x33:
      swapReg(reg.e);
      break;

    // SWAP H
    case 0x34:
      swapReg(reg.h);
      break;

    // SWAP L
    case 0x35:
      swapReg(reg.l);
      break;

    // SWAP (HL)
    case 0x36: {
      u8 val = mmu.read_u8(reg.hl);
      swapReg(val);
      mmu.write_u8(reg.hl, val);
    } break;

    // SWAP A
    case 0x37:
      swapReg(reg.a);
      break;

    // SRL B
    case 0x38:
      srl(reg.b);
      break;

    // SRL C
    case 0x39:
      srl(reg.c);
      break;

    // SRL D
    case 0x3A:
      srl(reg.d);
      break;

    // SRL E
    case 0x3B:
      srl(reg.e);
      break;

    // SRL H
    case 0x3C:
      srl(reg.h);
      break;

    // SRL L
    case 0x3D:
      srl(reg.l);
      break;

    // SRL (HL)
    case 0x3E: {
      u8 val = mmu.read_u8(reg.hl);
      srl(val);
      mmu.write_u8(reg.hl, val);
    } break;

    // SRL A
    case 0x3F:
      srl(reg.a);
      break;

    // BIT 0, B
    case 0x40:
      bit(0, reg.b);
      break;

    // BIT 0, C
    case 0x41:
      bit(0, reg.c);
      break;

    // BIT 0, D
    case 0x42:
      bit(0, reg.d);
      break;

    // BIT 0, E
    case 0x43:
      bit(0, reg.e);
      break;

    // BIT 0, H
    case 0x44:
      bit(0, reg.h);
      break;

    // BIT 0, L
    case 0x45:
      bit(0, reg.l);
      break;

    // BIT 0, (HL)
    case 0x46:
      bit(0, mmu.read_u8(reg.hl));
      break;

    // BIT 0, A
    case 0x47:
      bit(0, reg.a);
      break;

    // BIT 1, B
    case 0x48:
      bit(1, reg.b);
      break;

    // BIT 1, C
    case 0x49:
      bit(1, reg.c);
      break;

    // BIT 1, D
    case 0x4A:
      bit(1, reg.d);
      break;

    // BIT 1, E
    case 0x4B:
      bit(1, reg.e);
      break;

    // BIT 1, H
    case 0x4C:
      bit(1, reg.h);
      break;

    // BIT 1, L
    case 0x4D:
      bit(1, reg.l);
      break;

    // BIT 1, (HL)
    case 0x4E:
      bit(1, mmu.read_u8(reg.hl));
      break;

    // BIT 1, A
    case 0x4F:
      bit(1, reg.a);
      break;

    // BIT 2, B
    case 0x50:
      bit(2, reg.b);
      break;

    // BIT 2, C
    case 0x51:
      bit(2, reg.c);
      break;

    // BIT 2, D
    case 0x52:
      bit(2, reg.d);
      break;

    // BIT 2, E
    case 0x53:
      bit(2, reg.e);
      break;

    // BIT 2, H
    case 0x54:
      bit(2, reg.h);
      break;

    // BIT 2, L
    case 0x55:
      bit(2, reg.l);
      break;

    // BIT 2, (HL)
    case 0x56:
      bit(2, mmu.read_u8(reg.hl));
      break;

    // BIT 2, A
    case 0x57:
      bit(2, reg.a);
      break;

    // BIT 3, B
    case 0x58:
      bit(3, reg.b);
      break;

    // BIT 3, C
    case 0x59:
      bit(3, reg.c);
      break;

    // BIT 3, D
    case 0x5A:
      bit(3, reg.d);
      break;

    // BIT 3, E
    case 0x5B:
      bit(3, reg.e);
      break;

    // BIT 3, H
    case 0x5C:
      bit(3, reg.h);
      break;

    // BIT 3, L
    case 0x5D:
      bit(3, reg.l);
      break;

    // BIT 3, (HL)
    case 0x5E:
      bit(3, mmu.read_u8(reg.hl));
      break;

    // BIT 3, A
    case 0x5F:
      bit(3, reg.a);
      break;

    // BIT 4, B
    case 0x60:
      bit(4, reg.b);
      break;

    // BIT 4, C
    case 0x61:
      bit(4, reg.c);
      break;

    // BIT 4, D
    case 0x62:
      bit(4, reg.d);
      break;

    // BIT 4, E
    case 0x63:
      bit(4, reg.e);
      break;

    // BIT 4, H
    case 0x64:
      bit(4, reg.h);
      break;

    // BIT 4, L
    case 0x65:
      bit(4, reg.l);
      break;

    // BIT 4, (HL)
    case 0x66:
      bit(4, mmu.read_u8(reg.hl));
      break;

    // BIT 4, A
    case 0x67:
      bit(4, reg.a);
      break;

    // BIT 5, B
    case 0x68:
      bit(5, reg.b);
      break;

    // BIT 5, C
    case 0x69:
      bit(5, reg.c);
      break;

    // BIT 5, D
    case 0x6A:
      bit(5, reg.d);
      break;

    // BIT 5, E
    case 0x6B:
      bit(5, reg.e);
      break;

    // BIT 5, H
    case 0x6C:
      bit(5, reg.h);
      break;

    // BIT 5, L
    case 0x6D:
      bit(5, reg.l);
      break;

    // BIT 5, (HL)
    case 0x6E:
      bit(5, mmu.read_u8(reg.hl));
      break;

    // BIT 5, A
    case 0x6F:
      bit(5, reg.a);
      break;

    // BIT 6, B
    case 0x70:
      bit(6, reg.b);
      break;

    // BIT 6, C
    case 0x71:
      bit(6, reg.c);
      break;

    // BIT 6, D
    case 0x72:
      bit(6, reg.d);
      break;

    // BIT 6, E
    case 0x73:
      bit(6, reg.e);
      break;

    // BIT 6, H
    case 0x74:
      bit(6, reg.h);
      break;

    // BIT 6, L
    case 0x75:
      bit(6, reg.l);
      break;

    // BIT 6, (HL)
    case 0x76:
      bit(6, mmu.read_u8(reg.hl));
      break;

    // BIT 6, A
    case 0x77:
      bit(6, reg.a);
      break;

    // BIT 7, B
    case 0x78:
      bit(7, reg.b);
      break;

    // BIT 7, C
    case 0x79:
      bit(7, reg.c);
      break;

    // BIT 7, D
    case 0x7A:
      bit(7, reg.d);
      break;

    // BIT 7, E
    case 0x7B:
      bit(7, reg.e);
      break;

    // BIT 7, H
    case 0x7C:
      bit(7, reg.h);
      break;

    // BIT 7, L
    case 0x7D:
      bit(7, reg.l);
      break;

    // BIT 7, (HL)
    case 0x7E:
      bit(7, mmu.read_u8(reg.hl));
      break;

    // BIT 7, A
    case 0x7F:
      bit(7, reg.a);
      break;

    // RES 0, B
    case 0x80:
      bitClear(reg.b, 0);
      break;

    // RES 0, C
    case 0x81:
      bitClear(reg.c, 0);
      break;

    // RES 0, D
    case 0x82:
      bitClear(reg.d, 0);
      break;

    // RES 0, E
    case 0x83:
      bitClear(reg.e, 0);
      break;

    // RES 0, H
    case 0x84:
      bitClear(reg.h, 0);
      break;

    // RES 0, L
    case 0x85:
      bitClear(reg.l, 0);
      break;

    // RES 0, (HL)
    case 0x86: {
      u8 val = mmu.read_u8(reg.hl);
      bitClear(val, 0);
      mmu.write_u8(reg.hl, val);
    } break;

    // RES 0, A
    case 0x87:
      bitClear(reg.a, 0);
      break;

    // RES 1, B
    case 0x88:
      bitClear(reg.b, 1);
      break;

    // RES 1, C
    case 0x89:
      bitClear(reg.c, 1);
      break;

    // RES 1, D
    case 0x8A:
      bitClear(reg.d, 1);
      break;

    // RES 1, E
    case 0x8B:
      bitClear(reg.e, 1);
      break;

    // RES 1, H
    case 0x8C:
      bitClear(reg.h, 1);
      break;

    // RES 1, L
    case 0x8D:
      bitClear(reg.l, 1);
      break;

    // RES 1, (HL)
    case 0x8E: {
      u8 val = mmu.read_u8(reg.hl);
      bitClear(val, 1);
      mmu.write_u8(reg.hl, val);
    } break;

    // RES 1, A
    case 0x8F:
      bitClear(reg.a, 1);
      break;

    // RES 2, B
    case 0x90:
      bitClear(reg.b, 2);
      break;

    // RES 2, C
    case 0x91:
      bitClear(reg.c, 2);
      break;

    // RES 2, D
    case 0x92:
      bitClear(reg.d, 2);
      break;

    // RES 2, E
    case 0x93:
      bitClear(reg.e, 2);
      break;

    // RES 2, H
    case 0x94:
      bitClear(reg.h, 2);
      break;

    // RES 2, L
    case 0x95:
      bitClear(reg.l, 2);
      break;

    // RES 2, (HL)
    case 0x96: {
      u8 val = mmu.read_u8(reg.hl);
      bitClear(val, 2);
      mmu.write_u8(reg.hl, val);
    } break;

    // RES 2, A
    case 0x97:
      bitClear(reg.a, 2);
      break;

    // RES 3, B
    case 0x98:
      bitClear(reg.b, 3);
      break;

    // RES 3, C
    case 0x99:
      bitClear(reg.c, 3);
      break;

    // RES 3, D
    case 0x9A:
      bitClear(reg.d, 3);
      break;

    // RES 3, E
    case 0x9B:
      bitClear(reg.e, 3);
      break;

    // RES 3, H
    case 0x9C:
      bitClear(reg.h, 3);
      break;

    // RES 3, L
    case 0x9D:
      bitClear(reg.l, 3);
      break;

    // RES 3, (HL)
    case 0x9E: {
      u8 val = mmu.read_u8(reg.hl);
      bitClear(val, 3);
      mmu.write_u8(reg.hl, val);
    } break;

    // RES 3, A
    case 0x9F:
      bitClear(reg.a, 3);
      break;

    // RES 4, B
    case 0xA0:
      bitClear(reg.b, 4);
      break;

    // RES 4, C
    case 0xA1:
      bitClear(reg.c, 4);
      break;

    // RES 4, D
    case 0xA2:
      bitClear(reg.d, 4);
      break;

    // RES 4, E
    case 0xA3:
      bitClear(reg.e, 4);
      break;

    // RES 4, H
    case 0xA4:
      bitClear(reg.h, 4);
      break;

    // RES 4, L
    case 0xA5:
      bitClear(reg.l, 4);
      break;

    // RES 4, (HL)
    case 0xA6: {
      u8 val = mmu.read_u8(reg.hl);
      bitClear(val, 4);
      mmu.write_u8(reg.hl, val);
    } break;

    // RES 4, A
    case 0xA7:
      bitClear(reg.a, 4);
      break;

    // RES 5, B
    case 0xA8:
      bitClear(reg.b, 5);
      break;

    // RES 5, C
    case 0xA9:
      bitClear(reg.c, 5);
      break;

    // RES 5, D
    case 0xAA:
      bitClear(reg.d, 5);
      break;

    // RES 5, E
    case 0xAB:
      bitClear(reg.e, 5);
      break;

    // RES 5, H
    case 0xAC:
      bitClear(reg.h, 5);
      break;

    // RES 5, L
    case 0xAD:
      bitClear(reg.l, 5);
      break;

    // RES 5, (HL)
    case 0xAE: {
      u8 val = mmu.read_u8(reg.hl);
      bitClear(val, 5);
      mmu.write_u8(reg.hl, val);
    } break;

    // RES 5, A
    case 0xAF:
      bitClear(reg.a, 5);
      break;

    // RES 6, B
    case 0xB0:
      bitClear(reg.b, 6);
      break;

    // RES 6, C
    case 0xB1:
      bitClear(reg.c, 6);
      break;

    // RES 6, D
    case 0xB2:
      bitClear(reg.d, 6);
      break;

    // RES 6, E
    case 0xB3:
      bitClear(reg.e, 6);
      break;

    // RES 6, H
    case 0xB4:
      bitClear(reg.h, 6);
      break;

    // RES 6, L
    case 0xB5:
      bitClear(reg.l, 6);
      break;

    // RES 6, (HL)
    case 0xB6: {
      u8 val = mmu.read_u8(reg.hl);
      bitClear(val, 6);
      mmu.write_u8(reg.hl, val);
    } break;

    // RES 6, A
    case 0xB7:
      bitClear(reg.a, 6);
      break;

    // RES 7, B
    case 0xB8:
      bitClear(reg.b, 7);
      break;

    // RES 7, C
    case 0xB9:
      bitClear(reg.c, 7);
      break;

    // RES 7, D
    case 0xBA:
      bitClear(reg.d, 7);
      break;

    // RES 7, E
    case 0xBB:
      bitClear(reg.e, 7);
      break;

    // RES 7, H
    case 0xBC:
      bitClear(reg.h, 7);
      break;

    // RES 7, L
    case 0xBD:
      bitClear(reg.l, 7);
      break;

    // RES 7, (HL)
    case 0xBE: {
      u8 val = mmu.read_u8(reg.hl);
      bitClear(val, 7);
      mmu.write_u8(reg.hl, val);
    } break;

    // RES 7, A
    case 0xBF:
      bitClear(reg.a, 7);
      break;

    // SET 0, B
    case 0xC0:
      bitSet(reg.b, 0);
      break;

    // SET 0, C
    case 0xC1:
      bitSet(reg.c, 0);
      break;

    // SET 0, D
    case 0xC2:
      bitSet(reg.d, 0);
      break;

    // SET 0, E
    case 0xC3:
      bitSet(reg.e, 0);
      break;

    // SET 0, H
    case 0xC4:
      bitSet(reg.h, 0);
      break;

    // SET 0, L
    case 0xC5:
      bitSet(reg.l, 0);
      break;

    // SET 0, (HL)
    case 0xC6: {
      u8 val = mmu.read_u8(reg.hl);
      bitSet(val, 0);
      mmu.write_u8(reg.hl, val);
    } break;

    // SET 0, A
    case 0xC7:
      bitSet(reg.a, 0);
      break;

    // SET 1, B
    case 0xC8:
      bitSet(reg.b, 1);
      break;

    // SET 1, C
    case 0xC9:
      bitSet(reg.c, 1);
      break;

    // SET 1, D
    case 0xCA:
      bitSet(reg.d, 1);
      break;

    // SET 1, E
    case 0xCB:
      bitSet(reg.e, 1);
      break;

    // SET 1, H
    case 0xCC:
      bitSet(reg.h, 1);
      break;

    // SET 1, L
    case 0xCD:
      bitSet(reg.l, 1);
      break;

    // SET 1, (HL)
    case 0xCE: {
      u8 val = mmu.read_u8(reg.hl);
      bitSet(val, 1);
      mmu.write_u8(reg.hl, val);
    } break;

    // SET 1, A
    case 0xCF:
      bitSet(reg.a, 1);
      break;

    // SET 2, B
    case 0xD0:
      bitSet(reg.b, 2);
      break;

    // SET 2, C
    case 0xD1:
      bitSet(reg.c, 2);
      break;

    // SET 2, D
    case 0xD2:
      bitSet(reg.d, 2);
      break;

    // SET 2, E
    case 0xD3:
      bitSet(reg.e, 2);
      break;

    // SET 2, H
    case 0xD4:
      bitSet(reg.h, 2);
      break;

    // SET 2, L
    case 0xD5:
      bitSet(reg.l, 2);
      break;

    // SET 2, (HL)
    case 0xD6: {
      u8 val = mmu.read_u8(reg.hl);
      bitSet(val, 2);
      mmu.write_u8(reg.hl, val);
    } break;

    // SET 2, A
    case 0xD7:
      bitSet(reg.a, 2);
      break;

    // SET 3, B
    case 0xD8:
      bitSet(reg.b, 3);
      break;

    // SET 3, C
    case 0xD9:
      bitSet(reg.c, 3);
      break;

    // SET 3, D
    case 0xDA:
      bitSet(reg.d, 3);
      break;

    // SET 3, E
    case 0xDB:
      bitSet(reg.e, 3);
      break;

    // SET 3, H
    case 0xDC:
      bitSet(reg.h, 3);
      break;

    // SET 3, L
    case 0xDD:
      bitSet(reg.l, 3);
      break;

    // SET 3, (HL)
    case 0xDE: {
      u8 val = mmu.read_u8(reg.hl);
      bitSet(val, 3);
      mmu.write_u8(reg.hl, val);
    } break;

    // SET 3, A
    case 0xDF:
      bitSet(reg.a, 3);
      break;

    // SET 4, B
    case 0xE0:
      bitSet(reg.b, 4);
      break;

    // SET 4, C
    case 0xE1:
      bitSet(reg.c, 4);
      break;

    // SET 4, D
    case 0xE2:
      bitSet(reg.d, 4);
      break;

    // SET 4, E
    case 0xE3:
      bitSet(reg.e, 4);
      break;

    // SET 4, H
    case 0xE4:
      bitSet(reg.h, 4);
      break;

    // SET 4, L
    case 0xE5:
      bitSet(reg.l, 4);
      break;

    // SET 4, (HL)
    case 0xE6: {
      u8 val = mmu.read_u8(reg.hl);
      bitSet(val, 4);
      mmu.write_u8(reg.hl, val);
    } break;

    // SET 4, A
    case 0xE7:
      bitSet(reg.a, 4);
      break;

    // SET 5, B
    case 0xE8:
      bitSet(reg.b, 5);
      break;

    // SET 5, C
    case 0xE9:
      bitSet(reg.c, 5);
      break;

    // SET 5, D
    case 0xEA:
      bitSet(reg.d, 5);
      break;

    // SET 5, E
    case 0xEB:
      bitSet(reg.e, 5);
      break;

    // SET 5, H
    case 0xEC:
      bitSet(reg.h, 5);
      break;

    // SET 5, L
    case 0xED:
      bitSet(reg.l, 5);
      break;

    // SET 5, (HL)
    case 0xEE: {
      u8 val = mmu.read_u8(reg.hl);
      bitSet(val, 5);
      mmu.write_u8(reg.hl, val);
    } break;

    // SET 5, A
    case 0xEF:
      bitSet(reg.a, 5);
      break;

    // SET 6, B
    case 0xF0:
      bitSet(reg.b, 6);
      break;

    // SET 6, C
    case 0xF1:
      bitSet(reg.c, 6);
      break;

    // SET 6, D
    case 0xF2:
      bitSet(reg.d, 6);
      break;

    // SET 6, E
    case 0xF3:
      bitSet(reg.e, 6);
      break;

    // SET 6, H
    case 0xF4:
      bitSet(reg.h, 6);
      break;

    // SET 6, L
    case 0xF5:
      bitSet(reg.l, 6);
      break;

    // SET 6, (HL)
    case 0xF6: {
      u8 val = mmu.read_u8(reg.hl);
      bitSet(val, 6);
      mmu.write_u8(reg.hl, val);
    } break;

    // SET 6, A
    case 0xF7:
      bitSet(reg.a, 6);
      break;

    // SET 7, B
    case 0xF8:
      bitSet(reg.b, 7);
      break;

    // SET 7, C
    case 0xF9:
      bitSet(reg.c, 7);
      break;

    // SET 7, D
    case 0xFA:
      bitSet(reg.d, 7);
      break;

    // SET 7, E
    case 0xFB:
      bitSet(reg.e, 7);
      break;

    // SET 7, H
    case 0xFC:
      bitSet(reg.h, 7);
      break;

    // SET 7, L
    case 0xFD:
      bitSet(reg.l, 7);
      break;

    // SET 7, (HL)
    case 0xFE: {
      u8 val = mmu.read_u8(reg.hl);
      bitSet(val, 7);
      mmu.write_u8(reg.hl, val);
    } break;

    // SET 7, A
    case 0xFF:
      bitSet(reg.a, 7);
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
  u8 flags = mmu.memory[IF];
  bitClear(flags, interrupt);
  mmu.memory[IF] = flags;

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
