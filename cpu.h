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

  u32 cpu_clock_m, cpu_clock_t;
  u32 cycles;

  bool interrupt;

  MMU mmu;

  CPU();
  void reset();

  void decrement_reg(u8 &reg1);

  bool execute(u8 op);
};

#endif
