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
  int load(std::string filename);

  u8 read_byte(u16 address);
  u16 read_word(u16 address);
  void write_byte(u16 address, u8 value);
  void write_word(u16 address, u16 value);

};

#endif
