#include "mystdlib.h"
#include "basic_utils.h"
#include "basic_extern.h"

void trim(char* s) {
    short i = 0, k;
    while (s[i] != 0 && s[i] <= ' ') {
        i++;
    }
    k = 0;
    while (s[i] != 0) {
        s[k++] = s[i++];
    }
    do {
        s[k--] = 0;
    } while (k >= 0 && s[k] <= ' ');
}

char* skipSpaces(char* s) {
    while (isSpace(*s)) {
        s++;
    }
    return s;
}

char* skipDigits(char* s) {
    while (isDigit(*s)) {
        s++;
    }
    return s;
}

char charInStr(char c, char* s) {
    while (*s != 0) {
        if (*s == c) {
            return 1;
        }
        s++;
    }
    return 0;
}

char cmpNStrToStr(nstring* ns, char* s) {
    if (ns->len != strlen(s)) {
        return 0;
    }
    return memcmp(&(ns->text), s, ns->len) == 0;
}

void outputChar(char c) {
    sysPutc(c);
}

void outputStr(char* s) {
    while (*s) {
        sysPutc(*(s++));
    }
}

void outputNStr(nstring* t) {
	short i;
    for (i = 0; i < t->len; i++) {
        sysPutc(t->text[i]);
    }
};

void outputInt(long n) {
    long d = 1000000000;
    if (n < 0) {
        outputChar('-');
        n = -n;
    }
    if (n == 0) {
        sysPutc('0');
        return;
    }
    while (d > n) {
        d /= 10;
    }
    while (d > 0) {
        sysPutc(n / d + '0');
        n %= d;
        d /= 10;
    }
}

int decFromStr(char* s) {
    schar sign = 1;
    char base = 10;
    int res = 0;
    if (*s == '-') {
        sign = -1;
        s += 1;
    }
    if (*s == '0') {
        s += 1;
        if (toUpper(*s) == 'X') {
            base = 16;
            s += 1;
        } else if (toUpper(*s) == 'B') {
            base = 2;
            s += 1;
        } else {
            base = 8;
        }
    }
    while (isDigitBased(*s, base)) {
        res = res * base + makeDigit(*s++, base);
    }
    return res * sign;
}

void outputCr() {
    sysPutc('\n');
}

char toUpper(char c) {
    return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

char isAlpha(char c) {
    c = toUpper(c);
    return c >= 'A' && c <= 'Z';
}

char isDigit(char c) {
    return c >= '0' && c <= '9';
}

char isDigitBased(char c, char base) {
    if (c < '0') {
        return 0;
    } else if (c <= '9') {
        return base > c - '0';
    } else {
        c = toUpper(c);
        return c >= 'A' && base > c - 'A' + 10;
    }
}

char makeDigit(char c, char base) {
    if (c < '0') {
        return 0;
    } else if (c <= '9') {
        return c - '0';
    } else {
        return toUpper(c) - 'A' + 10;
    }
}

char isAlNum(char c) {
    return isDigit(c) || isAlpha(c);
}

char isSpace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\v':
        case '\n':
        case '\r':
        case '\f':
            return 1;
    }
    return 0;
}

numeric hashOfNStr(nstring* t) {
    unsigned char i = 0, n = t->len;
    numeric res = 0;
    while (i < n) {
        res = (res << 1) ^ t->text[i];
        i++;
    }
    return res;
}

void pause(numeric millis) {
    numeric t0 = sysMillis(1);
    while (sysMillis(1) - t0 < millis) {
    }
}

