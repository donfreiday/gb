/* gb: a Gameboy Emulator
   Author: Don Freiday */

#include "mmu.h"

MMU::MMU() {

}

bool MMU::load(char* filename) {
  // load tetris
  /*
  std::ifstream file;
  file.open("tetris.gb", std::ios::binary);
  file.seekg(file.beg);
  file.read((char*)(memory), 0x8000);
  file.close();*/

  //Load bios. Dont forget PC must be set to 0 in CPU
  std::ifstream file;
  file.open("bios.gb", std::ios::binary | std::ios::ate);
  if(!file.is_open()) {
    printf("Failed to open ROM: %s!\n", filename);
    return false;
  }
  printf("Loaded ROM: %s\n", filename);

  file.seekg(file.beg);
  file.read((char*)(memory), 0x8000);
  file.close();

  // todo: hack alert!
  // The bootrom expects to find a compressed Nintendo logo in the header
  // of the ROM. So we'll just load tetris after the bootrom
  file.open("tetris.gb", std::ios::binary);
  file.seekg(0x100);
  file.read((char*)(memory+0x100), 0x8000-0x100);
  file.close();

  return true;
}

u8 MMU::read_u8(u16 address) {
  return memory[address];
}

u16 MMU::read_u16(u16 address) {
  return *(u16*)(memory+address);
}

void MMU::write_u8(u16 address, u8 value) {
  // Reset the current scanline if the game tries to write to it
  if (address == 0xFF44) {
   memory[address] = 0 ;
  }
  else if (address == 0xFF46) {
    DMA(value);
  }
  else {
    memory[address] = value;
  }
}

void MMU::write_u16(u16 address, u16 value) {
  // Reset the current scanline if the game tries to write to it
  if (address == 0xFF44) {
   *(u16*)(memory+address) = 0;
  }
  else if (address == 0xFF46) {
    DMA(value);
  }
  else {
    *(u16*)(memory+address) = value;
  }
}

// Source address is: (data that was being written to FF46) / 100 or equivalently, data << 8
// Destination is: sprite RAM FE00-FE9F, 0xA0 bytes
void MMU::DMA(u8 src) {
  u16 srcAddress = src;
  srcAddress<<=8;
  for(u8 i = 0; i < 0xA0; i++) {
    write_u8(0xFE00 + i, read_u8(srcAddress+i));
  }
}
