#include <stdint.h>
//#include "int_lib.h"

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
// Returns: a % b
uint32_t __umodsi3(uint32_t a, uint32_t b) {
  return __umodXi3(a, b);
}
