#ifndef GB_CPU
#define GB_CPU

#include <stdio.h>
#include "common.h"

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
    u8 m;
    u8 t;
  } reg;

  u8* memory;

  CPU();
  void reset();
  void execute(u8 op);
};

#endif
