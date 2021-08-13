/*****************************************************************
*                           Mini BASIC                           *
*                        by Malcolm McLean                       *
*                           version 1.0                          *
*****************************************************************/

#ifdef __cplusplus
    extern "C" {
#endif

#if defined(__K64F__)
  #include    <stdio.h>
  #include    <unistd.h>
  #include    <math.h>
  #include    <limits.h>
  #include    <stdlib.h>
  #include    <stdarg.h>
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

  #include    <math.h>
  #include    <stdlib.h>
  #include    <../../libraries/include/stdmisc.h>	    

  #define acos    acosf
  #define floor   floorf
  #define sin     sinf
  #define cos     cosf
  #define tan     tanf
  #define log     logf
  #define pow     powf
  #define sqrt    sqrtf
  #define asin    asinf
  #define atan    atanf
  #define fmod    fmodf
#else
  #error "Target CPU not defined, use __ZPU__ or __K64F__"
#endif

#include "interrupts.h"
#include "ff.h"            /* Declarations of FatFs API */
#include <string.h>
#include <readline.h>
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


FORLOOP forstack[MAXFORS]; /* stack for for loop conrol */
int nfors;                 /* number of fors on stack */

VARIABLE *variables;       /* the script's variables */
int nvariables;            /* number of variables */

DIMVAR *dimvariables;      /* dimensioned arrays */
int ndimvariables;         /* number of dimensioned arrays */

LINE *lines = NULL;        /* list of line starts */
int nlines = 0;            /* number of BASIC lines in program */

const char *string;        /* string we are parsing */
int token;                 /* current token (lookahead) */
int errorflag;             /* set when error in input encountered */


void cleanup(void);

void reporterror(int lineno);
int findline(int no);

int line(int);
void doprint(int);
void dolet(int);
void dodim(int);
int doif(int);
int dogoto(int);
int doinput(int);
void dorem(int);
int dofor(int);
int donext(int);

void lvalue(LVALUE *lv);

int boolexpr(void);
int boolfactor(void);
int relop(void);


double expr(void);
double term(void);
double factor(void);
double instr(void);
double variable(void);
double dimvariable(void);


VARIABLE *findvariable(const char *id);
DIMVAR *finddimvar(const char *id);
DIMVAR *dimension(const char *id, int ndims, ...);
void *getdimvar(DIMVAR *dv, ...);
VARIABLE *addfloat(const char *id);
VARIABLE *addstring(const char *id);
DIMVAR *adddimvar(const char *id);

char *stringexpr(void);
char *chrstring(void);
char *strstring(void);
char *leftstring(void);
char *rightstring(void);
char *midstring(void);
char *stringstring(void);
char *stringdimvar(void);
char *stringvar(void);
char *stringliteral(void);

int integer(double x);

void match(int tok);
void seterror(int errorcode);
int getnextline(int);
int gettoken(const char *str);
int tokenlen(const char *str, int token);

int isstring(int token);
double getvalue(const char *str, int *len);
void getid(const char *str, char *out, int *len);

void mystrgrablit(char *dest, const char *src);
char *mystrend(const char *str, char quote);
int mystrcount(const char *str, char ch);
char *mystrdup(const char *str);
char *mystrconcat(const char *str, const char *cat);
double factorial(double x);

// Method to run the basic script, the lines array is prebuilt.
// Returns: 0 on success, 1 on error condition.
//
int execBasicScript(void)
{
    int    curline = 0;
    int    nextline;
    int    answer = 0;
    int8_t keyIn;

    // Sanity check, no script no run!
    //
    if(nlines == 0)
        return(0);


    // Initialise variables.
    nvariables = 0;
    variables = 0;
    dimvariables = 0;
    ndimvariables = 0;

    while(curline != -1)
    {
        string = lines[curline].str;
        token = gettoken(string);
        errorflag = 0;
        nextline = line(curline);
        if(errorflag)
        {
            reporterror(lines[curline].no);
            answer = 1;
            break;
        }

      //if(nextline == -1)
      //    break;

      #if defined __SHARPMZ__
        // Get key from system, request ansi key sequence + non-blocking (=2).
        keyIn = getKey(2);
      #else
        // Get key from system, request non-blocking (=0).
        keyIn = getKey(0);
      #endif

        // Check to see if user is requesting an action.
        if(nextline == -1 || keyIn == CTRL_C)
        {
            printf("\nExecution stopped, user request.\n");
            break;
        }

        if(nextline == 0)
        {
            curline++;
            if(curline == nlines)
              break;
        }
        else
        {
            curline = findline(nextline);
            if(curline == -1)
            {
                printf("line %d not found\n", nextline);
                answer = 1;
                break;
            }
        }
    }

    cleanup();
    return answer;
}

/*
  frees all the memory we have allocated
*/
void cleanup(void)
{
  int i;
  int ii;
  int size;

  for(i=0;i<nvariables;i++)
    if(variables[i].sval)
      sys_free(variables[i].sval);
  if(variables)
      sys_free(variables);
  variables = 0;
  nvariables = 0;

  for(i=0;i<ndimvariables;i++)
  {
    if(dimvariables[i].type == STRID)
    {
      if(dimvariables[i].str)
      {
        size = 1;
        for(ii=0;ii<dimvariables[i].ndims;ii++)
          size *= dimvariables[i].dim[ii];
        for(ii=0;ii<size;ii++)
          if(dimvariables[i].str[ii])
            sys_free(dimvariables[i].str[ii]);
        sys_free(dimvariables[i].str);
      }
    }
    else
      if(dimvariables[i].dval)
        sys_free(dimvariables[i].dval);
  }

  if(dimvariables)
    sys_free(dimvariables);
 
  dimvariables = 0;
  ndimvariables = 0;

  if(lines)
    sys_free(lines);

  lines = 0;
  nlines = 0;
}

/*
  error report function.
  for reporting errors in the user's script.
  checks the global errorflag.
  Params: lineno - the line on which the error occurred
*/
void reporterror(int lineno)
{
  switch(errorflag)
  {
    case ERR_CLEAR:
      assert(0);
      break;
    case ERR_SYNTAX:
      printf("Syntax error line %d\n", lineno);
      break;
    case ERR_OUTOFMEMORY:
      printf("Out of memory line %d\n", lineno);
      break;
    case ERR_IDTOOLONG:
      printf("Identifier too long line %d\n", lineno);
      break;
    case ERR_NOSUCHVARIABLE:
      printf("No such variable line %d\n", lineno);
      break;
    case ERR_BADSUBSCRIPT:
      printf("Bad subscript line %d\n", lineno);
      break;
    case ERR_TOOMANYDIMS:
      printf("Too many dimensions line %d\n", lineno);
      break;
    case ERR_TOOMANYINITS:
      printf("Too many initialisers line %d\n", lineno);
      break;
    case ERR_BADTYPE:
      printf("Illegal type line %d\n", lineno);
      break;
    case ERR_TOOMANYFORS:
      printf("Too many nested fors line %d\n", lineno);
      break;
    case ERR_NONEXT:
      printf("For without matching next line %d\n", lineno);
      break;
    case ERR_NOFOR:
      printf("Next without matching for line %d\n", lineno);
      break;
    case ERR_DIVIDEBYZERO:
      printf("Divide by zero lne %d\n", lineno);
      break;
    case ERR_NEGLOG:
      printf("Negative logarithm line %d\n", lineno);
      break;
    case ERR_NEGSQRT:
      printf("Negative square root line %d\n", lineno);
      break;
    case ERR_BADSINCOS:
      printf("Sine or cosine out of range line %d\n", lineno);
      break;
    case ERR_EOF:
      printf("End of input file %d\n", lineno);
      break;
    case ERR_ILLEGALOFFSET:
      printf("Illegal offset line %d\n", lineno);
      break;
    case ERR_TYPEMISMATCH:
      printf("Type mismatch line %d\n", lineno);
      break;
    case ERR_INPUTTOOLONG:
      printf("Input too long line %d\n", lineno);
      break;
    case ERR_BADVALUE:
      printf("Bad value at line %d\n", lineno);
      break;
    case ERR_NOTINT:
      printf("Not an integer at line %d\n", lineno);
      break;
    default:
      printf("ERROR line %d\n", lineno);
      break;
  }
}

/*
  binary search for a line
  Params: no - line number to find
  Returns: index of the line, or -1 on fail.
*/
int findline(int no)
{
  int high;
  int low;
  int mid;

  low = 0;
  high = nlines-1;
  while(high > low + 1)
  {
    mid = (high + low)/2;
    if(lines[mid].no == no)
      return mid;
    if(lines[mid].no > no)
      high = mid;
    else
      low = mid;
  }

  if(lines[low].no == no)
    mid = low;
  else if(lines[high].no == no)
    mid = high;
  else
    mid = -1;

  return mid;
}

/*
  Parse a line. High level parse function
*/
int line(int curline)
{
  int answer = 0;
  const char *str;

  match(VALUE);

  switch(token)
  {
    case PRINT:
      doprint(curline);
      break;
    case LET:
      dolet(curline);
      break;
    case DIM:
      dodim(curline);
      break;
    case IF:
      answer = doif(curline);
      break;
    case GOTO:
      answer = dogoto(curline);
      break;
    case BINPUT:
      answer = doinput(curline);
      break;
    case REM:
      dorem(curline);
      return 0;
      break;
    case FOR:
      answer = dofor(curline);
      break;
    case NEXT:
      answer = donext(curline);
      break;

    // Poke a value into a memory location.
    //
    case POKE:
      answer = dopoke(curline);
      break;
      
    default:
      seterror(ERR_SYNTAX);
      break;
  }

  if(token != EOS)
  {
    /*match(VALUE);*/
    /* check for a newline */
    str = string;
    while(isspace(*str))
    {
      if(*str == '\n')
        break;
      str++;
    }

    if(*str != '\n')
      seterror(ERR_SYNTAX);
  }

  return answer;
}

/*
  the PRINT statement
*/
void doprint(int curline)
{
  char *str;
  double x;
  char output[20];

  match(PRINT);

  while(1)
  {
    if(isstring(token))
    {
      str = stringexpr();
      if(str)
      {
        printf("%s", str);
        sys_free(str);
      }
    }
    else
    {
      x = expr();
      sprintf(output, "%g", x);
      //printf("%g", x);
      fputs(output, stdout);
    }
    if(token == COMMA)
    {
      printf(" ");
      match(COMMA);
    }
    else
      break;
  }
  if(token == SEMICOLON)
  {
    match(SEMICOLON);
  }
  else
    printf("\r\n");
}

/*
  the LET statement
*/
void dolet(int curline)
{
  LVALUE lv;
  char *temp;

  match(LET);
  lvalue(&lv);
  match(EQUALS);
  switch(lv.type)
  {
    case FLTID:
      *lv.dval = expr();
      break;
    case STRID:
      temp = *lv.sval;
      *lv.sval = stringexpr();
      if(temp)
        sys_free(temp);
      break;
    default:
      break;
  }
}

/*
  the DIM statement
*/
void dodim(int curline)
{
  int ndims = 0;
  double dims[6];
  char name[32];
  int len;
  DIMVAR *dimvar;
  int i;
  int size = 1;

  match(DIM);

  switch(token)
  {
    case DIMFLTID:
    case DIMSTRID:
      getid(string, name, &len);
      match(token);
      dims[ndims++] = expr();
      while(token == COMMA)
      {
        match(COMMA);
        dims[ndims++] = expr();
        if(ndims > 5)
        {
          seterror(ERR_TOOMANYDIMS);
          return;
        }
      } 

      match(CPAREN);
      
      for(i=0;i<ndims;i++)
      {
        if(dims[i] < 0 || dims[i] != (int) dims[i])
        {
          seterror(ERR_BADSUBSCRIPT);
          return;
        }
      }
      switch(ndims)
      {
        case 1:
          dimvar = dimension(name, 1, (int) dims[0]);
          break;
        case 2:
          dimvar = dimension(name, 2, (int) dims[0], (int) dims[1]);
          break;
        case 3:
          dimvar = dimension(name, 3, (int) dims[0], (int) dims[1], (int) dims[2]);
          break;
        case 4:
          dimvar = dimension(name, 4, (int) dims[0], (int) dims[1], (int) dims[2], (int) dims[3]);
          break;
        case 5:
          dimvar = dimension(name, 5, (int) dims[0], (int) dims[1], (int) dims[2], (int) dims[3], (int) dims[4]);
          break;
      }
      break;
    default:
        seterror(ERR_SYNTAX);
        return;
  }
  if(dimvar == 0)
  {
    /* out of memory */
    seterror(ERR_OUTOFMEMORY);
    return;
  }


  if(token == EQUALS)
  {
    match(EQUALS);

    for(i=0;i<dimvar->ndims;i++)
      size *= dimvar->dim[i];

    switch(dimvar->type)
    {
      case FLTID:
        i = 0;
        dimvar->dval[i++] = expr();
        while(token == COMMA && i < size)
        {
          match(COMMA);
          dimvar->dval[i++] = expr();
          if(errorflag)
            break;
        }
        break;
      case STRID:
        i = 0;
        if(dimvar->str[i])
          sys_free(dimvar->str[i]);
        dimvar->str[i++] = stringexpr();

        while(token == COMMA && i < size)
        {
          match(COMMA);
          if(dimvar->str[i])
            sys_free(dimvar->str[i]);
          dimvar->str[i++] = stringexpr();
          if(errorflag)
            break;
        }
        break;
    }
    
    if(token == COMMA)
      seterror(ERR_TOOMANYINITS);
  }

}

/*
  the IF statement.
  if jump taken, returns new line no, else returns 0
*/
int doif(int curline)
{
  int condition;
  int jump;

  match(IF);
  condition = boolexpr();
  match(THEN);
  jump = integer( expr() );
  if(condition)
    return jump;
  else
    return 0;
}

/*
  the GOTO satement
  returns new line number
*/
int dogoto(int curline)
{
  match(GOTO);
  return integer( expr() );
}

/*
  The FOR statement.

  Pushes the for stack.
  Returns line to jump to, or -1 to end program

*/
int dofor(int curline)
{
  LVALUE lv;
  char        id[32];
  char        nextid[32];
  int         len;
  int         idx;
  double      initval;
  double      toval;
  double      stepval;
  const char  *savestring;
  int         answer;

  match(FOR);
  getid(string, id, &len);

  lvalue(&lv);
  if(lv.type != FLTID)
  {
    seterror(ERR_BADTYPE);
    return -1;
  }
  match(EQUALS);
  initval = expr();
  match(TO);
  toval = expr();
  if(token == STEP)
  {
    match(STEP);
    stepval = expr();
  }
  else
  {
    stepval = 1.0;
  }

  *lv.dval = initval;

  if(nfors > MAXFORS - 1)
  {
    seterror(ERR_TOOMANYFORS);
    return -1;
  }

  if((stepval < 0 && initval < toval) || (stepval > 0 && initval > toval))
  {
    savestring = string;
    for(idx=curline+1; idx < nlines; idx++)
    {
        string = lines[idx].str;
        errorflag = 0;
        token = gettoken(string);
        match(VALUE);
        if(token == NEXT)
        {
            match(NEXT);
            if(token == FLTID || token == DIMFLTID)
            {
                getid(string, nextid, &len);
                if(!strcmp(id, nextid))
                {
                    answer = getnextline(curline);
                    string = savestring;
                    token = gettoken(string);
                    return answer ? answer : -1;
                }
            }
        }
    }

    seterror(ERR_NONEXT);
    return -1;
  }
  else
  {
      strcpy(forstack[nfors].id, id);
      forstack[nfors].nextline = getnextline(curline);
      forstack[nfors].step = stepval;
      forstack[nfors].toval = toval;
      nfors++;
      return 0;
  }
}

/*
  the NEXT statement
  updates the counting index, and returns line to jump to
*/
int donext(int curline)
{
  char id[32];
  int len;
  LVALUE lv;

  match(NEXT);

  if(nfors)
  {
    getid(string, id, &len);
    lvalue(&lv);
    if(lv.type != FLTID)
    {
      seterror(ERR_BADTYPE);
      return -1;
    }
    *lv.dval += forstack[nfors-1].step;
    if( (forstack[nfors-1].step < 0 && *lv.dval < forstack[nfors-1].toval) ||
        (forstack[nfors-1].step > 0 && *lv.dval > forstack[nfors-1].toval) )
    {
      nfors--;
      return 0;
    }
    else
    {
      return forstack[nfors-1].nextline;
    }
  }
  else
  {
    seterror(ERR_NOFOR);
    return -1;
  }
}


/*
  the INPUT statement
*/
int doinput(int curline)
{
    LVALUE lv;
    char buff[1024];
    char *end;

    match(BINPUT);
    lvalue(&lv);

    switch(lv.type)
    {
        case FLTID:
            do {
                readline((uint8_t *)buff, sizeof(buff), 0, NULL, NULL);
                if(buff[0] == CTRL_C)
                {
                    return(-1);
                }
                *lv.dval = strtod(buff, &end);
            } while(end == buff);
            break;

        case STRID:
            if(*lv.sval)
            {
                sys_free(*lv.sval);
                *lv.sval = 0;
            }

            // Readline will return entered data, null terminated, upto endof limit given.
            readline((uint8_t *)buff, sizeof(buff), 0, NULL, NULL);
            if(buff[0] == CTRL_C)
            {
                return(-1);
            }

            *lv.sval = mystrdup(buff);
            if(!*lv.sval)
            {
                seterror(ERR_OUTOFMEMORY);
                return(-1);
            }
            break;

        default:
            return(0);
    }
    return(0);
}

/*
  the REM statement.
  Note is unique as the rest of the line is not parsed

*/
void dorem(int curline)
{
  match(REM);
  return;
}

// The POKE statement. Format is POKE WIDTH, ADDR, DATA
// Takes a value and *pokes* (writes) it into a memory location.
//
int dopoke(int curline)
{
    uint32_t   addr;
    uint32_t   data;
    uint32_t   width;

    match(POKE);
    width = integer( expr() );
    match(COMMA);
    addr = integer( expr() );
    match(COMMA);
    data = integer( expr() );
    switch(width)
    {
        case 8:
            *(uint8_t *)(addr) = data;
            break;
        case 16:
            *(uint16_t *)(addr) = data;
            break;
        case 32:
            *(uint32_t *)(addr) = data;
            break;

        default:
            seterror( ERR_BADVALUE );
            break;
    }
    return 0;
}

/*
  Get an lvalue from the environment
  Params: lv - structure to fill.
  Notes: missing variables (but not out of range subscripts)
         are added to the variable list.
*/
void lvalue(LVALUE *lv)
{
  char name[32];
  int len;
  VARIABLE *var;
  DIMVAR *dimvar;
  int index[5];
  void *valptr = 0;
  int type;
  
  lv->type = ERROR;
  lv->dval = 0;
  lv->sval = 0;

  switch(token)
  {
    case FLTID:
      getid(string, name, &len);
      match(FLTID);
      var = findvariable(name);
      if(!var)
        var = addfloat(name);
      if(!var)
      {
        seterror(ERR_OUTOFMEMORY);
        return;
      }
      lv->type = FLTID;
      lv->dval = &var->dval;
      lv->sval = 0;
      break;
    case STRID:
      getid(string, name, &len);
      match(STRID);
      var = findvariable(name);
      if(!var)
        var = addstring(name);
      if(!var)
      {
        seterror(ERR_OUTOFMEMORY);
        return;
      }
      lv->type = STRID;
      lv->sval = &var->sval;
      lv->dval = 0;
      break;
    case DIMFLTID:
    case DIMSTRID:
      type = (token == DIMFLTID) ? FLTID : STRID;
      getid(string, name, &len);
      match(token);
      dimvar = finddimvar(name);
      if(dimvar)
      {
        switch(dimvar->ndims)
        {
          case 1:
            index[0] = integer( expr() );
            if(errorflag == 0)
              valptr = getdimvar(dimvar, index[0]);
            break;
          case 2:
            index[0] = integer( expr() );
            match(COMMA);
            index[1] = integer( expr() );
            if(errorflag == 0)
              valptr = getdimvar(dimvar, index[0], index[1]);
            break;
          case 3:
            index[0] = integer( expr() );
            match(COMMA);
            index[1] = integer( expr() );
            match(COMMA);
            index[2] = integer( expr() );
            if(errorflag == 0)
              valptr = getdimvar(dimvar, index[0], index[1], index[2]);
            break;
          case 4:
            index[0] = integer( expr() );
            match(COMMA);
            index[1] = integer( expr() );
            match(COMMA);
            index[2] = integer( expr() );
            match(COMMA);
            index[3] = integer( expr() );
            if(errorflag == 0)
              valptr = getdimvar(dimvar, index[0], index[1], index[2], index[3]);
            break;
          case 5:
            index[0] = integer( expr() );
            match(COMMA);
            index[1] = integer( expr() );
            match(COMMA);
            index[2] = integer( expr() );
            match(COMMA);
            index[3] = integer( expr() );
            match(COMMA);
            index[4] = integer( expr() );
            if(errorflag == 0)
              valptr = getdimvar(dimvar, index[0], index[1], index[2], index[3]);
            break;
        }
        match(CPAREN);
      }
      else
      {
        seterror(ERR_NOSUCHVARIABLE);
        return;
      }
      if(valptr)
      {
        lv->type = type;
        if(type == FLTID)
          lv->dval = valptr;
        else if(type == STRID)
          lv->sval = valptr;
        else
          assert(0);
      }
      break;
    default:
      seterror(ERR_SYNTAX);
  }
}

/*
  parse a boolean expression
  consists of expressions or strings and relational operators,
  and parentheses
*/
int boolexpr(void)
{
  int left;
  int right;
  
  left = boolfactor();

  while(1)
  {
    switch(token)
    {
      case AND:
        match(AND);
        right = boolexpr();
        return (left && right) ? 1 : 0;
      case OR:
        match(OR);
        right = boolexpr();
        return (left || right) ? 1 : 0;
      default:
        return left;
    }
  }
}

/*
  boolean factor, consists of expression relop expression
    or string relop string, or ( boolexpr() )
*/
int boolfactor(void)
{
  int answer;
  double left;
  double right;
  int op;
  char *strleft;
  char *strright;
  int cmp;

  switch(token)
  {
    case OPAREN:
      match(OPAREN);
      answer = boolexpr();
      match(CPAREN);
      break;
    default:
      if(isstring(token))
      {
        strleft = stringexpr();
        op = relop();
        strright = stringexpr();
        if(!strleft || !strright)
        {
          if(strleft)
            sys_free(strleft);
          if(strright)
            sys_free(strright);
          return 0;
        }
        cmp = strcmp(strleft, strright);
        switch(op)
        {
          case ROP_EQ:
              answer = cmp == 0 ? 1 : 0;
              break;
          case ROP_NEQ:
              answer = cmp == 0 ? 0 : 1;
              break;
          case ROP_LT:
              answer = cmp < 0 ? 1 : 0;
              break;
          case ROP_LTE:
              answer = cmp <= 0 ? 1 : 0;
              break;
          case ROP_GT:
              answer = cmp > 0 ? 1 : 0;
              break;
          case ROP_GTE:
              answer = cmp >= 0 ? 1 : 0;
              break;
          default:
            answer = 0;
        }
        sys_free(strleft);
        sys_free(strright);
      }
      else
      {
        left = expr();
        op = relop();
        right = expr();
        switch(op)
        {
          case ROP_EQ:
              answer = (left == right) ? 1 : 0;
              break;
          case ROP_NEQ:
              answer = (left != right) ? 1 : 0;
              break;
          case ROP_LT:
              answer = (left < right) ? 1 : 0;
              break;
          case ROP_LTE:
              answer = (left <= right) ? 1 : 0;
              break;
          case ROP_GT:
              answer = (left > right) ? 1 : 0;
              break;
          case ROP_GTE:
              answer = (left >= right) ? 1 : 0;
              break;
          default:
             errorflag = 1;
             return 0;

        }
      }
      
  }

  return answer;
}

/*
  get a relational operator
  returns operator parsed or ERROR
*/
int relop(void)
{
  switch(token)
  {
    case EQUALS:
      match(EQUALS);
      return ROP_EQ;
    case GREATER:
      match(GREATER);
      if(token == EQUALS)
      {
        match(EQUALS);
        return ROP_GTE;
      }
      return ROP_GT; 
    case LESS:
      match(LESS);
      if(token == EQUALS)
      {
        match(EQUALS);
        return ROP_LTE;
      }
      else if(token == GREATER)
      {
        match(GREATER);
        return ROP_NEQ;
      }
      return ROP_LT;
    default:
      seterror(ERR_SYNTAX);
      return ERROR;
  }
}

/*
  parses an expression
*/
double expr(void)
{
  double left;
  double right;

  left = term();

  while(1)
  {
    switch(token)
    {
    case PLUS:
      match(PLUS);
      right = term();
      left += right;
      break;
    case MINUS:
      match(MINUS);
      right = term();
      left -= right;
      break;
    default:
      return left;
    }
  }
}

/*
  parses a term 
*/
double term(void)
{
  double left;
  double right;

  left = factor();
  
  while(1)
  {
    switch(token)
    {
    case MULT:
      match(MULT);
      right = factor();
      left *= right;
      break;
    case DIV:
      match(DIV);
      right = factor();
      if(right != 0.0)
        left /= right;
      else
        seterror(ERR_DIVIDEBYZERO);
      break;
    case MOD:
      match(MOD);
      right = factor();
      left = fmod(left, right);
      break;
    default:
      return left;
    }
  }

}

/*
  parses a factor
*/
double factor(void)
{
    double   answer = 0;
    char     *str;
    char     *end;
    int      len;
    uint32_t addr;
    uint32_t width;
  
    switch(token)
    {
      case OPAREN:
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        break;
      case VALUE:
        answer = getvalue(string, &len);
        match(VALUE);
        break;
      case MINUS:
        match(MINUS);
        answer = -factor();
        break;
      case FLTID:
        answer = variable();
        break;
      case DIMFLTID:
        answer = dimvariable();
        break;
      case BE:
        answer = exp(1.0);
        match(BE);
        break;
      case BPI:
        answer = acos(0.0) * 2.0;
        match(BPI);
        break;
      case SIN:
        match(SIN);
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        answer = sin(answer);
        break;
      case COS:
        match(COS);
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        answer = cos(answer);
        break;
      case TAN:
        match(TAN);
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        answer = tan(answer);
        break;
      case LN:
        match(LN);
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        if(answer > 0)
          answer = log(answer);
        else
          seterror(ERR_NEGLOG);
        break;
      case POW:
        match(POW);
        match(OPAREN);
        answer = expr();
        match(COMMA);
        answer = pow(answer, expr());
        match(CPAREN);
        break;
      case SQRT:
        match(SQRT);
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        if(answer >= 0.0)
          answer = sqrt(answer);
        else
          seterror(ERR_NEGSQRT);
        break;
      case ABS:
        match(ABS);
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        answer = fabs(answer);
        break;
      case LEN:
        match(LEN);
        match(OPAREN);
        str = stringexpr();
        match(CPAREN);
        if(str)
        {
          answer = strlen(str);
          sys_free(str);
        }
        else
          answer = 0;
        break;
      case ASCII:
        match(ASCII);
        match(OPAREN);
        str = stringexpr();
        match(CPAREN);
        if(str)
        {
          answer = *str;
          sys_free(str);
        }
        else
          answer = 0;
        break;
      case ASIN:
        match(ASIN);
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        if(answer >= -1 && answer <= 1)
          answer = asin(answer);
        else
          seterror(ERR_BADSINCOS);
        break;
      case ACOS:
        match(ACOS);
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        if(answer >= -1 && answer <= 1)
          answer = acos(answer);
        else
          seterror(ERR_BADSINCOS);
        break;
      case ATAN:
        match(ATAN);
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        answer = atan(answer);
        break;
      case INT:
        match(INT);
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        answer = floor(answer);
        break;
      case RND:
        match(RND);
        match(OPAREN);
        answer = expr();
        match(CPAREN);
        answer = integer(answer);
        if(answer > 1)
          answer = floor(rand()/(RAND_MAX + 1.0) * answer);
        else if(answer == 1)
          answer = rand()/(RAND_MAX + 1.0);
        else
        {
          if(answer < 0)
            srand( (unsigned) -answer);
          answer = 0;
        }
        break;
      case VAL:
        match(VAL);
        match(OPAREN);
        str = stringexpr();
        match(CPAREN);
        if(str)
        {
          answer = strtod(str, 0);
          sys_free(str);
        }
        else
          answer = 0;
        break;
      case VALLEN:
        match(VALLEN);
        match(OPAREN);
        str = stringexpr();
        match(CPAREN);
        if(str)
        {
          strtod(str, &end);
          answer = end - str;
          sys_free(str);
        }
        else
          answer = 0.0;
        break;
      case INSTR:
        answer = instr();
        break;
  
      // Peek a given location and return value. Format: PEEK(width, addr)
      //
      case PEEK:
        match(PEEK);
        match(OPAREN);
        width = integer( expr() );
        match(COMMA);
        addr = integer( expr() );
        match(CPAREN);
        switch(width)
        {
            case 8:
                answer = *(uint8_t *)(addr);
                break;
            case 16:
                answer = *(uint16_t *)(addr);
                break;
            case 32:
                answer = *(uint32_t *)(addr);
                break;

            default:
                seterror( ERR_BADVALUE );
                break;
        }
        break;
       
      default:
        if(isstring(token))
          seterror(ERR_TYPEMISMATCH);
        else
          seterror(ERR_SYNTAX);
        break;
    }
  
    while(token == SHRIEK)
    {
      match(SHRIEK);
      answer = factorial(answer);
    }
  
    return answer;
}

/*
  calcualte the INSTR() function.
*/
double instr(void)
{
  char *str;
  char *substr;
  char *end;
  double answer = 0;
  int offset;

  match(INSTR);
  match(OPAREN);
  str = stringexpr();
  match(COMMA);
  substr = stringexpr();
  match(COMMA);
  offset = integer( expr() );
  offset--;
  match(CPAREN);

  if(!str || ! substr)
  {
    if(str)
      sys_free(str);
    if(substr)
      sys_free(substr);
    return 0;
  }

  if(offset >= 0 && offset < (int) strlen(str))
  {
    end = strstr(str + offset, substr);
    if(end)
      answer = end - str + 1.0;
  }

  sys_free(str);
  sys_free(substr);

  return answer;
}
/*
  get the value of a scalar variable from string
  matches FLTID
*/
double variable(void)
{
  VARIABLE *var;
  char id[32];
  int len;

  getid(string, id, &len);
  match(FLTID);
  var = findvariable(id);
  if(var)
    return var->dval;
  else
  {
    seterror(ERR_NOSUCHVARIABLE);
    return 0.0;
  }
}

/*
  get value of a dimensioned variable from string.
  matches DIMFLTID
*/
double dimvariable(void)
{
  DIMVAR *dimvar;
  char id[32];
  int len;
  int index[5];
  double *answer;

  answer = NULL;
  getid(string, id, &len);
  match(DIMFLTID);
  dimvar = finddimvar(id);
  if(!dimvar)
  {
    seterror(ERR_NOSUCHVARIABLE);
    return 0.0;
  }

  if(dimvar)
  {
    switch(dimvar->ndims)
    {
      case 1:
        index[0] = integer( expr() );
        answer = getdimvar(dimvar, index[0]);
        break;
      case 2:
        index[0] = integer( expr() );
        match(COMMA);
        index[1] = integer( expr() );
        answer = getdimvar(dimvar, index[0], index[1]);
        break;
      case 3:
        index[0] = integer( expr() );
        match(COMMA);
        index[1] = integer( expr() );
        match(COMMA);
        index[2] = integer( expr() );
        answer = getdimvar(dimvar, index[0], index[1], index[2]);
        break;
      case 4:
        index[0] = integer( expr() );
        match(COMMA);
        index[1] = integer( expr() );
        match(COMMA);
        index[2] = integer( expr() );
        match(COMMA);
        index[3] = integer( expr() );
        answer = getdimvar(dimvar, index[0], index[1], index[2], index[3]);
        break;
      case 5:
        index[0] = integer( expr() );
        match(COMMA);
        index[1] = integer( expr() );
        match(COMMA);
        index[2] = integer( expr() );
        match(COMMA);
        index[3] = integer( expr() );
        match(COMMA);
        index[4] = integer( expr() );
        answer = getdimvar(dimvar, index[0], index[1], index[2], index[3], index[4]);
        break;

    }

    match(CPAREN);
  }

  if(answer)
    return *answer;

  return 0.0;

}

/*
  find a scalar variable invariables list
  Params: id - id to get
  Returns: pointer to that entry, 0 on fail
*/
VARIABLE *findvariable(const char *id)
{
  int i;

  for(i=0;i<nvariables;i++)
    if(!strcmp(variables[i].id, id))
      return &variables[i];
  return 0;
}

/*
  get a dimensioned array by name
  Params: id (includes opening parenthesis)
  Returns: pointer to array entry or 0 on fail
*/
DIMVAR *finddimvar(const char *id)
{
  int i;

  for(i=0;i<ndimvariables;i++)
    if(!strcmp(dimvariables[i].id, id))
      return &dimvariables[i];
  return 0;
}

/*
  dimension an array.
  Params: id - the id of the array (include leading ()
          ndims - number of dimension (1-5)
          ... - integers giving dimension size, 
*/
DIMVAR *dimension(const char *id, int ndims, ...)
{
  DIMVAR *dv;
  va_list vargs;
  int size = 1;
  int oldsize = 1;
  int i;
  int dimensions[5];
  double *dtemp;
  char **stemp;

  assert(ndims <= 5);
  if(ndims > 5)
    return 0;

  dv = finddimvar(id);
  if(!dv)
    dv = adddimvar(id);
  if(!dv)
  {
    seterror(ERR_OUTOFMEMORY);
    return 0;
  }

  if(dv->ndims)
  {
    for(i=0;i<dv->ndims;i++)
      oldsize *= dv->dim[i];
  }
  else
    oldsize = 0;

  va_start(vargs, ndims);
  for(i=0;i<ndims;i++)
  {
    dimensions[i] = va_arg(vargs, int);
    size *= dimensions[i];
  }
  va_end(vargs);

  switch(dv->type)
  {
    case FLTID:
      dtemp = sys_realloc(dv->dval, size * sizeof(double));
      if(dtemp)
        dv->dval = dtemp;
      else
      {
        seterror(ERR_OUTOFMEMORY);
        return 0;
      }
      break;
    case STRID:
      if(dv->str)
      {
        for(i=size;i<oldsize;i++)
          if(dv->str[i])
          {
            sys_free(dv->str[i]);
            dv->str[i] = 0;
          }
      }
      stemp = sys_realloc(dv->str, size * sizeof(char *));
      if(stemp)
      {
        dv->str = stemp;
        for(i=oldsize;i<size;i++)
          dv->str[i] = 0;
      }
      else
      {
        for(i=0;i<oldsize;i++)
          if(dv->str[i])
          {
            sys_free(dv->str[i]);
            dv->str[i] = 0;
          }
        seterror(ERR_OUTOFMEMORY);
        return 0;
      }
      break;
    default:
      assert(0);
  }

  for(i=0;i<5;i++)
    dv->dim[i] = dimensions[i];
  dv->ndims = ndims;

  return dv;
}

/*
  get the address of a dimensioned array element.
  works for both string and real arrays.
  Params: dv - the array's entry in variable list
          ... - integers telling which array element to get
  Returns: the address of that element, 0 on fail
*/ 
void *getdimvar(DIMVAR *dv, ...)
{
  va_list vargs;
  int index[5];
  int i;
  void *answer = 0;

  va_start(vargs, dv);
  for(i=0;i<dv->ndims;i++)
  {
    index[i] = va_arg(vargs, int);
    index[i]--;
  }
  va_end(vargs);

  for(i=0;i<dv->ndims;i++)
    if(index[i] >= dv->dim[i] || index[i] < 0)
    {
      seterror(ERR_BADSUBSCRIPT);
      return 0;
    }

  if(dv->type == FLTID)
  {
    switch(dv->ndims)
    {
      case 1:
          answer = &dv->dval[ index[0] ]; 
        break;
      case 2:
        answer = &dv->dval[ index[1] * dv->dim[0] 
            + index[0] ];
        break;
      case 3:
        answer = &dv->dval[ index[2] * (dv->dim[0] * dv->dim[1]) 
            + index[1] * dv->dim[0] 
            + index[0] ];
        break;
      case 4:
        answer = &dv->dval[ index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2]) 
            + index[2] * (dv->dim[0] * dv->dim[1]) 
            + index[1] * dv->dim[0] 
            + index[0] ];
      case 5:
        answer = &dv->dval[ index[4] * (dv->dim[0] + dv->dim[1] + dv->dim[2] + dv->dim[3])
          + index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2])
          + index[2] * (dv->dim[0] + dv->dim[1])
          + index[1] * dv->dim[0]
          + index[0] ];
      break;
    }
  }
  else if(dv->type == STRID)
  {
    switch(dv->ndims)
    {
      case 1:
          answer = &dv->str[ index[0] ]; 
        break;
      case 2:
        answer = &dv->str[ index[1] * dv->dim[0] 
            + index[0] ];
        break;
      case 3:
        answer = &dv->str[ index[2] * (dv->dim[0] * dv->dim[1]) 
            + index[1] * dv->dim[0] 
            + index[0] ];
        break;
      case 4:
        answer = &dv->str[ index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2]) 
            + index[2] * (dv->dim[0] * dv->dim[1]) 
            + index[1] * dv->dim[0] 
            + index[0] ];
      case 5:
        answer = &dv->str[ index[4] * (dv->dim[0] + dv->dim[1] + dv->dim[2] + dv->dim[3])
          + index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2])
          + index[2] * (dv->dim[0] + dv->dim[1])
          + index[1] * dv->dim[0]
          + index[0] ];
      break;
    }
  }

  return answer;
}

