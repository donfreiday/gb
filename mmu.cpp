/* gb: a Gameboy Emulator
   Author: Don Freiday */

#include "mmu.h"

MMU::MMU() { reset(); }

void MMU::reset() {
  unmapBootrom = false;
  memset(memory, 0, sizeof(memory));
  memory[JOYP] = 0xCF;
}

bool MMU::load(char* filename) {
  romFile = filename;
  // Load bios. Dont forget PC must be set to 0 in CPU
  std::ifstream file;
  file.open("bios.gb", std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return false;
  }

  file.seekg(file.beg);
  file.read((char*)(memory), 0x8000);
  file.close();

  // todo: hack alert!
  // The bootrom expects to find a compressed Nintendo logo in the header
  // of the ROM. So we'll just load tetris after the bootrom
  file.open(filename, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }
  file.seekg(0x100);
  file.read((char*)(memory + 0x100), 0x8000 - 0x100);
  file.close();

  return true;
}

u8 MMU::read_u8(u16 address) {
  switch (address) {
    case 0xFF00:  // Joypad
      return joypad->read(memory[0xFF00]);
      break;

    default:
      return memory[address];
      break;
  }
}

u16 MMU::read_u16(u16 address) {
  u16 result = read_u8(address + 1);
  result <<= 8;
  result |= read_u8(address);
  return result;
}

void MMU::write_u8(u16 address, u8 value) {
  // No writing to cartridge ROM
  if (address < 0x8000) {
    return;
  } else if (address == DIV) {
    memory[DIV] = 0;
  }
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

    // unmap bootrom, map rom. todo: hack hack hack
    case 0xFF50: {
      unmapBootrom = true;
      std::ifstream file;
      file.open("tetris.gb", std::ios::binary);
      file.seekg(file.beg);
      file.read((char*)(memory), 0x100);
      file.close();
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