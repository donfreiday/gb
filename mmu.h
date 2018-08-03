/* gb: a Gameboy Emulator
   Author: Don Freiday */

#ifndef GB_MMU
#define GB_MMU

#include <fstream>
#include <vector>
#include "common.h"
#include "joypad.h"

class MMU {
 public:
  struct cart {};

  std::vector<u8> memory;  // 16bit address bus
  std::vector<u8> bios; 
  std::vector<u8> rom; 

  MMU();
  bool load(char* filename);
  void reset();

  u8 read_u8(u16 address);
  u16 read_u16(u16 address);
  void write_u8(u16 address, u8 value);
  void write_u16(u16 address, u16 value);

  int getRomSize();

  bool memMapChanged; // Flag for debugger

  Joypad* joypad;

  char* romFile;

 private:
  void DMA(u16 src);
};

#endif
