#include "mystdlib.h"
#include "basic_tokens.h"
#include "basic_editor.h"
#include "basic_exectoks.h"
#include "basic_utils.h"
#include "basic_extern.h"
#include "basic_textual.h"

static short listLine, listPage;
token* toksBody;
char mainState;

#if 0
void printToken(token* t) {
    switch (t->type) {
        case TT_NUMBER:
            outputStr("INT=");
            outputInt(t->body.integer);
            break;
        case TT_NAME:
            outputStr("NAME=");
            outputNStr(&(t->body.str));
            break;
        case TT_VARIABLE:
            outputStr("VAR=");
            outputNStr(&(t->body.str));
            break;
        case TT_FUNCTION:
            outputStr("FN=");
            outputNStr(&(t->body.str));
            break;
        case TT_COMMAND:
            outputStr("CMD=");
            outputInt(t->body.command);
            break;
        case TT_LITERAL:
            outputStr("STR=\"");
            outputNStr(&(t->body.str));
            outputStr("\"");
            break;
        case TT_COMMENT:
            outputStr("REM=\"");
            outputNStr(&(t->body.str));
            outputStr("\"");
            break;
        case TT_SYMBOL:
            outputStr("SYM=");
            outputChar(t->body.symbol);
            break;
        case TT_ARRAY:
            outputStr("ARR=");
            outputChar(t->body.symbol);
            break;
        case TT_FUNC_END:
            outputStr("FE=%d");
            outputInt(t->body.symbol);
            break;
        case TT_NONE:
            outputChar('N');
            break;
        case TT_SEPARATOR:
            outputChar(';');
            break;
        default:
            outputChar('E');
            break;
    }
    outputChar('\n');
}

void printTokens(token* t) {
    while (1) {
        printToken(t);
        outputChar(' ');
        if (tokenClass(t) == TT_NONE) {
            break;
        }
        t = nextToken(t);
    }
    outputCr();
}
#else
void printTokens(token* t) {
}
#endif

void printProgram(void) {
    prgline* p = findLine(listLine);
    if (p->num == 0 && listLine > 1) {
        p = findLine(1);
    }
    short lineCount = 0;
    while (p->num != 0 && lineCount < listPage) {
        listLine = p->num + 1;
        outputInt(p->num);
        outputChar(' ');
        outputNStr(&(p->str));
        outputCr();
        p = findLine(p->num + 1);
        lineCount += 1;
    }
}

void listProgram(token* t) {
    t = nextToken(nextToken(t));
    if (t->type == TT_NUMBER) {
        listLine = t->body.integer;
        t = nextToken(t);
        if (t->type == TT_NUMBER) {
            listPage = t->body.integer;
        }
    }
    printProgram();
}

void executeSteps() {
    token* t = nextToken(nextToken(toksBody));
    mainState |= STATE_STEPS;
    executeNonParsed(t->type == TT_NUMBER ? t->body.integer : 1);
}

void executeRun() {
    if (editorSave()) {
        editorLoadParsed();
        initParsedRun();
    } else {
        executeNonParsed(-1);
    }
}

void manualSave(void) {
    editorSave();
    outputConstStr(ID_COMMON_STRINGS, 6, NULL); // Saved
    outputChar(' ');
    outputInt(prgSize > 2 ? prgSize + 2 : 0);
    outputChar(' ');
    outputConstStr(ID_COMMON_STRINGS, 8, NULL); // bytes
    outputCr();
}

void manualLoad(void) {
    if (editorLoad()) {
        outputConstStr(ID_COMMON_STRINGS, 7, NULL); // Loaded
        outputChar(' ');
        outputInt(prgSize + 2);
        outputChar(' ');
        outputConstStr(ID_COMMON_STRINGS, 8, NULL); // bytes
        outputCr();
    } else {
        outputConstStr(ID_COMMON_STRINGS, 9, NULL); // bytes
        outputCr();
    }
}

void prgReset(void) {
    resetEditor();
    resetTokenExecutor();
}

void showInfo(void) {
    outputConstStr(ID_COMMON_STRINGS, 1, NULL); // code:
    outputInt(prgSize);
    outputCr();
    outputConstStr(ID_COMMON_STRINGS, 2, NULL); // vars:
    outputInt(varSize());
    outputCr();
    outputConstStr(ID_COMMON_STRINGS, 3, NULL); // next:
    outputInt(nextLineNum);
    outputCr();
}

void metaOrError() {
    numeric h = tokenHash(toksBody);
    if (h == 0x3B6) { // LIST
        listProgram(toksBody);
    } else if (h == 0x312) { // STEP
        executeSteps();
    } else if (h == 0x1AC) { // RUN
        executeRun();
    } else if (h == 0x375) { // SAVE
        manualSave();
    } else if (h == 0x39A) { // LOAD
        manualLoad();
    } else if (h == 0x69A) { // RESET
        prgReset();
    } else if (h == 0x3B3) { // INFO
        showInfo();
    } else {
        getParseErrorMsg(lineSpace);
        outputStr(lineSpace);
        outputChar(' ');
        outputInt((long)(getParseErrorPos() - lineSpace) + 1);
        outputCr();
    }
}

void processLine() {
    if (lineSpace[0] == 0) {
        return;
    }
    parseLine(lineSpace, toksBody);
    printTokens(toksBody);
    if (getParseErrorPos() != NULL) {
        metaOrError();
        return;
    }
    if (toksBody->type != TT_NUMBER) {
        executeTokens(toksBody);
    } else {
        injectLine(skipSpaces(skipDigits(lineSpace)), toksBody->body.integer);
    }
}

void preload(char* line, token* t) {
    if (editorLoadParsed()) {
        outputConstStr(ID_COMMON_STRINGS, 10, NULL); // code found, autorun message
        outputCr();
        setDelay(1000);
        mainState = STATE_PRELOAD;
    } else {
        prgReset();
    }
}

void init(short dataSize, short lineSize, short progSize) {
    outputCr();
    outputConstStr(ID_COMMON_STRINGS, 0, NULL); // TBASIC vX.X
    outputCr();
    initEditor(dataSpace + dataSize, progSize);
    initTokenExecutor(dataSpace, dataSize);
    listLine = 1;
    listPage = 10;
    mainState = STATE_INTERACTIVE;
    toksBody = (token*)(void*) (lineSpace + lineSize);
    preload(lineSpace, toksBody);
}

void waitPreloadRunDelay() {
    if (lastInput > 0) {
        mainState &= ~STATE_PRELOAD;
        outputConstStr(ID_COMMON_STRINGS, 11, NULL); // canceled
        outputCr();
        editorLoad();
    } else if (checkDelay()) {
        mainState &= ~STATE_PRELOAD;
        initParsedRun();
    }
}

void dispatch() {
    if (lastInput == 3) {
        mainState |= STATE_BREAK;
    }
    if ((mainState & (STATE_RUN | STATE_SLOWED)) == STATE_RUN) {
        executeParsedRun();
        return;
    }
    switch (mainState & STATE_SLOWED) {
        case STATE_DELAY:
            dispatchDelay();
            return;
        case STATE_INPUT:
            dispatchInput();
            lastInput = 0;
            return;
        case STATE_BREAK:
            dispatchBreak();
            lastInput = 0;
            return;
    }
    if ((mainState & STATE_STEPS) != 0) {
        executeNonParsed(0);
    } else if ((mainState & STATE_PRELOAD) != 0) {
        waitPreloadRunDelay();
    } else {
        if (lastInput > 0) {
            if (readLine()) {
                processLine();
            }
            lastInput = 0;
        }
    }
    return;
}

