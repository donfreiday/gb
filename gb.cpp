// gb: a Gameboy Emulator by Don Freiday
// File: gb.cpp
// Description: Emulator core
//
// Links the CPU, GPU, MMU, APU, Joypad, and Debugger
// Handles SDL input events and operates emulator components

#include "gb.h"

gb::gb() {
  debugEnabled = true;
  gpu.mmu = &cpu.mmu;
  cpu.mmu.joypad = &joypad;
  gpu.reset();
}

bool gb::loadROM(char* filename) { return cpu.mmu.load(filename); }

void gb::run() {
  bool quit = false;
  SDL_Event e;
#ifndef __EMSCRIPTEN__
  if (debugEnabled) {
    debugger.init(&cpu, &gpu);
    debugger.run();
  }
#endif

  while (!quit) {
    // Process events each vsync (scanline == 0x144)
    if (cpu.mmu.memory[LY] == 144 && SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          quit = true;
          break;

        case SDL_KEYDOWN:
          handleSDLKeydown(e.key.keysym.sym);
          break;

        case SDL_KEYUP:
          handleSDLKeyup(e.key.keysym.sym);
          break;

        default:
          break;
      }
    }
#ifndef __EMSCRIPTEN__
    debugEnabled ? debugger.run() : step();
#else
    step();
#endif
  }
}

void gb::step() {
  cpu.checkInterrupts();
  cpu.execute();
  gpu.step(cpu.cpu_clock_t);
}

void gb::handleSDLKeydown(SDL_Keycode key) {
  switch (key) {
    case SDLK_d:
      debugEnabled = !debugEnabled;
      break;

    case SDLK_UP:
    case SDLK_DOWN:
    case SDLK_RIGHT:
    case SDLK_LEFT:
    case SDLK_z:
    case SDLK_x:
    case SDLK_RETURN:
    case SDLK_SPACE:
      joypad.keyPressed(key);
      break;

    default:
      break;
  }
}

void gb::handleSDLKeyup(SDL_Keycode key) {
  switch (key) {
    case SDLK_UP:
    case SDLK_DOWN:
    case SDLK_RIGHT:
    case SDLK_LEFT:
    case SDLK_z:
    case SDLK_x:
    case SDLK_RETURN:
    case SDLK_SPACE:
      joypad.keyReleased(key);
      break;

    case SDLK_ESCAPE:
#ifndef __EMSCRIPTEN__
      debugger.runToBreak = !debugger.runToBreak;
#endif
      break;

    default:
      break;
  }
}

int main(int argc, char* args[]) {
#ifndef __EMSCRIPTEN__
  if (argc < 2) {
    printf("Please specify a ROM file.\n");
    return -1;
  }
  gb core;
  if (core.loadROM(args[1])) {
    core.run();
  } else {
    printf("Invalid ROM file: %s\n", args[1]);
    return -1;
  }
#else
  gb core;
  if (!core.loadROM("roms/tetris.gb")) {
    return -1;
  }
  core.run();
#endif
  return 0;
}
