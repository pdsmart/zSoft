///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            ed.c
// Created:         April 2020
// Author(s):       Philip Smart
// Description:     A stripped down memory lean version of the Kilo editor. Kilo uses more than 3x the
//                  size of the file being editted through malloc which can be a problem on the zpu
//                  depending upon model and also limits the filesize to 64K on the K4F. Hence stripping
//                  out nicities such as highlights and render.
//
//                  This module has been adapted to work with mbasic and provide editor facilities to
//                  use it interactively.
// Credits:         Salvatore Sanfilippo <antirez at gmail dot com> 2016 for his 0.0.1 verson of Kilo
//                  upon which this editor is based.
// Copyright:       (c) 2019-2020 Philip Smart <philip.smart@net2net.org> zOS/ZPUTA enhancements, bug
//                  fixes, new features and adaptation to the zOS/ZPUTA framework.
//
// History:         April 2020   - Ported v0.0.1 of Kilo and stripped it of features to get a leaner
//                                 memory using editor. Some functionality has been lost from the 
//                                 original editor but it is still very useable.
//
// Notes:           See Makefile to enable/disable conditional components
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

#define ED_VERSION "1.0"

#ifdef __cplusplus
    extern "C" {
#endif

#if defined(__K64F__)
  #include    <stdio.h>
  #include    <string.h>
  #include    <unistd.h>
  #include    <stdarg.h>
  #include    <usb_serial.h>
  #include    "k64f_soc.h"
  #include    <ctype.h>
  #include    <../../libraries/include/stdmisc.h>
  #undef      stdout
  #undef      stdin
  #undef      stderr
  extern FILE *stdout;
  extern FILE *stdin;
  extern FILE *stderr;
#elif defined(__ZPU__)
  #include    <stdint.h>
  #include    <stdio.h>
  #include    "zpu_soc.h"
  #include    <stdlib.h>
#else
  #error      "Target CPU not defined, use __ZPU__ or __K64F__"
#endif
#include "interrupts.h"
#include "ff.h"            /* Declarations of FatFs API */
#include <string.h>
#include "utils.h"
//
#if defined __ZPUTA__
  #include "zputa_app.h"
#elif defined __ZOS__
  #include "zOS_app.h"
#else
  #error OS not defined, use __ZPUTA__ or __ZOS__      
#endif
//
#include "mbasic.h"



// Globals needed by ed.
struct editorConfig E;

// Externals from the BASIC interpreter module.
extern LINE *lines;
extern int  nlines;

// Simple method to create a local delay timer not dependent on system libraries.
//
void syswait(uint32_t wait)
{
    uint32_t startTime;

    startTime = sysmillis();
    while((sysmillis() - startTime) < wait);
}

int8_t getKeyChar(uint32_t waitTime)
{
    int8_t   keyIn;
    uint32_t timeout = sysmillis();

    do {
      #if defined __SHARPMZ__
        // Get key from system, request ansi key sequence + non-blocking (=2).
        keyIn = getKey(2);
      #else
        // Get key from system, request non-blocking (=0).
        keyIn = getKey(0);
      #endif
    } while(keyIn == -1 && (sysmillis() - timeout) < waitTime);

    return(keyIn);
}

/* Read a key from the terminal put in raw mode, trying to handle
 * escape sequences. */
int editorReadKey(void)
{
    int8_t seq[3];
    int8_t c;

    // Wait for a key to be pressed.
    while((c = (char)getKeyChar(500)) == -1);

    /* escape sequence */
    if((char)c == ESC)
    {
        /* If this is just an ESC, we'll timeout here. */
        if((seq[0] = getKeyChar(500)) == -1) return ESC;
        if((seq[1] = getKeyChar(500)) == -1) return ESC;

        /* ESC [ sequences. */
        if ((char)seq[0] == '[')
        {
            if ((char)seq[1] >= '0' && (char)seq[1] <= '9')
            {
                /* Extended escape, read additional byte. */
                if((seq[2] = getKeyChar(500)) == -1) return ESC;
                if ((char)seq[2] == '~')
                {
                    switch((char)seq[1])
                    {
                        case '1': return HOME_KEY;
                        case '2': return INSERT_KEY;
                        case '3': return DEL_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                    }
                }
            } else {
                switch((char)seq[1])
                {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        }

        /* ESC O sequences. */
        else if ((char)seq[0] == 'O')
        {
            switch((char)seq[1])
            {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
                case 'Q': return F2_KEY;
                case 'R': return F3_KEY;
                case 'S': return F3_KEY;
            }
        }
        return ESC;
    } else
    {
        return (int)c;
    }
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor is stored at *rows and *cols and 0 is returned.
*/
int getCursorPosition(uint32_t *rows, uint32_t *cols)
{
    char         buf[32];
    char         *ptr;
    unsigned int i = 0;
    int          c;

    // Save cursor position.
    //
    printf("%c%c", 0x1b, '7');

    // Cursor to maximum X/Y location, report cursor location
    //
    fputs("\x1b[0;0H", stdout);
    syswait(10);
    fputs("\x1b[999;999H", stdout);
    syswait(10);
    fputs("\x1b[6n", stdout);

    // Read the response: ESC [ rows ; cols R
    //
    while (i < sizeof(buf)-1)
    {
        if((c = getKeyChar(2000)) == -1) break;
        if((i == 0 && (char)c != ESC) || (i == 1 && (char)c != '[')) return -1;
        if((char)c == ';')
            buf[i] = ' ';
        else
            buf[i] = (char)c;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = 0x00;

    // Parse it.
    //
    ptr = &buf[2];
    if(!uxatoi(&ptr, rows)) return -1;
    if(!uxatoi(&ptr, cols)) return -1;

    // Restore cursor.
    //
    printf("%c%c", 0x1b, '8');
    return 0;
}

/* Try to get the number of columns in the current terminal. If the ioctl()
 * call fails the function will try to query the terminal itself.
 * Returns 0 on success, -1 on error.
*/
int getWindowSize(int *rows, int *cols)
{
    uint32_t termRows, termCols;
    if(getCursorPosition(&termRows,&termCols) == 0)
    {
		*cols = (int)termCols;
		*rows = (int)termRows;
    } else
    {
		*cols = 80;
		*rows = 24;
    }
    return 0;
}

/* ======================= Editor rows implementation ======================= */

/* Insert a row at the specified position, shifting the other rows on the bottom
 * if required.
*/
int editorInsertRow(int at, char *s, size_t len)
{
	int j;

    if (at > E.numrows) return 0;

    E.row = sys_realloc(E.row,sizeof(erow)*(E.numrows+1));
    if(E.row == NULL) { printf("editorInsertRow: Memory exhausted\n"); return 1; }

    if (at != E.numrows)
    {
        memmove(E.row+at+1,E.row+at,sizeof(E.row[0])*(E.numrows-at));
        for (j = at+1; j <= E.numrows; j++) E.row[j].idx++;
    }

    E.row[at].size = len;
    E.row[at].chars = sys_malloc(len+1);
    if(E.row[at].chars == NULL) { printf("editorInsertRow: Memory exhausted\n"); return 1; }
    memcpy(E.row[at].chars,s,len+1);
    E.row[at].idx = at;
    E.numrows++;
    E.dirty++;
    return 0;
}

/* Free row's heap allocated stuff. */
void editorFreeRow(erow *row)
{
    if(row->chars != NULL)
    {
        sys_free(row->chars);
        row->chars = NULL;
    }
}

// Method to release all used memory back to the heap.
//
void editorCleanup(void)
{
    int  idx;

    // Go through and free all row memory.
    //
    for(idx=0; idx < E.numrows; idx++)
    {
        if(E.row[idx].chars != NULL)
        {
            sys_free(E.row[idx].chars);
        }
    }

    // Free the editor config structure.
    if(E.row)
        sys_free(E.row);

    // Free the BASIC script memory if used.
    //
    if(lines)
        sys_free(lines);

    // Reset necessary variables.
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    lines = NULL;
    nlines = 0;
    return;
}

/* Remove the row at the specified position, shifting the remainign on the
 * top. */
void editorDelRow(int at) {
	int j;
    erow *row;

    if (at >= E.numrows) return;
    row = E.row+at;
    editorFreeRow(row);
    memmove(E.row+at,E.row+at+1,sizeof(E.row[0])*(E.numrows-at-1));
    for (j = at; j < E.numrows-1; j++) E.row[j].idx++;
    E.numrows--;
    E.dirty++;
}

/* Insert a character at the specified position in a row, moving the remaining
 * chars on the right if needed.
*/
int editorRowInsertChar(erow *row, int at, int c)
{
    if (at > row->size)
    {
        /* Pad the string with spaces if the insert location is outside the
         * current length by more than a single character.
        */
        int padlen = at-row->size;

        /* In the next line +2 means: new char and null term.
        */
        row->chars = sys_realloc(row->chars,row->size+padlen+2);
        if(row->chars == NULL) { printf("editorRowInsertChar: Memory exhausted\n"); return 1; }
        memset(row->chars+row->size,' ',padlen);
        row->chars[row->size+padlen+1] = '\0';
        row->size += padlen+1;
    } else {
        /* If we are in the middle of the string just make space for 1 new
         * char plus the (already existing) null term.
        */
        row->chars = sys_realloc(row->chars,row->size+2);
        if(row->chars == NULL) { printf("editorRowInsertChar: Memory exhausted\n"); return 1; }
        memmove(row->chars+at+1,row->chars+at,row->size-at+1);
        row->size++;
    }
    row->chars[at] = c;
    E.dirty++;

    return 0;
}

/* Append the string 's' at the end of a row */
int editorRowAppendString(erow *row, char *s, size_t len)
{
    row->chars = sys_realloc(row->chars,row->size+len+1);
    if(row->chars == NULL) { printf("editorRowAppendString: Memory exhausted\n"); return 1; }
    memcpy(row->chars+row->size,s,len);

    row->size += len;
    row->chars[row->size] = '\0';
    E.dirty++;

    return 0;
}

/* Delete the character at offset 'at' from the specified row. */
int editorRowDelChar(erow *row, int at)
{
    if (row->size <= at) return 0;
    memmove(row->chars+at,row->chars+at+1,row->size-at);
    row->size--;
    E.dirty++;
    return 0;
}

/* Insert the specified char at the current prompt position. */
void editorInsertChar(int c)
{
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    /* If the row where the cursor is currently located does not exist in our
     * logical representaion of the file, add enough empty rows as needed. */
    if (!row)
    {
        while(E.numrows <= filerow)
        {
            if(editorInsertRow(E.numrows,"",0) != 0) return;
        }
    }
    row = &E.row[filerow];
    editorRowInsertChar(row,filecol,c);
    if (E.cx == E.screencols-1)
        E.coloff++;
    else
        E.cx++;
    E.dirty++;
}

/* Inserting a newline is slightly complex as we have to handle inserting a
 * newline in the middle of a line, splitting the line as needed.
*/
int editorInsertNewline(void)
{
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row)
    {
        if (filerow == E.numrows)
        {
            if(editorInsertRow(filerow,"",0) != 0) return 1;
            goto fixcursor;
        }
        return 0;
    }
    /* If the cursor is over the current line size, we want to conceptually
     * think it's just over the last character. */
    if (filecol >= row->size) filecol = row->size;
    if (filecol == 0)
    {
        if(editorInsertRow(filerow,"",0) != 0) return 1;
    } else
    {
        /* We are in the middle of a line. Split it between two rows. */
        if(editorInsertRow(filerow+1,row->chars+filecol,row->size-filecol) != 0) return 1;
        row = &E.row[filerow];
        row->chars[filecol] = '\0';
        row->size = filecol;
    }
fixcursor:
    if (E.cy == E.screenrows-1)
    {
        E.rowoff++;
    } else
    {
        E.cy++;
    }
    E.cx = 0;
    E.coloff = 0;
    return 0;
}

/* Delete the char at the current prompt position. */
int editorDelChar()
{
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row || (filecol == 0 && filerow == 0)) return 0;
    if (filecol == 0)
    {
        /* Handle the case of column 0, we need to move the current line
         * on the right of the previous one.
        */
        filecol = E.row[filerow-1].size;
        editorRowAppendString(&E.row[filerow-1],row->chars,row->size);
        editorDelRow(filerow);
        row = NULL;
        if (E.cy == 0)
            E.rowoff--;
        else
            E.cy--;
        E.cx = filecol;
        if (E.cx >= E.screencols)
        {
            int shift = (E.screencols-E.cx)+1;
            E.cx -= shift;
            E.coloff += shift;
        }
    } else
    {
        editorRowDelChar(row,filecol-1);
        if (E.cx == 0 && E.coloff)
            E.coloff--;
        else
            E.cx--;
    }
    E.dirty++;
    return 0;
}

/* Load the specified program in the editor memory and returns 0 on success
 * or 1 on error.
*/
int editorOpen(char *filename)
{
    FIL     fp;
    FRESULT fr;
    char    buf[132];
    uint8_t linelen;
    int     rowNo;

    E.dirty = 0;
    E.filename = filename;

    fr = f_open(&fp, filename, FA_OPEN_ALWAYS | FA_READ);
    if(fr)
    {
        printf("Failed to open file:%s\n", filename);
        return 2;
    }

    // First parse to get number of lines in file to better allocate memory.
    //
    E.numrows = 0;
    while(f_gets(buf, sizeof(buf), &fp) != NULL)
    {
        E.numrows++;
    }
    fr = f_lseek(&fp, 0);
    if(fr)
    {
        printf("Failed to rewind file:%s\n", filename);
        return 3;
    }

    // If there is data in the file, load it.
    //
    if(E.numrows)
    {
        // Allocate memory for the row array.
        //
        E.row = sys_malloc(sizeof(erow)*(E.numrows));
        if(E.row == NULL) { printf("editorOpen: Memory exhausted\n"); return 1; }
	memset(E.row, '\0', sizeof(erow)*(E.numrows));

        // Now go through the file again and populate each row with the data.
        //
        rowNo = 0;
        while(f_gets(buf, sizeof(buf), &fp) != NULL)
        {
            linelen = strlen(buf);
            if (linelen && (buf[linelen-1] == '\n' || buf[linelen-1] == '\r'))
                buf[--linelen] = '\0';

            E.row[rowNo].size = linelen;
            E.row[rowNo].chars = sys_malloc(linelen+1);
            if(E.row[rowNo].chars == NULL) { printf("editorOpen: Memory exhausted\n"); return 1; }
            memcpy(E.row[rowNo].chars, buf, linelen+1);
            E.row[rowNo].idx = rowNo;
            rowNo++;
        }
    }
    f_close(&fp);
    return 0;
}


/* Save the current file on disk. Return 0 on success, 1 on error.
*/
int editorSave(char *newFileName)
{
    FIL          fp;
    FRESULT      fr;
    unsigned int bytes;
    int          totlen = 0;
    int          j;

    // Open existing file or create.
    fr = f_open(&fp, (newFileName == NULL ? E.filename : newFileName), FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
    if(fr)
    {
        sprintf(E.statusmsg, "Failed to open file:%s\n", (newFileName == NULL ? E.filename : newFileName));
        E.statusmsg_time = sysmillis();
        return 1;
    }

    /* Use truncate to zero the file the loop through the line buffers and
     * write the contents to the file.
    */
    if (f_truncate(&fp) != 0) goto writeerr;

    // Loop through the row records and write out the characters.
    //
    totlen = 0;
    for (j = 0; j < E.numrows; j++)
    {
        fr = f_write(&fp, E.row[j].chars, E.row[j].size, &bytes);
        if(fr != 0) goto writeerr;
        fr = f_putc('\n', &fp);
        if(fr == -1) goto writeerr;
        totlen += bytes + 1;
    }
    f_close(&fp);

    E.dirty = 0;
    sprintf(E.statusmsg, "%d bytes written on disk", totlen);
    E.statusmsg_time = sysmillis();
    return 0;

writeerr:
    f_close(&fp);
    sprintf(E.statusmsg, "Can't save! I/O error");
    E.statusmsg_time = sysmillis();
    return 1;
}

// Method to add or update a line within the editor buffer given its BASIC line number.
// This involves scanning the editor buffer looking for a BASIC line number match or
// a position inbetween to add it.
//
int editorAddBasicLine(int basicLineNo, char *line, size_t lineSize)
{
    int  idx;
    int  retCode = 0;

    // Go through the basic script using its lines numbers to compare against the given one.
    //
    for(idx=0; idx < nlines; idx++)
    {
        // If we get a match, delete original line and replace with new line.
        if(lines[idx].no == basicLineNo)
        {
            editorDelRow(lines[idx].eno);
            if(lineSize > 0)
                retCode = editorInsertRow(lines[idx].eno, line, lineSize);
            break;

        // If given line is greater than current then there is no match, add the new line prior to
        // this one.
        } else if(basicLineNo < lines[idx].no)
        {
            if(lineSize > 0)
                retCode = editorInsertRow(lines[idx].eno, line, lineSize);
            break;
        }
    }

    // If the line wasnt found and no line was greater, add line at bottom of file.
    if(idx == nlines)
    {
        if(lineSize > 0)
            retCode = editorInsertRow(idx, line, lineSize);
    }

    // If all went ok, refresh the script.
    if(retCode == 0)
    {
        retCode = editorBuildScript();
    }
    
    return(retCode);
}

// A method to print out the BASIC source code to the terminal.
//
void editorList(int startLine, int endLine)
{
    // Locals.
    int   idx;

    // Simple command, just go through the buffer and if it is within the constraints, print it.
    for(idx=0; idx < nlines; idx++)
    {
        if((startLine == -1 || (startLine != -1 && lines[idx].no >= startLine)) && (endLine == -1 || (endLine != -1 && lines[idx].no <= endLine)))
        {
            fputs(lines[idx].str, stdout);
            fputc('\n', stdout);
        }
    }
    return;
}

// Build the basic interpreter line array from the editor structures.
//
int editorBuildScript(void)
{
    char     *paramptr;
    long     lineNo;
    uint32_t totalSize;
    int      idx;

    // Sanity check.
    //
    if(E.numrows == 0)
    {
        return(2);
    }

    // Allocate memory for the basic lines array.
    //
    lines = sys_realloc(lines, E.numrows * sizeof(LINE));
    if(lines == NULL)
    {
        printf("Out of memory converting editor buffer to BASIC script.\n");
        return(1);
    }

    // Convert editor buffer to a lines array suitable for use by the BASIC interpreter. The lines array is based on BASIC line numbers so these are extracted
    // from the text and inserted into each array structure element.
    totalSize = 0;
    nlines = 0;
    for (idx = 0; idx < E.numrows; idx++)
    {
        paramptr = E.row[idx].chars;

        // Get the line number then populate the basic line array.
        //
        if(xatoi(&paramptr, &lineNo))
        {
            lines[nlines].no   = (int)lineNo;
            lines[nlines].eno  = idx;
            lines[nlines].str  = E.row[idx].chars;
            totalSize         += strlen(E.row[idx].chars);
            nlines++;
        } else if(strlen(paramptr) > 0)
        {
            printf("Syntax error on line:%s\n", E.row[idx].chars);
            return(3);
        }
    }

    // Check ordering of line numbers, has to be in order.
    //
    for(idx=1; idx < nlines; idx++)
    {
        if(lines[idx].no <= lines[idx-1].no)
        {
          printf("Program lines %d and %d not in order\n", lines[idx-1].no, lines[idx].no);
          return(4);
        }
    }

    // Ok, basic line array created.
    //
    return(0);
}


/* ============================= Terminal update ============================ */

/* Output buffer append method. The idea behind this is to avoid flickering
 * effects on the VT100 terminal. The original method buffered everything 
 * and sent it in one go, but on MPU's the resources are limited, so the 
 * buffer is smaller and more flushes are made as needed.
*/
struct abuf {
    char *b;
    int len;
};

void abAppend(const char *s, int len, int flush)
{
    static struct abuf *ab = NULL;

    // First call, allocate memory for the buffer.
    if(ab == NULL)
    {
        ab = sys_malloc(sizeof(struct abuf));
        if(ab == NULL) { printf("abAppend: Memory exhausted\n"); return; }
        ab->b = sys_malloc(MAX_APPEND_BUFSIZE);
        if(ab->b == NULL) { printf("abAppend: Memory exhausted\n"); return; }
        ab->len = 0;
    }

    // If adding this new text will overflow the buffer, flush before adding.
    if((ab->len + len) >= MAX_APPEND_BUFSIZE || flush)
    {
        const char *ptr = ab->b;
        for(ptr=ab->b; ptr < ab->b+ab->len; ptr++)
            fputc(*ptr, stdout);

        // If we are flushing then write out the additional data passed in to the method.
        //
        if(flush)
        {
            for(ptr=s; ptr < s+len; ptr++)
            fputc(*ptr, stdout);

        }
        ab->len = 0;
    }

    // If we arent flushing, tag the passed data onto the buffer.
    //
    if(!flush)
    {
        memcpy(ab->b + ab->len, s, len);
        ab->len += len;
    } else
    {
        // On flush we free up the memory, extra cycles in alloc and free but keeps the heap balanced and no memory leaks when we exit.
        //
        sys_free(ab->b);
        sys_free(ab);
        ab = NULL;
    }
}

/* This function writes the whole screen using VT100 escape characters
 * starting from the logical state of the editor in the global state 'E'.
*/
int editorRefreshScreen(void)
{
    int    y;
    int    lastLine = -1;
    erow   *r;
    char   buf[32];

    abAppend("\x1b[?25l",6, 0); /* Hide cursor. */
    abAppend("\x1b[H",3, 0);    /* Go home. */
    for (y = 0; y < E.screenrows; y++)
    {
        int filerow = E.rowoff+y;

        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows/3)
            {
                char welcome[80];
                sprintf(welcome, "Ed(itor) -- version %s\x1b[0K\r\n", ED_VERSION);
                int welcomelen = strlen(welcome);
                int padding = (E.screencols-welcomelen)/2;
                if (padding)
                {
                    abAppend("~",1, 0);
                    padding--;
                }
                while(padding--) abAppend(" ",1, 0);
                abAppend(welcome,welcomelen, 0);
            } else {
                if(lastLine == -1) lastLine = y;
                abAppend("~\x1b[0K\r\n",7, 0);
            }
            continue;
        }

        r = &E.row[filerow];

        int len = r->size - E.coloff;
        if (len > 0) {
            if (len > E.screencols) len = E.screencols;
            char *c = r->chars+E.coloff;
            int idx, j;
            for (j = 0; j < len; j++)
            {
                if(c[j] == TAB)
                {
                    for(idx=0; idx < ED_TAB_SIZE; idx++) buf[idx] = ' ';
                    buf[ED_TAB_SIZE] = '\0';
                    abAppend(buf, ED_TAB_SIZE, 0);
                }
                else if (!isprint((int)c[j]))
                {
                    char sym;
                    abAppend("\x1b[7m",4, 0);
                    if (c[j] <= 26)
                        sym = '@'+c[j];
                    else
                        sym = '?';
                    abAppend(&sym,1, 0);
                    abAppend("\x1b[0m",4, 0);
                } else 
                {
                    abAppend(c+j,1, 0);
                } 
            }
        }
        abAppend("\x1b[39m",5, 0);
        abAppend("\x1b[0K",4, 0);
        abAppend("\r\n",2, 0);
    }

    /* Create a two rows status. First row: */
    abAppend("\x1b[0K",4, 0);
    abAppend("\x1b[7m",4, 0);
    char status[80], rstatus[80];
    sprintf(status, "%-20s - %d lines %s", E.filename, E.numrows, E.dirty > 0  ? "(modified)" : "");
    int len = strlen(status);
    sprintf(rstatus, "%d/%d",E.rowoff+E.cy+1,E.numrows);
    int rlen = strlen(rstatus);
    if (len > E.screencols) len = E.screencols;
    abAppend(status,len, 0);
    while(len < E.screencols) {
        if (E.screencols - len == rlen) {
            abAppend(rstatus,rlen, 0);
            break;
        } else {
            abAppend(" ",1, 0);
            len++;
        }
    }
    abAppend("\x1b[0m\r\n",6, 0);

    /* Second row depends on E.statusmsg and the status message update time. */
    abAppend("\x1b[0K",4, 0);
    int msglen = strlen(E.statusmsg);
    if (msglen && sysmillis() - E.statusmsg_time < 5000)
        abAppend(E.statusmsg,msglen <= E.screencols ? msglen : E.screencols, 0);

    /* Put cursor at its current position. Note that the horizontal position
     * at which the cursor is displayed may be different compared to 'E.cx'
     * because of TABs.
    */
    int j;
    int cx = 1;
    int filerow = E.rowoff+E.cy;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    if (row)
    {
        for (j = E.coloff; j < (E.cx+E.coloff); j++)
        {
            if (j < row->size && row->chars[j] == TAB) cx += ((ED_TAB_SIZE)-((cx)%ED_TAB_SIZE));
            cx++;
        }
    }
    sprintf(buf,"\x1b[%d;%dH",E.cy+1,cx);
    abAppend(buf,strlen(buf), 0);

    /* Show cursor and Flush to complete.
    */
    abAppend("\x1b[?25h",6, 1);

    return(lastLine);
}

/* =============================== Find mode ================================ */

void editorFind(void) {
    char query[ED_QUERY_LEN+1] = {0};
    int qlen = 0;
    int last_match = -1; /* Last line where a match was found. -1 for none. */
    int find_next = 0; /* if 1 search next, if -1 search prev. */

    /* Save the cursor position in order to restore it later. */
    int saved_cx = E.cx, saved_cy = E.cy;
    int saved_coloff = E.coloff, saved_rowoff = E.rowoff;

    while(1)
    {
        sprintf(E.statusmsg, "Search: %s (Use ESC/Arrows/Enter)", query);
        E.statusmsg_time = sysmillis();
        editorRefreshScreen();

        int c = editorReadKey();
        if (c == DEL_KEY || c == CTRL_H || c == BACKSPACE) {
            if (qlen != 0) query[--qlen] = '\0';
            last_match = -1;
        } else if (c == ESC || c == ENTER) {
            if (c == ESC) {
                E.cx = saved_cx; E.cy = saved_cy;
                E.coloff = saved_coloff; E.rowoff = saved_rowoff;
            }
	    E.statusmsg[0] = '\0';
            E.statusmsg_time = sysmillis();
            return;
        } else if (c == ARROW_RIGHT || c == ARROW_DOWN) {
            find_next = 1;
        } else if (c == ARROW_LEFT || c == ARROW_UP) {
            find_next = -1;
        } else if (isprint(c)) {
            if (qlen < ED_QUERY_LEN) {
                query[qlen++] = c;
                query[qlen] = '\0';
                last_match = -1;
            }
        }

        /* Search occurrence. */
        if (last_match == -1) find_next = 1;
        if (find_next) {
            char *match = NULL;
            int match_offset = 0;
            int i, current = last_match;

            for (i = 0; i < E.numrows; i++) {
                current += find_next;
                if (current == -1) current = E.numrows-1;
                else if (current == E.numrows) current = 0;
                match = strstr(E.row[current].chars,query);
                if (match) {
                    match_offset = match-E.row[current].chars;
                    break;
                }
            }
            find_next = 0;

            if (match) {
                last_match = current;
                E.cy = 0;
                E.cx = match_offset;
                E.rowoff = current;
                E.coloff = 0;
                /* Scroll horizontally as needed. */
                if (E.cx > E.screencols) {
                    int diff = E.cx - E.screencols;
                    E.cx -= diff;
                    E.coloff += diff;
                }
            }
        }
    }
}

/* ========================= Editor events handling  ======================== */

/* Handle cursor position change because arrow keys were pressed. */
void editorMoveCursor(int key)
{
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    int rowlen;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    switch(key)
    {
        case ARROW_LEFT:
            if (E.cx == 0) {
                if (E.coloff) {
                    E.coloff--;
                } else {
                    if (filerow > 0) {
                        E.cy--;
                        E.cx = E.row[filerow-1].size;
                        if (E.cx > E.screencols-1) {
                            E.coloff = E.cx-E.screencols+1;
                            E.cx = E.screencols-1;
                        }
                    }
                }
            } else {
                E.cx -= 1;
            }
            break;

        case ARROW_RIGHT:
            if (row && filecol < row->size) {
                if (E.cx == E.screencols-1) {
                    E.coloff++;
                } else {
                    E.cx += 1;
                }
            } else if (row && filecol == row->size) {
                E.cx = 0;
                E.coloff = 0;
                if (E.cy == E.screenrows-1) {
                    E.rowoff++;
                } else {
                    E.cy += 1;
                }
            }
            break;

        case ARROW_UP:
            if (E.cy == 0) {
                if (E.rowoff) E.rowoff--;
            } else {
                E.cy -= 1;
            }
            break;

        case ARROW_DOWN:
            if (filerow < E.numrows) {
                if (E.cy == E.screenrows-1) {
                    E.rowoff++;
                } else {
                    E.cy += 1;
                }
            }
            break;

            break;
            E.cx = E.screencols-1;
            E.coloff = 0;
            break;
    
        case HOME_KEY:
            E.cx = 0;
            E.coloff = 0;
            break;

        case END_KEY:
            E.cx = row->size;
            E.coloff = 0;
            if (E.cx > E.screencols-1)
            {
                E.coloff = E.cx-E.screencols+1;
                E.cx = E.screencols-1;
            }
            break;
    }
    /* Fix cx if the current line has not enough chars. */
    filerow = E.rowoff+E.cy;
    filecol = E.coloff+E.cx;
    row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    rowlen = row ? row->size : 0;
    if (filecol > rowlen)
    {
        E.cx -= filecol-rowlen;
        if (E.cx < 0)
        {
            E.coloff += E.cx;
            E.cx = 0;
        }
    }
}

/* Process events arriving from the standard input, which is, the user
 * is typing stuff on the terminal.
*/
uint32_t editorProcessKeypress(void)
{
    /* When the file is modified, requires Ctrl-q to be pressed N times
     * before actually quitting.
    */
    static int quit_times = ED_QUIT_TIMES;
    int        cxSave, cySave;
    int        lastLine;
    int        c;

    c = editorReadKey();
    switch(c)
    {
        case ENTER:         /* Enter */
            editorInsertNewline();
            break;

        case CTRL_C:        /* Ctrl-c */
            /* We ignore ctrl-c, it can't be so simple to lose the changes
             * to the edited file.
            */
            break;

        case CTRL_Q:        /* Ctrl-q */
            /* Quit if the file was already saved.
            */
            if (E.dirty > 0 && quit_times > 0)
            {
                sprintf(E.statusmsg, "WARNING!!! File has unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
                E.statusmsg_time = sysmillis();
                quit_times--;
            }
            if(quit_times == 0 || E.dirty == 0)
            {
                cxSave = E.cx;
                cySave = E.cy;
                E.cy = E.screenrows-1;
                lastLine = editorRefreshScreen();
                printf("\x1b[%03d;%03dH", (lastLine == -1 ? E.screenrows-1 : lastLine+1), 1);
                printf("\x1b[0J" );

                // Restore the cursor so it opens in the same place when edit is restarted.
                E.cx = cxSave;
                E.cy = cySave;
                return(1);
            } else
                return(0);

        case CTRL_S:        /* Ctrl-s */
            editorSave(NULL);
            break;

        case CTRL_F:
            editorFind();
            break;

        case BACKSPACE:     /* Backspace */
        case CTRL_H:        /* Ctrl-h */
            editorDelChar();
            break;

        case DEL_KEY:
            editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
	    break;

        case PAGE_UP:
        case PAGE_DOWN:
            if (c == PAGE_UP && E.cy != 0)
                E.cy = 0;
            else if (c == PAGE_DOWN && E.cy != E.screenrows-1)
                E.cy = E.screenrows-1;
            {
            int times = E.screenrows;
            while(times--)
                editorMoveCursor(c == PAGE_UP ? ARROW_UP: ARROW_DOWN);
            }
            break;

        case HOME_KEY:
        case END_KEY:
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;

        case CTRL_L: /* ctrl+l, clear screen */
            /* Just refresh the line as side effect. */
            break;

        case ESC:
            /* Nothing to do for ESC in this mode. */
            break;

        default:
            editorInsertChar(c);
            break;
    }

    quit_times = ED_QUIT_TIMES; /* Reset it to the original value. */
    return(0);
}

int editorFileWasModified(void) {
    return E.dirty;
}

uint32_t initEditor(void)
{
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    if (getWindowSize(&E.screenrows,&E.screencols) == -1)
    {
        printf("Unable to query the screen for size (columns / rows)");
        return(1);
    }
    E.screenrows -= 2; /* Get room for status bar. */
    return(0);
}

#ifdef __cplusplus
}
#endif
