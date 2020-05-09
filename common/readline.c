/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            readline.c
// Created:         April 2020
// Author(s):       Alfred Klomp (www.alfredklomp.com) - original readline.c
//                  Philip Smart - Port to K64f/ZPU and many upgrades, ie. additions of history buffer logic.
// Description:     A readline module for embedded systems without the overhead of GNU readline or the
//                  need to use > C99 standard as the ZPU version of GCC doesnt support it!
//                  The readline modules originally came from Alfred Klomp's Raduino project and whilst
//                  I was searching for an alternative to GNU after failing to compile it with the ZPU
//                  version of GCC (as well as the size), I came across RADUINO and this readline
//                  module was very well written and easily portable to this project.
//
// Credits:         
// Copyright:       (c) -> 2016    - Alfred Klomp (www.alfredklomp.com) original code
//                  (C) 2020       - Philip Smart <philip.smart@net2net.org> porting, history buf.
//                                   local commands and updates.
//
// History:         April 2020     - With the advent of the tranZPUter SW, I needed a better command
//                                   line module for entering, recalling and editting commands. This
//                                   module after adding history buffer mechanism is the ideal
//                                   solution.
//                                   features of zOS where applicable.
//
// Notes:           See Makefile to enable/disable conditional components
//                  __SD_CARD__           - Add the SDCard logic.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// This source file is free software: you can redistribute it and#or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This source file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
    extern "C" {
#endif

#if defined __K64F__
  #include    <stdio.h>
  #include    <string.h>
  #include    <stdint.h>
  #include    <../libraries/include/stdmisc.h>
#else
  #include    <stdio.h>
  #include    <stdint.h>
  #include    <stdmisc.h>
#endif

#if defined __SD_CARD__
  #include    <ff.h>
#endif

#include      <stdlib.h>
#include      <stdbool.h>
#include      <stddef.h>

#if defined __APP__
  #if defined __K64F__
    #undef      stdin
    #undef      stdout
    #undef      stderr
    extern FILE *stdout;
    extern FILE *stdin;
    extern FILE *stderr;
  #endif

  #define     malloc     sys_malloc
  #define     realloc    sys_realloc
  #define     calloc     sys_calloc
  #define     free       sys_free
  void        *sys_malloc(size_t);            // Allocate memory managed by the OS.
  void        *sys_realloc(void *, size_t);   // Reallocate a block of memory managed by the OS.
  void        *sys_calloc(size_t, size_t);    // Allocate and zero a block of memory managed by the OS.
  void        sys_free(void *);               // Free memory managed by the OS.
#endif

// List of local commands processed by readline.
#define CMD_HISTORY        0x01
#define CMD_RECALL         0x02

// Special key codes recognised by readline.
#define CTRL_A             0x01
#define CTRL_B             0x02
#define CTRL_C             0x03
#define CTRL_D             0x04
#define CTRL_E             0x05
#define CTRL_F             0x06
#define BACKSPACE          0x08
#define CTRL_K             0x0b
#define ENTER              0x0d
#define CTRL_N             0x0e
#define CTRL_P             0x10
#define ESC                0x1b
#define RIGHTBRACKET       0x5b
#define TILDA              0x7e 

// Line buffer control variables.
static uint8_t llen, lpos;

#define MAX_HISTORY_LINES 20
static uint8_t *history[MAX_HISTORY_LINES] = {};
static uint8_t histFreeSlot = 0;

#if defined __SD_CARD__
static FIL     *histFp = NULL;
static uint8_t histDisabled = 0;
#endif

// Keys we distinguish:
enum keytype {
    KEY_REGULAR,
    KEY_BKSP,
    KEY_CTRL_A,
    KEY_CTRL_B,
    KEY_CTRL_C,
    KEY_CTRL_D,
    KEY_CTRL_E,
    KEY_CTRL_F,
    KEY_CTRL_K,
    KEY_CTRL_N,
    KEY_CTRL_P,
    KEY_ENTER,
    KEY_INSERT,
    KEY_HOME,
    KEY_DEL,
    KEY_END,
    KEY_PGUP,
    KEY_PGDN,
    KEY_ARROWUP,
    KEY_ARROWDN,
    KEY_ARROWRT,
    KEY_ARROWLT,
};

// Scancodes for special keys, sorted lexicographically:
static const uint8_t key_ctrl_a[]     = { CTRL_A                                };
static const uint8_t key_ctrl_b[]     = { CTRL_B                                };
static const uint8_t key_ctrl_c[]     = { CTRL_C                                };
static const uint8_t key_ctrl_d[]     = { CTRL_D                                };
static const uint8_t key_ctrl_e[]     = { CTRL_E                                };
static const uint8_t key_ctrl_f[]     = { CTRL_F                                };
static const uint8_t key_bksp[]       = { BACKSPACE                             };
static const uint8_t key_ctrl_k[]     = { CTRL_K                                };
static const uint8_t key_enter[]      = { ENTER                                 };
static const uint8_t key_ctrl_n[]     = { CTRL_N                                };
static const uint8_t key_ctrl_p[]     = { CTRL_P                                };
static const uint8_t key_home[]       = { ESC,      RIGHTBRACKET, '1', TILDA    };
static const uint8_t key_insert[]     = { ESC,      RIGHTBRACKET, '2', TILDA    };
static const uint8_t key_del[]        = { ESC,      RIGHTBRACKET, '3', TILDA    };
static const uint8_t key_end_1[]      = { ESC,      '0',          'F'           };
static const uint8_t key_end_2[]      = { ESC,      RIGHTBRACKET, '4', TILDA    };
static const uint8_t key_pgup[]       = { ESC,      RIGHTBRACKET, '5', TILDA    };
static const uint8_t key_pgdn[]       = { ESC,      RIGHTBRACKET, '6', TILDA    };
static const uint8_t key_arrowup[]    = { ESC,      RIGHTBRACKET, 'A'           };
static const uint8_t key_arrowdn[]    = { ESC,      RIGHTBRACKET, 'B'           };
static const uint8_t key_arrowrt[]    = { ESC,      RIGHTBRACKET, 'C'           };
static const uint8_t key_arrowlt[]    = { ESC,      RIGHTBRACKET, 'D'           };

// Table of special keys. We could store this in PROGMEM, but we don't,
// because the code to retrieve the data is much larger than the savings.
static const struct key {
    const uint8_t    *code;
    uint8_t          len;
    enum keytype     type;
}
// Table is order sensitive.
keys[] = {
    { key_ctrl_a,     sizeof(key_ctrl_a),     KEY_CTRL_A     },
    { key_ctrl_b,     sizeof(key_ctrl_b),     KEY_CTRL_B     },
    { key_ctrl_c,     sizeof(key_ctrl_c),     KEY_CTRL_C     },
    { key_ctrl_d,     sizeof(key_ctrl_d),     KEY_CTRL_D     },
    { key_ctrl_e,     sizeof(key_ctrl_e),     KEY_CTRL_E     },
    { key_ctrl_f,     sizeof(key_ctrl_f),     KEY_CTRL_F     },
    { key_bksp,       sizeof(key_bksp),       KEY_BKSP       },
    { key_ctrl_k,     sizeof(key_ctrl_k),     KEY_CTRL_K     },
    { key_enter,      sizeof(key_enter),      KEY_ENTER      },
    { key_ctrl_n,     sizeof(key_ctrl_n),     KEY_CTRL_N     },
    { key_ctrl_p,     sizeof(key_ctrl_p),     KEY_CTRL_P     },
    { key_home,       sizeof(key_home),       KEY_HOME       },
    { key_insert,     sizeof(key_insert),     KEY_INSERT     },
    { key_del,        sizeof(key_del),        KEY_DEL        },
    { key_end_1,      sizeof(key_end_1),      KEY_END        },
    { key_end_2,      sizeof(key_end_2),      KEY_END        },
    { key_pgup,       sizeof(key_pgup),       KEY_PGUP       },
    { key_pgdn,       sizeof(key_pgdn),       KEY_PGDN       },
    { key_arrowup,    sizeof(key_arrowup),    KEY_ARROWUP    },
    { key_arrowdn,    sizeof(key_arrowdn),    KEY_ARROWDN    },
    { key_arrowrt,    sizeof(key_arrowrt),    KEY_ARROWRT    },
    { key_arrowlt,    sizeof(key_arrowlt),    KEY_ARROWLT    },
};

// Local command list.
//
typedef struct {
    const char  *cmd;
    uint8_t     key;
} t_cmdstruct;

// Table of supported local commands. The table contains the command and the case value to act on for the command.
static t_cmdstruct cmdTable[] = {
    { "history",   CMD_HISTORY },
    { "hist",      CMD_HISTORY },
    { "!",         CMD_RECALL },
};

// Define the number of local commands in the array.
#define NCMDKEYS (sizeof(cmdTable)/sizeof(t_cmdstruct))

static bool
key_matches (const uint8_t c, const int8_t idx, const int8_t pos)
{
    // If key is too short, skip:
    if (pos >= keys[idx].len)
        return false;

    // Character must match:
    return (c == keys[idx].code[pos]);
}

static bool find_key (const uint8_t c, int8_t *idx, const int8_t pos)
{
    int8_t  i;

    // If character matches current index, return:
    if (c == keys[*idx].code[pos])
        return true;

    // If character is less than current index, seek up:
    if (c < keys[*idx].code[pos])
    {
        for (i = *idx - 1; i >= 0; i--)
        {
            if (key_matches(c, i, pos))
            {
                *idx = i;
                return true;
            }
        }
        return false;
    }

    // If character is greater than current index, seek down:
    else {
        for (i = *idx + 1; i < sizeof(keys) / sizeof(keys[0]); i++) {
            if (key_matches(c, i, pos)) {
                *idx = i;
                return true;
            }
        }
        return false;
    }
}

// Get next character from serial port.
static bool next_char (enum keytype *type, uint8_t *val)
{
    static int8_t idx, pos;
    int8_t keyIn;

    // Process all available bytes from the serial port.
    do {
     #if defined __K64F__
	 int usb_serial_getchar(void);
         keyIn = usb_serial_getchar();
     #elif defined __ZPU__
         keyIn = getserial_nonblocking();
     #else
         #error "Target CPU not defined, use __ZPU__ or __K64F__"
     #endif

        if(keyIn != -1)
        {
            // Try to find a matching key from the special keys table:
            if (find_key(keyIn, &idx, pos))
            {
    
                // If we are at max length, we're done:
                if (++pos == keys[idx].len)
                {
                    *type = keys[idx].type;
                    idx = pos = 0;
                    return true;
                }

                // Otherwise, need more input to be certain:
                continue;
            } else
            {
                // No matching special key: it's a regular character:
                idx = pos = 0;
                *val = keyIn;
                *type = KEY_REGULAR;
                return true;
            }
        }
    } while(true);

    return false;
}

// Short method to scrub previous line contents off screen and replace with new contents.
void refreshLine(uint8_t *line, uint8_t llen, uint8_t *lpos)
{
    while (*lpos)
    {
        fputc('\b', stdout);
        fputc(' ',  stdout);
        fputc('\b', stdout);
        (*lpos)--;
    }
    printf((char *)line);
    *lpos = llen;
    return;
}

// Short method to add a command line onto the end of the history list. If the SD Card is enabled, add the command
// to the end of the history file as well.
//
void addToHistory(char *buf, uint8_t bytes, uint8_t addHistFile)
{
    // If the slot in history is already being used, free the memory before allocating a new block suitable to the new line size.
    if(history[histFreeSlot] != NULL)
        free(history[histFreeSlot]);
    history[histFreeSlot] = malloc(bytes+1);

    // If malloc was successful, populate the memory with the command entered.
    if(history[histFreeSlot] != NULL)
    {
        // Copy the command into history and update the pointer, wrap around if needed.
        memcpy(history[histFreeSlot], buf, bytes+1);
        histFreeSlot++;
        if(histFreeSlot >= MAX_HISTORY_LINES)
            histFreeSlot = 0;
    }

    // Add the command to the history file if selected and the SD Card is enabled.
    //
    if(addHistFile)
    {
      #if defined __SD_CARD__
        unsigned int writeBytes;

        if(histFp != NULL && histDisabled == 0)
        {
            // Write out the line into the history file.
            f_write(histFp, buf, bytes, &writeBytes);
            f_putc('\n', histFp);
            f_sync(histFp);
        }
      #endif
    }
}

// Method to clear out the history buffer and return used memory back to the heap.
//
void clearHistory(void)
{
    int   idx;

    // Simply loop through and free used memory, reset pointer to the start.
    //
    for(idx = 0; idx < MAX_HISTORY_LINES; idx++)
    {
        if(history[idx] != NULL)
        {
            free(history[idx]);
            history[idx] = NULL;
        }
    }
    histFreeSlot = 0;

    // If the history file is being used, close it and free up its memory.
  #if defined __SD_CARD__
    if(histFp != NULL)
    {
        f_close(histFp);
        free(histFp);
        histFp = NULL;
    }
  #endif
}

// Local command to display the history contents.
void cmdPrintHistory(void)
{
    // Locals.
    char         buf[120];
    FRESULT      fr;
    int          bytes;
    uint32_t     lineCnt = 1;

    // Rewind the history file to the beginning.
    //
    fr = f_lseek(histFp, 0);
    if(!fr)
    {
        // Simply loop from start to end adding to buffer, if the file history is larger than the memory buffer we just loop round.
        do {
            bytes = -1;
            if(f_gets(buf, sizeof(buf), histFp) != NULL)
            {
                // Get number of bytes read.
                bytes = strlen(buf);
                if(bytes > 0)
                {
                    // Remove the CR/newline terminator.
                    bytes      -= 1;
                    buf[bytes]  = 0x0;

                    // Print it out.
                    printf("%04lu  %s\n", lineCnt++, buf);
                }
            }
        } while(bytes != -1);
    }
}

// Local command to load a specific command into the line buffer.
uint8_t cmdRecallHistory(uint8_t *line, uint8_t *llen, uint32_t lineNo)
{
    // Locals.
    char         buf[120];
    FRESULT      fr;
    int          bytes;
    uint32_t     lineCnt = 1;
    uint8_t      retCode = 1;

    // Rewind the history file to the beginning.
    //
    fr = f_lseek(histFp, 0);
    if(!fr)
    {
        // Simply loop from start to end until we reach the required line, load it into the line buffer and forward to the end of file.
        do {
            bytes = -1;
            if(f_gets(buf, sizeof(buf), histFp) != NULL)
            {
                // Get number of bytes read.
                bytes = strlen(buf);
                lineCnt++;
                if(bytes > 0)
                {
                    // Remove the CR/newline terminator.
                    buf[--bytes]  = 0x0;
                }
            }
        } while(bytes != -1 && lineNo != lineCnt);

        // Go to end of file.
        fr = f_lseek(histFp, f_size(histFp));
        if(fr)
        {
            printf("Failed to reset the history file to EOF.");
        }

        if(lineNo == lineCnt)
        {
            strcpy((char *)line, buf);
            *llen = strlen((char *)line);
            retCode = 0;
        }
    }
    return(retCode);
}


// Check to see if the given command is local and process as necessary.
//
uint8_t localCommand(uint8_t *line, uint8_t *lineSize)
{
    uint8_t idx;
    char    *paramptr = (char *)line;
    long    lineNo;

    // Skip leading whitespace.
    while(*paramptr == ' ' && paramptr < (char *)line + *lineSize) paramptr++;

    // Loop through all the commands and try to find a match.
    for (idx=0; idx < NCMDKEYS; idx++)
    {
        t_cmdstruct *sym = &cmdTable[idx];
        if (strncmp(sym->cmd, paramptr,strlen(sym->cmd)) == 0)
        {
            // Process the command
            switch(sym->key)
            {
                // Print out the history buffer.
                //
                case CMD_HISTORY:
                    cmdPrintHistory();
                    return(1);

                // Recall a command from the history buffer.
                //
                case CMD_RECALL:
                    paramptr++;
                    if(xatoi(&paramptr, &lineNo))
                    {
                        if(cmdRecallHistory(line, lineSize, lineNo-1) == 0)
                        {
                            // Return no local command result as we want the caller to process the newly filled line buffer.
                            return(0);
                        }
                    }
                    break;

                // This case shouldnt exist as a handler for each command should exist.
                // If this case is triggered then fall through as though there was no command.
                default:
                    break;
            }
        }
    }

    // Not a local command.
    return(0);
}

// Take characters from the Rx FIFO and create a line
uint8_t *readline (uint8_t *line, int lineSize, const char *histFile)
{
    enum keytype   type;
    uint8_t        val = 0;
    int8_t         i;
    uint8_t        histPnt = histFreeSlot;

  #if defined __SD_CARD__
    char         buf[120];
    FRESULT      fr;
    int          bytes;

    // If we havent opened the history file and read the contents AND file based history hasnt been disabled, open the history file and read
    // the contents.
    if(histFp == NULL && histDisabled == 0 && histFile != NULL)
    {
        // Allocate a file control block on the heap and open the history file.
        histFp = malloc(sizeof(FIL));

        if(histFp != NULL)
        {
            fr = f_open(histFp, histFile, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
            if(fr != FR_OK)
            {
                printf("Cannot open/create history file, disabling.\n");
            } else
            {
                // Simply loop from start to end adding to buffer, if the file history is larger than the memory buffer we just loop round.
                histPnt = 0;
                do {
                    bytes = -1;
                    if(f_gets(buf, sizeof(buf), histFp) == buf)
                    {
                        // Get number of bytes read.
                        bytes = strlen(buf);
                        if(bytes > 0)
                        {
                            // Remove the CR/newline terminator.
                            bytes      -= 1;
                            buf[bytes]  = 0x0;
                        
                            // Add the line into our history list.
                            addToHistory(buf, bytes, false);

                            // Update the pointer, if it overflows, wrap around.
                            if(++histPnt >= MAX_HISTORY_LINES)
                                histPnt = 0;
                        }
                    }
                } while(bytes != -1);
            }
        } else
        {
            histDisabled = 1;
        }
    }
    // If the history filename hasnt been provided and it was previously being used, this is a signal to close and stop using history, free it up.
    //
    if(histFile == NULL && histFp != NULL)
    {
        clearHistory();
    }

  #endif

    while (next_char(&type, &val))
    {
        switch (type)
        {
            case KEY_REGULAR:
                // Refuse insert if we are at the end of the line:
                if (lpos == lineSize)
                       break;
    
                // If there is string to the right, move it over one place:
                if (llen > lpos)
                    for (i = llen; i >= lpos; i--)
                        line[i + 1] = line[i];
    
                // Insert character:
                line[lpos++] = val;
    
                // Increase line length, if possible:
                if (llen < lineSize)
                    llen++;
    
                // Paint new string:
                for (i = lpos - 1; i < llen; i++)
                    fputc(line[i], stdout);
    
                // Backtrack to current position:
                for (i = lpos; i < llen; i++)
                    fputc('\b', stdout);
    
                break;

            case KEY_CTRL_C:
                //  Ctrl-c means abort current line, return CTRL_C in the first character of the buffer.
                line[0] = CTRL_C;
                line[1] = '\0';
                refreshLine((uint8_t *)"", 0, &lpos);
                llen = lpos = 0;
                return line;

            case KEY_CTRL_D:
                break;

            case KEY_BKSP:
                // Nothing to do if we are at the start of the line:
                if (lpos == 0)
                    break;
    
                // Line becomes one character shorter:
                llen--;
                lpos--;
    
                fputc('\b', stdout);
    
                // Move characters one over to the left:
                for (i = lpos; i < llen; i++) {
                    line[i] = line[i + 1];
                    fputc(line[i], stdout);
                }
                fputc(' ', stdout);
    
                // Backtrack to current position:
                for (i = lpos; i <= llen; i++)
                    fputc('\b', stdout);
    
                break;
    
            case KEY_ENTER:
                // Null-terminate the line and return pointer:
                line[llen] = '\0';
                fputc('\n', stdout);
    
                // See if this is a local readline command, if so process otherwise return buffer to caller.
                if(localCommand(line, &llen) == 0)
                {
                    // If a command is entered, add to history.
                    if(llen > 0)
                    {
                        // Add the line into our history list.
                        addToHistory((char *)line, llen, true);
                    }
                } else
                {
                    // Return an empty buffer so caller can make any necessary updates.
                    line[0] = '\0';
                }
                llen = lpos = 0;
                return line;

            case KEY_CTRL_A:
            case KEY_HOME:
                while (lpos) {
                    fputc('\b', stdout);
                    lpos--;
                }
                break;
    
            case KEY_DEL:
                // Nothing to do if we are at the end of the line:
                if (lpos == llen)
                    break;
    
                llen--;
    
                // Move characters one over to the left:
                for (i = lpos; i < llen; i++) {
                    line[i] = line[i + 1];
                    fputc(line[i], stdout);
                }
                fputc(' ', stdout);
    
                // Backtrack to current position:
                for (i = lpos; i <= llen; i++)
                    fputc('\b', stdout);
    
                break;

            case KEY_INSERT:
                break;
    
            case KEY_CTRL_E:
            case KEY_END:
                while (lpos < llen)
                    fputc(line[lpos++], stdout);
                break;
    
            case KEY_PGUP:
                break;
    
            case KEY_PGDN:
                break;

            case KEY_CTRL_K:
                // Clear the buffer.
                refreshLine((uint8_t *)"", 0, &lpos);
                llen = lpos = 0;
                break;
    
            case KEY_CTRL_P:
            case KEY_ARROWUP:
                // Go to the previous history buffer.
                if(histPnt == 0 && history[MAX_HISTORY_LINES-1] != NULL)
                {
                    llen = strlen((char *)history[MAX_HISTORY_LINES-1]);
                    memcpy(line, history[MAX_HISTORY_LINES-1], llen+1);
                    histPnt = MAX_HISTORY_LINES-1;
                    refreshLine(line, llen, &lpos);
                } else
                if(history[histPnt-1] != NULL)
                {
                    llen = strlen((char *)history[histPnt-1]);
                    memcpy(line, history[histPnt-1], llen+1);
                    histPnt--;
                    refreshLine(line, llen, &lpos);
                } else
                if(history[histPnt] != NULL)
                {
                    llen = strlen((char *)history[histPnt]);
                    memcpy(line, history[histPnt], llen+1);
                    refreshLine(line, llen, &lpos);
                }
                break;
    
            case KEY_CTRL_N:
            case KEY_ARROWDN:
                // Go to the next history buffer.
                if(histPnt == MAX_HISTORY_LINES-1 && history[0] != NULL)
                {
                    llen = strlen((char *)history[0]);
                    memcpy(line, history[0], llen+1);
                    histPnt = MAX_HISTORY_LINES-1;
                    refreshLine(line, llen, &lpos);
                } else
                if(history[histPnt+1] != NULL)
                {
                    llen = strlen((char *)history[histPnt+1]);
                    memcpy(line, history[histPnt+1], llen+1);
                    histPnt++;
                    refreshLine(line, llen, &lpos);
                } else
               // if(history[histPnt] != NULL)
                {
                    llen = 0; //strlen((char *)history[histPnt]);
                    line[0] = '\0';
                    //memcpy(line, history[histPnt], llen+1);
                    refreshLine(line, llen, &lpos);
                }
                break;
    
            case KEY_CTRL_F:
            case KEY_ARROWRT:
                if (lpos < llen)
                    fputc(line[lpos++], stdout);
                break;
    
            case KEY_CTRL_B:
            case KEY_ARROWLT:
                if (lpos == 0)
                    break;
    
                fputc('\b', stdout);
                lpos--;
                break;
        }
    }

    line[0] = 0x00;
    return(line);
}

#ifdef __cplusplus
    }
#endif
