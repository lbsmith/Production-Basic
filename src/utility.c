/*
 * $Id: utility.c,v 1.20 2000/06/04 22:36:11 lees Exp $
 */

#include <string.h>
#include <termios.h>
#include <sys/time.h>
#include "basic.h"

extern int chaninfo;

// found on snippets.org
char *rmwhite(char *str)
{
  char *obuf, *nbuf;

  if (str) {
    for (obuf = str, nbuf= str; *obuf; ++obuf) 
      if (!isspace(*obuf)) *nbuf++ = *obuf;
    obuf = NULL;
  }
  return str;
}

int itoh(byte num, char *pt)
{
  unsigned char nm[2];

  nm[0]=(char)((int)num/16);
  nm[1]=(char)((int)num%16);
  for (num=0;num<2;(num)++) {   
    switch (nm[num]) {
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
        nm[num]+=55;
        break;
      default:
        nm[num]+=48;
        break;
    }
  }
  *pt=(char)nm[0];
  *(pt+1)=(char)nm[1];
  return(0);
}

// homebrewed htoi, there's gotta be a better way of doing this...
// however this implementation is straightforward, effective and easy to use
int myhtoi(char c1, char c2)
{
  int out = 0;
 
  switch (c1) {
    case '0':
      break;
    case '1':
      out += 16;
      break;
    case '2':
      out += 32;
      break;
    case '3':
      out += 48;
      break;
    case '4':
      out += 64;
      break;
    case '5':
      out += 80;
      break;
    case '6':
      out += 96;
      break;
    case '7':
      out += 112;
      break;
    case '8':
      out += 128;
      break;
    case '9':
      out += 144;
      break;
    case 'A':
      out += 160;
      break;
    case 'B':
      out += 176;
      break;
    case 'C':
      out += 192;
      break;
    case 'D':
      out += 208;
      break;
    case 'E':
      out += 224;
      break;
    case 'F':
      out += 240;
      break;
    default:
//      rterror(ERR_VARIABLE_USAGE);
      break;
  }

  switch (c2) {
    case '0':
//      out += 0;
      break;
    case '1':
      out += 1;
      break;
    case '2':
      out += 2;
      break;
    case '3':
      out += 3;
      break;
    case '4':
      out += 4;
      break;
    case '5':
      out += 5;
      break;
    case '6':
      out += 6;
      break;
    case '7':
      out += 7;
      break;
    case '8':
      out += 8;
      break;
    case '9':
      out += 9;
      break;
    case 'A':
      out += 10;
      break;
    case 'B':
      out += 11;
      break;
    case 'C':
      out += 12;
      break;
    case 'D':
      out += 13;
      break;
    case 'E':
      out += 14;
      break;
    case 'F':
      out += 15;
      break;
    default:
//      rterror(ERR_VARIABLE_USAGE);
      break;
  }

  return out;
}

filetbl *get_chaninfo(int channel)
{
  filetbl *tempfile = envinfo.firstfile;
 
  if (tempfile == NULL) return NULL;

  do {
    if (channel == tempfile->channel) return tempfile;
    else tempfile = tempfile->nextfile;
  } while (tempfile != NULL);

  return NULL;
}

void reduce(tbfloat *flt)
{
  char *out;
  int x, y = 0;

  if (flt->exp) {
    out = SafeMalloc(14);
    sprintf(out, "%ld", flt->mantisa.i);

    for (x=strlen(out)-1; x>0; x--) { if (out[x] != '0') break; y++; }
    for (x=0; x<y; x++) { flt->mantisa.i /= 10; flt->exp++; }

    GC_free(out);
  }
}

tbfloat int2flt(int num)
{
  tbfloat flt;

  flt.mantisa.i = num;
  flt.exp = 0;

  return flt;
}

tbfloat asc2flt(char *str)
{
  double tmp;
  int x;

  rmwhite(str);

  for (x=0; x<strlen(str); x++) {
    if (!isdigit(str[x]) && str[x] != '-' && str[x] != '.' &&
         str[x] != ' ' && str[x] != '\n') 
      rterror(ERR_VARIABLE_USAGE);
  }
 
  tmp = atof(str);

  return dbl2flt(tmp);
}

