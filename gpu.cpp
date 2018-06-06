#include "gpu.h"

#define LCD_CONTROL_REGISTER 0xFF40
#define LCD_STATUS_REGISTER 0xFF41
#define LCD_SCROLLY 0xFF42
#define LCD_SCROLLX 0xFF43
#define LCD_CURRENT_SCANLINE 0xFF44
#define LCD_WINDOWY 0xFF4A
#define LCD_WINDOWX 0xFF4B

#define CPU_INTERRUPT_REQUEST 0xFF0F

GPU::GPU(MMU &mem) {
  mmu = mem;
  reset();
}

GPU::~GPU() {
  close();
}

void GPU::reset() {
  width = 166;
  height = 144;
  lines = 456;
  init();
}

// Starts up SDL and creates window
bool GPU::init() {
  if(SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL failed to initialize! SDL_Error: %s\n", SDL_GetError());
    return false;
  }
  window = SDL_CreateWindow("gb", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
  screenSurface = SDL_GetWindowSurface(window);
  backSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, 0, 0, 0, 0);
  SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0xFF, 0xFF, 0xFF ) ); // Fill the screen with white
  SDL_UpdateWindowSurface( window );
  return true;
}

// Shuts down SDL and frees resources
void GPU::close() {
  SDL_DestroyWindow(window);
  window = NULL;
  SDL_Quit();
}

/*
144 visible scanlines, 8 invisible.
Scanlines are drawn one at a time from 0 to 153.
Between 144 and 153 is the vblank period.
It takes 456 cpu cycles to draw one scanline and move on to the next.
*/
void GPU::step(int cycles) {
  updateLCD();
  if (lcdEnabled()) {
    lines -= cycles;
  }
  else {
    return;
  }

  if (lines <= 0) {
    // If a game writes to LCD_CURRENT_SCANLINE it will be reset to zero;
    // this is emulated in mmu.write functions - hence the direct increment
    mmu.memory[LCD_CURRENT_SCANLINE]++;
    lines = 456;
    u8 currentLine = mmu.read_u8(LCD_CURRENT_SCANLINE);

    // Mode 1: vblank
    if(currentLine == 144) {
      mmu.memory[CPU_INTERRUPT_REQUEST] &= 1;
    }
    else if(currentLine > 153) { //end of vblank
        mmu.memory[LCD_CURRENT_SCANLINE] = 0;
    }
    else if(currentLine < 144) {
      renderScanline();
    }
  }
}

/* The lower two bits of the LCD status register are the modes:
00: H-Blank
01: V-Blank
10: Searching Sprites Atts
11: Transferring Data to LCD Driver */
// todo: interrupt handling, coincidence flag
void GPU::updateLCD() {
  u8 status = mmu.read_u8(LCD_STATUS_REGISTER);

  if (!lcdEnabled()) {
    lines = 456;
    mmu.write_u8(LCD_CURRENT_SCANLINE, 0);
    status &= 0xFC; // clear lower two bits (mode)
    status |= 1; // mode 1
    mmu.write_u8(LCD_STATUS_REGISTER, status);
    return;
  }

  u8 currentLine = mmu.read_u8(LCD_CURRENT_SCANLINE);
  u8 currentMode = status & 0x3;
  u8 mode = 0;
  bool interrupt = 0;

  // mode 1: vblank
  if (currentLine >= 144) {
    mode = 1;
    status |= 1; // set bit 0
    status &= ~2; // clear bit 1
  }
  // mode 2: Searching Sprites Attributes
  else if (lines >= (456-80)) {
    mode = 2;
    status |= 2; // set bit 1
    status &= ~1; // clear bit 0
  }
  // mode 3: Transferring Data to LCD Driver
  else if (lines >= (456-80-172)) {
    status |= 1; // set bit 0
    status |= 2; // set bit 1
  }
  // mode 0: hblank
  else {
    mode = 0;
    status &= ~1; // clear bit 0
    status &= ~2; // clear bit 1
  }
  mmu.write_u8(LCD_STATUS_REGISTER, status);
}

bool GPU::lcdEnabled() {
  return (mmu.read_u8(LCD_CONTROL_REGISTER) & (1 << 7)); // Bit 7 of the LCD control register
}

// Write scanline to framebuffer
void GPU::renderScanline() {
  u8 control = mmu.read_u8(LCD_CONTROL_REGISTER);

  // Bit 0: BG Display enable
  if (control & 1) {
    renderBackground();
  }

  // Bit 1: OBJ (Sprite) display enable
  if (control & 2) {
    renderSprites();
  }

}

/* Background is 256x256 pixels or 32x32 tiles, of which only 160x144 pixels are visible

  Visible area is determined by SCROLLX and SCROLLY (0xFF42 and 0xFF43)

  Background layout is between 0x9800-0x9BFF and 0x9C00-0x9FFF, which is shared with the window layer
  Bit 3 of the LCD control register determines which region to use for background,
  Bit 6 determines the region for the window.

  Tile identifier from background layout is different for each region:
  0x9800-0x9BFF: unsigned byte
  0x9C00-0x9FFF: signed byte

  Tile data is either between 0x8000-0x8FFF or 0x9C00-0x9FFF, depending on bit 4 of the LCD control registers

  Each tile is 8x8 pixels or 16 bytes.

  LCD_CONTROL_REGISTER
  Bit 7 - LCD Display Enable (0=Off, 1=On)
  Bit 6 - Window Tile Map Display Select (0=9800-9BFF, 1=9C00-9FFF)
  Bit 5 - Window Display Enable (0=Off, 1=On)
  Bit 4 - BG & Window Tile Data Select (0=8800-97FF, 1=8000-8FFF)
  Bit 3 - BG Tile Map Display Select (0=9800-9BFF, 1=9C00-9FFF)
  Bit 2 - OBJ (Sprite) Size (0=8x8, 1=8x16)
  Bit 1 - OBJ (Sprite) Display Enable (0=Off, 1=On)
  Bit 0 - BG Display (for CGB see below) (0=Off, 1=On)
*/
void GPU::renderBackground() {
  u16 tileData = mmu.read_u8(LCD_CONTROL_REGISTER) & (1 << 4) ? 0x8000 : 0x8800;
  u16 bgTileMap = mmu.read_u8(LCD_CONTROL_REGISTER) & (1 << 3) ? 0x9C00 : 0x9800;

  // yPos calculates which of 32 vertical tiles the current scanline is drawing
  u8 yPos = mmu.read_u8(LCD_SCROLLY) + mmu.read_u8(LCD_CURRENT_SCANLINE);

  // Determine which of the 8 vertical pixels of the current tile the scanline is on
  u16 tileRow = (((u8)(yPos/8))*32);

  for (int pixel = 0; pixel < 160; pixel++) {
    u8 xPos = pixel + mmu.read_u8(LCD_SCROLL_X);

    // Determine which of the 32 horizontal tiles this xPos falls within
    u16 tileCol = (xPos/8);

    u16 tileAddress = bgTileMap + tileRow + tileCol;

    // Tile ID can be signed or unsigned depending on tile data memory region
    s16 tileNum = (tileData == 0x8800) ? mmu.read_s8(tileAddress) : mmu.read_u8(tileAddress);
  }
}

void GPU::renderSprites() {

}

// Swap SDL buffers
void GPU::swapsurface() {

}