/*
  add a real varaible to our variable list
  Params: id - id of varaible to add.
  Returns: pointer to new entry in table
*/
VARIABLE *addfloat(const char *id)
{
   VARIABLE *vars;

  vars = sys_realloc(variables, (nvariables + 1) * sizeof(VARIABLE));
  if(vars)
  {
    variables = vars;
    strcpy(variables[nvariables].id, id);
    variables[nvariables].dval = 0;
    variables[nvariables].sval = 0;
    nvariables++;
    return &variables[nvariables-1];
  }
  else
    seterror(ERR_OUTOFMEMORY);

  return 0; 
}

/*
  add a string variable to table.
  Params: id - id of variable to get (including trailing $)
  Retruns: pointer to new entry in table, 0 on fail.       
*/
VARIABLE *addstring(const char *id)
{
  VARIABLE *vars;

  vars = sys_realloc(variables, (nvariables + 1) * sizeof(VARIABLE));
  if(vars)
  {
    variables = vars;
    strcpy(variables[nvariables].id, id);
    variables[nvariables].sval = 0;
    variables[nvariables].dval = 0;
    nvariables++;
    return &variables[nvariables-1];
  }
  else
    seterror(ERR_OUTOFMEMORY);

  return 0;
}

/*
  add a new array to our symbol table.
  Params: id - id of array (include leading ()
  Returns: pointer to new entry, 0 on fail.
*/
DIMVAR *adddimvar(const char *id)
{
  DIMVAR *vars;

  vars = sys_realloc(dimvariables, (ndimvariables + 1) * sizeof(DIMVAR));
  if(vars)
  {
    dimvariables = vars;
    strcpy(dimvariables[ndimvariables].id, id);
    dimvariables[ndimvariables].dval = 0;
    dimvariables[ndimvariables].str = 0;
    dimvariables[ndimvariables].ndims = 0;
    dimvariables[ndimvariables].type = strchr(id, '$') ? STRID : FLTID;
    ndimvariables++;
    return &dimvariables[ndimvariables-1];
  }
  else
    seterror(ERR_OUTOFMEMORY);
 
  return 0;
}

