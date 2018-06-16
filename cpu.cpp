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

// thx to cinoop
// char const *disassembly; u8 operandLength; u8 cycles;
CPU::instruction instructions[256] = {
	{ "NOP", 0, 4 },                      // 0x00
	{ "LD BC, 0x%04X", 2, 12 },           // 0x01
	{ "LD (BC), A", 0, 8 },               // 0x02
	{ "INC BC", 0, 8 },                   // 0x03
	{ "INC B", 0, 4 },                    // 0x04
	{ "DEC B", 0, 4 },                    // 0x05
	{ "LD B, 0x%02X", 1, 8},              // 0x06
	{ "RLCA", 0, 8 },                     // 0x07
	{ "LD (0x%04X), SP", 2, 20 },         // 0x08
	{ "ADD HL, BC", 0, 8 },               // 0x09
	{ "LD A, (BC)", 0, 8 },               // 0x0a
	{ "DEC BC", 0, 8 },                   // 0x0b
	{ "INC C", 0, 4 },                    // 0x0c
	{ "DEC C", 0, 4 },                    // 0x0d
	{ "LD C, 0x%02X", 1, 8 },             // 0x0e
	{ "RRCA", 0, 8 },                     // 0x0f
	{ "STOP", 1, 4 },                     // 0x10
	{ "LD DE, 0x%04X", 2, 12 },           // 0x11
	{ "LD (DE), A", 0, 8 },               // 0x12
	{ "INC DE", 0, 8 },                   // 0x13
	{ "INC D", 0, 4 },                    // 0x14
	{ "DEC D", 0, 4 },                    // 0x15
	{ "LD D, 0x%02X", 1, 8 },             // 0x16
	{ "RLA", 0, 8 },                      // 0x17
	{ "JR 0x%02X", 1, 8 },                // 0x18
	{ "ADD HL, DE", 0, 8 },               // 0x19
	{ "LD A, (DE)", 0, 8 },               // 0x1a
	{ "DEC DE", 0, 8 },                   // 0x1b
	{ "INC E", 0, 4 },                    // 0x1c
	{ "DEC E", 0, 4 },                    // 0x1d
	{ "LD E, 0x%02X", 1, 8 },             // 0x1e
	{ "RRA", 0, 8 },                      // 0x1f
	{ "JR NZ, 0x%02X", 1, 0 },            // 0x20
	{ "LD HL, 0x%04X", 2, 12 },           // 0x21
	{ "LDI (HL), A", 0, 8 },              // 0x22
	{ "INC HL", 0, 8 },                   // 0x23
	{ "INC H", 0, 4 },                    // 0x24
	{ "DEC H", 0, 4 },                    // 0x25
	{ "LD H, 0x%02X", 1, 8 },             // 0x26
	{ "DAA", 0, 4 },                      // 0x27
	{ "JR Z, 0x%02X", 1, 0 },             // 0x28
	{ "ADD HL, HL", 0, 8 },               // 0x29
	{ "LDI A, (HL)", 0, 8 },              // 0x2a
	{ "DEC HL", 0, 8 },                   // 0x2b
	{ "INC L", 0, 4 },                    // 0x2c
	{ "DEC L", 0, 4 },                    // 0x2d
	{ "LD L, 0x%02X", 1, 8 },             // 0x2e
	{ "CPL", 0, 4 },                      // 0x2f
	{ "JR NC, 0x%02X", 1, 8 },            // 0x30
	{ "LD SP, 0x%04X", 2, 12 },           // 0x31
	{ "LDD (HL), A", 0, 8 },              // 0x32
	{ "INC SP", 0, 8 },                   // 0x33
	{ "INC (HL)", 0, 12 },                // 0x34
	{ "DEC (HL)", 0, 12 },                // 0x35
	{ "LD (HL), 0x%02X", 1, 12 },         // 0x36
	{ "SCF", 0, 4 },                      // 0x37
	{ "JR C, 0x%02X", 1, 0 },             // 0x38
	{ "ADD HL, SP", 0, 8 },               // 0x39
	{ "LDD A, (HL)", 0, 8 },              // 0x3a
	{ "DEC SP", 0, 8 },                   // 0x3b
	{ "INC A", 0, 4 },                    // 0x3c
	{ "DEC A", 0, 4 },                    // 0x3d
	{ "LD A, 0x%02X", 1, 8 },             // 0x3e
	{ "CCF", 0, 4 },                      // 0x3f
	{ "LD B, B", 0, 4 },                  // 0x40
	{ "LD B, C", 0, 4 },                  // 0x41
	{ "LD B, D", 0, 4 },                  // 0x42
	{ "LD B, E", 0, 4 },                  // 0x43
	{ "LD B, H", 0, 4 },                  // 0x44
	{ "LD B, L", 0, 4 },                  // 0x45
	{ "LD B, (HL)", 0, 8 },               // 0x46
	{ "LD B, A", 0, 4 },                  // 0x47
	{ "LD C, B", 0, 4 },                  // 0x48
	{ "LD C, C", 0, 4 },                  // 0x49
	{ "LD C, D", 0, 4 },                  // 0x4a
	{ "LD C, E", 0, 4 },                  // 0x4b
	{ "LD C, H", 0, 4 },                  // 0x4c
	{ "LD C, L", 0, 4 },                  // 0x4d
	{ "LD C, (HL)", 0, 8 },               // 0x4e
	{ "LD C, A", 0, 4 },                  // 0x4f
	{ "LD D, B", 0, 4 },                  // 0x50
	{ "LD D, C", 0, 4 },                  // 0x51
	{ "LD D, D", 0, 4 },                  // 0x52
	{ "LD D, E", 0, 4 },                  // 0x53
	{ "LD D, H", 0, 4 },                  // 0x54
	{ "LD D, L", 0, 4 },                  // 0x55
	{ "LD D, (HL)", 0, 8 },               // 0x56
	{ "LD D, A", 0, 4 },                  // 0x57
	{ "LD E, B", 0, 4 },                  // 0x58
	{ "LD E, C", 0, 4 },                  // 0x59
	{ "LD E, D", 0, 4 },                  // 0x5a
	{ "LD E, E", 0, 4 },                  // 0x5b
	{ "LD E, H", 0, 4 },                  // 0x5c
	{ "LD E, L", 0, 4 },                  // 0x5d
	{ "LD E, (HL)", 0, 8 },               // 0x5e
	{ "LD E, A", 0, 4 },                  // 0x5f
	{ "LD H, B", 0, 4 },                  // 0x60
	{ "LD H, C", 0, 4 },                  // 0x61
	{ "LD H, D", 0, 4 },                  // 0x62
	{ "LD H, E", 0, 4 },                  // 0x63
	{ "LD H, H", 0, 4 },                  // 0x64
	{ "LD H, L", 0, 4 },                  // 0x65
	{ "LD H, (HL)", 0, 8 },               // 0x66
	{ "LD H, A", 0, 4 },                  // 0x67
	{ "LD L, B", 0, 4 },                  // 0x68
	{ "LD L, C", 0, 4 },                  // 0x69
	{ "LD L, D", 0, 4 },                  // 0x6a
	{ "LD L, E", 0, 4 },                  // 0x6b
	{ "LD L, H", 0, 4 },                  // 0x6c
	{ "LD L, L", 0, 4 },                  // 0x6d
	{ "LD L, (HL)", 0, 8 },               // 0x6e
	{ "LD L, A", 0, 4 },                  // 0x6f
	{ "LD (HL), B", 0, 8 },               // 0x70
	{ "LD (HL), C", 0, 8 },               // 0x71
	{ "LD (HL), D", 0, 8 },               // 0x72
	{ "LD (HL), E", 0, 8 },               // 0x73
	{ "LD (HL), H", 0, 8 },               // 0x74
	{ "LD (HL), L", 0, 8 },               // 0x75
	{ "HALT", 0, 4 },                     // 0x76
	{ "LD (HL), A", 0, 8 },               // 0x77
	{ "LD A, B", 0, 4 },                  // 0x78
	{ "LD A, C", 0, 4 },                  // 0x79
	{ "LD A, D", 0, 4 },                  // 0x7a
	{ "LD A, E", 0, 4 },                  // 0x7b
	{ "LD A, H", 0, 4 },                  // 0x7c
  { "LD A, L", 0, 4 },                  // 0x7d
	{ "LD A, (HL)", 0, 8 },               // 0x7e
	{ "LD A, A", 0, 4 },                  // 0x7f
	{ "ADD A, B", 0, 4 },                 // 0x80
	{ "ADD A, C", 0, 4 },                 // 0x81
	{ "ADD A, D", 0, 4 },                 // 0x82
	{ "ADD A, E", 0, 4 },                 // 0x83
	{ "ADD A, H", 0, 4 },                 // 0x84
	{ "ADD A, L", 0, 4 },                 // 0x85
	{ "ADD A, (HL)", 0, 8 },              // 0x86
	{ "ADD A", 0, 4 },                    // 0x87
	{ "ADC B", 0, 4 },                    // 0x88
	{ "ADC C", 0, 4 },                    // 0x89
	{ "ADC D", 0, 4 },                    // 0x8a
	{ "ADC E", 0, 4 },                    // 0x8b
	{ "ADC H", 0, 4 },                    // 0x8c
	{ "ADC L", 0, 4 },                    // 0x8d
	{ "ADC (HL)", 0, 8 },                 // 0x8e
	{ "ADC A", 0, 4 },                    // 0x8f
	{ "SUB B", 0, 4 },                    // 0x90
	{ "SUB C", 0, 4 },                    // 0x91
	{ "SUB D", 0, 4 },                    // 0x92
	{ "SUB E", 0, 4 },                    // 0x93
	{ "SUB H", 0, 4 },                    // 0x94
	{ "SUB L", 0, 4 },                    // 0x95
	{ "SUB (HL)", 0, 8 },                 // 0x96
	{ "SUB A", 0, 4 },                    // 0x97
	{ "SBC B", 0, 4 },                    // 0x98
	{ "SBC C", 0, 4 },                    // 0x99
	{ "SBC D", 0, 4 },                    // 0x9a
	{ "SBC E", 0, 4 },                    // 0x9b
	{ "SBC H", 0, 4 },                    // 0x9c
	{ "SBC L", 0, 4 },                    // 0x9d
	{ "SBC (HL)", 0, 8 },                 // 0x9e
	{ "SBC A", 0, 4 },                    // 0x9f
	{ "AND B", 0, 4 },                    // 0xa0
	{ "AND C", 0, 4 },                    // 0xa1
	{ "AND D", 0, 4 },                    // 0xa2
	{ "AND E", 0, 4 },                    // 0xa3
	{ "AND H", 0, 4 },                    // 0xa4
	{ "AND L", 0, 4 },                    // 0xa5
	{ "AND (HL)", 0, 8 },                 // 0xa6
	{ "AND A", 0, 4 },                    // 0xa7
	{ "XOR B", 0, 4 },                    // 0xa8
	{ "XOR C", 0, 4 },                    // 0xa9
	{ "XOR D", 0, 4 },                    // 0xaa
	{ "XOR E", 0, 4 },                    // 0xab
	{ "XOR H", 0, 4 },                    // 0xac
	{ "XOR L", 0, 4 },                    // 0xad
	{ "XOR (HL)", 0, 8 },                 // 0xae
	{ "XOR A", 0, 4 },                    // 0xaf
	{ "OR B", 0, 4 },                     // 0xb0
	{ "OR C", 0, 4 },                     // 0xb1
  { "OR D", 0, 4 },                     // 0xb2
	{ "OR E", 0, 4 },                     // 0xb3
  { "OR H", 0, 4 },                     // 0xb4
  { "OR L", 0, 4 },                     // 0xb5
  { "OR (HL)", 0, 8 },                  // 0xb6
  { "OR A", 0, 4 },                     // 0xb7
  { "CP B", 0, 4 },                     // 0xb8
  { "CP C", 0, 4 },                     // 0xb9
	{ "CP D", 0, 4 },                     // 0xba
	{ "CP E", 0, 4 },                     // 0xbb
	{ "CP H", 0, 4 },                     // 0xbc
	{ "CP L", 0, 4 },                     // 0xbd
	{ "CP (HL)", 0, 8 },                  // 0xbe
	{ "CP A", 0, 4 },                     // 0xbf
	{ "RET NZ", 0, 0 },                   // 0xc0
	{ "POP BC", 0, 12 },                  // 0xc1
	{ "JP NZ, 0x%04X", 2, 0 },            // 0xc2
	{ "JP 0x%04X", 2, 12 },               // 0xc3
	{ "CALL NZ, 0x%04X", 2, 0 },          // 0xc4
	{ "PUSH BC", 0, 16 },                 // 0xc5
	{ "ADD A, 0x%02X", 1, 8 },            // 0xc6
	{ "RST 0x00", 0, 16 },                // 0xc7
	{ "RET Z", 0, 0 },                    // 0xc8
	{ "RET", 0, 4 },                      // 0xc9
	{ "JP Z, 0x%04X", 2, 0 },             // 0xca
	{ "CB %02X", 1, 0 },                  // 0xcb
	{ "CALL Z, 0x%04X", 2, 0 },           // 0xcc
	{ "CALL 0x%04X", 2, 12 },             // 0xcd
	{ "ADC 0x%02X", 1, 8 },               // 0xce
	{ "RST 0x08", 0, 16 },                // 0xcf
	{ "RET NC", 0, 0 },                   // 0xd0
	{ "POP DE", 0, 12 },                  // 0xd1
	{ "JP NC, 0x%04X", 2, 0 },            // 0xd2
	{ "UNKNOWN", 0, 0 },                  // 0xd3
	{ "CALL NC, 0x%04X", 2, 0 },          // 0xd4
	{ "PUSH DE", 0, 16 },                 // 0xd5
	{ "SUB 0x%02X", 1, 8 },               // 0xd6
	{ "RST 0x10", 0, 16 },                // 0xd7
	{ "RET C", 0, 0 },                    // 0xd8
	{ "RETI", 0, 16 },                    // 0xd9
	{ "JP C, 0x%04X", 2, 0 },             // 0xda
	{ "UNKNOWN", 0, 0 },                  // 0xdb
	{ "CALL C, 0x%04X", 2, 0 },           // 0xdc
	{ "UNKNOWN", 0, 0 },                  // 0xdd
	{ "SBC 0x%02X", 1, 8 },               // 0xde
	{ "RST 0x18", 0, 16 },                // 0xdf
	{ "LD (0xFF00 + 0x%02X), A", 1, 12 }, // 0xe0
	{ "POP HL", 0, 12 },                  // 0xe1
	{ "LD (0xFF00 + C), A", 0, 8 },       // 0xe2
	{ "UNKNOWN", 0, 0 },                  // 0xe3
	{ "UNKNOWN", 0, 0 },                  // 0xe4
	{ "PUSH HL", 0, 16 },                 // 0xe5
	{ "AND 0x%02X", 1, 8 },               // 0xe6
	{ "RST 0x20", 0, 16 },                // 0xe7
	{ "ADD SP,0x%02X", 1, 16 },           // 0xe8
	{ "JP HL", 0, 4 },                    // 0xe9
	{ "LD (0x%04X), A", 2, 16 },          // 0xea
	{ "UNKNOWN", 0, 0 },                  // 0xeb
	{ "UNKNOWN", 0, 0 },                  // 0xec
	{ "UNKNOWN", 0, 0 },                  // 0xed
	{ "XOR 0x%02X", 1, 8 },               // 0xee
	{ "RST 0x28", 0, 16 },                // 0xef
	{ "LD A, (0xFF00 + 0x%02X)", 1, 12 }, // 0xf0
	{ "POP AF", 0, 12 },                  // 0xf1
	{ "LD A, (0xFF00 + C)", 0, 8 },       // 0xf2
	{ "DI", 0, 4 },                       // 0xf3
	{ "UNKNOWN", 0, 0 },                  // 0xf4
	{ "PUSH AF", 0, 16 },                 // 0xf5
	{ "OR 0x%02X", 1, 8 },                // 0xf6
	{ "RST 0x30", 0, 16 },                // 0xf7
	{ "LD HL, SP+0x%02X", 1, 12 },        // 0xf8
	{ "LD SP, HL", 0, 8 },                // 0xf9
	{ "LD A, (0x%04X)", 2, 6 },           // 0xfa
	{ "EI", 0, 4 },                       // 0xfb
	{ "UNKNOWN", 0, 0 },                  // 0xfc
	{ "UNKNOWN", 0, 0 },                  // 0xfd
	{ "CP 0x%02X", 1, 8 },                // 0xfe
	{ "RST 0x38", 0, 16 },                // 0xff
};
CPU::instruction instructions_CB[256] = {
	{ "RLC B", 0, 8 },                    // 0x00
	{ "RLC C", 0, 8 },                    // 0x01
	{ "RLC D", 0, 8 },                    // 0x02
	{ "RLC E", 0, 8 },                    // 0x03
	{ "RLC H", 0, 8 },                    // 0x04
	{ "RLC L", 0, 8 },                    // 0x05
	{ "RLC (HL)", 0, 16 },                // 0x06
	{ "RLC A", 0, 8 },                    // 0x07
	{ "RRC B", 0, 8 },                    // 0x08
	{ "RRC C", 0, 8 },                    // 0x09
	{ "RRC D", 0, 8 },                    // 0x0a
	{ "RRC E", 0, 8 },                    // 0x0b
	{ "RRC H", 0, 8 },                    // 0x0c
	{ "RRC L", 0, 8 },                    // 0x0d
	{ "RRC (HL)", 0, 16 },                // 0x0e
	{ "RRC A", 0, 8 },                    // 0x0f
	{ "RL B", 0, 8 },                     // 0x10
	{ "RL C", 0, 8 },                     // 0x11
	{ "RL D", 0, 8 },                     // 0x12
	{ "RL E", 0, 8 },                     // 0x13
	{ "RL H", 0, 8 },                     // 0x14
	{ "RL L", 0, 8 },                     // 0x15
	{ "RL (HL)", 0, 16 },                 // 0x16
	{ "RL A", 0, 8 },                     // 0x17
	{ "RR B", 0, 8 },                     // 0x18
	{ "RR C", 0, 8 },                     // 0x19
	{ "RR D", 0, 8 },                     // 0x1a
	{ "RR E", 0, 8 },                     // 0x1b
	{ "RR H", 0, 8 },                     // 0x1c
	{ "RR L", 0, 8 },                     // 0x1d
	{ "RR (HL)", 0, 16 },                 // 0x1e
	{ "RR A", 0, 8 },                     // 0x1f
	{ "SLA B", 0, 8 },                    // 0x20
	{ "SLA C", 0, 8 },                    // 0x21
	{ "SLA D", 0, 8 },                    // 0x22
	{ "SLA E", 0, 8 },                    // 0x23
	{ "SLA H", 0, 8 },                    // 0x24
	{ "SLA L", 0, 8 },                    // 0x25
	{ "SLA (HL)", 0, 16 },                // 0x26
	{ "SLA A", 0, 8 },                    // 0x27
	{ "SRA B", 0, 8 },                    // 0x28
	{ "SRA C", 0, 8 },                    // 0x29
	{ "SRA D", 0, 8 },                    // 0x2a
	{ "SRA E", 0, 8 },                    // 0x2b
	{ "SRA H", 0, 8 },                    // 0x2c
	{ "SRA L", 0, 8 },                    // 0x2d
	{ "SRA (HL)", 0, 16 },                // 0x2e
	{ "SRA A", 0, 8 },                    // 0x2f
	{ "SWAP B", 0, 8 },                   // 0x30
	{ "SWAP C", 0, 8 },                   // 0x31
	{ "SWAP D", 0, 8 },                   // 0x32
	{ "SWAP E", 0, 8 },                   // 0x33
	{ "SWAP H", 0, 8 },                   // 0x34
	{ "SWAP L", 0, 8 },                   // 0x35
	{ "SWAP (HL)", 0, 16 },               // 0x36
	{ "SWAP A", 0, 8 },                   // 0x37
	{ "SRL B", 0, 8 },                    // 0x38
	{ "SRL C", 0, 8 },                    // 0x39
	{ "SRL D", 0, 8 },                    // 0x3a
	{ "SRL E", 0, 8 },                    // 0x3b
	{ "SRL H", 0, 8 },                    // 0x3c
	{ "SRL L", 0, 8 },                    // 0x3d
	{ "SRL (HL)", 0, 16 },                // 0x3e
	{ "SRL A", 0, 8 },                    // 0x3f
	{ "BIT 0, B", 0, 8 },                 // 0x40
	{ "BIT 0, C", 0, 8 },                 // 0x41
	{ "BIT 0, D", 0, 8 },                 // 0x42
	{ "BIT 0, E", 0, 8 },                 // 0x43
	{ "BIT 0, H", 0, 8 },                 // 0x44
	{ "BIT 0, L", 0, 8 },                 // 0x45
	{ "BIT 0, (HL)", 0, 12 },             // 0x46
	{ "BIT 0, A", 0, 8 },                 // 0x47
	{ "BIT 1, B", 0, 8 },                 // 0x48
	{ "BIT 1, C", 0, 8 },                 // 0x49
	{ "BIT 1, D", 0, 8 },                 // 0x4a
	{ "BIT 1, E", 0, 8 },                 // 0x4b
	{ "BIT 1, H", 0, 8 },                 // 0x4c
	{ "BIT 1, L", 0, 8 },                 // 0x4d
	{ "BIT 1, (HL)", 0, 12 },             // 0x4e
	{ "BIT 1, A", 0, 8 },                 // 0x4f
	{ "BIT 2, B", 0, 8 },                 // 0x50
	{ "BIT 2, C", 0, 8 },                 // 0x51
	{ "BIT 2, D", 0, 8 },                 // 0x52
	{ "BIT 2, E", 0, 8 },                 // 0x53
	{ "BIT 2, H", 0, 8 },                 // 0x54
	{ "BIT 2, L", 0, 8 },                 // 0x55
	{ "BIT 2, (HL)", 0, 12 },             // 0x56
	{ "BIT 2, A", 0, 8 },                 // 0x57
	{ "BIT 3, B", 0, 8 },                 // 0x58
	{ "BIT 3, C", 0, 8 },                 // 0x59
	{ "BIT 3, D", 0, 8 },                 // 0x5a
	{ "BIT 3, E", 0, 8 },                 // 0x5b
	{ "BIT 3, H", 0, 8 },                 // 0x5c
	{ "BIT 3, L", 0, 8 },                 // 0x5d
	{ "BIT 3, (HL)", 0, 12 },             // 0x5e
	{ "BIT 3, A", 0, 8 },                 // 0x5f
	{ "BIT 4, B", 0, 8 },                 // 0x60
	{ "BIT 4, C", 0, 8 },                 // 0x61
	{ "BIT 4, D", 0, 8 },                 // 0x62
	{ "BIT 4, E", 0, 8 },                 // 0x63
	{ "BIT 4, H", 0, 8 },                 // 0x64
	{ "BIT 4, L", 0, 8 },                 // 0x65
	{ "BIT 4, (HL)", 0, 12 },             // 0x66
	{ "BIT 4, A", 0, 8 },                 // 0x67
	{ "BIT 5, B", 0, 8 },                 // 0x68
	{ "BIT 5, C", 0, 8 },                 // 0x69
	{ "BIT 5, D", 0, 8 },                 // 0x6a
	{ "BIT 5, E", 0, 8 },                 // 0x6b
	{ "BIT 5, H", 0, 8 },                 // 0x6c
	{ "BIT 5, L", 0, 8 },                 // 0x6d
	{ "BIT 5, (HL)", 0, 12 },             // 0x6e
	{ "BIT 5, A", 0, 8 },                 // 0x6f
	{ "BIT 6, B", 0, 8 },                 // 0x70
	{ "BIT 6, C", 0, 8 },                 // 0x71
	{ "BIT 6, D", 0, 8 },                 // 0x72
	{ "BIT 6, E", 0, 8 },                 // 0x73
	{ "BIT 6, H", 0, 8 },                 // 0x74
	{ "BIT 6, L", 0, 8 },                 // 0x75
	{ "BIT 6, (HL)", 0, 12 },             // 0x76
	{ "BIT 6, A", 0, 8 },                 // 0x77
	{ "BIT 7, B", 0, 8 },                 // 0x78
	{ "BIT 7, C", 0, 8 },                 // 0x79
	{ "BIT 7, D", 0, 8 },                 // 0x7a
	{ "BIT 7, E", 0, 8 },                 // 0x7b
	{ "BIT 7, H", 0, 8 },                 // 0x7c
	{ "BIT 7, L", 0, 8 },                 // 0x7d
	{ "BIT 7, (HL)", 0, 12 },             // 0x7e
	{ "BIT 7, A", 0, 8 },                 // 0x7f
	{ "RES 0, B", 0, 8 },                 // 0x80
	{ "RES 0, C", 0, 8 },                 // 0x81
	{ "RES 0, D", 0, 8 },                 // 0x82
	{ "RES 0, E", 0, 8 },                 // 0x83
	{ "RES 0, H", 0, 8 },                 // 0x84
	{ "RES 0, L", 0, 8 },                 // 0x85
	{ "RES 0, (HL)", 0, 12 },             // 0x86
	{ "RES 0, A", 0, 8 },                 // 0x87
	{ "RES 1, B", 0, 8 },                 // 0x88
	{ "RES 1, C", 0, 8 },                 // 0x89
	{ "RES 1, D", 0, 8 },                 // 0x8a
	{ "RES 1, E", 0, 8 },                 // 0x8b
	{ "RES 1, H", 0, 8 },                 // 0x8c
	{ "RES 1, L", 0, 8 },                 // 0x8d
	{ "RES 1, (HL)", 0, 12 },             // 0x8e
	{ "RES 1, A", 0, 8 },                 // 0x8f
	{ "RES 2, B", 0, 8 },                 // 0x90
	{ "RES 2, C", 0, 8 },                 // 0x91
	{ "RES 2, D", 0, 8 },                 // 0x92
	{ "RES 2, E", 0, 8 },                 // 0x93
	{ "RES 2, H", 0, 8 },                 // 0x94
	{ "RES 2, L", 0, 8 },                 // 0x95
	{ "RES 2, (HL)", 0, 12 },             // 0x96
	{ "RES 2, A", 0, 8 },                 // 0x97
	{ "RES 3, B", 0, 8 },                 // 0x98
	{ "RES 3, C", 0, 8 },                 // 0x99
	{ "RES 3, D", 0, 8 },                 // 0x9a
	{ "RES 3, E", 0, 8 },                 // 0x9b
	{ "RES 3, H", 0, 8 },                 // 0x9c
	{ "RES 3, L", 0, 8 },                 // 0x9d
	{ "RES 3, (HL)", 0, 12 },             // 0x9e
	{ "RES 3, A", 0, 8 },                 // 0x9f
	{ "RES 4, B", 0, 8 },                 // 0xa0
	{ "RES 4, C", 0, 8 },                 // 0xa1
	{ "RES 4, D", 0, 8 },                 // 0xa2
	{ "RES 4, E", 0, 8 },                 // 0xa3
	{ "RES 4, H", 0, 8 },                 // 0xa4
	{ "RES 4, L", 0, 8 },                 // 0xa5
	{ "RES 4, (HL)", 0, 12 },             // 0xa6
	{ "RES 4, A", 0, 8 },                 // 0xa7
	{ "RES 5, B", 0, 8 },                 // 0xa8
	{ "RES 5, C", 0, 8 },                 // 0xa9
	{ "RES 5, D", 0, 8 },                 // 0xaa
	{ "RES 5, E", 0, 8 },                 // 0xab
	{ "RES 5, H", 0, 8 },                 // 0xac
	{ "RES 5, L", 0, 8 },                 // 0xad
	{ "RES 5, (HL)", 0, 12 },             // 0xae
	{ "RES 5, A", 0, 8 },                 // 0xaf
	{ "RES 6, B", 0, 8 },                 // 0xb0
	{ "RES 6, C", 0, 8 },                 // 0xb1
	{ "RES 6, D", 0, 8 },                 // 0xb2
	{ "RES 6, E", 0, 8 },                 // 0xb3
	{ "RES 6, H", 0, 8 },                 // 0xb4
	{ "RES 6, L", 0, 8 },                 // 0xb5
	{ "RES 6, (HL)", 0, 12 },             // 0xb6
	{ "RES 6, A", 0, 8 },                 // 0xb7
	{ "RES 7, B", 0, 8 },                 // 0xb8
	{ "RES 7, C", 0, 8 },                 // 0xb9
	{ "RES 7, D", 0, 8 },                 // 0xba
	{ "RES 7, E", 0, 8 },                 // 0xbb
	{ "RES 7, H", 0, 8 },                 // 0xbc
	{ "RES 7, L", 0, 8 },                 // 0xbd
	{ "RES 7, (HL)", 0, 12 },             // 0xbe
	{ "RES 7, A", 0, 8 },                 // 0xbf
	{ "SET 0, B", 0, 8 },                 // 0xc0
	{ "SET 0, C", 0, 8 },                 // 0xc1
	{ "SET 0, D", 0, 8 },                 // 0xc2
	{ "SET 0, E", 0, 8 },                 // 0xc3
	{ "SET 0, H", 0, 8 },                 // 0xc4
	{ "SET 0, L", 0, 8 },                 // 0xc5
	{ "SET 0, (HL)", 0, 12 },             // 0xc6
	{ "SET 0, A", 0, 8 },                 // 0xc7
	{ "SET 1, B", 0, 8 },                 // 0xc8
	{ "SET 1, C", 0, 8 },                 // 0xc9
	{ "SET 1, D", 0, 8 },                 // 0xca
	{ "SET 1, E", 0, 8 },                 // 0xcb
	{ "SET 1, H", 0, 8 },                 // 0xcc
	{ "SET 1, L", 0, 8 },                 // 0xcd
	{ "SET 1, (HL)", 0, 12 },             // 0xce
	{ "SET 1, A", 0, 8 },                 // 0xcf
	{ "SET 2, B", 0, 8 },                 // 0xd0
	{ "SET 2, C", 0, 8 },                 // 0xd1
	{ "SET 2, D", 0, 8 },                 // 0xd2
	{ "SET 2, E", 0, 8 },                 // 0xd3
	{ "SET 2, H", 0, 8 },                 // 0xd4
	{ "SET 2, L", 0, 8 },                 // 0xd5
	{ "SET 2, (HL)", 0, 12 },             // 0xd6
	{ "SET 2, A", 0, 8 },                 // 0xd7
	{ "SET 3, B", 0, 8 },                 // 0xd8
	{ "SET 3, C", 0, 8 },                 // 0xd9
	{ "SET 3, D", 0, 8 },                 // 0xda
	{ "SET 3, E", 0, 8 },                 // 0xdb
	{ "SET 3, H", 0, 8 },                 // 0xdc
	{ "SET 3, L", 0, 8 },                 // 0xdd
	{ "SET 3, (HL)", 0, 12 },             // 0xde
	{ "SET 3, A", 0, 8 },                 // 0xdf
	{ "SET 4, B", 0, 8 },                 // 0xe0
	{ "SET 4, C", 0, 8 },                 // 0xe1
	{ "SET 4, D", 0, 8 },                 // 0xe2
	{ "SET 4, E", 0, 8 },                 // 0xe3
	{ "SET 4, H", 0, 8 },                 // 0xe4
	{ "SET 4, L", 0, 8 },                 // 0xe5
	{ "SET 4, (HL)", 0, 12 },             // 0xe6
	{ "SET 4, A", 0, 8 },                 // 0xe7
	{ "SET 5, B", 0, 8 },                 // 0xe8
	{ "SET 5, C", 0, 8 },                 // 0xe9
	{ "SET 5, D", 0, 8 },                 // 0xea
	{ "SET 5, E", 0, 8 },                 // 0xeb
	{ "SET 5, H", 0, 8 },                 // 0xec
	{ "SET 5, L", 0, 8 },                 // 0xed
	{ "SET 5, (HL)", 0, 12 },             // 0xee
	{ "SET 5, A", 0, 8 },                 // 0xef
	{ "SET 6, B", 0, 8 },                 // 0xf0
	{ "SET 6, C", 0, 8 },                 // 0xf1
	{ "SET 6, D", 0, 8 },                 // 0xf2
	{ "SET 6, E", 0, 8 },                 // 0xf3
	{ "SET 6, H", 0, 8 },                 // 0xf4
	{ "SET 6, L", 0, 8 },                 // 0xf5
	{ "SET 6, (HL)", 0, 12 },             // 0xf6
	{ "SET 6, A", 0, 8 },                 // 0xf7
	{ "SET 7, B", 0, 8 },                 // 0xf8
	{ "SET 7, C", 0, 8 },                 // 0xf9
	{ "SET 7, D", 0, 8 },                 // 0xfa
	{ "SET 7, E", 0, 8 },                 // 0xfb
	{ "SET 7, H", 0, 8 },                 // 0xfc
	{ "SET 7, L", 0, 8 },                 // 0xfd
	{ "SET 7, (HL)", 0, 12 },             // 0xfe
	{ "SET 7, A", 0, 8 },                 // 0xff
};

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
	reg.a -= n;
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

  // Update clocks and cycle count
  cpu_clock_t = instructions[op].cycles;
  cpu_clock_m = instructions[op].cycles / 4; // 4 CPU cycles is one machine cycle
  cycles += instructions[op].cycles;

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
		printf("\naf=%04X bc=%04X de=%04X hl=%04X sp=%04X pc=%04X ime=%04x", reg.af, reg.bc, reg.de, reg.hl, reg.sp, reg.pc, ime);
		printf(" flags=");
		bitTest(reg.f, FLAG_ZERO) ? printf("Z") : printf("z");
		bitTest(reg.f, FLAG_SUBTRACT) ? printf("N") : printf("n");
		bitTest(reg.f, FLAG_HALF_CARRY) ? printf("H") : printf("h");
		bitTest(reg.f, FLAG_CARRY) ? printf("C") : printf("c");
	}
	if(debug || debugVerbose) { printf("\n"); }

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
