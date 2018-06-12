/* gb: a Gameboy Emulator
   Author: Don Freiday */

#ifndef GB_CPU
#define GB_CPU

#include <stdio.h>
#include "common.h"
#include "mmu.h"

class CPU {

public:
  struct registers {
		union {
			struct {
				u8 f;
				u8 a;
			};
			u16 af;
		};

		union {
			struct {
				u8 c;
				u8 b;
			};
			u16 bc;
		};

		union {
			struct {
				u8 e;
				u8 d;
			};
			u16 de;
		};

		union {
			struct {
				u8 l;
				u8 h;
			};
			u16 hl;
		};

		u16 pc, sp;
  } reg;

  // cpu_clock_m: machine cycles, fetch->decode->execute->store; one m time is 4 t time
  // cpu_clock_t: cpu cycles; the gb cpu runs at 4194304 Hz
  u32 cpu_clock_m, cpu_clock_t;
  u32 cycles;

  bool interrupt;

  MMU mmu;

  CPU();
  void reset();

  template <typename t>
  void decrementReg(t &reg1);

  template <typename t>
  void incrementReg(t &reg1);

  template <typename t>
  void rotateRightCarry(t &reg1);

  template <typename t>
  void rotateLeft(t &reg1);

  template <typename t>
  void subtract(t num);

  void bitTestReg(u8 reg1, u8 pos);

  bool execute();
  bool execute_CB(u8 op); // execute extended instruction set

  void checkInterrupts();
  void doInterrupt(u8 interrupt);

  // Debug flags
  bool debug, debugVerbose;

  struct instruction { // thx to cinoop
    char const *disassembly;
    u8 operandLength;
    u8 cycles;
  };
};

#endif
