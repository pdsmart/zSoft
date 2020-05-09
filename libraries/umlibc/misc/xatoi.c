/*------------------------------------------------------------------------/
/  Universal String Handler for Console Input and Output
/-------------------------------------------------------------------------/
/
/ Copyright (C) 2014, ChaN, all right reserved.
/
/ xprintf module is an open source software. Redistribution and use of
/ xprintf module in source and binary forms, with or without modification,
/ are permitted provided that the following condition is met:
/
/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/
/-------------------------------------------------------------------------*/

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
#endif


/*----------------------------------------------*/
/* Get a value of the string                    */
/*----------------------------------------------*/
/*    "123 -5   0x3ff 0b1111 0377  w "
        ^                           1st call returns 123 and next ptr
           ^                        2nd call returns -5 and next ptr
                   ^                3rd call returns 1023 and next ptr
                          ^         4th call returns 15 and next ptr
                               ^    5th call returns 255 and next ptr
                                  ^ 6th call fails and returns 0
*/

int xatoi (                 /* 0:Failed, 1:Successful */
           char **str,      /* Pointer to pointer to the string */
           long *res        /* Pointer to the valiable to store the value */
)
{
    unsigned long val;
    unsigned char c, r, s = 0;


    *res = 0;

    while ((c = **str) == ' ') (*str)++;    /* Skip leading spaces */

    if (c == '-') {        /* negative? */
        s = 1;
        c = *(++(*str));
    }

    if (c == '0') {
        c = *(++(*str));
        switch (c) {
        case 'x':        /* hexdecimal */
            r = 16; c = *(++(*str));
            break;
        case 'b':        /* binary */
            r = 2; c = *(++(*str));
            break;
        default:
            if (c <= ' ') return 1;    /* single zero */
            if (c < '0' || c > '9') return 0;    /* invalid char */
            r = 8;        /* octal */
        }
    } else {
        if (c < '0' || c > '9') return 0;    /* EOL or invalid char */
        r = 10;            /* decimal */
    }

    val = 0;
    while (c > ' ') {
        if (c >= 'a') c -= 0x20;
        c -= '0';
        if (c >= 17) {
            c -= 7;
            if (c <= 9) return 0;    /* invalid char */
        }
        if (c >= r) return 0;        /* invalid char for current radix */
        val = val * r + c;
        c = *(++(*str));
    }

    if (s) val = 0 - val;            /* apply sign if needed */

    *res = val;
    return 1;
}

#ifdef __cplusplus
    }
#endif
