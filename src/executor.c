/*
 * $Id: executor.c,v 1.25 2000/06/04 22:36:32 lees Exp $
 */

#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "basic.h"

tbstring fnc_strcat(tbstring a, tbstring b);

execelement execstack[STACK_DEPTH];
int execcounter = 0;

forstruct fstack[STACK_DEPTH];
int fcounter = 0;

int gosubstack[STACK_DEPTH];
int gosubcounter = 0;

int whilestack[STACK_DEPTH];
int whilecounter = 0;

byte linectr = 0, colctr = 0;

tbfloat arith(tbfloat val1, tbfloat val2, int op);
int compare(tbfloat val1, tbfloat val2, int op);
void gosubpush(int value);
int gosubpop(void);
extern int forlist(statement *stmt, int start);
int strcompare(tbstring val1, tbstring val2, int op);
int simplify(program *pgm, statement *stmt, int x);
void dump_var(FILE *fp, symbol *sym);

void resetstacks()
{
  gosubcounter = 0;
  whilecounter = 0;
  execcounter = 0;
  fcounter = 0;
}

void execpush(execelement elmt)
{
  execstack[execcounter].type = elmt.type;
  if (execstack[execcounter].type == 1) {
    execstack[execcounter].fltarg = elmt.fltarg;
  } else {
    execstack[execcounter].strarg = elmt.strarg;
  }  
  execcounter++;
}

execelement execpop(void)
{
  execcounter--;
  return execstack[execcounter];
}

void strexecpush3(tbstring str)
{
  if (str.len >= 0xFFFF) rterror(ERR_STRING_SIZE);
  execstack[execcounter].type = 2;
  execstack[execcounter].strarg = str;
  execcounter++;
}

// strings which may contain 0x00 
void strexecpush2(char *s, unsigned short len)
{
  if (len >= 0xFFFF) rterror(ERR_STRING_SIZE);
  execstack[execcounter].type = 2;
  execstack[execcounter].strarg.str = s;
  execstack[execcounter].strarg.len = len;
  execcounter++;
}

// normal strings
void strexecpush(char *s)
{
  execstack[execcounter].type = 2;
  execstack[execcounter].strarg.str = s;
  execstack[execcounter].strarg.len = strlen(s);
  if (strlen(s) >= 0xFFFF) rterror(ERR_STRING_SIZE);
  execcounter++;
}

tbstring strexecpop()
{
  execcounter--; 
  if (execstack[execcounter].type != 2) rterror(ERR_SYNTAX);
  return execstack[execcounter].strarg;
}

void fltexecpush(tbfloat value)
{
  reduce(&value);
  execstack[execcounter].type = 1;
  execstack[execcounter].fltarg = value;
  execcounter++;
}

tbfloat fltexecpop()
{
  execcounter--;
  return execstack[execcounter].fltarg;
}

void stackreport()
{
  int x = 0;

  for (x=0; x<execcounter; x++) { 
    if(execstack[x].type == 1) {
      printf("[ %d : %d ]\n", x, flt2int(execstack[x].fltarg));
    } else {
      printf("[ %d : %s ]\n", x, execstack[x].strarg.str);
    }
  }
}

void flipstack(int stoplevel)
{
  execelement elmt[STACK_DEPTH];
  int x = 0, y = 0;

  if (!stoplevel) {
    y = execcounter;

    for (x = 0; execcounter > 0; x++) 
      elmt[x] = execpop();

    for (x = 0; x < y; x++) 
      execpush(elmt[x]);

  } else {
    for (x = 0; x < stoplevel; x++) 
      elmt[x] = execpop();

    for (x = 0; x < stoplevel; x++)
      execpush(elmt[x]);
  }
}

tbfloat add(tbfloat val1, tbfloat val2)
{
  tbfloat result;

  do {
    if (val1.exp < val2.exp) {
      val2.mantisa.i *= 10;
      val2.exp--;
    } else if (val2.exp < val1.exp) {
      val1.mantisa.i *= 10;
      val1.exp--;
    }
  } while (val1.exp != val2.exp);

  result.exp = val1.exp;
  result.mantisa.i = val1.mantisa.i + val2.mantisa.i;

  return result;
}

