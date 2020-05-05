#include "mystdlib.h"
#include "mytypes.h"
#include "basic_tokens.h"
#include "basic_tokenint.h"
#include "basic_editor.h"
#include "basic_utils.h"
#include "basic_extern.h"
#include "basic_textual.h"

extern char mainState;
extern token* toksBody;

numeric* calcStack;
short nextLineNum = 1;
static prgline* progLine;
short sp, spInit;
varHolder* vars;
char numVars;
short arrayBytes;
labelCacheElem* labelCache;
short labelsCached;
numeric lastDim;
static numeric execStepsCount;
static numeric delayT0, delayLimit;

void execRem(void);
void execPrint(void);
void execInput(void);
void execIf(void);
void execGoto(void);
void execGosub(void);
void execReturn(void);
void execEnd(void);
void execLet(void);
void execLeta(void);
void execDim(void);
void execDelay(void);
void execData(void);
void execEmit(void);

void (*executors[])(void) = {
    execRem,
    execPrint,
    execInput,
    execIf,
    execGoto,
    execGosub,
    execReturn,
    execEnd,
    execLet,
    execLeta,
    execDim,
    execDelay,
    execData,
    execEmit,
};

void resetTokenExecutor(void) {
    numVars = 0;
    arrayBytes = 0;
    sp = spInit;
    vars[0].name = 0;
}

short varSize(void) {
    return numVars * sizeof(varHolder) + arrayBytes;
}

void initTokenExecutor(char* space, short size) {
    spInit = (size / sizeof(*calcStack));
    vars = (varHolder*)(void*)space;
    calcStack = (numeric*)(void*)space;
    resetTokenExecutor();
}

short shortVarName(nstring* name) {
    short n = name->text[0];
    if (name->len > 1) {
        n += name->text[1] * 127;
    }
    return n;
}

short shortArrayName(char letter) {
    return 0x7F00 | letter;
}

