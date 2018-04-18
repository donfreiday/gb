/* gb: a Gameboy Emulator
   Author: Don Freiday
   todo: everything */

#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include <stdio.h>

const int SCREEN_WIDTH = 166;
const int SCREEN_HEIGHT = 144;

bool init(); // Starts up SDL and creates window
int loop(); // Event loop
void close(); // Shuts down SDL and frees resources

SDL_Window* gWindow = NULL;
SDL_Surface* gScreenSurface = NULL;

// Starts up SDL and creates window
bool init() {
  if(SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL failed to initialize! SDL_Error: %s\n", SDL_GetError());
    return false;
  }
  gWindow = SDL_CreateWindow("gb", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  gScreenSurface = SDL_GetWindowSurface(gWindow);
  return true;
}

int loop() {
  std::ifstream rom;
  rom.open("tetris.gb");
  if(!rom.is_open()) {
    printf("Failed to open ROM: tetris.gb!");
    return -1;
  }

  // Run until X is pressed on window
  bool quit = false;
  SDL_Event e;
  while(!quit) {
    while(SDL_PollEvent(&e) != 0) {
      if(e.type == SDL_QUIT) {
        quit = true;
      }
      else if (e.type == SDL_KEYDOWN) {
        switch(e.key.keysym.sym) {
          case SDLK_UP:
          printf("UP\n");
          break;

          case SDLK_DOWN:
          printf("DOWN\n");
          break;

          case SDLK_RIGHT:
          printf("RIGHT\n");
          break;

          case SDLK_LEFT:
          printf("LEFT\n");
          break;

          case SDLK_z:
          printf("A\n");
          break;

          case SDLK_x:
          printf("B\n");
          break;

          default:
          break;
        }
      }
    }
  }
  rom.close();
  return 0;
}

// Shuts down SDL and frees resources
void close() {
  SDL_DestroyWindow(gWindow);
  gWindow = NULL;

  SDL_Quit();
}

int main(int argc, char* args[]) {
  if (!init()) { return -1; }
  bool result = loop();
  close();
  return result;
}
