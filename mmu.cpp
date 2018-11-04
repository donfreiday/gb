// gb: a Gameboy Emulator by Don Freiday
// File: mmu.cpp
// Description: Memory management unit
//
// Memory map, BIOS and ROM file loading, DMA

#include "mmu.h"

MMU::MMU() { reset(); }

void MMU::reset() {
  memory.resize(0x10000, 0);
  ram.resize(0x2000, 0);

  // Hack
  memory[JOYP] = 0xCF;

  // Flag for running boot rom
  hleBios = true;
  memMapChanged = false;

  // Setup MBC info
  mbc.romOffset = 0x4000;
  mbc.romBank = 0;
  mbc.mode = 0;
  mbc.ramOffset = 0;
  mbc.type = 0;  // this will be determined in load()
}

bool MMU::load(char* filename) {
  romFile = filename;

  // Load bios. Dont forget PC must be set to 0 in CPU
  std::ifstream file;
  file.open("roms/bios.gb", std::ios::binary | std::ios::ate);

  if (!file.is_open()) {
    hleBios = true;
  } else {
    hleBios = false;
    file.seekg(0, file.end);
    u32 biosSize = file.tellg();
    bios.resize(biosSize, 0);
    file.seekg(0, file.beg);
    file.read((char*)(&bios[0]), biosSize);
    file.close();
  }

  // Load ROM
  file.open(filename, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }
  file.seekg(0, file.end);
  u32 romSize = file.tellg();
  rom.resize(romSize, 0);
  file.seekg(0, file.beg);
  file.read((char*)(&rom[0]), romSize);
  file.close();

  memory.assign(rom.begin(), rom.begin() + 0x8000);  // First 32kb is bank 0

  if (!hleBios) {
    memory.assign(bios.begin(), bios.end());
  }

  mbc.type = rom[0x147];

  return true;
}

u8 MMU::read_u8(u16 address) {
  // ROM, switched bank
  if (address >= 0x4000 && address <= 0x7FFF) {
    return rom[mbc.romOffset + (address & 0x3FFF)];
  }

  // RAM, switched bank
  else if (address >= 0xA000 && address <= 0x8FFF) {
    return ram[mbc.ramOffset + (address & 0x1FFF)];
  }

  // Shadow of working RAM, less final 512 bytes
  else if (address >= 0xE000 && address <= 0xFDFF) {
    return memory[address - 0x1000];
  }

  // Joypad
  else if (address == 0xFF00) {
    return joypad->read(memory[0xFF00]);
  }

  // Default
  return memory[address];
}

u16 MMU::read_u16(u16 address) {
  u16 result = read_u8(address + 1);
  result <<= 8;
  result |= read_u8(address);
  return result;
}

void MMU::write_u8(u16 address, u8 value) {
  // External RAM switch
  if (address <= 0x1FFF) {
  }

  // ROM
  else if (address >= 0x2000 && address <= 0x3FFF) {
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
  else if (address >= 0x4000 && address <= 0x5FFF) {
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
  
  else if (address >= 0x6000 && address <= 0x7FFF) {
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
  else if (address >= 0xA000 && address <= 0xBFFF) {
    ram[mbc.ramOffset + (address & 0x1FFF)] = value;
  } 
  
  // WRAM shadow
  else if (address >= 0xE000 && address <= 0xFDFF) {
    memory[address - 0x1000] = value;
  }

   else if (address == DIV) {
    memory[DIV] = 0;
  }

  // Other cases
  switch (address) {
    // Joypad
    case 0xFF00:
      memory[address] = joypad->read(value);
      break;

    // CPU IF always polls high on bits 5-7
    case 0xFF0F:
      value |= 0xE0;
      memory[address] = value;
      break;

    // Block writes to LCD_STAT
    // todo: BGB does this, but is this correct behavior?
    case 0xFF41:
      break;

    // Reset the current scanline if the game tries to write to it
    case 0xFF44:
      memory[address] = 0;
      break;

    // DMA
    case 0xFF46:
      memory[0xFF46] = value;
      DMA(value);
      break;

    // unmap bootrom
    case 0xFF50: {
      memory.assign(rom.begin(), rom.begin() + 0x8000);  
      memMapChanged = true;                              
      break;
    }

    default:
      memory[address] = value;
      break;
  }
}

void MMU::write_u16(u16 address, u16 value) {
  u8 byte1 = (u8)(value & 0x00FF);
  u8 byte2 = (u8)((value & 0xFF00) >> 8);
  write_u8(address, byte1);
  write_u8(address + 1, byte2);
}

// Source address is: (data that was being written to FF46) / 100 or
// equivalently, data << 8 Destination is: sprite RAM FE00-FE9F, 0xA0 bytes
void MMU::DMA(u16 src) {
  src <<= 8;
  for (u8 i = 0; i < 0xA0; i++) {
    memory[OAM_ATTRIB + i] = memory[src + i];
  }
}

// todo:
int MMU::getRomSize() { return 32000; }