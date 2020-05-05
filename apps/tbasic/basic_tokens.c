#include "mystdlib.h"
#include "basic_tokens.h"
#include "basic_expr.h"
#include "basic_utils.h"
#include "basic_extern.h"
#include "basic_textual.h"

char* cur;
token* curTok;
token* prevTok;
char* parseError;
char parseErrorCode;

static short cmdCodeByHash(numeric h) {
    // replaced array with switch to save RAM
    switch (h) {
        case 0x018F: // REM
            return CMD_REM;
        case 0x067C: // PRINT
            return CMD_PRINT;
        case 0x075E: // INPUT
            return CMD_INPUT;
        case 0x00D4: // IF
            return CMD_IF;
        case 0x03E3: // GOTO
            return CMD_GOTO;
        case 0x07AC: // GOSUB
            return CMD_GOSUB;
        case 0x0D0E: // RETURN
            return CMD_RETURN;
        case 0x01CC: // END
            return CMD_END;
        case 0x01CF: // DIM
            return CMD_DIM;
        case 0x0783: // DELAY
            return CMD_DELAY;
        case 0x03CD: // DATA
            return CMD_DATA;
        case 0x03DA: // EMIT
            return CMD_EMIT;
        default:
            return extraCommandByHash(h);
    }
}

char* getCurTokPos() {
    return cur;
}

short tokenSize(token* t) {
    switch (t->type) {
        case TT_NUMBER:
            return 1 + sizeof(t->body.integer);
        case TT_NAME:
        case TT_COMMENT:
        case TT_LITERAL:
        case TT_FUNCTION:
        case TT_VARIABLE:
            return 2 + t->body.str.len;
        case TT_FUNC_END:
        case TT_SYMBOL:
        case TT_ARRAY:
        case TT_COMMAND:
            return 1 + sizeof(t->body.symbol);
        case TT_NONE:
        case TT_ERROR:
        case TT_SEPARATOR:
            return 1;
    }
    return 0;
}

short tokenChainSize(token* src) {
    token* t = src;
    while (t->type != TT_NONE) {
        t = nextToken(t);
    }
    return (short)((char*)(void*)t - (char*)(void*)src) + 1;
}

void copyToken(token* dst, token* src) {
    memcpy(dst, src, tokenSize(src));
}

void setTokenError(char* pos, char code) {
    parseErrorCode = code;
    parseError = pos;
    curTok->type = TT_ERROR;
}

token* nextToken(token* t) {
    return (token*)(void*) ((char*)(void*)t + tokenSize(t));
}

void skipTokenInInput(char offset) {
    cur = skipSpaces(cur + offset);
}

static void advance(char* s) {
    cur = skipSpaces(s);
    prevTok = curTok;
    curTok = nextToken(curTok);
}

static void substCommandFound(char code) {
    curTok->type = TT_COMMAND;
    curTok->body.command = code;
    nextToken(curTok)->type = TT_ERROR;
}

void trySubstCmd(void) {
    short i;
    numeric cmdHash = tokenHash(curTok);
    i = cmdCodeByHash(cmdHash);
    if (i >= 0) {
        substCommandFound(i);
        return;
    }
}

char parseName(char checkCmd) {
    short i = 0;
    if (!isAlpha(*cur)) {
        return 0;
    }
    curTok->type = TT_NAME;
    while (isAlNum(cur[i])) {
        curTok->body.str.text[i] = toUpper(cur[i]);
        i++;
    }
    curTok->body.str.len = i;
    if (checkCmd) {
        trySubstCmd();
    }
    advance(cur + i);
    return 1;
}

char parseChar(void) {
    if (cur[0] != '\'') {
        return 0;
    }
    curTok->type = TT_NUMBER;
    curTok->body.integer = ((unsigned char*) cur)[1];
    cur += 2;
    advance(cur);
    return 1;
}

char parseNumber(void) {
    char base = 10;
    if (!isDigit(*cur)) {
        return parseChar();
    }
    curTok->type = TT_NUMBER;
    curTok->body.integer = 0;
    if (cur[0] == '0') {
        if (toUpper(cur[1]) == 'X') {
            base = 16;
            cur += 2;
        } else if (toUpper(cur[1]) == 'B') {
            base = 2;
            cur += 2;
        } else {
            base = 8;
        }
    }
    while (isDigitBased(*cur, base)) {
        curTok->body.integer = curTok->body.integer * base + makeDigit(*cur, base);
        cur++;
    }
    advance(cur);
    return 1;
}