/*
  high level string parsing function.
  Returns: a malloced pointer, or 0 on error condition.
  caller must free!
*/
char *stringexpr(void)
{
  char *left;
  char *right;
  char *temp;

  switch(token)
  {
    case DIMSTRID:
      left = mystrdup(stringdimvar());
      break;
    case STRID:
      left = mystrdup(stringvar());
      break;
    case QUOTE:
      left = stringliteral();
      break;
    case CHRSTRING:
      left = chrstring();
      break;
    case STRSTRING:
      left = strstring();
      break;
    case LEFTSTRING:
      left = leftstring();
      break;
    case RIGHTSTRING:
      left = rightstring();
      break;
    case MIDSTRING:
      left = midstring();
      break;
    case STRINGSTRING:
      left = stringstring();
      break;
    default:
      if(!isstring(token))
        seterror(ERR_TYPEMISMATCH);
      else
        seterror(ERR_SYNTAX);
      return mystrdup("");
  }

  if(!left)
  {
    seterror(ERR_OUTOFMEMORY);
    return 0;
  }

  switch(token)
  {
    case PLUS:
      match(PLUS);
      right = stringexpr();
      if(right)
      {
        temp = mystrconcat(left, right);
        sys_free(right);
        if(temp)
        {
          sys_free(left);
          left = temp;
        }
        else
          seterror(ERR_OUTOFMEMORY);
      }
      else
        seterror(ERR_OUTOFMEMORY);
      break;
    default:
      return left;
  }

  return left;
}

