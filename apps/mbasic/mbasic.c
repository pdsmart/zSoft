/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:            mbasic.h
// Created:         April 2029
// Author(s):       Malcom McLean
// Description:     Mini Basic encapsulated as a standalone App for the zOS/ZPU test application.
//                  This program implements a loadable appliation which can be loaded from SD card by
//                  the zOS/ZPUTA application. The idea is that commands or programs can be stored on the
//                  SD card and executed by zOS/ZPUTA just like an OS such as Linux. The primary purpose
//                  is to be able to minimise the size of zOS/ZPUTA for applications where minimal ram is
//                  available.
//                  Basic is a very useable language to make quick tests and thus I have ported the 
//                  Mini Basic v1.0 by Malcolm Mclean to the ZPU/K64F and enhanced as necessary.
//
// Credits:         
// Copyright:       (C) Malcom Mclean
//                  (c) 2019-2020 Philip Smart <philip.smart@net2net.org> zOS/ZPUTA Basic enhancements
//
// History:         April 2020   - Ported v1.0 of Mini Basic from Malcolm McLean to work on the ZPU and
//                                 K64F, updates to function with the K64F processor and zOS.
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

#ifdef __cplusplus
    extern "C" {
#endif

#if defined(__K64F__)
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include "k64f_soc.h"
  #define uint32_t __uint32_t
  #define uint16_t __uint16_t
  #define uint8_t  __uint8_t
  #define int32_t  __int32_t
  #define int16_t  __int16_t
  #define int8_t   __int8_t
#elif defined(__ZPU__)
  #include <zstdio.h>
  #include <zpu-types.h>
  #include "zpu_soc.h"
  #include <stdlib.h>
#else
  #error "Target CPU not defined, use __ZPU__ or __K64F__"
#endif
#include "interrupts.h"
#include "ff.h"            /* Declarations of FatFs API */
#include "diskio.h"
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "xprintf.h"
#include "utils.h"
#include <readline.h>
#include "basic.h"
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

// Utility functions.
#include "tools.c"

// Version info.
#define VERSION      "v1.0"
#define VERSION_DATE "10/04/2020"
#define APP_NAME     "MBASIC"

// List of commands processed by the glue logic between mini basic and ed.
#define CMD_EDIT           0x01
#define CMD_RUN            0x02
#define CMD_LIST           0x03
#define CMD_NEW            0x04
#define CMD_LOAD           0x05
#define CMD_SAVE           0x06
#define CMD_HELP           0x0f
#define CMD_QUIT           0x10

// Command list.
//
typedef struct {
    const char  *cmd;
    uint8_t     key;
} t_basic_cmdstruct;

// Table of supported commands. The table contains the command and the case value to act on for the command.
static t_basic_cmdstruct basicCmdTable[] = {
    { "edit",     CMD_EDIT },
    { "run",      CMD_RUN  },
    { "list",     CMD_LIST },
    { "new",      CMD_NEW  },
    { "load",     CMD_LOAD },
    { "save",     CMD_SAVE },
    { "help",     CMD_HELP },
    { "quit",     CMD_QUIT },
};

// Define the number of commands in the array.
#define NBASICCMDKEYS (sizeof(basicCmdTable)/sizeof(t_basic_cmdstruct))

extern struct editorConfig E;

// Print out the commands which are recognised by the interactive command processor.
//
void printBasicHelp(void)
{
    xprintf("\nMini-Basic %s Help\n", VERSION);
    xprintf("RUN                     - Execute the basic code.\n");
    xprintf("EDIT                    - Open the editor for editting the BASIC code.\n");
    xprintf("LIST [<start>, [<end>]] - List the code.\n");
    xprintf("NEW                     - Clear program from memory.\n");
    xprintf("LOAD \"<file>\"           - Load program into memory.\n");
    xprintf("SAVE \"<file>\"           - Save program to file.\n");
    xprintf("HELP                    - This help screen.\n");
    xprintf("QUIT                    - Exit BASIC and return to the OS.\n");
}

// List out the basic source code, either the full test or from the optional Start and End parameters.
//
int listBasicSource(char *params)
{
    long startLine = -1;
    long endLine = -1;

    // Get optional parameters.
    //
    if(xatoi(&params, &startLine))
    {
        if(!xatoi(&params, &endLine))
            endLine = -1;
    } else
        startLine = -1;

    // Sanity check.
    //
    if(endLine < startLine)
    {
        xprintf("Illegal line range.\n");
    } else
    {
        // List out the source from the given lines.
        //
        editorList((int)startLine, (int)endLine);
    }
    return 0;
}

// This is not a true interactive basic, it is built from Mini Basic which just executes a file/script and ED a VT100 editor.
// I have thrown in readline to read commands, usage is:
// 1. Issue edit command, use wysiwig editor to create basic program or edit existing program. Exit when finished.
// 2. Issue run command, editor buffer converted to basic script and sent to the minibasic engine. Results are output on screen.
// 3. Interate between 1 and 2.
// 4. Issue quit command to return to zOS/ZPUTA.
int cmdProcessor()
{
    uint32_t  retCode = 0;
    int       idx;
    int       processing = 1;
    int       ready = 1;
    long      lineNo;
    char      *paramptr;
    char      *fileName;
    char      buf[132];

    // Signon.
    xprintf("Mini-Basic %s\n", VERSION);

    // Loop until a quit command is given.
    //
    while(processing == 1)
    {
        // Indicate we are waiting for a command as needed.
        if(ready)
            xputs("Ready\n");

        // Get command.
        //
        readline((uint8_t *)buf, sizeof(buf), HISTORY_FILE);
        paramptr = buf;

        // Refresh the ordered basic script from the editor buffer prior to commands operating on it.
        editorBuildScript();

        // Skip leading whitespace.
        while(*paramptr == ' ' && paramptr < (char *)buf + sizeof(buf)) paramptr++;
    
        // Loop through all the commands and try to find a match.
        for (idx=0, ready=1; idx < NBASICCMDKEYS; idx++)
        {
            t_basic_cmdstruct *sym = &basicCmdTable[idx];
            if (strncmp(sym->cmd, paramptr, strlen(sym->cmd)) == 0)
            {
                // Process the command
                switch(sym->key)
                {
                    // Edit the basic program.
                    //
                    case CMD_EDIT:
                        xsprintf(E.statusmsg, "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");
                        E.statusmsg_time = sysmillis();
                        retCode = 0;
                        while(retCode == 0)
                        {
                            editorRefreshScreen();
                            retCode = editorProcessKeypress();
                        }
                        break;
    
                    // Run the basic program.
                    //
                    case CMD_RUN:
                        // Just call the basic interpreter, the basic script is already in memory and prepared.
                        execBasicScript();
                        break;

                    // List command, just output the contents of the editor.
                    //
                    case CMD_LIST:
                        listBasicSource(paramptr+strlen(sym->cmd));
                        break;

                    // Clear program from editor and start a fresh.
                    //
                    case CMD_NEW:
                        editorCleanup();
                        break;

                    // Load a new program into memory.
                    //
                    case CMD_LOAD:
                        // First check if the current file has been saved.
                        //
                        if(!E.dirty)
                        {
                            // Get filename and if valid, open the requested file.
                            paramptr += strlen(sym->cmd);
                            fileName = getStrParam(&paramptr);
                            if(fileName != NULL && strlen(fileName) > 0)
                            {
                                editorCleanup();
                                editorOpen(fileName);
                            } else
                            {
                                xprintf("Bad filename, use format \"<file>\" if name contains spaces or just <file> if it doesnt.\n");
                            }
                        } else
                        {
                                xprintf("Current program hasnt been saved, use 'save' to use current filename, \n'save \"<file>\"' to store to <file> or 'new' to delete current program.\n");
                        }
                        break;

                    // Save current programn from memory into file.
                    //
                    case CMD_SAVE:
                        paramptr += strlen(sym->cmd);
                        fileName = getStrParam(&paramptr);
                        if(editorSave((fileName != NULL && strlen(fileName) > 0) ? fileName : NULL) == 1)
                        {
                            xprintf("File save failed, check name, disk space and permissions.\n");
                        }
                        break;

                    // Help, show what interactive commands are available.
                    //
                    case CMD_HELP:
                        printBasicHelp();
                        break;

                    // Exit from the application.
                    //
                    case CMD_QUIT:
                        editorCleanup();
                        processing = 0;
                        break;
    
                    // Default case is an error.
                    default:
                        ready=0;
                        xprintf("Bad Command\n");
                        break;
                }
                break;
            }
        }
        if(idx == NBASICCMDKEYS)
        {
            // If a number has been entered as the first characters then assume it is a BASIC line to be inserted/amended/deleted.
            if(xatoi(&paramptr, &lineNo))
            {
                // Check to see if the line is empty, a line number on its own is a deletion.
                //
                while(*paramptr != '\0' && *paramptr == ' ') paramptr++;

                retCode = editorAddBasicLine((int)lineNo, buf, strlen(paramptr) == 0 ? 0 : strlen(buf));
            } else
            // If an entry has been typed but it is not a control key, then it must be an error as it hasnt been recognised.
            if(strlen(paramptr) > 0 && *paramptr > 0x1f)
            {
                ready=0;
                xprintf("Bad Command\n");
            }
        }
    }

    return(retCode);
}

// Main entry and start point of a zOS/ZPUTA Application. Only 2 parameters are catered for and a 32bit return code, additional parameters can be added by changing the appcrt0.s
// startup code to add them to the stack prior to app() call.
//
// Return code for the ZPU is saved in _memreg by the C compiler, this is transferred to _memreg in zOS/ZPUTA in appcrt0.s prior to return.
// The K64F ARM processor uses the standard register passing conventions, return code is stored in R0.
//
uint32_t app(uint32_t param1, uint32_t param2)
{
    // Initialisation.
    //
    char      *ptr = (char *)param1;
    char      *pathName;
    uint32_t  retCode = 1;

    // Get optional name of basic file to load.
    //
    pathName    = getStrParam(&ptr);
    if((retCode = initEditor()) == 0)
    {
        if(strlen(pathName) == 0 || ((retCode = editorOpen(pathName)) == 0))
        {
            // From now on we are switching between the initialised editor and mbasic.
            //
            cmdProcessor();
        } else
        {
            // Memory exhausted?
            if(retCode == 1)
            {
                xprintf("Insufficient memory to process this file.\n", pathName);
            } else
            {
                xprintf("Failed to create or open file:%s\n", pathName);
            }
        }
    }
    return(retCode);
}

#ifdef __cplusplus
}
#endif