tbfloat multiply(tbfloat val1, tbfloat val2)
{
  tbfloat result;

  result.mantisa.i = val1.mantisa.i * val2.mantisa.i;
  result.exp = val1.exp + val2.exp;

  return result;
}

tbfloat divide(tbfloat val1, tbfloat val2)
{
  tbfloat result;

  if (val1.mantisa.i != 0) {
    do {
      val1.mantisa.i *= 10; val1.exp--;
    } while (val1.mantisa.i < 214748364); // temporary, make sure value doesn't
  }                                       // blow over top

  if (val2.mantisa.i == 0) rterror(ERR_NUM_VALUE_OVERFLOW);
  
  result.mantisa.i = val1.mantisa.i / val2.mantisa.i;
  result.exp = val1.exp - val2.exp;

  return result;
}

tbfloat subtract(tbfloat val1, tbfloat val2)
{
  tbfloat result;

  do {
    if (val1.exp < val2.exp) {
      val2.mantisa.i *= 10;
      val2.exp--;
    } else if (val2.exp < val1.exp) {
      val1.mantisa.i *= 10;
      val1.exp--;
    }
  } while (val1.exp != val2.exp);

  result.exp = val1.exp;
  result.mantisa.i = val1.mantisa.i - val2.mantisa.i;

  return result;
}

tbfloat exponent(tbfloat val1, tbfloat val2)
{
  tbfloat result;
  double x;

  x = pow(flt2dbl(val1), flt2dbl(val2));
  result = dbl2flt(x);
 
  return result;
}

tbfloat arith(tbfloat val1, tbfloat val2, int op)
{
  tbfloat endval;

  switch(op) 
  {
    case OP_AND:
      if (val1.mantisa.i && val2.mantisa.i) endval = int2flt(1);
      else endval = int2flt(0);
      break;
    case OP_OR:
      if (val1.mantisa.i || val2.mantisa.i) endval = int2flt(1);
      else endval = int2flt(0);
      break;
    case OP_XOR:
      if ((val1.mantisa.i || val2.mantisa.i) &&
         !(val1.mantisa.i && val2.mantisa.i)) endval = int2flt(1);
      else endval = int2flt(0);
      break;
    case OP_NEGATE:
      val2.mantisa.i = val2.mantisa.i - (2*val2.mantisa.i);
      endval = val2;
      break;
    case OP_ADD:
      endval = add(val1, val2);
      break;
    case OP_SUBTRACT:
      endval = subtract(val1, val2);
      break;
    case OP_DIVIDE:
      endval = divide(val1, val2);
      break;
    case OP_MULTIPLY:
      endval = multiply(val1, val2);
      break;
    case OP_EXPONENT:
      endval = exponent(val1, val2);
      break;
    default:
      endval = val2;
      break;
  }

  return endval;
}

void exec_userfunction(program *pgm, statement *stmt, int start, int end)
{
  statement tmpstmt;
  int a = 0, b = 0, y = 0, z = 0;
  symbol *tmpsym;
  
  b = execcounter;
  for (a=start+1; a<end+1; a++) a = a + simplify(pgm, stmt, a); 
  flipstack(execcounter-b);

  tmpstmt = *pgm->firststmt;
  if (pgm->userfunctions[stmt->metalist[start].shortarg].defined == 0) 
    rterror(ERR_UNDEFINED_FUNC);

  do {
    if (tmpstmt.linenum == pgm->userfunctions[stmt->metalist[start].shortarg].lineref) break;
    tmpstmt = *tmpstmt.nextstmt;
  } while (&tmpstmt != NULL); 
  for (y=0; y<=tmpstmt.metapos; y++)
    if (tmpstmt.metalist[y].operation == 0xF5) break;      
  for (a=y; a<=tmpstmt.metapos; a++)
    if (tmpstmt.metalist[a].operation == 0xF8) break;
  for (y=y; y<a; y++) {
    tmpsym = idx2sym(pgm, tmpstmt.metalist[y].shortarg);
    switch (tmpstmt.metalist[y].operation) {
      case GETVAL_STRINGARRAY:
      case GETVAL_STRING:
        tmpsym->string = strexecpop();
        break;
      case GETVAL_NUMERICARRAY:
      case GETVAL_NUMERIC:
        tmpsym->numeric = fltexecpop();
        break;
      default:
        break;
    }
  }
  for (z=y; z<tmpstmt.metapos; z++)
    simplify(pgm, &tmpstmt, z); 
}