/*
  parse the CHR$ token
*/
char *chrstring(void)
{
  double x;
  char buff[6];
  char *answer;

  match(CHRSTRING);
  match(OPAREN);
  x = integer( expr() );
  match(CPAREN);

  buff[0] = (char) x;
  buff[1] = 0;
  answer = mystrdup(buff);

  if(!answer)
    seterror(ERR_OUTOFMEMORY);

  return answer;
}

/*
  parse the STR$ token
*/
char *strstring(void)
{
  double x;
  char buff[64];
  char *answer;

  match(STRSTRING);
  match(OPAREN);
  x = expr();
  match(CPAREN);

  sprintf(buff, "%g", x);
  answer = mystrdup(buff);
  if(!answer)
    seterror(ERR_OUTOFMEMORY);
  return answer;
}

/*
  parse the LEFT$ token
*/
char *leftstring(void)
{
  char *str;
  int x;
  char *answer;

  match(LEFTSTRING);
  match(OPAREN);
  str = stringexpr();
  if(!str)
    return 0;
  match(COMMA);
  x = integer( expr() );
  match(CPAREN);

  if(x > (int) strlen(str))
    return str;
  if(x < 0)
  {
    seterror(ERR_ILLEGALOFFSET);
    return str;
  }
  str[x] = 0;
  answer = mystrdup(str);
  sys_free(str);
  if(!answer)
    seterror(ERR_OUTOFMEMORY);
  return answer;
}

