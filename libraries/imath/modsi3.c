#include <stdint.h>
//#include "int_lib.h"
// Returns: a % b
int32_t __modsi3(int32_t a, int32_t b) {
  return a - __divsi3(a, b) * b;
}