int simplify(program *pgm, statement *stmt, int x)
{
  tbstring tmp;
  char c[32];
  int a = 0, y = 0, count = 0, index = 0, start = 0, end = 0;
  tbfloat flt, flt2;
  execelement elmt;
  symbol *tmpsym;

  for (a=0; a<32; a++) c[a] = '\0';

  if (get_fncname(stmt->metalist[x].operation) || 
      get_sysvarname(stmt->metalist[x].operation)) {
    if ((stmt->metalist[x].operation == OP_AND) ||
        (stmt->metalist[x].operation == OP_OR) ||
        (stmt->metalist[x].operation == OP_XOR)) goto blah;
    exec_function(pgm, stmt, x);
  } else 
blah:
  switch(stmt->metalist[x].operation)
  {
    case MNEMONICREF: 
      strexecpush(mnemtable[get_mnemonic(stmt->metalist[x].stringarg)].command);
      break;
    case 0xF6:
      printf("\x1B[%d`", flt2int(fltexecpop()));
      break;
    case 0xF7:
      printf("\x1B[%d;%df", flt2int(fltexecpop()), flt2int(fltexecpop()));
      break;
    case 0x8DC9:
      for (y=x+1; y<stmt->metapos+1; y++) {
        if (stmt->metalist[y].operation == 0xF5) a++;
        if (stmt->metalist[y].operation == 0xF8) a--;
        if (a == 0) break;
      }
      exec_userfunction(pgm, stmt, x, y);
      return a;
    case 0xE1:
      envinfo.curchannel = flt2int(fltexecpop());
      break;
    case OP_SRT:
      break;
    case OP_OPT:
      tmp = strexecpop();
      envinfo.curopt = SafeMalloc(tmp.len);
      envinfo.curopt = tmp.str;
      break;
    case OP_SEP: 
      tmp = strexecpop();
      envinfo.cursep = tmp.str[0];
      break;
    case OP_BNK:
      break;
    case OP_DOM:
      break;
    case OP_END:
      envinfo.eofline = flt2int(fltexecpop());
      break;
    case OP_IND:
      envinfo.curindex = flt2int(fltexecpop());
      break;
    case OP_IOL:
      break;
    case OP_SIZ:
      envinfo.cursize = flt2int(fltexecpop());
      break;
    case OP_ISZ:
      envinfo.curisz = flt2int(fltexecpop());
      break;
    case OP_KEY:
      break;
    case OP_TBL:
      break;
    case OP_TIM:
      break;
    case OP_ERR:
      envinfo.errline = flt2int(fltexecpop());
      break;
    case OP_LEN:
      break;
    case OP_PWD:
      break;
    case OP_ATR:
      break;
    case SHORTLITERAL:
    case HEXSTRING:
      strexecpush2(stmt->metalist[x].stringarg, stmt->metalist[x].intarg);
      break;
    case GETVAL_STRINGARRAY:
    case GETVAL_STRING: 
      if (stmt->metalist[x].shortarg & 0x2000)
        index = stmt->metalist[x].shortarg - 0x2000;
      else if (stmt->metalist[x].shortarg & 0x4000)
        index = stmt->metalist[x].shortarg - 0x4000;
      else index = stmt->metalist[x].shortarg;
      if (stmt->metalist[x].shortarg & 0x2000) {
        start = flt2int(fltexecpop())-1;
        if (start == -1) rterror(ERR_INTEGER_RANGE);
      } else if (stmt->metalist[x].shortarg & 0x4000) {
        end = flt2int(fltexecpop());
        start = flt2int(fltexecpop())-1;
        if (start == -1) rterror(ERR_INTEGER_RANGE);
        end += start;
      } else {
        start = 0;
      }
      if (stmt->metalist[x].shortarg & 0x8000) {
        index -= 0x8000;
        if (stmt->metalist[x].operation == GETVAL_STRINGARRAY) {
          flt = fltexecpop();
          flt2 = fltexecpop();
          tmpsym = get_arraysym(idx2sym(pgm, index), 
                   flt2int(flt), flt2int(flt2),
                   flt2int(fltexecpop()));
        } else {
          tmpsym = get_arraysym(idx2sym(pgm, index), -1, -1, 
                                flt2int(fltexecpop()));
        }
      } else if (stmt->metalist[x].operation == GETVAL_STRINGARRAY) {
        flt = fltexecpop();
        tmpsym = get_arraysym(idx2sym(pgm, index), -1, 
                       flt2int(flt), flt2int(fltexecpop()));
      } else {
        tmpsym = idx2sym(pgm, index);
      }
      if (tmpsym->string.len == 0) {
        if (stmt->metalist[x].shortarg > 0x2000)
          rterror(ERR_INVALID_SUBSTRING_REF);
        else 
          strexecpush("\0");
        break;
      }
      if (!(stmt->metalist[x].shortarg & 0x4000))
        end = tmpsym->string.len;

      if ((start > tmpsym->string.len) || 
         (end > tmpsym->string.len))
        rterror(ERR_INVALID_SUBSTRING_REF);
      tmp.str = SafeMalloc(end-start); 
      tmp.len = end-start;
      for (y=start; y<end; y++) {
        tmp.str[count] = tmpsym->string.str[y];  
        count++;
      } 
      strexecpush3(tmp);
      break;
    case GETVAL_NUMERICARRAY:
    case GETVAL_NUMERIC:
      index = stmt->metalist[x].shortarg;
      if (stmt->metalist[x].shortarg & 0x8000) {
        index -= 0x8000;
        if (stmt->metalist[x].operation == GETVAL_NUMERICARRAY) {
          flt = fltexecpop();
          flt2 = fltexecpop();
          tmpsym = get_arraysym(idx2sym(pgm, index), 
                   flt2int(flt), flt2int(flt2),
                   flt2int(fltexecpop()));
        } else {
          tmpsym = get_arraysym(idx2sym(pgm, index), -1, -1, 
                                flt2int(fltexecpop()));
        }
      } else if (stmt->metalist[x].operation == GETVAL_NUMERICARRAY) {
        flt = fltexecpop();
        tmpsym = get_arraysym(idx2sym(pgm, index), -1, 
                       flt2int(flt), flt2int(fltexecpop()));
      } else {
        tmpsym = idx2sym(pgm, index);
      }
      fltexecpush(tmpsym->numeric);
      break;
    case LABELREF:
      fltexecpush(int2flt(pgm->labels[stmt->metalist[x].shortarg].lineref));
      break;
    case LINEREF:
      fltexecpush(int2flt(stmt->metalist[x].shortarg));
      break;
    case FLOAT:
      fltexecpush(stmt->metalist[x].floatarg);
      break;
    case OP_NEGATE:
      flt = fltexecpop();
      flt.mantisa.i = -flt.mantisa.i;
      fltexecpush(flt);
      break;
    case OP_STRCAT:
      strexecpush3(fnc_strcat(strexecpop(), strexecpop()));
      break;
    case OP_ADD:
    case OP_SUBTRACT:
    case OP_MULTIPLY:
    case OP_DIVIDE:
    case OP_EXPONENT:
    case OP_AND:
    case OP_XOR:
    case OP_OR:
      fltexecpush(arith(fltexecpop(), fltexecpop(), stmt->metalist[x].operation));
      break;
    case OP_EQUALSCMP:
    case OP_GREATTHAN:
    case OP_LESSTHAN:
    case OP_GTEQ:
    case OP_LTEQ:
    case OP_NOTEQUAL:
      elmt = execpop();
      if (elmt.type == 1) {
        fltexecpush(int2flt(compare(fltexecpop(), elmt.fltarg, 
          stmt->metalist[x].operation)));
      } else {
        fltexecpush(int2flt(strcompare(strexecpop(), elmt.strarg,
          stmt->metalist[x].operation)));
      }
      break;
  }  
  return 0;
}

