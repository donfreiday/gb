/* gb: a Gameboy Emulator
   Author: Don Freiday */

#ifndef GB_MMU
#define GB_MMU

#include <stdio.h>
#include <fstream>
#include "common.h"
#include "joypad.h"

class MMU {
 public:
  struct cart {};

  unsigned char memory[0xFFFF];  // 16bit address bus

  MMU();
  bool load();

  u8 read_u8(u16 address);
  u16 read_u16(u16 address);
  void write_u8(u16 address, u8 value);
  void write_u16(u16 address, u16 value);

  int getRomSize();

  Joypad* joypad;

  bool unmapBootrom;

 private:
  void DMA(u8 src);
};

#endif
