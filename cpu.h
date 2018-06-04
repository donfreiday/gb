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

  void decrement_reg(u8 &reg1);
  void rotate_right_carry(u8 &reg1);

  bool execute();
  bool execute_CB(u8 op); // execute extended instruction set

  void checkInterrupts();
  void doInterrupt(u8 interrupt);

  struct instruction { // thx to cinoop
    char const *disassembly;
    u8 operandLength;
    u8 cycles;
  };

};

#endif