void gosubpush(int value)
{
  gosubstack[gosubcounter] = value;
  gosubcounter++;
}

int gosubpop(void)
{
  if (gosubcounter == 0) rterror(ERR_RETURN_WITHOUT_GOSUB);
  gosubcounter--;
  return gosubstack[gosubcounter];
}

int strcompare(tbstring val1, tbstring val2, int op)
{
  switch (op) {
    case OP_EQUALSCMP:
      if (!strcmp(val1.str, val2.str)) return 1;
      break;
    case OP_GREATTHAN:
      if (strcmp(val1.str, val2.str) > 0) return 1;
      break;
    case OP_LESSTHAN:
      if (strcmp(val1.str, val2.str) < 0) return 1;
      break;
    case OP_GTEQ:
      if (!strcmp(val1.str, val2.str)) return 1;
      if (strcmp(val1.str, val2.str) > 0) return 1;
      break;
    case OP_LTEQ:
      if (!strcmp(val1.str, val2.str)) return 1;
      if (strcmp(val1.str, val2.str) < 0) return 1;
      break;
    case OP_NOTEQUAL:
      if (strcmp(val1.str, val2.str)) return 1;
      break;
    default:
      if (val2.str) return 1; 
      break;
  }
  return 0;
}

int compare(tbfloat val1, tbfloat val2, int op)
{
  long long x, y;

  do {
    if (val1.exp < val2.exp) {
      if (val2.mantisa.i == 0) break;
      val2.mantisa.i *= 10;
      val2.exp++;
    } else if (val2.exp < val1.exp) {
      if (val1.mantisa.i == 0) break;
      val1.mantisa.i *= 10;
      val1.exp++;
    }
  } while (val1.exp != val2.exp);

  x = val1.mantisa.i;
  y = val2.mantisa.i;

  switch (op) {
    case OP_EQUALSCMP:
      if (x == y) return 1;
      break;
    case OP_GREATTHAN:
      if (x > y) return 1;
      break;
    case OP_LESSTHAN:
      if (x < y) return 1;
      break;
    case OP_GTEQ:
      if (x >= y) return 1;
      break;
    case OP_LTEQ:
      if (x <= y) return 1;
      break;
    case OP_NOTEQUAL:
      if (x != y) return 1;
      break;
    default:
      if (y != 0) return 1;
      break;
  }
  return 0;
}

