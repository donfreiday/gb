
#include "mmu.h"

MMU::MMU() {

}

int MMU::load(std::string filename) {
  // Open file
  std::ifstream file;
  file.open(filename, std::ios::binary | std::ios::ate);
  if(!file.is_open()) {
    printf("Failed to open ROM: %s!", filename);
    return -1;
  }

  // Get file size
  int filesize = file.tellg();
  file.seekg(file.beg);

  // We're going to work with 32KByte only for now; no MBC
  file.seekg(filesize - 0x8000);
  file.read((char*)(memory), 0x8000);
  file.close();
}

u8 MMU::read_byte(u16 address) {
  return memory[address];
}

u16 MMU::read_word(u16 address) {
  return *(u16*)(memory+address);
}

void MMU::write_byte(u16 address, u8 value) {

}

void MMU::write_word(u16 address, u16 value) {

}
