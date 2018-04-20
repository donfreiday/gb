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

  u32 clock_m, clock_t;
  u32 cycles;

  MMU mmu;

  CPU();
  void reset();
  void execute(u8 op);
};

#endif