void forpush(forstruct x)
{
  fstack[fcounter] = x;
  fcounter++;
}

forstruct forpop()
{
  fcounter--;
  if (fcounter < 0) rterror(ERR_NEXT_WITHOUT_FOR);
  return fstack[fcounter];
}

void whilepush(int value)
{
  whilestack[whilecounter] = value;
  whilecounter++;
}

int whilepop()
{
  if (whilecounter == 0) rterror(ERR_NEXT_WITHOUT_FOR);
  whilecounter--;
  return whilestack[whilecounter];
}

void debug(FILE *fp, char *fmt, ...)
{
  va_list v;
  char tmp[MAX_STRING_LENGTH];
  int x = 0;

  va_start(v, fmt);

  vsprintf(tmp, fmt, v);

  for (x = 0; x < strlen(tmp); x++) {
    if (tmp[x] == '\n') 
      if (linectr != 100) linectr++;
  }

  fprintf(fp, tmp);
      
  if (fp == stdout) {
    if ((linectr >= 24) && (linectr < 100)) {
      printf("<<Any key to continue, ctl-L to stop pagination>>");
      if (readkey() == 0xC) linectr = 100;
      else linectr = 0;
      colctr = 0;
    } 
  }
}

int exec_dump(program *pgm, statement *stmt)
{
  int x;
  filetbl *tmpfile = envinfo.firstfile;
  filetbl *tempfile;
  symbol *tempsym;
  FILE *fp;

  linectr = 0;

  if (envinfo.curchannel != -1) {
    tempfile = get_chaninfo(envinfo.curchannel);
    if (!tempfile) rterror(ERR_FILE_DEVICE_USAGE);
    fp = tempfile->fp;
  } else fp = stdout;

  debug(fp, "%s\n", mnemtable[get_mnemonic("CS")].command);
  debug(fp, "*** ENVIRONMENT LEVEL = %d\t\t\bPROGRAM = ", envinfo.envlevel);
  if (pgm->filename) {
    debug(fp, "%s\n", pgm->filename);
    debug(fp, "\n");
  } else { 
    debug(fp, "No program currently loaded\n");
    debug(fp, "\n");
  }

  if (pgm->firstsym->nextsym) {
    tempsym = pgm->firstsym->nextsym;
    do {
      dump_var(fp, tempsym);
      tempsym = tempsym->nextsym;
    } while (tempsym != NULL);
    debug(fp, "\n");
  }
  if (pgm->numfunctions > 0) {
    for (x=1; x<=pgm->numfunctions; x++) {
      if (pgm->userfunctions[x].defined == 1) 
        debug(fp, "User defined function %s found on line %d\n", 
          pgm->userfunctions[x].name, pgm->userfunctions[x].lineref);
    }
  }
  if (gosubcounter) {
    for (x=0; x<gosubcounter; x++) 
      debug(fp, "GOSUB on line %d going to line xxx\n", gosubstack[x]);
  }
  if (whilecounter) {
    for (x=0; x<whilecounter; x++) 
      debug(fp, "WHILE/WEND loop starting at line %d\n", whilestack[x]);
  }
  if (fcounter) {
    for (x=0; x<fcounter; x++) {
      debug(fp, "FOR/NEXT loop starting at line %d\n", fstack[x].repeatline-1);
      debug(fp, "  Variable = %s    \t", fstack[x].storeto->name);
      debug(fp, "Step value = %s    \t", flt2asc(fstack[x].stepval));
      debug(fp, "Limit = %s    \n", flt2asc(fstack[x].endval));
    }
  }
  if ((tmpfile != 0) && (tmpfile->channel != 0x7FBC)) {
    debug(fp, "\n*** CHANNEL INFORMATION\n");
    debug(fp, "\n");
    debug(fp, "Files\n");
    debug(fp, "  Channel\tFile Name\n");
    do { 
      if (tmpfile->channel == 0x7FBC) break;
      else debug(fp, "    %.5d\t%s\n", tmpfile->channel, 
                                         tmpfile->filename);
      tmpfile = tmpfile->nextfile;
    } while (tmpfile != NULL);
  }
  debug(fp, "\nLast Error = %d     \t", envinfo.lasterror);
  debug(fp, "Last Executed Line = %d   \n", pgm->lastexec);
  debug(fp, "Gotoline = %d   \n", envinfo.gotoline);
  debug(fp, "\n*** END OF DUMP\n");

  return 1;
}

