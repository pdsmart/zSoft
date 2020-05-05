#ifdef __cplusplus
    extern "C" {
#endif

#if defined __K64F__
    #include <stdlib.h>
    #include <string.h>
    #define uint32_t __uint32_t
    #define uint16_t __uint16_t
    #define uint8_t  __uint8_t
    #define int32_t  __int32_t
    #define int16_t  __int16_t
    #define int8_t   __int8_t
    #include "k64f_soc.h"
#else
    #include <stdint.h>
    #include "zpu_soc.h"
#endif

#include "interrupts.h"

extern void (*_inthandler_fptr)();

#if defined __K64F__
void SetIntHandler(void(*handler)())
{
    return;
}
#elif defined __ZPU__
void SetIntHandler(void(*handler)())
{
    _inthandler_fptr=handler;
}
#else
  #error "Target CPU not defined, use __ZPU__ or __K64F__"
#endif

#if !defined(FUNCTIONALITY) || FUNCTIONALITY <= 2
// Method to enable individual interrupts.
//
static uint32_t intrSetting = 0;
void EnableInterrupt(uint32_t intrMask)
{
  #if defined __ZPU__
    uint32_t currentIntr = INTERRUPT_CTRL(INTR0);
    INTERRUPT_CTRL(INTR0) = 0;
    currentIntr &= ~intrMask;
    currentIntr |= intrMask;
    intrSetting = currentIntr;
    INTERRUPT_CTRL(INTR0) = intrSetting; 
  #elif defined __K64F__
    intrSetting = 0;
  #else
    #error "Target CPU not defined, use __ZPU__ or __K64F__"
  #endif
}

// Method to disable individual interrupts.
//
void DisableInterrupt(uint32_t intrMask)
{
  #if defined __ZPU__
    intrSetting = INTERRUPT_CTRL(INTR0);
    INTERRUPT_CTRL(INTR0) = 0;
    intrSetting &= ~intrMask;
    INTERRUPT_CTRL(INTR0) = intrSetting;
  #elif defined __K64F__
  #else
    #error "Target CPU not defined, use __ZPU__ or __K64F__"
  #endif
}

// Method to disable interrupts.
//
inline void DisableInterrupts(void)
{
#if defined __ZPU__
    INTERRUPT_CTRL(INTR0) = 0;
#endif
}

// Method to enable interrupts.
//
inline void EnableInterrupts(void)
{
#if defined __ZPU__
    INTERRUPT_CTRL(INTR0) = intrSetting;
#endif
}

#endif // Functionality.

#ifdef __cplusplus
    }
#endif
