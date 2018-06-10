
#include "mmu.h"

MMU::MMU() {

}

bool MMU::load(char* filename) {
  // Open file
  std::ifstream file;
  file.open(filename, std::ios::binary | std::ios::ate);
  if(!file.is_open()) {
    printf("Failed to open ROM: %s!\n", filename);
    return false;
  }
  printf("Loaded ROM: %s\n", filename);

  // Get file size
  //int filesize = file.tellg();

  // We're going to work with 32KByte only for now; no MBC
  //file.seekg(filesize - 0x8000);
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
  memory[address] = value;
}

void MMU::write_u16(u16 address, u16 value) {
  // Reset the current scanline if the game tries to write to it
  if (address == 0xFF44) {
   *(u16*)(memory+address) = 0;
  }
  *(u16*)(memory+address) = value;
}