tbstring fnc_strcat(tbstring a, tbstring b)
{
  tbstring buffer;
  int x = 0;

  if (a.len + b.len > 0xFFFF) rterror(ERR_STRING_SIZE);
  
  buffer.str = SafeMalloc(a.len + b.len);

  for (x = 0; x <= a.len; x++)
    buffer.str[x] = a.str[x];
 
  for (x = 0; x <= b.len; x++)
    buffer.str[x+a.len] = b.str[x];

  buffer.len = a.len + b.len;

  return buffer;
}

// after it runs fputc, append to a string the hex value
// of the char 
void dump_sym(FILE *fp, symbol *top, byte showname, byte type)
{
  int a = 0, y = 0, z = 0;
  char hexstring[84], tmp[20];

  colctr = 0;

  if (showname == 1) fprintf(fp, "%s", top->name);
  if (type == 1) {
    debug(fp, "\x1B[40`Length = ");
    debug(fp, "%d, ", top->string.len);
    debug(fp, "Numrefs = %d\n", top->numrefs);
    if (top->string.len != 0) {
      debug(fp, "     1: \"");
      for (a = 0; a < top->string.len; a++) {
        colctr++;
        if (isprint(top->string.str[a])) {
          fputc(top->string.str[a], fp);
          y += sprintf(hexstring+y, "%.2X", top->string.str[a]); z++;
        } else {
          fputc('.', fp);
          sprintf(tmp, "%.2X", top->string.str[a]);
          y += sprintf(hexstring+y, "%c%c", tmp[strlen(tmp)-2], 
                     tmp[strlen(tmp)-1]); 
          z++;
        }
        if ((z >= 4) && (colctr < 20) && (a != top->string.len-1)) 
        { z = 0; y += sprintf(hexstring+y, " "); }
        if (colctr >= 20) { 
          if (linectr != 100) linectr++;
          colctr = 0;
          z = 0;
          fprintf(fp, "\"\x1B[33`$%s$\n", hexstring);
          memset(&hexstring, '\0', 84); y = 0;
          memset(&tmp, '\0', 20);
          if (fp == stdout) {
            if ((linectr >= 24) && (linectr < 100)) {
              printf("<<Any key to continue, ctl-L to stop pagination>>");
              if (readkey() == 0xC) linectr = 100;
              else linectr = 0;
              colctr = 0;
              z = 0;
            } 
          }
          if ((a+2) < top->string.len) fprintf(fp, " % 5d: \"", a+2); 
        }
      }
      if (colctr != 0) debug(fp, "\"\x1B[33`$%s$\n", hexstring);
      memset(&hexstring, '\0', 84); y = 0;
      memset(&tmp, '\0', 20);
      if (fp == stdout) {
        if ((linectr >= 24) && (linectr < 100)) {
          printf("<<Any key to continue, ctl-L to stop pagination>>");
          if (readkey() == 0xC) linectr = 100;
          else linectr = 0;
          colctr = 0;
          z = 0;
        } 
      }
    }
  } else debug(fp, " = %s\x1B[40`Numrefs = %d\n", 
                 flt2asc(top->numeric), top->numrefs);
} 

