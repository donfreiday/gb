/* gb: a Gameboy Emulator
   Author: Don Freiday */

#include "gpu.h"

GPU::GPU() {}

GPU::~GPU() {
  window = NULL;
  SDL_Quit();
}

void GPU::reset() {
  window = NULL;
  SDL_Quit();
  width = 160;
  height = 144;
  scanline = 0;
  mmu->memory[LY] = scanline;
  mmu->memory[STAT] = 0x84;
  modeclock = 0;
  memset(screenData, 0, sizeof(screenData));
  frameStartTime = 0;
  frameCurrentTime = 0;
  initSDL();
}

// Starts up SDL and creates window
bool GPU::initSDL() {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    printf("SDL failed to initialize! SDL_Error: %s\n", SDL_GetError());
    return false;
  }
  window =
      SDL_CreateWindow("gb", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       width, height, SDL_WINDOW_OPENGL);

  // Rendering context
  mainContext = SDL_GL_CreateContext(window);

  // Set our OpenGL version.
  // SDL_GL_CONTEXT_CORE gives us only the newer version, deprecated functions
  // are disabled
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // 3.2 is part of the modern versions of OpenGL, but most video cards whould
  // be able to run it
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  // Turn on double buffering with a 24bit Z buffer.
  // You may need to change this to 16 or 32 for your system
  if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0) {
    SDL_GetError();
  }

  // This makes our buffer swap syncronized with the monitor's vertical refresh
  SDL_GL_SetSwapInterval(1);

  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapWindow(window);

  return true;
}

/*
144 visible scanlines, 8 invisible.
Scanlines are drawn one at a time from 0 to 153.
Between 144 and 153 is the vblank period.
It takes 456 cpu cycles to draw one scanline and move on to the next.

*/
void GPU::step(u8 cycles) {
  u8 status = mmu->read_u8(STAT);

  // If the LCD is disabled:
  if (!(bitTest(mmu->read_u8(LCDC), LCDC_DISPLAY_ENABLE))) {
    modeclock = 0;
    scanline = 0;
    mode = 2;               // todo: hack to match BGB LCD timings
    status &= (0xFF << 2);  // clear mode bits in LCD status register
    status |= mode;
    mmu->memory[STAT] = status;
    mmu->memory[LY] = scanline;  // writes to this address are trapped in MMU
    return;
  }

  modeclock += cycles;
  bool interrupt = false;

  switch (mode) {
    // OAM read mode, 80 cycles
    case 2:
      if (modeclock >= 80) {
        modeclock = 0;
        mode = 3;
      }
      break;

    // VRAM read mode, 172 cycles. End of mode 3 is end of scanline
    case 3:
      if (modeclock >= 172) {
        modeclock = 0;
        mode = 0;  // hblank
        interrupt = bitTest(mmu->read_u8(STAT), STAT_MODE0_INT_ENABLE);
        renderScanline();
      }
      break;

    // hblank, 204 cycles
    case 0:
      if (modeclock >= 204) {
        modeclock = 0;
        scanline++;
        if (scanline == 143) {
          interrupt = bitTest(mmu->read_u8(STAT), STAT_MODE1_INT_ENABLE);
          mode = 1;  // vblank
          renderScreen();
        } else {
          mode = 2;
          interrupt = bitTest(mmu->read_u8(STAT), STAT_MODE2_INT_ENABLE);
        }
      }
      break;

    // vblank, 456 cycles (10 scanlines)
    case 1:
      if (scanline == 144) {
        requestInterrupt(0);
      }
      if (modeclock >= 456) {
        modeclock = 0;
        scanline++;
        if (scanline > 153) {
          mode = 2;  // restart scanning mode
          interrupt = bitTest(mmu->read_u8(STAT), STAT_MODE2_INT_ENABLE);
          scanline = 0;
        }
      }
      break;
  }

  if (interrupt) {
    requestInterrupt(1);
  }

  // Handle coincidence flag and check for interrupt enabled
  if (scanline == mmu->read_u8(LYC)) {
    bitSet(status, STAT_LYC_FLAG);
    if (bitTest(status, STAT_LYC_INT_ENABLE)) {
      requestInterrupt(1);
    }
  } else {
    bitClear(status, STAT_LYC_FLAG);
  }

  status &= (0xFF << 2);  // clear the mode flag bits
  status |= mode;
  mmu->memory[STAT] = status;
  mmu->memory[LY] = scanline;  // writes to this address are trapped in mmu
}

