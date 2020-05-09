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
#ifndef MBASIC_H
#define MBASIC_H

#ifdef __cplusplus
    extern "C" {
#endif

// Constants.

// Application execution constants.
//

// Redefinitions.
#ifndef assert
  #define assert(a)
#endif
#ifndef malloc
  #define malloc                    sys_malloc
#endif
#ifndef realloc
  #define realloc                   sys_realloc
#endif
#ifndef calloc
  #define calloc                    sys_calloc
#endif

// Main app configuration variables.
//
#define HISTORY_FILE                "mbasic.hst"

// Ed configuration variables.
//
#define MAX_APPEND_BUFSIZE          1024
#define ED_QUIT_TIMES               3
#define ED_QUERY_LEN                256
#define ED_TAB_SIZE                 4

// Mini Basic configuration.
/* tokens defined */
#define EOS                         0 
#define VALUE                       1
#define BPI                         2
#define BE                          3

#define DIV                         10
#define MULT                        11
#define OPAREN                      12
#define CPAREN                      13
#define PLUS                        14
#define MINUS                       15
#define SHRIEK                      16
#define COMMA                       17
#define MOD                         200

#define ERROR                       20
#define EOL                         21
#define EQUALS                      22
#define STRID                       23
#define FLTID                       24
#define DIMFLTID                    25
#define DIMSTRID                    26
#define QUOTE                       27
#define GREATER                     28
#define LESS                        29
#define SEMICOLON                   30

#define PRINT                       100
#define LET                         101
#define DIM                         102
#define IF                          103
#define THEN                        104
#define AND                         105
#define OR                          106
#define GOTO                        107
#define BINPUT                      108
#define REM                         109
#define FOR                         110
#define TO                          111
#define NEXT                        112
#define STEP                        113
#define POKE                        114

#define SIN                         5
#define COS                         6
#define TAN                         7
#define LN                          8
#define POW                         9
#define SQRT                        18
#define ABS                         201
#define LEN                         202
#define ASCII                       203
#define ASIN                        204
#define ACOS                        205
#define ATAN                        206
#define INT                         207
#define RND                         208
#define VAL                         209
#define VALLEN                      210
#define INSTR                       211
#define PEEK                        212

#define CHRSTRING                   300
#define STRSTRING                   301
#define LEFTSTRING                  302
#define RIGHTSTRING                 303
#define MIDSTRING                   304
#define STRINGSTRING                305

/* relational operators defined */

#define ROP_EQ                      1         /* equals */
#define ROP_NEQ                     2        /* doesn't equal */
#define ROP_LT                      3         /* less than */
#define ROP_LTE                     4        /* less than or equals */
#define ROP_GT                      5         /* greater than */
#define ROP_GTE                     6        /* greater than or equals */

/* error codes (in BASIC script) defined */
#define ERR_CLEAR                   0
#define ERR_SYNTAX                  1
#define ERR_OUTOFMEMORY             2
#define ERR_IDTOOLONG               3
#define ERR_NOSUCHVARIABLE          4
#define ERR_BADSUBSCRIPT            5
#define ERR_TOOMANYDIMS             6
#define ERR_TOOMANYINITS            7
#define ERR_BADTYPE                 8
#define ERR_TOOMANYFORS             9
#define ERR_NONEXT                  10
#define ERR_NOFOR                   11
#define ERR_DIVIDEBYZERO            12
#define ERR_NEGLOG                  13
#define ERR_NEGSQRT                 14
#define ERR_BADSINCOS               15
#define ERR_EOF                     16
#define ERR_ILLEGALOFFSET           17
#define ERR_TYPEMISMATCH            18
#define ERR_INPUTTOOLONG            19
#define ERR_BADVALUE                20
#define ERR_NOTINT                  21

#define MAXFORS                     32    /* maximum number of nested fors */

struct editorSyntax {
    char **filematch;
    char **keywords;
    char singleline_comment_start[2];
    char multiline_comment_start[3];
    char multiline_comment_end[3];
    int flags;
};

/* This structure represents a single line of the file we are editing. */
typedef struct erow {
    int idx;                /* Row index in the file, zero-based. */
    int size;               /* Size of the row, excluding the null term. */
    char *chars;            /* Row content. */
} erow;

struct editorConfig {
    int cx,cy;              /* Cursor x and y position in characters */
    int rowoff;             /* Offset of row displayed. */
    int coloff;             /* Offset of column displayed. */
    int screenrows;         /* Number of rows that we can show */
    int screencols;         /* Number of cols that we can show */
    int numrows;            /* Number of rows */
    erow *row;              /* Rows */
    int dirty;              /* File modified but not saved. */
    char *filename;         /* Currently open filename */
    char statusmsg[80];
    uint32_t statusmsg_time;
    struct editorSyntax *syntax;    /* Current syntax highlight, or NULL. */
};

enum KEY_ACTION{
        KEY_NULL = 0,       /* NULL */
        CTRL_C = 3,         /* Ctrl-c */
        CTRL_D = 4,         /* Ctrl-d */
        CTRL_F = 6,         /* Ctrl-f */
        CTRL_H = 8,         /* Ctrl-h */
        TAB = 9,            /* Tab */
        CTRL_L = 12,        /* Ctrl+l */
        ENTER = 13,         /* Enter */
        CTRL_Q = 17,        /* Ctrl-q */
        CTRL_S = 19,        /* Ctrl-s */
        CTRL_U = 21,        /* Ctrl-u */
        ESC = 27,           /* Escape */
        BACKSPACE =  127,   /* Backspace */
        /* The following are just soft codes, not really reported by the
         * terminal directly.
        */
        ARROW_LEFT = 1000,
        ARROW_RIGHT,
        ARROW_UP,
        ARROW_DOWN,
        DEL_KEY,
        HOME_KEY,
        INSERT_KEY,
        END_KEY,
        PAGE_UP,
        PAGE_DOWN,
        F1_KEY,
        F2_KEY,
        F3_KEY,
        F4_KEY,
        F5_KEY,
        F6_KEY,
        F7_KEY,
        F8_KEY,
        F9_KEY,
        F10_KEY,
        F11_KEY,
        F12_KEY
};


typedef struct
{
  int no;                  /* line number */
  int eno;                 /* Editor line number */
  const char *str;         /* points to start of line */
}LINE;

typedef struct
{
  char id[32];             /* id of variable */
  double dval;             /* its value if a real */
  char *sval;              /* its value if a string (malloced) */
} VARIABLE;

typedef struct
{
  char id[32];             /* id of dimensioned variable */
  int type;                /* its type, STRID or FLTID */
  int ndims;               /* number of dimensions */
  int dim[5];              /* dimensions in x y order */
  char **str;              /* pointer to string data */
  double *dval;            /* pointer to real data */
} DIMVAR;

typedef struct            
{
  int type;                /* type of variable (STRID or FLTID or ERROR) */   
  char **sval;             /* pointer to string data */
  double *dval;            /* pointer to real data */
} LVALUE;

typedef struct
{
  char id[32];             /* id of control variable */
  int nextline;            /* line below FOR to which control passes */
  double toval;            /* terminal value */
  double step;             /* step size */
} FORLOOP;

// Prototypes.
uint32_t           sysmillis(void);
void               syswait(uint32_t);
int8_t             getKey(uint32_t);
int                editorReadKey(void);
int                getCursorPosition(uint32_t *, uint32_t *);
int                getWindowSize(int *, int *);
int                editorInsertRow(int, char *, size_t);
void               editorFreeRow(erow *);
void               editorCleanup(void);
void               editorDelRow(int);
int                editorRowInsertChar(erow *, int, int);
int                editorRowAppendString(erow *, char *, size_t);
int                editorRowDelChar(erow *, int);
void               editorInsertChar(int);
int                editorInsertNewline(void);
int                editorDelChar(void);
int                editorOpen(char *);
int                editorSave(char *);
void               editorList(int, int);
int                editorBuildScript(void);
void               abAppend(const char *, int, int);
int                editorRefreshScreen(void);
void               editorFind(void) ;
void               editorMoveCursor(int);
uint32_t           editorProcessKeypress(void);
int                editorFileWasModified(void);
int                editorAddBasicLine(int, char *, size_t);
uint32_t           initEditor(void);
int                execBasicScript(void);
void               cleanup(void);
void               reporterror(int);
int                findline(int);
int                line(int);
void               doprint(int);
void               dolet(int);
void               dodim(int);
int                doif(int);
int                dogoto(int);
int                dofor(int);
int                donext(int);
int                doinput(int);
void               dorem(int);
int                dopoke(int);
void               lvalue(LVALUE *);
int                boolexpr(void);
int                boolfactor(void);
int                relop(void);
double             expr(void);
double             term(void);
double             factor(void);
double             instr(void);
double             variable(void);
double             dimvariable(void);
VARIABLE          *findvariable(const char *);
DIMVAR            *finddimvar(const char *);
DIMVAR            *dimension(const char *, int , ...);
void              *getdimvar(DIMVAR *, ...);
VARIABLE          *addfloat(const char *);
VARIABLE          *addstring(const char *);
DIMVAR            *adddimvar(const char *);
char              *stringexpr(void);
char              *chrstring(void);
char              *strstring(void);
char              *leftstring(void);
char              *rightstring(void);
char              *midstring(void);
char              *stringstring(void);
char              *stringdimvar(void);
char              *stringvar(void);
char              *stringliteral(void);
int                integer(double);
void               match(int);
void               seterror(int);
int                getnextline(int);
int                gettoken(const char *);
int                tokenlen(const char *, int);
int                isstring(int);
double             getvalue(const char *, int *);
void               getid(const char *, char *, int *);
void               mystrgrablit(char *, const char *);
char              *mystrend(const char *, char);
int                mystrcount(const char *, char);
char              *mystrdup(const char *);
char              *mystrconcat(const char *, const char *);
double             factorial(double);
uint32_t           app(uint32_t, uint32_t);
void              *sys_malloc(size_t);            // Allocate memory managed by the OS.
void              *sys_realloc(void *, size_t);   // Reallocate a block of memory managed by the OS.
void              *sys_calloc(size_t, size_t);    // Allocate and zero a block of memory managed by the OS.
void               sys_free(void *);               // Free memory managed by the OS.


#ifdef __cplusplus
}
#endif
#endif // MBASIC_H