void dump_var(FILE *fp, symbol *top)
{
  symbol *tmpsym, *tmpsym2, *tmpsym3;
  int a = 0, b = 0, c = 0, type = 0;

  if (top->arraylink) {
    debug(fp, "%s[]\t\t\t\t\t\bNumber of elements = %d\n", top->name,
             abs(top->dim1*top->dim2*top->dim3));
    debug(fp, "  Defined dimensions = 0:%d", top->dim1-1);
    if (top->dim2 != -1) debug(fp, ", 0:%d", top->dim2-1);
    if (top->dim3 != -1) debug(fp, ", 0:%d", top->dim3-1);
    debug(fp, "\n");
    if (top->name[top->length-1] == '$') type = 1;
    else type = 0;
    tmpsym = top->arraylink;
    for (a = 0; a < top->dim1; a++) {
      if (top->dim2 != -1) {
        tmpsym2 = tmpsym->arraylink;
        for (b = 0; b < top->dim2; b++) {
          if (top->dim3 != -1) {
            tmpsym3 = tmpsym2->arraylink;
            for (c = 0; c < top->dim3; c++) {
              debug(fp, "  [%d,%d,%d]", a, b, c);
              dump_sym(fp, tmpsym3, 0, type);
              tmpsym3 = tmpsym3->nextsym;
            }
          } else { 
            debug(fp, "  [%d,%d]", a, b);
            dump_sym(fp, tmpsym2, 0, type);
          }
          tmpsym2 = tmpsym2->nextsym;
        }
      } else { 
        debug(fp, "  [%d]", a);
        dump_sym(fp, tmpsym, 0, type);
      }
      tmpsym = tmpsym->nextsym;
    }
    if ((top->string.len != 0) || (top->numeric.mantisa.i != 0))
    { dump_sym(fp, top, 1, type); }
  } else {
    if (top->name[top->length-1] == '$')
    { dump_sym(fp, top, 1, 1); }
    else { dump_sym(fp, top, 1, 0); }
  }
}