/*
  parse the RIGHT$ token
*/
char *rightstring(void)
{
  int x;
  char *str;
  char *answer;

  match(RIGHTSTRING);
  match(OPAREN);
  str = stringexpr();
  if(!str)
    return 0;
  match(COMMA);
  x = integer( expr() );
  match(CPAREN);

  if( x > (int) strlen(str))
    return str;

  if(x < 0)
  {
    seterror(ERR_ILLEGALOFFSET);
    return str;
  }
  
  answer = mystrdup( &str[strlen(str) - x] );
  sys_free(str);
  if(!answer)
    seterror(ERR_OUTOFMEMORY);
  return answer;
}

/*
  parse the MID$ token
*/
char *midstring(void)
{
  char *str;
  int x;
  int len;
  char *answer;
  char *temp;

  match(MIDSTRING);
  match(OPAREN);
  str = stringexpr();
  match(COMMA);
  x = integer( expr() );
  match(COMMA);
  len = integer( expr() );
  match(CPAREN);

  if(!str)
    return 0;

  if(len == -1)
    len = strlen(str) - x + 1;

  if( x > (int) strlen(str) || len < 1)
  {
    sys_free(str);
    answer = mystrdup("");
    if(!answer)
      seterror(ERR_OUTOFMEMORY);
    return answer;
  }
  
  if(x < 1.0)
  {
    seterror(ERR_ILLEGALOFFSET);
    return str;
  }

  temp = &str[x-1];

  answer = sys_malloc(len + 1);
  if(!answer)
  {
    seterror(ERR_OUTOFMEMORY);
    return str;
  }
  strncpy(answer, temp, len);
  answer[len] = 0;
  sys_free(str);

  return answer;
}