char parseNone(void) {
    if (*cur != 0) {
        setTokenError(cur, 5);
        return 0;
    }
    curTok->type = TT_NONE;
    return 1;
}

char parseComment(void) {
    unsigned char len = strlen(cur);
    curTok->type = TT_COMMENT;
    curTok->body.str.len = len;
    memcpy(&(curTok->body.str.text), cur, len);
    advance(cur + len);
    return parseNone();
}

char parseLiteral() {
    if (*cur != '"') {
        return 0;
    }
    short i = 1;
    curTok->type = TT_LITERAL;
    while (cur[i] != 0 && cur[i] != '"') {
        curTok->body.str.text[i - 1] = cur[i];
        i++;
    }
    curTok->body.str.len = i - 1;
    advance(cur + i + (cur[i] == '"' ? 1 : 0));
    return 1;
}

char parseSymbol() {
    curTok->type = TT_SYMBOL;
    char c = 0;
    if (cur[0] == '<') {
        if (cur[1] == '>') {
            c = '#';
        } else if (cur[1] == '=') {
            c = '{';
        }
    } else if (cur[0] == '>' && cur[1] == '=') {
        c = '}';
    }
    if (c != 0) {
        cur++;
    } else {
        c = cur[0];
    }
    curTok->body.symbol = toUpper(c);
    advance(cur + 1);
    return 1;
}

char parseLineNumber(void) {
    char* start = cur;
    if (!parseNumber()) {
        return 1;
    }
    if (prevTok->body.integer < 1 || prevTok->body.integer > MAX_LINE_NUMBER) {
        setTokenError(start, 7);
        return 0;
    }
    return 1;
}

char parseSemicolon(void) {
    if (*cur != ';') {
        setTokenError(cur, 4);
        return 0;
    }
    curTok->type = TT_SEPARATOR;
    advance(cur + 1);
    return 1;
}

char* skipSubscripts() {
    char* p = cur;
    char br = 0;
    while (*p != 0) {
        if (*p == '(') {
            br += 1;
        } else if (*p == ')') {
            br -= 1;
            if (br == 0) {
                break;
            }
        }
        p++;
    }
    return p;
}

char assignmentSyntax(void) {
    char* p;
    if (*cur == '=') {
        return CMD_LET;
    } else if (*cur != '(') {
        setTokenError(cur, 2);
        return 0;
    }
    p = skipSubscripts();
    if (*p == 0) {
        setTokenError(p, 10);
        return 0;
    }
    p = skipSpaces(p + 1);
    if (*p != '=') {
        setTokenError(cur, 2);
        return 0;
    }
    return CMD_LETA;
}

char parseSubscripts(void) {
    char* p = skipSubscripts();
    *p = ';';
    cur++;
    if (!parseExpression()) {
        return 0;
    }
    parseSemicolon();
    *p = ')';
    return 1;
}

char parseAssignment(void) {
    char synt = assignmentSyntax();
    if (!synt) {
        return 0;
    }
    curTok->type = TT_COMMAND;
    memmove((char*)(void*)prevTok + tokenSize(curTok), prevTok, tokenSize(prevTok));
    prevTok->type = TT_COMMAND;
    prevTok->body.command = synt;
    curTok = nextToken(prevTok);
    if (synt == CMD_LETA) {
        curTok->type = TT_ARRAY;
        curTok->body.symbol = curTok->body.str.text[0];
    }
    prevTok = curTok;
    curTok = nextToken(curTok);
    if (synt == CMD_LETA) {
        if (!parseSubscripts()) {
            return 0;
        }
    }
    cur = skipSpaces(cur + 1);
    return parseExpression() && parseNone();
}

char parseExprOrLiteral(void) {
    if (parseLiteral()) {
        return 1;
    }
    return parseExpression();
}

