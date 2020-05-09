#ifndef UART_H
#define UART_H

/* Hardware registers for a supporting UART to the ZPUFlex project. */

#ifdef __cplusplus
extern "C" {
#endif

// Method to direct output to stdout or stddebug.
void set_serial_output(uint8_t);

int    _putchar(unsigned char);
int    dbgputchar(int);
void   _dbgputchar(unsigned char);
int    uart_puts(const char *);
char   getserial();
char   getdbgserial();
int8_t getserial_nonblocking();
int8_t getdbgserial_nonblocking();

// Only bring in stream functions for stdio.h
#if !defined(FUNCTIONALITY)
int    uart_putchar(char, FILE *);
int    uart_getchar(FILE *);
#endif

// Macros to put breadcrumbs out to the screen, for reference and debugging purposes.
#define breadcrumb(x)     UART_DATA(UART0)=x;

// Debug only macros which dont generate code when debugging disabled.
#ifdef DEBUG

    // Macro to print to the debug channel.
    //
    #define debugf(a, ...) ({\
                set_serial_output(1);\
                printf(a, ##__VA_ARGS__);\
                set_serial_output(0);\
               })
    #define dbg_putchar(a) ({\
                dbgputchar(a);\
               })
    #define dbg_puts(a) ({\
                set_serial_output(1);\
                uart_puts(a);\
                set_serial_output(0);\
               })
    #define dbg_breadcrumb(x) UART_DATA(UART1)=x;

#else

    #define dbg_putchar(a)
    #define dbg_puts(a)
    #define debugf(a, ...)
    #define dbg_breadcrumb(x) 

#endif

#ifdef __cplusplus
}
#endif

#endif // UART_H

