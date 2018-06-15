#include "joypad.h"

Joypad::Joypad() {
  state = 0;
}
void Joypad::keyPressed(u8 key) {

}
void Joypad::keyReleased(u8 key) {
  bitSet(state, key);
}
u8 Joypad::getState() {
  return 0;
}
