#ifndef COMMON
#define COMMON

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long int s64;

template <typename t>
bool bitTest(t num, u8 pos) {
  return num & (1 << pos);
}

template <typename t>
void bitSet(t &num, u8 pos) {
  num |= (1 << pos);
}

template <typename t>
void bitClear(t &num, u8 pos) {
  num &= ~(1 << pos);
}

#endif