char *flt2asc(tbfloat flt)
{
  char *out, *tmp;
  int x = 0, y = 0;

  out = SafeMalloc(sizeof(char)*14);
  tmp = SafeMalloc(sizeof(char)*14);

  for (x=0; x<14; x++) tmp[x] = '\0';
  x = 0;

  if (flt.exp) {
    sprintf(out, "%ld", flt.mantisa.i);

    if (strlen(out) < (-flt.exp)) {
      if (flt.exp < 0) {
        strcat(tmp, ".");
        for (x=1; x<(-flt.exp)-strlen(out)+1; x++) tmp[x] = '0';
        for (y=0; y<strlen(out); y++) { tmp[x] = out[y]; x++; }
      } else {
        for (x=0; x<strlen(out); x++) { tmp[x] = out[y]; y++; }
        for (x=0; x<flt.exp; x++) strcat(tmp, "0");
      }
    } else { 
      for (x=0; x<strlen(out)+flt.exp; x++)
        tmp[x] = out[x];
      strcat(tmp, "."); x++;
      for (x=x; x<strlen(out)+1; x++)
        tmp[x] = out[x-1];
    }
  } else sprintf(tmp, "%ld", flt.mantisa.i);

  GC_free(out);

  return tmp;
}

int flt2int(tbfloat flt)
{
  int x, y;
  char tmp[32], tmp2[32];
  
  if (flt.exp) {
    sprintf(tmp, "%ld", flt.mantisa.i);
    for (y=0; y<strlen(tmp)+flt.exp; y++) 
      tmp2[y] = tmp[y];
    x = atoi(tmp2);
  } else x = flt.mantisa.i;

  return x;
}

double flt2dbl(tbfloat flt)
{
  double dbl = 0;
  char tmp[32], tmp2[32];
  int x, count = 0;

  if (flt.exp) {
    sprintf(tmp, "%ld", flt.mantisa.i);
    for (x=0; x<strlen(tmp)+flt.exp; x++) 
    { tmp2[count] = tmp[x]; count++; }
    tmp2[count] = '.'; count++;
    for (x=x; x<strlen(tmp); x++)
    { tmp2[count] = tmp[x]; count++; }
    dbl = atof(tmp2);
  } else dbl = flt.mantisa.i;

  return dbl;
}

tbfloat dbl2flt(double dbl)
{
  tbfloat flt;
  char tmp[32];
  int x, y, decimal = 0;

  sprintf(tmp, "%.10g", dbl);
  if (strlen(tmp) > 10) {
    y=10; 
    for (x=y; y<strlen(tmp); y++) tmp[y] = '\0';
  } else y=strlen(tmp);
  for (x=0; x<y; x++) if (tmp[x] == '.') break;
  decimal = x;
  for (x=decimal; x<y; x++) tmp[x] = tmp[x+1];

  flt.mantisa.i = atoi(tmp);
  flt.exp = -(strlen(tmp)-decimal);
  
  return flt;
}

int get_userfnc(char *name)
{
  int i;

  for (i=0; i<gprog->numfunctions; i++)
    if (!strcmp(name, gprog->userfunctions[i].name)) return i;
  
  return gprog->numfunctions;
}

int get_sysvar(char *name)
{
  int i;

  for (i=0; *sysvartbl[i].name; i++)
    if (!strcmp(name, sysvartbl[i].name)) return i;

  return 0;
}

int get_sysvaridx(int opcode)
{
  register int i;

  for (i=0; *sysvartbl[i].name; i++)
    if (opcode == sysvartbl[i].code) return i;

  return 0;
}

char *get_sysvarname(int num)
{
  register int x;

  if (num == 0) return 0;

  for (x=0; *sysvartbl[x].name; x++)
    if (num == sysvartbl[x].code) return sysvartbl[x].name;

  return 0;
}

