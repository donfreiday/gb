// gb: a Gameboy Emulator by Don Freiday
// File: joypad.h
// Description: Maintains joypad state

#include "joypad.h"

/*
 Bit 7 - Not used
 Bit 6 - Not used
 Bit 5 - P15 Select Button Keys      (0=Select)
 Bit 4 - P14 Select Direction Keys   (0=Select)
 Bit 3 - P13 Input Down  or Start    (0=Pressed) (Read Only)
 Bit 2 - P12 Input Up    or Select   (0=Pressed) (Read Only)
 Bit 1 - P11 Input Left  or Button B (0=Pressed) (Read Only)
 Bit 0 - P10 Input Right or Button A (0=Pressed) (Read Only)

 Todo: INT 60 - Joypad Interrupt
 */

#define DOWN 3   // 0000 1000
#define UP 2     // 0000 0100
#define LEFT 1   // 0000 0010
#define RIGHT 0  // 0000 0001

#define START 3   // 0000 1000
#define SELECT 2  // 0000 0100
#define B 1       // 0000 0010
#define A 0       // 0000 0001

Joypad::Joypad() {
  buttons = 0xDF;     // 1101 1111
  directions = 0xEF;  // 1110 1111
}

void Joypad::keyPressed(SDL_Keycode key) {
  switch (key) {
    case SDLK_DOWN:
      bitClear(directions, DOWN);
      break;

    case SDLK_UP:
      bitClear(directions, UP);
      break;

    case SDLK_LEFT:
      bitClear(directions, LEFT);
      break;

    case SDLK_RIGHT:
      bitClear(directions, RIGHT);
      break;

    case SDLK_RETURN:
      bitClear(buttons, START);
      break;

    case SDLK_SPACE:
      bitClear(buttons, SELECT);
      break;

    case SDLK_z:
      bitClear(buttons, A);
      break;

    case SDLK_x:
      bitClear(buttons, B);
      break;
  }
}

void Joypad::keyReleased(SDL_Keycode key) {
  switch (key) {
    case SDLK_DOWN:
      bitSet(directions, DOWN);
      break;

    case SDLK_UP:
      bitSet(directions, UP);
      break;

    case SDLK_LEFT:
      bitSet(directions, LEFT);
      break;

    case SDLK_RIGHT:
      bitSet(directions, RIGHT);
      break;

    case SDLK_RETURN:
      bitSet(buttons, START);
      break;

    case SDLK_SPACE:
      bitSet(buttons, SELECT);
      break;

    case SDLK_z:
      bitSet(buttons, A);
      break;

    case SDLK_x:
      bitSet(buttons, B);
      break;
  }
}

void Joypad::handleControllers(
    SDL_GameController* controllers[MAX_CONTROLLERS]) {
  /*for (int i = 0; i < MAX_CONTROLLERS; i++) {
    if (controllers[i] && SDL_GameControllerGetAttached(controllers[i])) {
      if (SDL_GameControllerGetButton(controllers[i],
                                      SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
        bitClear(directions, DOWN);
      } else {
        bitSet(directions, DOWN);
      }

      if (SDL_GameControllerGetButton(controllers[i],
                                      SDL_CONTROLLER_BUTTON_DPAD_UP)) {
        bitClear(directions, UP);
      } else {
        bitSet(directions, UP);
      }

      if (SDL_GameControllerGetButton(controllers[i],
                                      SDL_CONTROLLER_BUTTON_DPAD_LEFT)) {
        bitClear(directions, LEFT);
      } else {
        bitSet(directions, LEFT);
      }

      if (SDL_GameControllerGetButton(controllers[i],
                                      SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) {
        bitClear(directions, RIGHT);
      } else {
        bitSet(directions, RIGHT);
      }

      if (SDL_GameControllerGetButton(controllers[i],
                                      SDL_CONTROLLER_BUTTON_START)) {
        bitClear(buttons, START);
      } else {
        bitSet(directions, START);
      }

      if (SDL_GameControllerGetButton(controllers[i],
                                      SDL_CONTROLLER_BUTTON_BACK)) {
        bitClear(buttons, SELECT);
      } else {
        bitSet(directions, SELECT);
      }

      if (SDL_GameControllerGetButton(controllers[i],
                                      SDL_CONTROLLER_BUTTON_A)) {
        bitClear(buttons, A);
      } else {
        bitSet(directions, A);
      }

      if (SDL_GameControllerGetButton(controllers[i],
                                      SDL_CONTROLLER_BUTTON_B)) {
        bitClear(buttons, B);
      } else {
        bitSet(directions, B);
      }
    }
  }*/
}

u8 Joypad::read(u8 request) {
  // Directions
  if (!bitTest(request, 5)) {
    return directions;
  }

  // Buttons
  else if (!bitTest(request, 4)) {
    return buttons;
  }

  return 0xFF;
}
