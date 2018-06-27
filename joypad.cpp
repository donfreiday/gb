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

/* state:
 Bit 7 - down
 Bit 6 - up
 Bit 5 - left
 Bit 4 - right
 Bit 3 - start
 Bit 2 - select
 Bit 1 - b
 Bit 0 - a
 */

Joypad::Joypad() { state = 0xFF; }

void Joypad::keyPressed(SDL_Keycode key) {
  switch (key) {
    case SDLK_DOWN:
      bitClear(state, 7);
      break;

    case SDLK_UP:
      bitClear(state, 6);
      break;

    case SDLK_LEFT:
      bitClear(state, 5);
      break;

    case SDLK_RIGHT:
      bitClear(state, 4);
      break;

    case SDLK_RETURN:
      bitClear(state, 3);
      break;

    case SDLK_SPACE:
      bitClear(state, 2);
      break;

    case SDLK_z:
      bitClear(state, 1);
      break;

    case SDLK_x:
      bitClear(state, 0);
      break;
  }
}

void Joypad::keyReleased(SDL_Keycode key) {
    switch(key) {
    case SDLK_DOWN:
      bitSet(state, 7);
      break;

    case SDLK_UP:
      bitSet(state, 6);
      break;

    case SDLK_LEFT:
      bitSet(state, 5);
      break;

    case SDLK_RIGHT:
      bitSet(state, 4);
      break;

    case SDLK_RETURN:
      bitSet(state, 3);
      break;

    case SDLK_SPACE:
      bitSet(state, 2);
      break;

    case SDLK_z:
      bitSet(state, 1);
      break;

    case SDLK_x:
      bitSet(state, 0);
      break;
    }
}

u8 Joypad::read(u8 request) { 
    u8 result = 0;

    // Directions
    if (bitTest(request, 4)) {
        bitSet(result, 4);
        result |= (state >> 4); 
    } else { // Buttons
        bitSet(result, 5);
        result |= (state << 4) >> 4; // Clear high bits and shift back
    }
    return result;
}
