// gb: a Gameboy Emulator by Don Freiday
// File: joypad.hpp
// Description: Maintains joypad state

#include "joypad.hpp"

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

void Joypad::handleEvent(SDL_Event e) {
  if (e.type == SDL_KEYDOWN) {
    switch (e.key.keysym.sym) {
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

      default:
        break;
    }
  } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
    switch (e.cbutton.button) {
      case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        bitClear(directions, DOWN);
        break;

      case SDL_CONTROLLER_BUTTON_DPAD_UP:
        bitClear(directions, UP);
        break;

      case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        bitClear(directions, LEFT);
        break;

      case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        bitClear(directions, RIGHT);
        break;

      case SDL_CONTROLLER_BUTTON_START:
        bitClear(buttons, START);
        break;

      case SDL_CONTROLLER_BUTTON_BACK:
        bitClear(buttons, SELECT);
        break;

      case SDL_CONTROLLER_BUTTON_A:
        bitClear(buttons, A);
        break;

      case SDL_CONTROLLER_BUTTON_B:
        bitClear(buttons, B);
        break;

      // I prefer square button for B :-D
      case 2:
        bitClear(buttons, B);
        break;

      default:
        break;
    }
  } else if (e.type == SDL_KEYUP) {
    switch (e.key.keysym.sym) {
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

      default:
        break;
    }
  } else if (e.type == SDL_CONTROLLERBUTTONUP) {
    switch (e.cbutton.button) {
      case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        bitSet(directions, DOWN);
        break;

      case SDL_CONTROLLER_BUTTON_DPAD_UP:
        bitSet(directions, UP);
        break;

      case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        bitSet(directions, LEFT);
        break;

      case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        bitSet(directions, RIGHT);
        break;

      case SDL_CONTROLLER_BUTTON_START:
        bitSet(buttons, START);
        break;

      case SDL_CONTROLLER_BUTTON_BACK:
        bitSet(buttons, SELECT);
        break;

      case SDL_CONTROLLER_BUTTON_A:
        bitSet(buttons, A);
        break;

      case SDL_CONTROLLER_BUTTON_B:
        bitSet(buttons, B);
        break;

        // I prefer square button for B :-D
      case 2:
        bitSet(buttons, B);
        break;

      default:
        break;
    }
  }
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