char parseVar(void) {
    if (!parseName(0)) {
        setTokenError(cur, 3);
        return 0;
    }
    prevTok->type = TT_VARIABLE;
    return 1;
}

char parseExprList(void) {
    if (!parseExpression()) {
        return 0;
    }
    while (*cur != 0) {
        if (!parseSemicolon() || !parseExpression()) {
            return 0;
        }
    }
    return parseNone();
}

char parsePrintList(void) {
    if (!parseExprOrLiteral()) {
        return 0;
    }
    while (*cur != 0) {
        if (!parseSemicolon() || !parseExprOrLiteral()) {
            return 0;
        }
    }
    return parseNone();
}

char parseDataList(void) {
    do {
        if (!parseNumber() && !parseLiteral()) {
            setTokenError(cur, 8);
            return 0;
        }
    } while (*cur != 0);
    return parseNone();
}

char parseNExpressions(char cnt) {
    if (cnt > 0) {
        if (!parseExpression()) {
            return 0;
        }
        while (--cnt > 0) {
            if (!parseSemicolon() || !parseExpression()) {
                return 0;
            }
        }
    }
    return parseNone();
}

char parseLabel(void) {
    if (parseNumber()) {
        return parseNone();
    }
    setTokenError(cur, 8);
    return 0;
}

char parseStatement(void);

char parseConditional(void) {
    if (!parseExpression()) {
        return 0;
    }
    return parseSemicolon() && parseStatement();
}

char parseAllocate() {
    if (!parseName(0)) {
        setTokenError(cur, 3);
        return 0;
    }
    prevTok->type = TT_ARRAY;
    prevTok->body.symbol = prevTok->body.str.text[0];
    curTok = nextToken(prevTok);
    if (!parseNumber()) {
        setTokenError(cur, 8);
        return 0;
    }
    if (*cur != 0) {
        if (!parseSymbol() || prevTok->body.symbol != 'B') {
            setTokenError(cur, 9);
            return 0;
        }
    }
    return parseNone();
}

void parseSpecialWithError() {
    curTok = nextToken(curTok);
    while (parseName(0) || parseNumber() || parseLiteral()) {
    }
    curTok->type = TT_ERROR;
}

char parseStatement(void) {
    char cmd;
    if (!parseName(1)) {
        setTokenError(cur, 1);
        return 0;
    } else if (prevTok->type != TT_COMMAND) {
        if (parseAssignment()) {
            return 1;
        } else {
            parseSpecialWithError();
            return 0;
        }
    }
    cmd = prevTok->body.command;
    if (cmd == CMD_REM) {
        return parseComment();
    } else if (cmd == CMD_GOTO || cmd == CMD_GOSUB) {
        return parseLabel();
    } else if (cmd == CMD_RETURN || cmd == CMD_END) {
        return parseNone();
    } else if (cmd == CMD_PRINT) {
        return parsePrintList();
    } else if (cmd == CMD_INPUT) {
        return parseVar() && parseNone();
    } else if (cmd == CMD_IF) {
        return parseConditional();
    } else if (cmd == CMD_DIM) {
        return parseAllocate();
    } else if (cmd == CMD_DELAY) {
        return parseNExpressions(1);
    } else if (cmd == CMD_DATA) {
        return parseDataList();
    } else if (cmd == CMD_EMIT) {
        return parseExprList();
    } else if (cmd >= CMD_EXTRA) {
        return parseNExpressions(extraCmdArgCnt[cmd - CMD_EXTRA]);
    }
    setTokenError(cur, 6);
    return 0;
}

void parseLine(char* line, token* tokens) {
    cur = line;
    curTok = tokens;
    prevTok = NULL;
    setTokenError(NULL, 0);
    if (parseLineNumber()) {
        if (*cur != 0) {
            parseStatement();
        } else {
            parseNone();
        }
    }
}

char tokenClass(token* t) {
    return t->type & 0xF0;
}

numeric tokenHash(token* t) {
    if (tokenClass(t) != TT_NAME) {
        return 0;
    }
    return hashOfNStr(&(t->body.str));
}

char* getParseErrorPos(void) {
    return parseError;
}

void getParseErrorMsg(char* s) {
    outputConstStr(ID_PARSING_ERRORS, parseErrorCode, s);
}