/*
  parse the string$ token
*/
char *stringstring(void)
{
  int x;
  char *str;
  char *answer;
  int len;
  int N;
  int i;

  match(STRINGSTRING);
  match(OPAREN);
  x = integer( expr() );
  match(COMMA);
  str = stringexpr();
  match(CPAREN);

  if(!str)
    return 0;

  N = x;

  if(N < 1)
  {
    sys_free(str);
    answer = mystrdup("");
    if(!answer)
      seterror(ERR_OUTOFMEMORY);
    return answer;
  }

  len = strlen(str);
  answer = sys_malloc( N * len + 1 );
  if(!answer)
  {
    sys_free(str);
    seterror(ERR_OUTOFMEMORY);
    return 0;
  }
  for(i=0; i < N; i++)
  {
    strcpy(answer + len * i, str);
  }
  sys_free(str);

  return answer;
}

/*
  read a dimensioned string variable from input.
  Returns: pointer to string (not malloced) 
*/
char *stringdimvar(void)
{
  char id[32];
  int len;
  DIMVAR *dimvar;
  char **answer;
  int index[5];

  answer = NULL;
  getid(string, id, &len);
  match(DIMSTRID);
  dimvar = finddimvar(id);

  if(dimvar)
  {
    switch(dimvar->ndims)
    {
      case 1:
        index[0] = integer( expr() );
        answer = getdimvar(dimvar, index[0]);
        break;
      case 2:
        index[0] = integer( expr() );
        match(COMMA);
        index[1] = integer( expr() );
        answer = getdimvar(dimvar, index[0], index[1]);
        break;
      case 3:
        index[0] = integer( expr() );
        match(COMMA);
        index[1] = integer( expr() );
        match(COMMA);
        index[2] = integer( expr() );
        answer = getdimvar(dimvar, index[0], index[1], index[2]);
        break;
      case 4:
        index[0] = integer( expr() );
        match(COMMA);
        index[1] = integer( expr() );
        match(COMMA);
        index[2] = integer( expr() );
        match(COMMA);
        index[3] = integer( expr() );
        answer = getdimvar(dimvar, index[0], index[1], index[2], index[3]);
        break;
      case 5:
        index[0] = integer( expr() );
        match(COMMA);
        index[1] = integer( expr() );
        match(COMMA);
        index[2] = integer( expr() );
        match(COMMA);
        index[3] = integer( expr() );
        match(COMMA);
        index[4] = integer( expr() );
        answer = getdimvar(dimvar, index[0], index[1], index[2], index[3], index[4]);
        break;

    }

    match(CPAREN);
  }
  else
    seterror(ERR_NOSUCHVARIABLE);

  if(!errorflag)
    if(*answer)
      return *answer;
     
  return "";
}

