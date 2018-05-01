
#include "mmu.h"

MMU::MMU() {

}

bool MMU::load(std::string filename) {
  // Open file
  std::ifstream file;
  file.open(filename, std::ios::binary | std::ios::ate);
  if(!file.is_open()) {
    //printf("Failed to open ROM: %s!", filename);
    return false;
  }

  // Get file size
  int filesize = file.tellg();
  file.seekg(file.beg);

  // We're going to work with 32KByte only for now; no MBC
  file.seekg(filesize - 0x8000);
  file.read((char*)(memory), 0x8000);
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
  memory[address] = value;
}

void MMU::write_u16(u16 address, u16 value) {
  *(u16*)(memory+address) = value;
}
