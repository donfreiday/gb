#ifndef GB_MMU
#define GB_MMU

#include <fstream>
#include <stdio.h>
#include "common.h"

class MMU {

public:
  struct cart {
  };

  unsigned char memory[0xFFFF]; // 16bit address bus

  MMU();
  bool load(std::string filename);

  u8 read_u8(u16 address);
  u16 read_u16(u16 address);
  void write_u8(u16 address, u8 value);
  void write_u16(u16 address, u16 value);

};

#endif
