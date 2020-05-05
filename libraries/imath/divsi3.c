#include <stdint.h>
//#include "int_lib.h"
// Returns: a / b
#define CHAR_BIT 8
int32_t __divsi3(int32_t a, int32_t b) {
  const int bits_in_word_m1 = (int)(sizeof(int32_t) * CHAR_BIT) - 1;
  int32_t s_a = a >> bits_in_word_m1; // s_a = a < 0 ? -1 : 0
  int32_t s_b = b >> bits_in_word_m1; // s_b = b < 0 ? -1 : 0

//xprintf("divsi3(1):a=%d, b=%d, s_a=%d, s_b=%s\n", a, b, s_a, s_b);
  a = (a ^ s_a) - s_a;               // negate if s_a == -1
  b = (b ^ s_b) - s_b;               // negate if s_b == -1
  s_a ^= s_b;                        // sign of quotient
//xprintf("divsi3(2):a=%d, b=%d, s_a=%d, s_b=%s\n", a, b, s_a, s_b);

  // On CPUs without unsigned hardware division support,
  //  this calls __udivsi3 (notice the cast to su_int).
  // On CPUs with unsigned hardware division support,
  //  this uses the unsigned division instruction.
  //
  return ((uint32_t)a / (uint32_t)b ^ s_a) - s_a; // negate if s_a == -1
}
