#include "gb.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

void mainLoop();
gb core;

int main(int argc, char* args[]) {
  // Load specified ROM and enter mainLoop
#ifndef __EMSCRIPTEN__
  core.debugEnabled = true;
  if (argc < 2) {
    printf("Please specify a ROM file.\n");
    return -1;
  }
  if (!core.loadROM(args[1])) {
    printf("Invalid ROM file: %s\n", args[1]);
    return -1;
  }
  mainLoop();

#else  // Emscripten ROM load and mainLoop callback setup
  // Debugger is implemented in curses, not yet working with emscripten
  core.debugEnabled = false;
  if (!core.loadROM("roms/tetris.gb")) {
    printf("Invalid ROM file: roms/tetris.gb\n");
    return -1;
  }
  emscripten_set_main_loop(mainLoop, 0, 1);
#endif

  return 0;
}

void mainLoop() {
  bool quit = false;
  SDL_Event e;

  if (core.debugEnabled) {
    core.debugger.init(&core.cpu, &core.gpu);
    core.debugger.run();
  }

  while (!quit) {
    // Process events each vsync (scanline == 0x144)
    if (core.cpu.mmu.memory[LY] == 144 && SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          quit = true;
          break;

        case SDL_KEYDOWN:
          core.handleSDLKeydown(e.key.keysym.sym);
          break;

        case SDL_KEYUP:
          core.handleSDLKeyup(e.key.keysym.sym);
          break;

        default:
          break;
      }
    }
    core.debugEnabled ? core.debugger.run() : core.step();
  }
}