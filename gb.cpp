// gb: a Gameboy Emulator by Don Freiday
// File: gb.cpp
// Description: Emulator core
//
// Links the CPU, GPU, MMU, APU, Joypad, and Debugger
// Handles SDL input events and operates emulator components

#include "gb.h"

gb::gb() {
  debugEnabled = false;
  gpu.mmu = &cpu.mmu;
  cpu.mmu.joypad = &joypad;
  gpu.reset();
}

bool gb::loadROM(char* filename) { return cpu.mmu.load(filename); }

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