// Write scanline to framebuffer
void GPU::renderScanline() {
  u8 control = mmu->read_u8(LCDC);
  if (bitTest(control, LCDC_BG_ENABLE)) {
    renderBackground();
  }
  if (bitTest(control, LCDC_OBJ_ENABLE)) {
    renderSprites();
  }
}

/* Background is 256x256 pixels or 32x32 tiles, of which only 160x144 pixels are
  visible

  Visible area is determined by SCROLLX and SCROLLY (0xFF42 and 0xFF43)

  Background layout is between 0x9800-0x9BFF and 0x9C00-0x9FFF, which is shared
  with the window layer Bit 3 of the LCD control register determines which
  region to use for background, Bit 6 determines the region for the window.

  Tile identifier from background layout is different for each region:
  0x9800-0x9BFF: unsigned byte
  0x9C00-0x9FFF: signed byte

  Tile data is either between 0x8000-0x8FFF or 0x9C00-0x9FFF, depending on bit 4
  of the LCD control registers

  Each tile is 8x8 pixels or 16 bytes.
*/
void GPU::renderBackground() {
  u16 tileData = mmu->read_u8(LCDC) & (1 << 4) ? 0x8000 : 0x8800;
  u16 bgTileMap = mmu->read_u8(LCDC) & (1 << 3) ? 0x9C00 : 0x9800;

  // yPos calculates which of 32 vertical tiles the current scanline is drawing
  u8 yPos = mmu->read_u8(SCY) + mmu->read_u8(LY);

  // Determine which of the 8 vertical pixels of the current tile the scanline
  // is on
  u16 tileRow = (((u8)(yPos / 8)) * 32);

  for (int pixel = 0; pixel < 160; pixel++) {
    u8 xPos = pixel + mmu->read_u8(SCX);

    // Determine which of the 32 horizontal tiles this xPos falls within
    u16 tileCol = (xPos / 8);

    u16 tileAddress = bgTileMap + tileRow + tileCol;

    // Tile ID can be signed or unsigned depending on tile data memory region
    s16 tileID = (tileData == 0x8800) ? (s8)(mmu->read_u8(tileAddress))
                                      : mmu->read_u8(tileAddress);

    // Find this tile ID in memory
    /* If the tile data memory area we are using is 0x8000-0x8FFF then the tile
    identifier read from the background layout regions is an UNSIGNED BYTE
    meaning the tile identifier will range from 0 - 255. However if we are using
    tile data area 0x8800-0x97FF then the tile identifier read from the
    background layout is a SIGNED BYTE meaning the tile identifier will range
    from -127 to 127. */
    u16 tileLocation = tileData;
    // printf("\ntileLocation: %04X",tileLocation);
    if (tileData == 0x8000) {
      tileLocation += (tileID * 16);
    } else {
      tileLocation += ((tileID + 128) * 16);
    }

    u8 line = yPos % 8;  // Current vertical line of tile
    line *= 2;           // Each vertical line is two bytes
    u8 data1 = mmu->read_u8(tileLocation + line);
    u8 data2 = mmu->read_u8(tileLocation + line + 1);

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

    int colorID = 0;
    // int colorID = (data2 & (1 << colorBit));
    // colorID <<= 1;
    if (data2 & (1 << colorBit)) {
      colorID |= 2;
    }
    if (data1 & (1 << colorBit)) {
      colorID |= 1;
    }
    // colorID |= (data1 & (1 << colorBit));

    COLOR color = paletteLookup(colorID, BGP);
    u8 red = 0, green = 0, blue = 0;
    switch (color) {
      case WHITE:
        red = 255;
        green = 255;
        blue = 255;
        break;
      case LIGHT_GRAY:
        red = 0xCC;
        green = 0xCC;
        blue = 0xCC;
        break;
      case DARK_GRAY:
        red = 0x77;
        green = 0x77;
        blue = 0x77;
        break;
      case BLACK:
        break;  // -Wswitch warning prevention
    }

    screenData[scanline][pixel][0] = red;
    screenData[scanline][pixel][1] = green;
    screenData[scanline][pixel][2] = blue;
  }
}

