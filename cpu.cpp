/* gb: a Gameboy Emulator
   Author: Don Freiday */

#include "cpu.h"
#include "mmu.h"

#define CPU_INTERRUPT_FLAG   0xFF0F
#define CPU_INTERRUPT_ENABLE 0xFFFF

#define FLAG_CARRY       4 // set if a carry occurred from the last arithmetic operation or if register A is the smaller value when executing the CP instruction
#define FLAG_HALF_CARRY  5 // set if a carry occurred from the lower nibble in the last math operation.
#define FLAG_SUBTRACT    6 // set if a subtraction was performed in the last math instruction.
#define FLAG_ZERO  		   7 // set when the result of an arithmetic operation is zero or two values match when using the CP

CPU::CPU() {
  reset();
}

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
  ime = true;
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
	if (bitTest(reg.a, 1)) {
		bitSet(reg.f, FLAG_CARRY); // Low bit of register is shifted into carry flag
	}
	reg.a >>= 1; // Shift register right, high bit becomes 0
	if (bitTest(reg.f, FLAG_CARRY)) {
		bitSet(reg.a, 7); // Carry flag shifts into high bit of register
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
		bitSet(reg.f,FLAG_HALF_CARRY);
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
		bitSet(reg.f,FLAG_HALF_CARRY);
	}
	bitClear(reg.f, FLAG_SUBTRACT);
	reg.a += n;
	if (reg.a == 0) {
		bitSet(reg.f, FLAG_ZERO);
	}
}