int get_fnc(char *name)
{
//  int low, high, mid, cmp;
  int x;

  for (x=0; *fnctable[x].name; x++) 
   if (!strcmp(name, fnctable[x].name)) return x;

/*  low = 1;
  high = envinfo.numfunctions-1;
  while (low <= high) {
    mid = (low + high) / 2;
    cmp = strcmp(name, fnctable[mid].name);
    printf("[%d]\n", cmp);
    if (cmp < 0)
      high = mid - 1;
    else if (cmp > 0)
      low = mid + 1;
    else return mid;
  }
*/
  return 0;
}

char *get_fncname(int num)
{
  register int x;

  if (num == 0) return 0;

  for (x=0; *fnctable[x].name; x++)
    if (num == fnctable[x].code) return fnctable[x].name;

  return 0;
}

int get_fncidx(int opcode)
{
  register int x;

  if (opcode == 0) return 0;

  for (x=0; *fnctable[x].name; x++)
    if (opcode == fnctable[x].code) return x;

  return 0;
}

int get_mnemonic(char *name)
{
  register int x;

  for (x=0; *mnemtable[x].name; x++)
    if (!strcmp(name, mnemtable[x].name)) return x;

  return 0;
}

int get_opcode(char *s)
{
  register int i;
  char *p;

  p = s;
  while(*p) { *p = toupper(*p); p++; }
  for (i=0; *cmdtable[i].command; i++)
    if (!strcmp(cmdtable[i].command, s)) return cmdtable[i].code;

  return 0;
}

//int get_opchannel(char *s)
int get_cmdindex(char *s)
{
  register int i;
  char *p;

  p = s;
  while(*p) { *p = toupper(*p); p++; }
  for (i=0; *cmdtable[i].command; i++) {
     if (!strcmp(cmdtable[i].command, s))
       return i;
  }

  return 0;
}

char *get_opname(int code)
{
  register int i;

  for (i=0; *cmdtable[i].command; i++)
    if (code == cmdtable[i].code) return cmdtable[i].command;

  return "LET";
}

int get_opercode(char *symbol)
{
  register int x;
  char *s;

  s = symbol;
  if (symbol[0] != '=') { while (*s) { *s = toupper(*s); s++; } }

  if (chaninfo == 1) x = 0; else x = 15;
  for (x=x; *optable[x].symbol; x++) 
    if (!strcmp(symbol, optable[x].symbol)) return optable[x].opcode;

  return OP_ERROR;
}

char *get_symbol(int opcode)
{
  register int x;
  char *tmp;
  
  tmp = SafeMalloc(5);
  
  if (opcode == OP_EQUALSCMP) return "=";
  if (opcode == OP_STRCAT) return "+";
  if (opcode == OP_NEGATE) return "-";

  for (x=0; *optable[x].symbol; x++) {
    if (opcode == optable[x].opcode) {
      if ((opcode == OP_AND) || (opcode == OP_OR) ||
          (opcode == OP_XOR)) {
        sprintf(tmp, " %s ", optable[x].symbol); 
        return tmp;
      } else return optable[x].symbol;
    }
  }

  return ",";
}

int get_symref(char *sym)
{
  symbol *tempsym = gprog->firstsym;

  do {
    if (!strcmp(sym, tempsym->name)) return tempsym->idx;
    tempsym = tempsym->nextsym;
  } while (tempsym != NULL);

  return gprog->numsymbols;
}

char *get_symname(int symref)
{
  symbol *tempsym = gprog->firstsym;

  do {
    if (tempsym->idx == symref) return tempsym->name;
    tempsym = tempsym->nextsym;
  } while (tempsym != NULL);
  return " "; 
}

char *get_labelname(int labelref)
{
  return gprog->labels[labelref].name;
}

int get_labelref(char *name)
{
  int x;

  for (x=1; x<gprog->numlabels+1; x++)
    if (!strcmp(name, gprog->labels[x].name)) return x;
  
  return 0;
}

void *SafeMalloc(long size)
{
  void *ptr;

  ptr = GC_malloc(size);

  if (!ptr) rterror(ERR_MEMORY_CAPACITY);

  return ptr;
}

