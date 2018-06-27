#ifndef COMMON
#define COMMON

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long int u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long int s64;

template <typename t>
bool bitTest(t num, u8 pos) {
  return num & (1 << pos);
}

template <typename t>
void bitSet(t &num, u8 pos) {
  num |= (1 << pos);
}

template <typename t>
void bitClear(t &num, u8 pos) {
  num &= ~(1 << pos);
}

const u16 IF = 0xFF0F;    // Interrupt flag
const u16 LCDC = 0xFF40;  // LCD control
const u16 STAT = 0xFF41;  // LCD status
const u16 SCY = 0xFF42;   // BG scroll Y
const u16 SCX = 0xFF43;   // BG scroll X
const u16 LY = 0xFF44;    // Scanline
const u16 LYC = 0xFF45;   // Scanline coincidence
const u16 DMA = 0xFF46;   // DMA Transfer and Start Address
const u16 BGP = 0xFF47;   // BG Palette Data
const u16 OBP0 = 0xFF48;  // Object Palette 0 Data
const u16 OBP1 = 0xFF49;  // Object Palette 1 Data
const u16 WY = 0xFF4A;    // Window Y
const u16 WX = 0xFF4B;    // Window X
const u16 IE = 0xFFFF;    // Interrupt enable

// LCDC bits
const u8 LCDC_DISPLAY_ENABLE = 7;
const u8 LCDC_WINDOW_TILE_MAP_SELECT = 6;  // 0 = 9800-9BFF, 1 = 9C00-9FFF
const u8 LCDC_WINDOW_ENABLE = 5;
const u8 LCDC_TILE_DATA_SELECT = 4;    // 0 = 8800-97FF, 1 = 9C00-9FFF
const u8 LCDC_BG_TILE_MAP_SELECT = 3;  // 0 = 9800-9BFF, 1 = 9C00-9FFF
const u8 LCDC_OBJ_SIZE = 2;            // 0 = 8x8, 1 = 8x16
const u8 LCDC_OBJ_ENABLE = 1;
const u8 LCDC_BG_ENABLE = 0;

// STAT bits; 6-3 are for interrupt selection
// 0-2: Mode :
// 00 = hblank
// 01 = vblank
// 10 = oam search
// 11 = lcd driver data tx
const u8 STAT_LYC_INT_ENABLE = 6;
const u8 STAT_MODE2_INT_ENABLE = 5;
const u8 STAT_MODE1_INT_ENABLE = 4;
const u8 STAT_MODE0_INT_ENABLE = 3;
const u8 STAT_LYC_FLAG = 2;  // 0: LYC != LY, 1: LYC=LY
const u8 STAT_MODE_HIGH = 1;
const u8 STAT_MODE_LOW = 0;

#endif
