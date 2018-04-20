#ifndef GB_CPU
#define GB_CPU

#include <stdio.h>
#include "common.h"
#include "mmu.h"

class CPU {

public:
  struct registers {
    u8 a;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
    u8 f;
    u16 pc;
    u16 sp;
  } reg;

  u32 clock_m, clock_t;
  u32 cycles;

  MMU mmu;

  CPU();
  void reset();
  void execute(u8 op);
};

#endif
