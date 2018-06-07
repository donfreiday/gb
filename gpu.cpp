#include "gpu.h"

#define LCD_CONTROL_REGISTER 0xFF40
#define LCD_STATUS_REGISTER 0xFF41
#define LCD_SCROLLY 0xFF42
#define LCD_SCROLLX 0xFF43
#define LCD_CURRENT_SCANLINE 0xFF44
#define BG_PALETTE_DATA 0xFF47
#define LCD_WINDOWY 0xFF4A
#define LCD_WINDOWX 0xFF4B
#define CPU_INTERRUPT_REQUEST 0xFF0F

GPU::GPU(MMU& mem) {
  mmu = &mem;
  reset();
}

GPU::~GPU() {
}

void GPU::reset() {
  width = 160;
  height = 144;
  lines = 456;
  initSDL();
}

// Starts up SDL and creates window
bool GPU::initSDL() {
  if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    printf("SDL failed to initialize! SDL_Error: %s\n", SDL_GetError());
    return false;
  }
  if(SDL_SetVideoMode(width, height, 8, SDL_OPENGL) == NULL) {
    printf("SDL failed to initialize! SDL_Error: %s\n", SDL_GetError());
    return false;
  }
  initGL();
  SDL_WM_SetCaption("gb", NULL);
  return true;
}

