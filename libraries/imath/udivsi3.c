//#include "int_lib.h"
#include <stdint.h>

#define clz(a) (sizeof(a) == sizeof(unsigned long long) ? __builtin_clzll(a) : __builtin_clz(a))
#define CHAR_BIT 8

// Adapted from Figure 3-40 of The PowerPC Compiler Writer's Guide
static __inline uint32_t __udivXi3(uint32_t n, uint32_t d) {
  const unsigned N = sizeof(uint32_t) * CHAR_BIT;
  // d == 0 cases are unspecified.
  unsigned sr = (d ? clz(d) : N) - (n ? clz(n) : N);
  // 0 <= sr <= N - 1 or sr is very large.
  if (sr > N - 1) // n < d
    return 0;
  if (sr == N - 1) // d == 1
    return n;
  ++sr;
  // 1 <= sr <= N - 1. Shifts do not trigger UB.
  uint32_t r = n >> sr;
  n <<= N - sr;
  uint32_t carry = 0;
  for (; sr > 0; --sr) {
    r = (r << 1) | (n >> (N - 1));
    n = (n << 1) | carry;
    // Branch-less version of:
    // carry = 0;
    // if (r >= d) r -= d, carry = 1;
    const int32_t s = (int32_t)(d - r - 1) >> (N - 1);
    carry = s & 1;
    r -= d & s;
  }
  n = (n << 1) | carry;
  return n;
}
// Mostly identical to __udivXi3 but the return values are different.
static __inline uint32_t __umodXi3(uint32_t n, uint32_t d) {
  const unsigned N = sizeof(uint32_t) * CHAR_BIT;
  // d == 0 cases are unspecified.
  unsigned sr = (d ? clz(d) : N) - (n ? clz(n) : N);
  // 0 <= sr <= N - 1 or sr is very large.
  if (sr > N - 1) // n < d
    return n;
  if (sr == N - 1) // d == 1
    return 0;
  ++sr;
  // 1 <= sr <= N - 1. Shifts do not trigger UB.
  uint32_t r = n >> sr;
  n <<= N - sr;
  uint32_t carry = 0;
  for (; sr > 0; --sr) {
    r = (r << 1) | (n >> (N - 1));
    n = (n << 1) | carry;
    // Branch-less version of:
    // carry = 0;
    // if (r >= d) r -= d, carry = 1;
    const int32_t s = (int32_t)(d - r - 1) >> (N - 1);
    carry = s & 1;
    r -= d & s;
  }
  return r;
}
// Returns: a / b
uint32_t __udivsi3(uint32_t a, uint32_t b) {
//xprintf("udivsi3(1):a=%d, b=%d, udixX=%d\n", a, b,__udivXi3(a, b) );
  return __udivXi3(a, b);
}


int32_t __clzsi2(int32_t a) {
  uint32_t x = (uint32_t)a;
  int32_t t = ((x & 0xFFFF0000) == 0) << 4; // if (x is small) t = 16 else 0
  x >>= 16 - t;                            // x = [0 - 0xFFFF]
  uint32_t r = t;                            // r = [0, 16]
  // return r + clz(x)
  t = ((x & 0xFF00) == 0) << 3;
  x >>= 8 - t; // x = [0 - 0xFF]
  r += t;      // r = [0, 8, 16, 24]
  // return r + clz(x)
  t = ((x & 0xF0) == 0) << 2;
  x >>= 4 - t; // x = [0 - 0xF]
  r += t;      // r = [0, 4, 8, 12, 16, 20, 24, 28]
  // return r + clz(x)
  t = ((x & 0xC) == 0) << 1;
  x >>= 2 - t; // x = [0 - 3]
  r += t;      // r = [0 - 30] and is even
  // return r + clz(x)
  //     switch (x)
  //     {
  //     case 0:
  //         return r + 2;
  //     case 1:
  //         return r + 1;
  //     case 2:
  //     case 3:
  //         return r;
  //     }
  return r + ((2 - x) & -((x & 2) == 0));
}
