#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#ifdef __cplusplus
extern "C" {
#endif

// Prototypes.
void SetIntHandler(void(*handler)());
void EnableInterrupt(uint32_t);
void DisableInterrupt(uint32_t);
extern void DisableInterrupts(void);
extern void EnableInterrupts(void);

#ifdef __cplusplus
}
#endif

#endif