unsigned char findVar(short name) {
    short hi = numVars;
    short lo = 0;
    short mid;
    while (hi > lo) {
        mid = (hi + lo) / 2;
        if (vars[mid].name < name) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

numeric getVar(short name) {
    unsigned char i = findVar(name);
    return (vars[i].name == name) ? vars[i].value : 0;
}

short getArrayOffset(char letter) {
    short name = shortArrayName(letter);
    unsigned char i = findVar(name);
    return (vars[i].name == name) ? vars[i].value : -1;
}

char checkLowVarsMemory(short toAddBytes) {
    if (((char*)(vars + numVars)) + arrayBytes + toAddBytes >= (char*)(calcStack + spInit - 5)) {
        outputCr();
        outputConstStr(ID_COMMON_STRINGS, 12, NULL);
        outputCr();
        return 1;
    }
    return 0;
}

void setVar(short name, numeric value) {
    unsigned char i = findVar(name);
    if (vars[i].name != name) {
        if (checkLowVarsMemory(sizeof(varHolder))) {
            return;
        }
        if (i < numVars) {
            memmove(vars + i + 1, vars + i, sizeof(varHolder) * (numVars - i) + arrayBytes);
        }
        vars[i].name = name;
        numVars += 1;
    }
    vars[i].value = value;
}

short findLabel(short num) {
    short hi = labelsCached;
    short lo = 0;
    short mid;
    while (hi > lo) {
        mid = (hi + lo) / 2;
        if (labelCache[mid].num < num) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

prgline* getCachedLabel(short num) {
    short i = findLabel(num);
    return labelCache[i].num == num ? (prgline*)(void*)(prgStore + labelCache[i].offset) : NULL;
}

void addCachedLabel(short num, short offset) {
    short idx = findLabel(num);
    if (idx < labelsCached) {
        memmove(labelCache + idx + 1, labelCache + idx, sizeof(labelCacheElem) * (labelsCached - idx));
    }
    labelCache[idx].num = num;
    labelCache[idx].offset = offset;
    labelsCached += 1;
}

static void advance(void) {
    if (curTok->type != TT_NONE) {
        curTok = nextToken(curTok);
    }
}

void calcOperation(char op) {
    numeric top = calcStack[sp++];
    switch (op) {
        case '+':
            calcStack[sp] += top;
            break;
        case '-':
            calcStack[sp] -= top;
            break;
        case '*':
            calcStack[sp] *= top;
            break;
        case '/':
            calcStack[sp] /= top;
            break;
        case '%':
            calcStack[sp] %= top;
            break;
        case '~':
            calcStack[--sp] = -top;
            break;
        case '!':
            calcStack[--sp] = !top;
            break;
        case '<':
            calcStack[sp] = calcStack[sp] < top;
            break;
        case '>':
            calcStack[sp] = calcStack[sp] > top;
            break;
        case '=':
            calcStack[sp] = calcStack[sp] == top;
            break;
        case '{':
            calcStack[sp] = calcStack[sp] <= top;
            break;
        case '}':
            calcStack[sp] = calcStack[sp] >= top;
            break;
        case '#':
            calcStack[sp] = calcStack[sp] != top;
            break;
        case '&':
            calcStack[sp] = calcStack[sp] && top;
            break;
        case '|':
            calcStack[sp] = calcStack[sp] || top;
            break;
        case '^':
            calcStack[sp] = calcStack[sp] ^ top;
            break;
    }
}

void calcFunction(nstring* name) {
    short i;
    numeric r;
    numeric h = hashOfNStr(name);
    if (h == 0x1FF) { // KEY
        i = calcStack[sp];
        calcStack[sp] = lastInput;
        if (i != 0) {
            lastInput = 0;
        }
        return;
    }
    if (h == 0xC9) { // MS
        calcStack[sp] = sysMillis(calcStack[sp]);
        return;
    }
    if (h == 0x1D3) { // ABS
        if (calcStack[sp] < 0) {
            calcStack[sp] = -calcStack[sp];
        }
        return;
    }
    i = extraFunctionByHash(h);
    if (i >= 0) {
        // arguments will appear in reverse order
        r = extraFunction(i, calcStack + sp);
        sp += extraFuncArgCnt[i] - 1;
        calcStack[sp] = r;
        return;
    }
    calcStack[sp] = 0;
}

void calcArray(char letter) {
    short offset = getArrayOffset(letter);
    if (offset == -1) {
        calcStack[sp] = 0;
        return;
    }
    char b = (offset & 0x8000) ? 1 : sizeof(numeric);
    offset = (offset & 0x7FFF) + b * calcStack[sp];
    char* p = ((char*)(void*)vars) + sizeof(varHolder) * numVars + offset;
    if (b > 1) {
        calcStack[sp] = *((numeric*)(void*)p);
    } else {
        calcStack[sp] = *((unsigned char*)(void*)p);
    }
}

numeric calcExpression(void) {
    while (1) {

        switch (curTok->type) {
            case TT_NONE:
            case TT_SEPARATOR:
                return calcStack[sp++];
            case TT_NUMBER:
                calcStack[--sp] = curTok->body.integer;
                break;
            case TT_VARIABLE:
                calcStack[--sp] = getVar(shortVarName(&(curTok->body.str)));
                break;
            case TT_SYMBOL:
                calcOperation(curTok->body.symbol);
                break;
            case TT_FUNCTION:
                calcFunction(&(curTok->body.str));
                break;
            case TT_ARRAY:
                calcArray(curTok->body.symbol);
                break;
        }
        advance();
    }
}

void execLet(void) {
    short varname = shortVarName(&(curTok->body.str));
    advance();
    setVar(varname, calcExpression());
}

void setArray(char symbol, short idx, numeric value) {
    short offset = getArrayOffset(symbol);
    if (offset == -1) {
        return;
    }
    char b = (offset & 0x8000) ? 1 : sizeof(numeric);
    offset = (offset & 0x7FFF) + b * idx;
    char* p = ((char*)(void*)vars) + sizeof(varHolder) * numVars + offset;
    if (b > 1) {
        *((numeric*)(void*)p) = value;
    } else {
        *((unsigned char*)(void*)p) = (value & 0xFF);
    }
}

void execLeta(void) {
    char a = curTok->body.symbol;
    advance();
    short idx = calcExpression();
    advance();
    setArray(a, idx, calcExpression());
}

void execDim(void) {
    short name = shortArrayName(curTok->body.symbol);
    lastDim = curTok->body.symbol & 0x1F;
    advance();
    short len = curTok->body.integer;
    advance();
    char itemSize;
    if (curTok->type == TT_NONE) {
        itemSize = sizeof(numeric);
    } else {
        advance();
        itemSize = 1;
    }
    unsigned char pos = findVar(name);
    if (vars[pos].name == name) {
        return;
    }
    if (checkLowVarsMemory(sizeof(varHolder) + len * itemSize)) {
        return;
    }
    setVar(name, arrayBytes | (itemSize == 1 ? 0x8000 : 0));
    arrayBytes += len * itemSize;
}

void execData(void) {
    char a = (lastDim & 0x1F) | 0x40; // capital letter
    unsigned char i;
    if (a < 'A' || a > 'Z') {
        return;
    }
    do {
        if (curTok->type == TT_NUMBER) {
            setArray(a, lastDim >> 5, curTok->body.integer);
            lastDim += (1 << 5);
        } else {
            for (i = 0; i < curTok->body.str.len; i += 1) {
                setArray(a, lastDim >> 5, curTok->body.str.text[i]);
                lastDim += (1 << 5);
            }
        }
        advance();
    } while (curTok->type != TT_NONE);
}

void setDelay(numeric millis) {
    delayT0 = sysMillis(1);
    delayLimit = millis;
}

void execDelay(void) {
    setDelay(calcExpression());
    mainState |= STATE_DELAY;
}

char checkDelay() {
    return sysMillis(1) - delayT0 > delayLimit;
}

void dispatchDelay() {
    if (checkDelay()) {
        mainState &= ~STATE_DELAY;
    }
}

void execRem(void) {
    while (curTok->type != TT_NONE) {
        advance();
    }
}

void execPrint(void) {
    while (1) {
        switch (curTok->type) {
            case TT_NONE:
                outputCr();
                return;
            case TT_SEPARATOR:
                break;
            case TT_LITERAL:
                outputNStr(&(curTok->body.str));
                break;
            default:
                outputInt(calcExpression());
                break;
        }
        advance();
    }
}

void execInput(void) {
    mainState |= STATE_INPUT;
    outputChar('?');
    outputChar(curTok->body.str.text[0]);
    outputChar('=');
}

void dispatchInput() {
    if (lastInput == 0) {
        return;
    }
    if (!readLine()) {
        return;
    }
    setVar(shortVarName(&(curTok->body.str)), decFromStr(lineSpace));
    advance();
    mainState &= ~STATE_INPUT;
}

void execEmit(void) {
    while (1) {
        switch (curTok->type) {
            case TT_NONE:
                return;
            case TT_SEPARATOR:
                break;
            default:
                outputChar(calcExpression() & 0xFF);
                break;
        }
        advance();
    }
}

void execIf(void) {
    if (calcExpression() == 0) {
        while (curTok->type != TT_NONE) {
            advance();
        }
    } else {
        advance();
    }
}

void execGoto(void) {
    nextLineNum = curTok->body.integer;
    advance();
}

void execGosub(void) {
    calcStack[--sp] = nextLineNum;
    nextLineNum = curTok->body.integer;
    advance();
}

void execReturn(void) {
    nextLineNum = calcStack[sp++];
}

void execEnd(void) {
    nextLineNum = 32767;
}

void execExtra(unsigned char cmd) {
    unsigned char n = extraCmdArgCnt[cmd];
    char i;
    sp -= n;
    for (i = 0; i < n; i++) {
        calcStack[sp + i] = calcExpression();
        advance();
    }
    extraCommand(cmd, calcStack + sp);
    sp += n;
}

void executeTokens(token* t) {
    curTok = t;
    while (t->type != TT_NONE) {
        advance();
        if (t->body.command < CMD_EXTRA) {
            executors[t->body.command]();
            if (t->body.command == CMD_INPUT) {
                break;
            }
        } else {
            execExtra(t->body.command - CMD_EXTRA);
        }
        t = curTok;
    }
}

void signalEndOfCode(void) {
    outputConstStr(ID_COMMON_STRINGS, 5, NULL);
    outputCr();
}

void stopExecution() {
    if ((mainState & STATE_RUN) != 0) {
        editorLoad();
    }
    mainState &= ~(STATE_RUN | STATE_STEPS | STATE_BREAK);
}

char executeStep() {
    prgline* p = findLine(nextLineNum);
    if (p->num == 0) {
        stopExecution();
        signalEndOfCode();
        return 1;
    }
    nextLineNum = p->num + 1;
    memcpy(lineSpace, p->str.text, p->str.len);
    lineSpace[p->str.len] = 0;
    parseLine(lineSpace, toksBody);
    executeTokens(toksBody);
    return 0;
}

void dispatchBreak() {
    stopExecution();
    execStepsCount = 0;
    sp = spInit;
    outputConstStr(ID_COMMON_STRINGS, 4, NULL); // BREAK
    outputCr();
}

void executeNonParsed(numeric count) {
    if (count != 0) {
        execStepsCount = count;
        return;
    }
    if (execStepsCount != -1) {
        execStepsCount -= 1;
    }
    if (executeStep()) {
        execStepsCount = 0;
    }
    if (execStepsCount == 0) {
        stopExecution();
    }
}

void initParsedRun(void) {
    nextLineNum = 1;
    progLine = findLine(nextLineNum);
    labelsCached = 0;
    labelCache = (labelCacheElem*)(void*)(prgStore + prgSize);
    mainState |= STATE_RUN;
}

void executeParsedRun(void) {
    prgline* next;
    if (progLine->num == 0 || nextLineNum == 0) {
        stopExecution();
        signalEndOfCode();
        return;
    }
    next = (prgline*)(void*)((char*)(void*)progLine
            + sizeof(progLine->num) + sizeof(progLine->str.len) + progLine->str.len);
    nextLineNum = next->num;
    executeTokens((token*)(void*)(progLine->str.text));
    if (next->num != nextLineNum) {
        progLine = getCachedLabel(nextLineNum);
        if (progLine == NULL) {
            progLine = findLine(nextLineNum);
            addCachedLabel(nextLineNum, (short)((char*)(void*)progLine - (char*)(void*)prgStore));
        }
    } else {
        progLine = next;
    }
}
