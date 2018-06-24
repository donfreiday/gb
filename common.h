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

const u16 LCD_SCROLLY = 0xFF42;
const u16 LCD_SCROLLX = 0xFF43;
const u16 LCD_SCANLINE = 0xFF44;
const u16 LCD_COINCIDENCE = 0xFF45;
const u16 BG_PALETTE_DATA = 0xFF47;
const u16 LCD_WINDOWY = 0xFF4A;
const u16 LCD_WINDOWX = 0xFF4B;
const u16 CPU_INTERRUPT_FLAG = 0xFF0F;
const u16 CPU_INTERRUPT_ENABLE = 0xFFFF;

// 8-bit LCD control register
const u16 LCD_CTL = 0xFF40;
const u8 LCD_CTL_DISPLAY_ENABLE = 7;
const u8 LCD_CTL_WINDOW_TILE_MAP_SELECT = 6;  // 0 = 9800-9BFF, 1 = 9C00-9FFF
const u8 LCD_CTL_WINDOW_ENABLE = 5;
const u8 LCD_CTL_TILE_DATA_SELECT = 4;    // 0 = 8800-97FF, 1 = 9C00-9FFF
const u8 LCD_CTL_BG_TILE_MAP_SELECT = 3;  // 0 = 9800-9BFF, 1 = 9C00-9FFF
const u8 LCD_CTL_OBJ_SIZE = 2;            // 0 = 8x8, 1 = 8x16
const u8 LCD_CTL_OBJ_ENABLE = 1;
const u8 LCD_CTL_BG_ENABLE = 0;

// Bits 6-3 are for interrupt selection
const u16 LCD_STAT = 0xFF41;
const u8 LCD_STAT_COINCIDENCE_INT_ENABLE = 6;
const u8 LCD_STAT_MODE2_INT_ENABLE = 5;
const u8 LCD_STAT_MODE1_INT_ENABLE = 4;
const u8 LCD_STAT_MODE0_INT_ENABLE = 3;
const u8 LCD_STAT_COINCIDENCE_FLAG = 2;  // 0: LYC != LY, 1: LYC=LY
const u8 LCD_STAT_MODE_FLAG_HIGH = 1;  // 00 = hblank, 01 = vblank, 10 = oam search, 11 = lcd driver data tx
const u8 LCD_STAT_MODE_FLAG_LOW = 0;

#endif