/*
  parse a string variable.
  Returns: pointer to string (not malloced) 
*/
char *stringvar(void)
{
  char id[32];
  int len;
  VARIABLE *var;

  getid(string, id, &len);
  match(STRID);
  var = findvariable(id);
  if(var)
  {
    if(var->sval)
      return var->sval;
    return "";
  }
  seterror(ERR_NOSUCHVARIABLE);
  return "";
}

/*
  parse a string literal
  Returns: malloced string literal
  Notes: newlines aren't allwed in literals, but blind
         concatenation across newlines is. 
*/
char *stringliteral(void)
{
  int len = 1;
  char *answer = 0;
  char *temp;
  char *substr;
  char *end;

  while(token == QUOTE)
  {
    while(isspace(*string))
      string++;

    end = mystrend(string, '"');
    if(end)
    {
      len = end - string;
      substr = sys_malloc(len);
      if(!substr)
      {
        seterror(ERR_OUTOFMEMORY);
        return answer;
      }
      mystrgrablit(substr, string);
      if(answer)
      {
        temp = mystrconcat(answer, substr);
        sys_free(substr);
        sys_free(answer);
        answer = temp;
        if(!answer)
        {
          seterror(ERR_OUTOFMEMORY);
          return answer;
        }
      }
      else
        answer = substr;
      string = end;
    }
    else
    {
      seterror(ERR_SYNTAX);
      return answer;
    }

    match(QUOTE);
  }

  return answer;
}

/*
  cast a double to an integer, triggering errors if out of range
*/
int integer(double x)
{
#if defined(__ZPU__)
  #define INT_MIN -2147483648L
  #define INT_MAX 2147483647L
#endif
  if( x < INT_MIN || x > INT_MAX )
    seterror( ERR_BADVALUE );
  if( x != floor(x) )
    seterror( ERR_NOTINT );
  return (int) x;
}

/*
  check that we have a token of the passed type 
  (if not set the errorflag)
  Move parser on to next token. Sets token and string.
*/
void match(int tok)
{
  if(token != tok)
  {
    seterror(ERR_SYNTAX);
    return;
  }

  while(isspace(*string))
    string++;

  string += tokenlen(string, token);
  token = gettoken(string);
  if(token == ERROR)
    seterror(ERR_SYNTAX);
}

/*
  set the errorflag.
  Params: errorcode - the error.
  Notes: ignores error cascades
*/
void seterror(int errorcode)
{
  if(errorflag == 0 || errorcode == 0)
    errorflag = errorcode;
}

/*
  get the next line number
  Params: curline - line being processed.
  Returns: line no of next line, 0 if end
  Notes: goes to next line then finds
         first line starting with a digit.
*/
int getnextline(int curline)
{
    int        idx;
    const char *str;

    for(idx=curline+1; idx < nlines; idx++)
    {
        str = lines[idx].str;
        while(*str && *str == ' ') str++;
        if(*str == 0)
            return 0;
        if(isdigit(*str))
            return atoi(str);
    }
    return 0;
}

/*
  get a token from the string
  Params: str - string to read token from
  Notes: ignores white space between tokens
*/
int gettoken(const char *str)
{
  while(isspace(*str))
    str++;

  if(isdigit(*str))
    return VALUE;
 
  switch(*str)
  {
    case 0:
      return EOS;
    case '\n':
      return EOL;
    case '/': 
      return DIV;
    case '*':
      return MULT;
    case '(':
      return OPAREN;
    case ')':
      return CPAREN;
    case '+':
      return PLUS;
    case '-':
      return MINUS;
    case '!':
      return SHRIEK;
    case ',':
      return COMMA;
    case ';':
      return SEMICOLON;
    case '"':
      return QUOTE;
    case '=':
      return EQUALS;
    case '<':
      return LESS;
    case '>':
      return GREATER;
    default:
      if(!strncmp(str, "e", 1) && !isalnum(str[1]))
        return BE;
      if(isupper(*str))
      {
      if(!strncmp(str, "SIN", 3) && !isalnum(str[3]))
        return SIN;
      if(!strncmp(str, "COS", 3) && !isalnum(str[3]))
        return COS;
      if(!strncmp(str, "TAN", 3) && !isalnum(str[3]))
        return TAN;
      if(!strncmp(str, "LN", 2) && !isalnum(str[2]))
        return LN;
      if(!strncmp(str, "POW", 3) && !isalnum(str[3]))
        return POW;
      if(!strncmp(str, "PI", 2) && !isalnum(str[2]))
        return BPI;
      if(!strncmp(str, "SQRT", 4) && !isalnum(str[4]))
        return SQRT;
      if(!strncmp(str, "PRINT", 5) && !isalnum(str[5]))
        return PRINT;
      if(!strncmp(str, "LET", 3) && !isalnum(str[3]))
        return LET;
      if(!strncmp(str, "DIM", 3) && !isalnum(str[3]))
        return DIM;
      if(!strncmp(str, "IF", 2) && !isalnum(str[2]))
        return IF;
      if(!strncmp(str, "THEN", 4) && !isalnum(str[4]))
        return THEN;
      if(!strncmp(str, "AND", 3) && !isalnum(str[3]))
        return AND;
      if(!strncmp(str, "OR", 2) && !isalnum(str[2]))
        return OR;
      if(!strncmp(str, "GOTO", 4) && !isalnum(str[4]))
        return GOTO;
      if(!strncmp(str, "INPUT", 5) && !isalnum(str[5]))
        return BINPUT;
      if(!strncmp(str, "REM", 3) && !isalnum(str[3]))
        return REM;
      if(!strncmp(str, "FOR", 3) && !isalnum(str[3]))
        return FOR;
      if(!strncmp(str, "TO", 2) && !isalnum(str[2]))
        return TO;
      if(!strncmp(str, "NEXT", 4) && !isalnum(str[4]))
        return NEXT;
      if(!strncmp(str, "STEP", 4) && !isalnum(str[4]))
        return STEP;
      if(!strncmp(str, "POKE", 4) && !isalnum(str[4]))
        return POKE;

      if(!strncmp(str, "MOD", 3) && !isalnum(str[3]))
        return MOD;
      if(!strncmp(str, "ABS", 3) && !isalnum(str[3]))
        return ABS;
      if(!strncmp(str, "LEN", 3) && !isalnum(str[3]))
        return LEN;
      if(!strncmp(str, "ASCII", 5) && !isalnum(str[5]))
        return ASCII;
      if(!strncmp(str, "ASIN", 4) && !isalnum(str[4]))
        return ASIN;
      if(!strncmp(str, "ACOS", 4) && !isalnum(str[4]))
        return ACOS;
      if(!strncmp(str, "ATAN", 4) && !isalnum(str[4]))
        return ATAN;
      if(!strncmp(str, "INT", 3) && !isalnum(str[3]))
         return INT;
      if(!strncmp(str, "RND", 3) && !isalnum(str[3]))
         return RND;
      if(!strncmp(str, "VAL", 3) && !isalnum(str[3]))
         return VAL;
      if(!strncmp(str, "VALLEN", 6) && !isalnum(str[6]))
        return VALLEN;
      if(!strncmp(str, "INSTR", 5) && !isalnum(str[5]))
        return INSTR;
      if(!strncmp(str, "PEEK", 4) && !isalnum(str[4]))
        return PEEK;

      if(!strncmp(str, "CHR$", 4))
        return CHRSTRING;
      if(!strncmp(str, "STR$", 4))
        return STRSTRING;
      if(!strncmp(str, "LEFT$", 5))
        return LEFTSTRING; 
      if(!strncmp(str, "RIGHT$", 6))
        return RIGHTSTRING;
      if(!strncmp(str, "MID$", 4))
        return MIDSTRING;
      if(!strncmp(str, "STRING$", 7))
        return STRINGSTRING;
      }
      /* end isupper() */

      if(isalpha(*str))
      {
        while(isalnum(*str))
          str++;
        switch(*str)
        {
          case '$':
            return str[1] == '(' ? DIMSTRID : STRID;
          case '(':
            return DIMFLTID;
          default:
            return FLTID;
        }
      }

      return ERROR;
  } 
}