// Compare A with n. This is basically an A - n subtraction instruction but the results are thrown away.
template <typename t>
void CPU::compare(t num) {
	reg.f = 0;
	if (reg.a < num) {
		bitSet(reg.f, FLAG_CARRY);
	}
	if ((reg.a & 0xF) < (num &0xF)) {
		bitSet(reg.f,FLAG_HALF_CARRY);
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

//Logically AND n with A, result in A.
template <typename t>
void CPU::andReg(t reg1) {
	reg.a &= reg1;
	reg.f = 0;
	if (reg.a == 0) {
		bitSet(reg.f, FLAG_ZERO);
	}
	bitSet(reg.f, FLAG_HALF_CARRY);
}

 //Swap upper & lower nibles of n.
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
	// Blargg instruction timing data for main opcodes.
	// Times are in machine cycles
	u8 timings[256] = {
		1,3,2,2,1,1,2,1,5,2,2,2,1,1,2,1,
		0,3,2,2,1,1,2,1,3,2,2,2,1,1,2,1,
		2,3,2,2,1,1,2,1,2,2,2,2,1,1,2,1,
		2,3,2,2,3,3,3,1,2,2,2,2,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		2,2,2,2,2,2,0,2,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		2,3,3,4,3,4,2,4,2,4,3,0,3,6,2,4,
		2,3,3,0,3,4,2,4,2,4,3,0,3,0,2,4,
		3,3,2,0,0,4,2,4,4,1,4,0,0,0,2,4,
		3,3,2,1,0,4,2,4,3,2,4,1,0,0,2,4
	};
	 for (int i = 0; i<256; i++) {
		 if(instructions[i].cycles != timings[i]*4) {
		 	printf("0x%02X: %s: should be %d\n", i, instructions[i].disassembly, timings[i]*4); 
		 }
	 }

	// Blargg instruction timing data for CB prefixed opcodes.
	// Times are in machine cycles
	u8 timingsCB[256] = {
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
    2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
    2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
    2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
    2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
    2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
    2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
    2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
    2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
    2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
    2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
    2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
    2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
    2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
    2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2 };
	for (int i = 0; i<256; i++) {
		if(instructions_CB[i].cycles != timingsCB[i]*4) {
			printf("0x%02X: %s: should be %d\n", i, instructions_CB[i].disassembly, timings[i]*4); 
		}
	 }

	u8 timingsConditionalTaken[256] = {
	1,3,2,2,1,1,2,1,5,2,2,2,1,1,2,1,
    0,3,2,2,1,1,2,1,3,2,2,2,1,1,2,1,
    3,3,2,2,1,1,2,1,3,2,2,2,1,1,2,1,
    3,3,2,2,3,3,3,1,3,2,2,2,1,1,2,1,
    1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
    1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
    1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
    2,2,2,2,2,2,0,2,1,1,1,1,1,1,2,1,
    1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
    1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
    1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
    1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
    5,3,4,4,6,4,2,4,5,4,4,0,6,6,2,4,
    5,3,4,0,6,4,2,4,5,4,4,0,6,0,2,4,
    3,3,2,0,0,4,2,4,4,1,4,0,0,0,2,4,
    3,3,2,1,0,4,2,4,3,2,4,1,0,0,2,4 };
	for (int i = 0; i<256; i++) {
		if(instructions[i].cycles != timingsConditionalTaken[i]*4) {
			printf("0x%02X: %s: %d/%d so add %d\n", i, instructions[i].disassembly, timingsConditionalTaken[i]*4, timings[i]*4, timingsConditionalTaken[i]*4-timings[i]*4); 
		}
	 }
	 

	if (debug) { printf("%04X: ", reg.pc); }

	// Fetch the opcode from MMU and increment PC
	u8 op = mmu.read_u8(reg.pc++);

  // Parse operand and print disassembly of instruction
  u16 operand;
  if (instructions[op].operandLength == 1) {
    operand = mmu.read_u8(reg.pc);
		if (debug) { printf(instructions[op].disassembly, operand); }
  }
  else if (instructions[op].operandLength == 2) {
    operand = mmu.read_u16(reg.pc);
    if (debug) { printf(instructions[op].disassembly, operand); }
  }
  else {
    if (debug) { printf("%s", instructions[op].disassembly); }
  }

  // Adjust PC
  reg.pc += instructions[op].operandLength;

  // Update cpu clock
  cpu_clock_t = instructions[op].cycles;

  // Go go go!
  switch(op) {
    // NOP
    case 0x00:
    break;

	// LD BC, nnnn
	case 0x01:
		reg.bc = operand;
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

	// LD DE, nnnn
	case 0x11:
		reg.de = operand;
	break;

	// INC DE
	case 0x13:
		incrementReg(reg.de);
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
	// Rotate A left through Carry flag.
	case 0x17:
		rotateLeft(reg.a);
	break;

	// LD A, (DE)
	case 0x1A:
		reg.a = mmu.read_u8(reg.de);
	break;

	// DEC E
	case 0x1D:
		decrementReg(reg.e);
	break;

	// LD E, nn
	case 0x1E:
		reg.e = operand;
	break;

	// JR nn
	case 0x18:
		reg.pc += (s8)operand;
	break;

    // JR nz nn
    // Relative jump by signed immediate if last result was not zero (zero flag = 0)
    case 0x20:
    	if (!bitTest(reg.f, FLAG_ZERO)) {
      		reg.pc += (s8)(operand);
			cpu_clock_t += 4;
    	}
	break;

	// INC H
	case 0x24:
		incrementReg(reg.h);
	break;

    // LD L, nn
    case 0x2E:
    	reg.l = operand;
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

	// JR Z, nn
	case 0x28:
		if(bitTest(reg.f, FLAG_ZERO)) {
			reg.pc += (s8)operand;
			cpu_clock_t += 4;
		}
	break;

	// LDI A, (HL)
	case 0x2A:
		reg.a = mmu.read_u8(reg.hl++);
	break;

	// CPL
	// Complement A register. (Flip all bits.)
	case 0x2F:
		reg.a = ~reg.a;
		bitSet(reg.f, FLAG_SUBTRACT);
		bitSet(reg.f, FLAG_HALF_CARRY);
	break;

    // LD SP, nnnn
    case 0x31:
    	reg.sp = operand;
    break;

    // LDD (hl--), a
    case 0x32:
    	mmu.write_u8(reg.hl--, reg.a);
    break;

	// INC (HL)
	case 0x34: {
		u8 byte = mmu.read_u8(reg.hl);
		incrementReg(byte);
		mmu.write_u8(reg.hl, byte);
	}
	break;

	// LD (HL), nn
	case 0x36:
		mmu.write_u8(reg.hl, operand);
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

	// LD B, A
	case 0x47:
		reg.b = reg.a;
	break;

	// LD C, A
	case 0x4F:
		reg.c = reg.a;
	break;

	// LD D, A
	case 0x57:
		reg.d = reg.a;
	break;

	// LD H, A
	case 0x67:
		reg.h = reg.a;
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
		andReg(reg.c);
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

	// ADD A, (HL)
	case 0x86:
		add(mmu.read_u8(reg.hl));
	break;

	// SUB B
	case 0x90:
		subtract(reg.b);
	break;

	// AND C
	case 0xA1:
		andReg(reg.c);
	break;

	// AND A
	case 0xA7:
		andReg(reg.a);
	break;

	// XOR C
	case 0xA9:
		xorReg(reg.c);
	break;

    // XOR A
    /* Compares each bit of its first operand to the corresponding bit of its second operand.
    If one bit is 0 and the other bit is 1, the corresponding result bit is set to 1.
    Otherwise, the corresponding result bit is set to 0.*/
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

	// CP (HL)
	case 0xBE:
		compare(mmu.read_u8(reg.hl));
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

    // JP nnnn
    case 0xC3:
    	reg.pc = operand;
    break;

	// PUSH BC
	case 0xC5:
		reg.sp -= 2;
		mmu.write_u16(reg.sp, reg.bc);
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

	// CB is a prefix
	case 0xCB:
		if (!execute_CB(operand)) {
			printf("^^^ Unimplemented instruction: %02X ^^^\n", op);
			printf("Code stub:\n\n// %s\ncase 0x%02X:\n\nbreak;\n\n", instructions_CB[operand].disassembly, operand);
		return false;
		}
		cpu_clock_t = instructions_CB[operand].cycles;
	break;

	// CALL nnnn
	case 0xCD:
		reg.sp -= 2;
		mmu.write_u16(reg.sp, reg.pc);
		reg.pc = operand;
	break;

	// POP DE
	case 0xD1:
		reg.de = mmu.read_u16(reg.sp);
		reg.sp += 2;
	break;

	// PUSH DE
	case 0xD5:
		reg.sp -= 2;
		mmu.write_u16(reg.sp, reg.de);
	break;

	// RETI
	case 0xD9:
		reg.pc = mmu.read_u16(reg.sp);
		reg.sp += 2;
		ime = true;
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

	// PUSH HL
	case 0xE5:
		reg.sp-=2;
		mmu.write_u16(reg.sp, reg.hl);
	break;

	// AND nn
	case 0xE6:
		andReg(operand);
	break;

	// LD (nnnn), A
	case 0xEA:
		mmu.write_u8(operand, reg.a);
	break;

	// RST 0x28
	case 0xEF:
		reg.sp -=2 ;
		mmu.write_u16(reg.sp, reg.pc);
		reg.pc = operand;
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

    // DI
    case 0xF3:
	    ime = false;
    break;

	// PUSH AF
	case 0xF5:
		reg.sp-=2;
		mmu.write_u16(reg.sp, reg.af);
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

    default:
			printf("^^^ Unimplemented instruction: 0x%02X ^^^\n\n", op);
			printf("Code stub:\n\n// %s\ncase 0x%02X:\n\nbreak;\n\n", instructions[op].disassembly, op);
	    return false;
    break;
  }

	if(debugVerbose) {
		printf("\naf=%04X bc=%04X de=%04X hl=%04X sp=%04X pc=%04X ime=%04x mcycles=%d tcycles=%X", reg.af, reg.bc, reg.de, reg.hl, reg.sp, reg.pc, ime, cpu_clock_m, cycles);
		printf(" flags=");
		bitTest(reg.f, FLAG_ZERO) ? printf("Z") : printf("z");
		bitTest(reg.f, FLAG_SUBTRACT) ? printf("N") : printf("n");
		bitTest(reg.f, FLAG_HALF_CARRY) ? printf("H") : printf("h");
		bitTest(reg.f, FLAG_CARRY) ? printf("C") : printf("c");
	}
	if(debug || debugVerbose) { printf("\n"); }
	cpu_clock_m = cpu_clock_t / 4;
	cycles += cpu_clock_t;

  return true;
}

// extended instruction set via 0xCB prefix
bool CPU::execute_CB(u8 op) {
	if(debug) { printf(": %s", instructions_CB[op].disassembly); }

	switch(op) {
		// RL C
		case 0x11:
			rotateLeft(reg.c);
		break;

		// SWAP A
		case 0x37:
			swapReg(reg.a);
		break;

		// BIT 7, H
		case 0x7C:
			bitTestReg(reg.h, 7);
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
Interrupt enable register: 0xFFFF allows disabling/enabling specific registers */
void CPU::checkInterrupts() {
	if (!ime || eiDelay) { // IME disabled or the last instruction was EI
		eiDelay = false;
		return;
	}
	u8 requested = mmu.read_u8(CPU_INTERRUPT_FLAG);
	if (requested > 0) {
		u8 enabled = mmu.read_u8(CPU_INTERRUPT_ENABLE);
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
	ime = false; // IME = disabled
	mmu.write_u8(CPU_INTERRUPT_FLAG, mmu.read_u8(CPU_INTERRUPT_FLAG)& ~interrupt); // Reset bit in Interrupt Request Register
	reg.sp -= 2;
	mmu.write_u16(reg.sp, reg.pc); // Push PC to the stack

	// Interrupts have specific service routines at defined memory locations
	switch(interrupt) {
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

		default: break;

	}
}
