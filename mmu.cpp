// gb: a Gameboy Emulator by Don Freiday
// File: mmu.cpp
// Description: Memory management unit
//
// Memory map, BIOS and ROM file loading, DMA

#include "mmu.hpp"

MMU::MMU() { reset(); }

void MMU::reset() {
  memory.resize(0x10000, 0);
  ram.resize(0x2000, 0);  // external RAM

  // Setup MBC info
  mbc.romOffset = 0x4000;
  mbc.romBank = 0;
  mbc.mode = 0;
  mbc.ramOffset = 0;
  mbc.type = 0;  // this will be determined in load()
}

bool MMU::load(char *filename) {
  romFilename = filename;

  std::ifstream file;
  file.open(filename, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  // Get file size and read ROM into vector
  file.seekg(0, file.end);
  u32 romSize = file.tellg();
  rom.resize(romSize, 0);
  file.seekg(0, file.beg);
  file.read((char *)(&rom[0]), romSize);
  file.close();

  // Map ROM to memory
  memory.assign(rom.begin(),
                rom.begin() + (0x7FFF > rom.size() ? rom.size() : 0x7FFF));

  // Get MBC type from ROM header
  mbc.type = rom[0x147];

  return true;
}

u8 MMU::read8(u16 addr) {
  // ROM, switched bank
  if (addr >= 0x4000 && addr <= 0x7FFF) {
    return rom[mbc.romOffset + (addr & 0x3FFF)];
  }

  // RAM, switched bank
  else if (addr >= 0xA000 && addr <= 0x8FFF) {
    return ram[mbc.ramOffset + (addr & 0x1FFF)];
  }

  // Shadow of working RAM, less final 512 bytes
  else if (addr >= 0xE000 && addr <= 0xFDFF) {
    return memory[addr - 0x1000];
  }

  // Joypad
  else if (addr == 0xFF00) {
    return joypad->read(memory[0xFF00]);
  }

  // Default
  else {
    return memory[addr];
  }
}

u16 MMU::read16(u16 addr) { return (read8(addr + 1) << 8 | read8(addr)); }

void MMU::write8(u16 addr, u8 value) {
  // External RAM switch
  if (addr <= 0x1FFF) {
  }

  // ROM
  else if (addr >= 0x2000 && addr <= 0x3FFF) {
    switch (mbc.type) {
      case 1:
      case 2:
      case 3:
        value &= 0x0F;
        if (!value) {
          value = 1;
        }
        mbc.romBank = (mbc.romBank & 0x60) + value;
        mbc.romOffset = mbc.romBank * 0x4000;
        break;
      default:
        break;
    }
  }

  // RAM
  else if (addr >= 0x4000 && addr <= 0x5FFF) {
    switch (mbc.type) {
      case 1:
      case 2:
      case 3:
        // RAM bank
        if (mbc.mode) {
          mbc.ramOffset = (value & 3) * 0x2000;
        } else {
          // ROM bank
          mbc.romBank = (mbc.romBank & 0x1F) + ((value & 3) << 5);
          mbc.romOffset = mbc.romBank * 0x4000;
        }
        break;
      default:
        break;
    }
  }

  else if (addr >= 0x6000 && addr <= 0x7FFF) {
    switch (mbc.type) {
      case 2:
      case 3:
        mbc.mode = value & 1;
        break;
      default:
        break;
    }
  }

  // RAM, external
  else if (addr >= 0xA000 && addr <= 0xBFFF) {
    ram[mbc.ramOffset + (addr & 0x1FFF)] = value;
  }

  // WRAM shadow
  else if (addr >= 0xE000 && addr <= 0xFDFF) {
    memory[addr - 0x1000] = value;
  }

  // Writes to DIV reset it to zero
  else if (addr == DIV) {
    memory[DIV] = 0;
  }

  // Joypad class handles its register
  else if (addr == JOYP) {
    memory[addr] = joypad->read(value);
  }

  // CPU IF always polls high on bits 5-7
  else if (addr == IF) {
    value |= 0xE0;
    memory[addr] = value;
  }

  // Writes to current scanline register reset it
  else if (addr == LY) {
    memory[addr] = 0;
  }

  // DMA
  else if (addr == DMA) {
    memory[0xFF46] = value;
    dma(value);
  }

  // Unmap bootrom
  else if (addr == 0xFF50) {
    return;
  }

  // Default
  else {
    memory[addr] = value;
  }
}

void MMU::write16(u16 addr, u16 value) {
  write8(addr, value & 0x00FF);
  write8(addr + 1, ((value & 0xFF00) >> 8));
}

// Source addr is: (data that was being written to FF46) / 100 or
// equivalently, data << 8 Destination is: sprite RAM FE00-FE9F, 0xA0 bytes
void MMU::dma(u16 src) {
  src <<= 8;
  for (u8 i = 0; i < 0xA0; i++) {
    memory[OAM_ATTRIB + i] = memory[src + i];
  }
}