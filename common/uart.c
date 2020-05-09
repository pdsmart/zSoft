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
#else
  #include <stdint.h>
  #if !defined(FUNCTIONALITY)
    #include <stdio.h>
  #endif
  #include <string.h>
  #include "zpu_soc.h"
#endif

#include "uart.h"

static uint8_t uart_channel = 0;

inline int _putchar(unsigned char c)
{
    uint32_t status;

    do {
        status = UART_STATUS(uart_channel == 0 ? UART0 : UART1);
    } while((UART_IS_TX_FIFO_ENABLED(status) && UART_IS_TX_FIFO_FULL(status)) || (UART_IS_TX_FIFO_DISABLED(status) && UART_IS_TX_DATA_LOADED(status)));
    UART_DATA(uart_channel == 0 ? UART0 : UART1) = (int)c;

    return(c);
}

inline void set_serial_output(uint8_t c)
{
    uart_channel = (c == 0 ? 0 : 1);
}


#if !defined(FUNCTIONALITY)
// Stream version of putchar for stdio.
//
int uart_putchar(char c, FILE *stream)
{
    uint32_t status;

    // Add CR when NL detected.
    if (c == '\n')
        uart_putchar('\r', stream);

    // Wait for a slot to become available in UART buffer and then send character.
    do {
        status = UART_STATUS(uart_channel == 0 ? UART0 : UART1);
    } while((UART_IS_TX_FIFO_ENABLED(status) && UART_IS_TX_FIFO_FULL(status)) || (UART_IS_TX_FIFO_DISABLED(status) && UART_IS_TX_DATA_LOADED(status)));
    UART_DATA(uart_channel == 0 ? UART0 : UART1) = c;

    return(0);
}
#endif

#if !defined(FUNCTIONALITY) || FUNCTIONALITY <= 2
inline int dbgputchar(int c)
{
    uart_channel = 1;
    _putchar(c);
    uart_channel = 0;

    return(c);
}

inline void _dbgputchar(unsigned char c)
{
    dbgputchar(c);
}
#endif

#ifdef USELOADB
int uart_puts(const char *msg)
{
    int result = 0;

    while (*msg) {
		_putchar(*msg++);
        ++result;
	}
    return(result);
}
#else
int uart_puts(const char *msg)
{
    int c;
    int result=0;
    // Because we haven't implemented loadb from ROM yet, we can't use *<char*>++.
    // Therefore we read the source data in 32-bit chunks and shift-and-split accordingly.
    int *s2=(int*)msg;

    do
    {
        int i;
        int cs=*s2++;
        for(i=0;i<4;++i)
        {
            c=(cs>>24)&0xff;
            cs<<=8;
            if(c==0)
                return(result);
            _putchar(c);
            ++result;
        }
    }
    while(c);
    return(result);
}
#endif

#if !defined(FUNCTIONALITY) || FUNCTIONALITY <= 1
char getserial()
{
    uint32_t reg;

    do {
        reg = UART_STATUS(uart_channel == 0 ? UART0 : UART1);
    } while(!UART_IS_RX_DATA_READY(reg));
    reg=UART_DATA(uart_channel == 0 ? UART0 : UART1);

    return((char)reg & 0xFF);
}

#if !defined(FUNCTIONALITY)
// Stream version of getchar for stdio.
//
int uart_getchar(FILE *stream)
{
    return((int)getserial());
}
#endif

int8_t getserial_nonblocking()
{
    int8_t reg;

    reg = UART_STATUS(uart_channel == 0 ? UART0 : UART1);
    if(!UART_IS_RX_DATA_READY(reg))
    {
        reg = -1;
    }
    else
    {
        reg = UART_DATA(uart_channel == 0 ? UART0 : UART1);
    }

    return(reg);
}

char getdbgserial()
{
    int32_t reg = 0;

    set_serial_output(1);
    reg = getserial();
    set_serial_output(0);
    return((char)reg & 0xFF);
}

int8_t getdbgserial_nonblocking()
{
    int8_t reg = 0;

    set_serial_output(1);
    reg = getserial_nonblocking();
    set_serial_output(0);
    return(reg);
}
#endif

#ifdef __cplusplus
    }
#endif