// todo: study this
void GPU::initGL() {
  glViewport(0, 0, width, height);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glOrtho(0, width, height, 0, -1.0, 1.0);
  glClearColor(0, 0, 0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glShadeModel(GL_FLAT);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DITHER);
  glDisable(GL_BLEND);
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
    // this is emulated in mmu->write functions - hence the direct increment
    mmu->memory[LCD_CURRENT_SCANLINE]++;
    lines = 456;
    u8 currentLine = mmu->read_u8(LCD_CURRENT_SCANLINE);

    // Mode 1: vblank
    if(currentLine == 144) {
      mmu->memory[CPU_INTERRUPT_REQUEST] &= 1;
    }
    else if(currentLine > 153) { //end of vblank
        mmu->memory[LCD_CURRENT_SCANLINE] = 0;
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
  u8 status = mmu->read_u8(LCD_STATUS_REGISTER);
  if (!lcdEnabled()) {
    lines = 456;
    mmu->write_u8(LCD_CURRENT_SCANLINE, 0);
    status &= 0xFC; // clear lower two bits (mode)
    status |= 1; // mode 1
    mmu->write_u8(LCD_STATUS_REGISTER, status);
    return;
  }

  u8 currentLine = mmu->read_u8(LCD_CURRENT_SCANLINE);
  //u8 currentMode = status & 0x3;
  //u8 mode = 0;
  //bool interrupt = 0;

  // mode 1: vblank
  if (currentLine >= 144) {
    //mode = 1;
    status |= 1; // set bit 0
    status &= ~2; // clear bit 1
  }
  // mode 2: Searching Sprites Attributes
  else if (lines >= (456-80)) {
    //mode = 2;
    status |= 2; // set bit 1
    status &= ~1; // clear bit 0
  }
  // mode 3: Transferring Data to LCD Driver
  else if (lines >= (456-80-172)) {
    //mode = 3;
    status |= 1; // set bit 0
    status |= 2; // set bit 1
  }
  // mode 0: hblank
  else {
    //mode = 0;
    status &= ~1; // clear bit 0
    status &= ~2; // clear bit 1
  }
  mmu->write_u8(LCD_STATUS_REGISTER, status);
}

bool GPU::lcdEnabled() {
  return (mmu->read_u8(LCD_CONTROL_REGISTER) & (1 << 7)); // Bit 7 of the LCD control register
}

// Write scanline to framebuffer
void GPU::renderScanline() {
  u8 control = mmu->read_u8(LCD_CONTROL_REGISTER);

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
  u16 tileData = mmu->read_u8(LCD_CONTROL_REGISTER) & (1 << 4) ? 0x8000 : 0x8800;
  u16 bgTileMap = mmu->read_u8(LCD_CONTROL_REGISTER) & (1 << 3) ? 0x9C00 : 0x9800;

  // yPos calculates which of 32 vertical tiles the current scanline is drawing
  u8 yPos = mmu->read_u8(LCD_SCROLLY) + mmu->read_u8(LCD_CURRENT_SCANLINE);

  // Determine which of the 8 vertical pixels of the current tile the scanline is on
  u16 tileRow = (((u8)(yPos/8))*32);

  for (int pixel = 0; pixel < 160; pixel++) {
    u8 xPos = pixel + mmu->read_u8(LCD_SCROLLX);

    // Determine which of the 32 horizontal tiles this xPos falls within
    u16 tileCol = (xPos/8);

    u16 tileAddress = bgTileMap + tileRow + tileCol;

    // Tile ID can be signed or unsigned depending on tile data memory region
    s16 tileID = (tileData == 0x8800) ? (s8)(mmu->read_u8(tileAddress)) : mmu->read_u8(tileAddress);

    // Find this tile ID in memory
    /* If the tile data memory area we are using is 0x8000-0x8FFF then the tile identifier read from the
    background layout regions is an UNSIGNED BYTE meaning the tile identifier will range from 0 - 255.
    However if we are using tile data area 0x8800-0x97FF then the tile identifier read from the background
    layout is a SIGNED BYTE meaning the tile identifier will range from -127 to 127. */
    u16 tileLocation = tileData;
    printf("\ntileLocation: %04X",tileLocation);
    if (tileData == 0x8000) {
      tileLocation += (tileID * 16);
    }
    else {
      tileLocation += ((tileID+128)*16);
    }

    u8 line = yPos % 8; // Current vertical line of tile
    line *= 2; // Each vertical line is two bytes
    u8 data1 = mmu->read_u8(tileLocation+line);
    u8 data2 = mmu->read_u8(tileLocation+line+1);



    /* A tile is 8x8 pixels; each horizontal line in a tile is two bytes.
      pixel# = 0 1 2 3 4 5 6 7
      data 2 = 1 0 1 0 1 1 1 0
      data 1 = 0 0 1 1 0 1 0 1

      Pixel 0 color id: 10
      Pixel 1 color id: 00
      Pixel 2 color id: 11
      Pixel 3 color id: 01
      Pixel 4 color id: 10
      Pixel 5 color id: 11
      Pixel 6 color id: 10
      Pixel 7 color id: 01

      Pixel 0 is bit 7 of data1 and data2: */
      int colorBit = xPos % 8;
      colorBit -= 7;
      colorBit *= -1;

      int colorNum = data2 & (1 << colorBit);
      colorNum <<= 1;
      colorNum |= data1 & (1 << colorBit);

      COLOR color = paletteLookup(colorNum, BG_PALETTE_DATA);
      u8 red = 0, green = 0, blue = 0;
      switch(color) {
        case WHITE:	red = 255; green = 255 ; blue = 255; break ;
        case LIGHT_GRAY:red = 0xCC; green = 0xCC ; blue = 0xCC; break ;
        case DARK_GRAY:	red = 0x77; green = 0x77 ; blue = 0x77; break ;
        case BLACK: break; // -Wswitch warning prevention
      }

      u8 scanline = mmu->read_u8(LCD_CURRENT_SCANLINE) ;

      screenData[scanline][pixel][0] = red;
      screenData[scanline][pixel][1] = green;
      screenData[scanline][pixel][2] = blue;
  }
}

// todo:+ comment on how this works
GPU::COLOR GPU::paletteLookup(u8 colorID, u16 address) {
  COLOR result = WHITE;
  u8 palette = mmu->read_u8(address);
  int high = 0;
  int low = 0;
  switch(colorID) {
    case 0: high = 1; low = 0; break;
    case 1: high = 3; low = 2; break;
    case 2: high = 5; low = 4; break;
    case 3: high = 7; low = 6; break;
  }
  int color = 0;
  color = palette&(1<<high);
  color <<= 1;
  color |= palette&(1<<low);

  switch(color) {
    case 0: result = WHITE; break;
    case 1: result = LIGHT_GRAY; break;
    case 2: result = DARK_GRAY; break;
    case 3: result = BLACK; break;
  }
  return result;
}

void GPU::renderSprites() {

}

void GPU::renderScreen() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 	glLoadIdentity();
 	glRasterPos2i(-1, 1);
	glPixelZoom(1, -1);
 	glDrawPixels(160, 144, GL_RGB, GL_UNSIGNED_BYTE, screenData);
	SDL_GL_SwapBuffers( ) ;
}

// Swap SDL buffers
void GPU::swapsurface() {

}