/*
  get the length of a token.
  Params: str - pointer to the string containing the token
          token - the type of the token read
  Returns: length of the token, or 0 for EOL to prevent
           it being read past.
*/
int tokenlen(const char *str, int token)
{
  int len = 0;
  char buff[32];

  switch(token)
  {
    case EOS:
      return 0;
    case EOL:
      return 1;
    case VALUE:
      getvalue(str, &len);
      return len;
    case DIMSTRID:
    case DIMFLTID:
    case STRID:
      getid(str, buff, &len);
      return len;
    case FLTID:
      getid(str, buff, &len);
      return len;
    case BPI:
      return 2;
    case BE:
      return 1;
    case SIN:
      return 3;
    case COS:
      return 3;
    case TAN:
      return 3;
    case LN:
      return 2;
    case POW:
      return 3;
    case SQRT:
      return 4;
    case DIV:
      return 1;
    case MULT:
      return 1; 
    case OPAREN:
      return 1;
    case CPAREN:
      return 1;
    case PLUS:
      return 1;
    case MINUS:
      return 1;
    case SHRIEK:
      return 1;
    case COMMA:
      return 1;
    case QUOTE:
      return 1;
    case EQUALS:
      return 1;
    case LESS:
      return 1;
    case GREATER:
      return 1;
    case SEMICOLON:
      return 1;
    case ERROR:
      return 0;
    case PRINT:
      return 5;
    case LET:
      return 3;
    case DIM:
      return 3;
    case IF:
      return 2;
    case THEN:
      return 4;
    case AND:
      return 3;
    case OR:
      return 2;
    case GOTO:
      return 4;
    case BINPUT:
      return 5;
    case REM:
      return 3;
    case FOR:
      return 3;
    case TO:
      return 2;
    case NEXT:
      return 4;
    case STEP:
      return 4;
    case POKE:
      return 4;
    case MOD:
      return 3;
    case ABS:
      return 3;
    case LEN:
      return 3;
    case ASCII:
      return 5;
    case ASIN:
      return 4;
    case ACOS:
      return 4;
    case ATAN:
      return 4;
    case INT:
      return 3;
    case RND:
      return 3;
    case VAL:
      return 3;
    case VALLEN:
      return 6;
    case INSTR:
      return 5;
    case PEEK:
      return 4;
    case CHRSTRING:
      return 4;
    case STRSTRING:
      return 4;
    case LEFTSTRING:
      return 5;
    case RIGHTSTRING:
      return 6;
    case MIDSTRING:
      return 4;
    case STRINGSTRING:
      return 7;
    default:
      assert(0);
      return 0;
  }
}

/*
  test if a token represents a string expression
  Params: token - token to test
  Returns: 1 if a string, else 0
*/
int isstring(int token)
{
  if(token == STRID || token == QUOTE || token == DIMSTRID 
      || token == CHRSTRING || token == STRSTRING 
      || token == LEFTSTRING || token == RIGHTSTRING 
      || token == MIDSTRING || token == STRINGSTRING)
    return 1;
  return 0;
}

/*
  get a numerical value from the parse string
  Params: str - the string to search
          len - return pinter for no chars read
  Retuns: the value of the string.
*/
double getvalue(const char *str, int *len)
{
  double answer;
  char *end;

  answer = strtod(str, &end);
  assert(end != str);
  *len = end - str;
  return answer;
}

/*
  getid - get an id from the parse string:
  Params: str - string to search
          out - id output [32 chars max ]
          len - return pointer for id length
  Notes: triggers an error if id > 31 chars
         the id includes the $ and ( qualifiers.
*/
void getid(const char *str, char *out, int *len)
{
  int nread = 0;
  while(isspace(*str))
    str++;
  assert(isalpha(*str));
  while(isalnum(*str))
  {
    if(nread < 31)
      out[nread++] = *str++;
    else
    {
      seterror(ERR_IDTOOLONG);
      break;
    }
  }
  if(*str == '$')
  {
    if(nread < 31)
      out[nread++] = *str++;
    else
     seterror(ERR_IDTOOLONG);
  }
  if(*str == '(')
  {
    if(nread < 31)
      out[nread++] = *str++;
    else
      seterror(ERR_IDTOOLONG);
  }
  out[nread] = 0;
  *len = nread;
}


/*
  grab a literal from the parse string.
  Params: dest - destination string
          src - source string
  Notes: strings are in quotes, double quotes the escape
*/
void mystrgrablit(char *dest, const char *src)
{
  assert(*src == '"');
  src++;
  
  while(*src)
  {
    if(*src == '"')
    {
      if(src[1] == '"')
      {
        *dest++ = *src;
        src++;
        src++;
      }
      else
        break;
    }
    else
     *dest++ = *src++;
  }

  *dest++ = 0;
}

/*
  find where a source string literal ends
  Params: src - string to check (must point to quote)
          quote - character to use for quotation
  Returns: pointer to quote which ends string
  Notes: quotes escape quotes
*/
char *mystrend(const char *str, char quote)
{
  assert(*str == quote);
  str++;

  while(*str)
  {
    while(*str != quote)
    {
      if(*str == '\n' || *str == 0)
        return 0;
      str++;
    }
    if(str[1] == quote)
      str += 2;
    else
      break;
  }

  return (char *) (*str? str : 0);
}

/*
  Count the instances of ch in str
  Params: str - string to check
          ch - character to count
  Returns: no time chs occurs in str. 
*/
int mystrcount(const char *str, char ch)
{
  int answer = 0;

  while(*str)
  {
    if(*str++ == ch)
      answer++;
  }

  return answer;
}

/*
  duplicate a string:
  Params: str - string to duplicate
  Returns: malloced duplicate.
*/
char *mystrdup(const char *str)
{
  char *answer;

  answer = sys_malloc(strlen(str) + 1);
  if(answer)
    strcpy(answer, str);

  return answer;
}

/*
  concatenate two strings
  Params: str - firsts string
          cat - second string
  Returns: malloced string.
*/
char *mystrconcat(const char *str, const char *cat)
{
  int len;
  char *answer;

  len = strlen(str) + strlen(cat);
  answer = sys_malloc(len + 1);
  if(answer)
  {
    strcpy(answer, str);
    strcat(answer, cat);
  }
  return answer;
}

/*
  compute x!  
*/
double factorial(double x)
{
  double answer = 1.0;
  double t;

  if( x > 1000.0)
    x = 1000.0;

  for(t=1;t<=x;t+=1.0)
    answer *= t;
  return answer;
}

