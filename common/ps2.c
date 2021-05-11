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
#elif defined __ZPU__
    #include <stdint.h>
    #include "zpu_soc.h"
#elif defined __M68K__
    #include <stdint.h>
    #include "m68k_soc.h"
#endif

#include "ps2.h"
#include "interrupts.h"
#include "keyboard.h"

void ps2_ringbuffer_init(struct ps2_ringbuffer *r)
{
    r->in_hw=0;
    r->in_cpu=0;
    r->out_hw=0;
    r->out_cpu=0;
}

void ps2_ringbuffer_write(struct ps2_ringbuffer *r,int in)
{
    while(r->out_hw==((r->out_cpu+1)&(PS2_RINGBUFFER_SIZE-1)))
        ;
//    printf("w: %d, %d\n, %d\n",r->out_hw,r->out_cpu,in);
    DisableInterrupts();
    r->outbuf[r->out_cpu]=in;
    r->out_cpu=(r->out_cpu+1) & (PS2_RINGBUFFER_SIZE-1);
    PS2Handler();
    EnableInterrupts();
}


int ps2_ringbuffer_read(struct ps2_ringbuffer *r)
{
    unsigned char result;
    if(r->in_hw==r->in_cpu)
        return(-1);    // No characters ready
    result=r->inbuf[r->in_cpu];
    r->in_cpu=(r->in_cpu+1) & (PS2_RINGBUFFER_SIZE-1);
    return(result);
}

int ps2_ringbuffer_count(struct ps2_ringbuffer *r)
{
    if(r->in_hw>=r->in_cpu)
        return(r->in_hw-r->in_cpu);
    return(r->in_hw+PS2_RINGBUFFER_SIZE-r->in_cpu);
}

struct ps2_ringbuffer kbbuffer;
struct ps2_ringbuffer mousebuffer;


#if defined __ZPU__ 
void PS2Handler()
{
    int kbd;
    int mouse;

    DisableInterrupts();
    
    kbd=PS2_KEYBOARD(PS2_0);
    mouse=PS2_MOUSE(PS2_0);

    if(kbd & (1<<BIT_PS2_RECV))
    {
        kbbuffer.inbuf[kbbuffer.in_hw]=kbd&0xff;
        kbbuffer.in_hw=(kbbuffer.in_hw+1) & (PS2_RINGBUFFER_SIZE-1);
    }
    if(kbd & (1<<BIT_PS2_CTS))
    {
        if(kbbuffer.out_hw!=kbbuffer.out_cpu)
        {
            PS2_KEYBOARD(PS2_0)=kbbuffer.outbuf[kbbuffer.out_hw];
            kbbuffer.out_hw=(kbbuffer.out_hw+1) & (PS2_RINGBUFFER_SIZE-1);
        }
    }
    if(mouse & (1<<BIT_PS2_RECV))
    {
        mousebuffer.inbuf[mousebuffer.in_hw]=mouse&0xff;
        mousebuffer.in_hw=(mousebuffer.in_hw+1) & (PS2_RINGBUFFER_SIZE-1);
    }
    if(mouse & (1<<BIT_PS2_CTS))
    {
        if(mousebuffer.out_hw!=mousebuffer.out_cpu)
        {
            PS2_MOUSE(PS2_0)=mousebuffer.outbuf[mousebuffer.out_hw];
            mousebuffer.out_hw=(mousebuffer.out_hw+1) & (PS2_RINGBUFFER_SIZE-1);
        }
    }
    //GetInterrupts();    // Clear interrupt bit
    EnableInterrupts();
}
#elif defined __K64F__
void PS2Handler()
{
    EnableInterrupts();
}
#elif defined __M68K__
void PS2Handler()
{
    EnableInterrupts();
}
#else
    #error "Target CPU not defined, use __ZPU__, __K64F__ or __M68K__"
#endif

void PS2Init()
{
    ps2_ringbuffer_init(&kbbuffer);
    ps2_ringbuffer_init(&mousebuffer);
    SetIntHandler(&PS2Handler);
    ClearKeyboard();
}

#ifdef __cplusplus
    }
#endif