void ExtractFilename(char *path, char *dest)
{
  char *src;

  src = path + strlen(path) - 1;

  while (src != path && *(src - 1) != '/')
    src--;
  while (*src)
    *dest++ = *src++;

  *dest = 0;
}

void dumpstmt(statement *stmt, FILE *fp)
{
  int y = 0;

  fprintf(fp,"** STATEMENT DUMP **\n");
  fprintf(fp,"length\t = %d\n", stmt->length);
  fprintf(fp,"linenum\t = %d\n", stmt->linenum);
  fprintf(fp,"%d labels = \n", stmt->numlabels);
  for (y=0; y<stmt->numlabels; y++) {
    fprintf(fp,"  label[%d]\n", y);
    fprintf(fp,"    labelnum\t = $%.4X$\n", stmt->labelset[y]->labelnum);
  }
  fprintf(fp,"opcode\t = $%.4X$\n", stmt->opcode);
  fprintf(fp,"%d elements = \n", stmt->metapos-1);
  for (y=1; y<stmt->metapos; y++) {
    fprintf(fp,"  metalist[%d]\n", y);
    fprintf(fp,"    operation\t = $%.4X$\n", stmt->metalist[y].operation);
    fprintf(fp,"    shortarg\t = $%.4X$\n", stmt->metalist[y].shortarg);
    fprintf(fp,"    stringarg\t = \"%s\"\n", stmt->metalist[y].stringarg);
    fprintf(fp,"    floatarg\t = %.10g\n", flt2dbl(stmt->metalist[y].floatarg));
    fprintf(fp,"    intarg\t = $%.2X$\n", stmt->metalist[y].intarg);
  }
  fprintf(fp,"** END DUMP **\n");
}

void dbg_step()
{
  // look at envlevel, walk down list of programs until that
  // level is reached, then use lastexec+1 to find out last 
  // statement executed, execute that statement and print the
  // list of the next statement
  printf("step %d\n", envinfo.lastexec);
}

symbol *idx2sym(program *pgm, unsigned short idx)
{
  symbol *tempsym = pgm->firstsym;
  int x = 0;

  if (idx & 0x8000) idx -= 0x8000;
  if (idx & 0x2000) idx -= 0x2000;
  if (idx & 0x4000) idx -= 0x4000;

  for (x = 0; x < idx; x++)
    tempsym = tempsym->nextsym;

  return tempsym;
}

symbol *get_arraysym(symbol *top, int array3, int array2, int array1)
{
  symbol *tempsym = top->arraylink;
  int x = 0;

  for (x = 0; x < array1; x++) {
    if (!tempsym) rterror(ERR_NONEXISTENT_SUBSCRIPT);
    tempsym = tempsym->nextsym;
  }

  if (array2 != -1) {
    tempsym = tempsym->arraylink;

    for (x = 0; x < array2; x++) {
      if (!tempsym) rterror(ERR_NONEXISTENT_SUBSCRIPT);
      tempsym = tempsym->nextsym;
    }

    if (array3 != -1) {
      tempsym = tempsym->arraylink;

      for (x = 0; x < array3; x++) {
        if (!tempsym) rterror(ERR_NONEXISTENT_SUBSCRIPT);
        tempsym = tempsym->nextsym;
      }
    }
  }

  return tempsym;
}

// thanks to kervel on irc for this code
char readkey(void)
{
  struct termios otio, ntio;
  fd_set rdfs;
  char ua;

  if (tcgetattr(fileno(stdin), &otio)) 
    rterror(ERR_UNDEFINED_MODE);
  
  ntio = otio;

  ntio.c_lflag &= !(ECHO | ICANON);
  ntio.c_cc[VMIN] = 1;
  ntio.c_cc[VTIME] = 0;
 
  if (tcsetattr(fileno(stdin), TCSAFLUSH, &ntio)) 
    rterror(ERR_UNDEFINED_MODE);

  setvbuf(stdin, NULL, _IONBF, 0);
  ua = 0;
  FD_ZERO(&rdfs);
  FD_SET(0, &rdfs);

  ua = getchar();

  tcsetattr(fileno(stdin), TCSANOW, &otio);

  return ua;
}