/*
Sprite data: 8000-8fff, 40 tiles

Each sprite has 4 bytes of attributes fe00-fe9f (OAM attribute table)
Byte 0: Y position - 16
Byte 1: X pos - 8
Byte 2: Pattern number / sprite ID to lookup in sprite data
Byte 3: attributes

Attributes:
bit 7: sprite to bg priority
  0: Sprite above bg and window
  1: sprite behind bg and window, unless bg/win are white
bit 6: y flip
bit 5: x flip
bit 4: palette number
  0: palette from ff48
  1: palette from ff49
bit 3-0: unused for DMG
*/
u8 prevXpos = 0;
void GPU::renderSprites() {
  u8 ySize = bitTest(mmu->memory[LCDC], LCDC_OBJ_SIZE) ? 16 : 8;
  for (u8 sprite = 0; sprite < 40; sprite++) {
    u8 index = sprite * 4;  // each oam attribute entry is 4 bytes
    u8 yPos = mmu->memory[OAM_ATTRIB + index] - 16;
    u8 xPos = mmu->memory[OAM_ATTRIB + index + 1] - 8;
    u8 tileLocation = mmu->memory[OAM_ATTRIB + index + 2];
    u8 attributes = mmu->memory[OAM_ATTRIB + index + 3];
    bool yFlip = bitTest(attributes, 6);
    bool xFlip = bitTest(attributes, 5);

    if(OAM_ATTRIB + index == 0xFe00) {
      if(xPos!=prevXpos) {
        prevXpos = xPos;
      }
    }


    // Is sprite located on the current scanline?
    if (scanline >= yPos && scanline < (yPos + ySize)) {
      u8 line = scanline - yPos;
      if (yFlip) {
        line -= ySize;
        line *= -1;
      }
      line *= 2;

      u16 tileData = (OAM_DATA + (tileLocation * 16)) + line;
      u8 data1 = mmu->memory[tileData];
      u8 data2 = mmu->memory[tileData + 1];

      for (int tilePixel = 7; tilePixel >= 0; tilePixel--) {
        int colorBit = tilePixel;
        if (xFlip) {
          colorBit -= 7;
          colorBit *= -1;
        }

        u8 colorID = 0;
        if (data2 & (1 << colorBit)) {
          colorID |= 2;
        }
        if (data1 & (1 << colorBit)) {
          colorID |= 1;
        }

        u16 paletteAddress = bitTest(attributes, 4) ? 0xFF49 : 0xFF48;
        COLOR color = paletteLookup(colorID, paletteAddress);

        // white is transparent
        if (color == WHITE) {
          continue;
        }

        u8 red = 0, green = 0, blue = 0;
        switch (color) {
          case WHITE:
            red = 255;
            green = 255;
            blue = 255;
            break;
          case LIGHT_GRAY:
            red = 0xCC;
            green = 0xCC;
            blue = 0xCC;
            break;
          case DARK_GRAY:
            red = 0x77;
            green = 0x77;
            blue = 0x77;
            break;
          case BLACK:
            break;  // -Wswitch warning prevention
        }

        u8 pixel = xPos - tilePixel;
        pixel += 7;

        screenData[scanline][pixel][0] = red;
        screenData[scanline][pixel][1] = green;
        screenData[scanline][pixel][2] = blue;
      }
    }
  }
}

// todo:+ comment on how this works
GPU::COLOR GPU::paletteLookup(u8 colorID, u16 address) {
  COLOR result = WHITE;
  u8 palette = mmu->read_u8(address);
  u8 high = 0;
  u8 low = 0;
  switch (colorID) {
    case 0:
      high = 1;
      low = 0;
      break;
    case 1:
      high = 3;
      low = 2;
      break;
    case 2:
      high = 5;
      low = 4;
      break;
    case 3:
      high = 7;
      low = 6;
      break;
  }
  u8 color = 0;
  if (palette & (1 << high)) {
    color |= 2;
  }
  if (palette & (1 << low)) {
    color |= 1;
  }

  switch (color) {
    case 0:
      result = WHITE;
      break;
    case 1:
      result = LIGHT_GRAY;
      break;
    case 2:
      result = DARK_GRAY;
      break;
    case 3:
      result = BLACK;
      break;
  }
  return result;
}

void GPU::renderScreen() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glRasterPos2i(-1, 1);
  glPixelZoom(1, -1);
  glDrawPixels(160, 144, GL_RGB, GL_UNSIGNED_BYTE, screenData);
  frameCurrentTime = SDL_GetTicks();
	if((frameCurrentTime - frameStartTime) < 16) { SDL_Delay(16 - (frameCurrentTime - frameStartTime));}
	frameStartTime = SDL_GetTicks();
  SDL_GL_SwapWindow(window);
  memset(screenData, 0xFF, sizeof(screenData));  // Clear buffer for next frame
}

void GPU::requestInterrupt(u8 interrupt) {
  u8 cpuInterrupts = mmu->read_u8(IF);
  bitSet(cpuInterrupts, interrupt);
  mmu->write_u8(IF, cpuInterrupts);
}
