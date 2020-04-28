#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

#include "../core/main.h"
#include "../core/utils.h"
#include "../core/textual.h"
#include "../core/tokens.h"
#include "../core/extern.h"

#define VARS_SPACE_SIZE 512
#define DATA_SPACE_SIZE 4096
#define LINE_SIZE 80

char extraCmdArgCnt[] = {2, 2, 0};

char extraFuncArgCnt[] = {1, 2};

static char* commonStrings = CONST_COMMON_STRINGS;
static char * parsingErrors = CONST_PARSING_ERRORS;

char dataSpace[DATA_SPACE_SIZE];
char lineSpace[LINE_SIZE * 3];

static FILE* fCurrent;
static short idCurrent = 0;

static struct termios oldTermSettings;

static void initSystem(void) {
    struct termios termSettings;
    tcgetattr(STDIN_FILENO, &oldTermSettings);
    termSettings = oldTermSettings;
    termSettings.c_lflag &= ~(ICANON | ECHO | ISIG);
    tcsetattr(STDIN_FILENO, TCSANOW, &termSettings);
    setvbuf(stdout, NULL, _IONBF, 0);
}

static void cleanup(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldTermSettings);
}

short sysGetc(void) {
    struct pollfd fd;
    fd.fd = STDIN_FILENO;
    fd.events = POLLIN;
    if (!poll(&fd, 1, 0)) {
        return -1;
    }
    return getchar();
}

void sysPutc(char c) {
    putchar(c);
}

void sysEcho(char c) {
    if (c == '\b') {
        sysPutc(c);
        sysPutc(' ');
    }
    sysPutc(c);
}

void sysQuit(void) {
    cleanup();
    exit(0);
}

void sysPoke(unsigned long addr, uchar value) {
    dataSpace[addr] = value;
}

uchar sysPeek(unsigned long addr) {
    return dataSpace[addr];
}

numeric sysMillis(numeric div) {
    struct timespec tp;
    long v;
    clock_gettime(CLOCK_REALTIME, &tp);
    v = (((numeric) tp.tv_sec) * 1000 + tp.tv_nsec / 1000000) & 0x7FFFFFFF;
    return div <= 1 ? v : v / div;
}

char translateInput(short c) {
    if (c == -1) {
        c = 0;
    }
    return (char) (c & 0xFF);
}

void outputConstStr(char strId, char index, char* w) {
    char* s;
    switch (strId) {
        case ID_COMMON_STRINGS:
            s = commonStrings;
            break;
        case ID_PARSING_ERRORS:
            s = parsingErrors;
            break;
        default:
            return;
    }
    while (index > 0) {
        while (*s++ != '\n') {
        }
        index -= 1;
    }
    while (*s != '\n') {
        if (w) {
            *(w++) = (*s++);
        } else {
            sysPutc(*s++);
        }
    }
    if (w) {
        *w = 0;
    }
}

static numeric power(numeric base, numeric exp) {
    return exp < 1 ? 1 : base * power(base, exp - 1);
}

short extraCommandByHash(numeric h) {
    switch (h) {
        case 0x036F: // POKE
            return CMD_EXTRA + 0;
        case 0x019C: // PIN - just prints argument values for test
            return CMD_EXTRA + 1;
        case 0x031A: // QUIT
            return CMD_EXTRA + 2;
        default:
            return -1;
    }
}

short extraFunctionByHash(numeric h) {
    switch (h) {
        case 0x0355: // PEEK
            return 0;
        case 0x06FC: // POWER - for test purpose
            return 1;
        default:
            return -1;
    }
}

void extraCommand(char cmd, numeric args[]) {
    switch (cmd) {
        case 0:
            sysPoke(args[0], args[1]);
            break;
        case 1:
            printf("PIN: %d,%d\n", args[0], args[1]);
            break;
        case 2:
            sysQuit();
            break;
    }
}

numeric extraFunction(char cmd, numeric args[]) {
    switch (cmd) {
        case 0:
            return sysPeek(args[0]);
        case 1:
            return power(args[1], args[0]);
    }
    return 0;
}

static char openStorage(char id, char op) {
    char fname[] = "store0.dat";
    char ops[] = "xb";
    fname[5] += id;
    ops[0] = op;
    fCurrent = fopen(fname, ops);
    return fCurrent != NULL ? 1 : 0;
}

char storageOperation(void* data, short size) {
    if (data == NULL) {
        if (idCurrent != 0) {
            fclose(fCurrent);
        }
        idCurrent = 0;
        if (size != 0) {
            idCurrent = abs(size);
            if (!openStorage(idCurrent, size > 0 ? 'w' : 'r')) {
                idCurrent = 0;
                return 0;
            }
        }
        return 1;
    }
    if (size > 0) {
        fwrite(data, size, 1, fCurrent);
    } else {
        fread(data, -size, 1, fCurrent);
    }
    return 1;
}

int main(void) {
    initSystem();
    init(VARS_SPACE_SIZE, 80, sizeof(dataSpace) - VARS_SPACE_SIZE);
    while(1) {
        lastInput = translateInput(sysGetc());
        dispatch();
    }
    return 0;
}

