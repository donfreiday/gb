// gb: a Gameboy Emulator by Don Freiday
// File: mmu.h
// Description: Memory management unit
//
// Memory map, BIOS and ROM file loading, DMA

#ifndef GB_MMU
#define GB_MMU

#include <fstream>
#include <vector>
#include "common.h"
#include "joypad.h"

class MMU
{
public:
  struct MBC
  {
    u8 type;
    u8 romBank;
    u16 romOffset;
    u16 ramOffset;
    u8 mode;
  } mbc;

  std::vector<u8> memory;
  std::vector<u8> bios;
  std::vector<u8> rom;
  std::vector<u8> ram; // External RAM

  MMU();

  bool load(char *filename);
  char *romFilename;

  void reset();

  u8 read8(u16 address);
  u16 read16(u16 address);
  void write8(u16 address, u8 value);
  void write16(u16 address, u16 value);

  // Joypad class handles reads/writes to its register
  Joypad *joypad;

private:
  void dma(u16 src);
};

#endif
