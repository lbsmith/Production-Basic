/*
 * $Id: basic.h,v 1.30 2000/06/04 22:35:04 lees Exp $
 */

#ifndef BASIC_H
#define BASIC_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "errors.h"
#include "opcodes.h"
#include "gc/include/gc.h"

typedef unsigned char byte;

#define VERSION "0.3.0"
#define SYSNAME "LINUX"
#define SYSID "U" // UNIX

#define TOKEN_LEN           80
#define STACK_DEPTH         32
#define MAX_STRING_LENGTH   255
#define MAX_STMT_METAS      40
#define MAX_FILE_FUNCTIONS  32
#define MAX_FILE_LABELS     128

#define TOK_DONE     1
#define TOK_OPERATOR 2
#define TOK_VARIABLE 3
#define TOK_NUMBER   4
#define TOK_COMMAND  5
#define TOK_STRING   6
#define TOK_RESERVED 7
#define TOK_COMMA    8
#define TOK_COLON    9
#define TOK_SEMICOLON 10
#define TOK_MNEMONIC 11
#define TOK_SETVAL   12
#define TOK_FUNCTION 13
#define TOK_SYSVAR   14
#define TOK_USERFUNCTION 15
#define TOK_ARRAY    16
#define TOK_ERROR    -1

#define OP_ERROR     -255
#define RESV_TO     1
#define RESV_STEP   2
#define RESV_THEN   3
#define RESV_ELSE   4

#define FILE_TEXT    1
#define FILE_INDEXED 2

extern struct functions {
  char name[3];
  int code;
  int numparms;
  int returntype;
} fnctable[];

extern struct sysvars {
  char name[12];
  int code;
  int returntype;
} sysvartbl[];

extern struct mnemonics {
  char name[10];
  char command[20];
} mnemtable[];

extern struct reserved {
  char name[20];
  int code;
} resvtable[];

extern struct errors {
  char *errortext;
  int errorcode;
} errortable[];

extern struct commands {
  char command[20];
  int code;
  int options;
} cmdtable[];

extern struct operators {
  char symbol[4];
  int opcode;
  int precedence;
} optable[];

typedef struct {
  union {
    long i;
    short s[2];
  } mantisa;
  char exp;
} tbfloat;

typedef struct {
  char *str;
  unsigned short len;
} tbstring;

typedef struct filetbl_s {
  unsigned short channel;
  int curisz;
  byte cursep;
  byte type;
  FILE *fp;
  char filename[8];
  unsigned short indexsize;
  unsigned short nextrecord;
  unsigned short numrecords;
  unsigned short recordsize;
  struct filetbl_s *nextfile;
} filetbl;

typedef struct stackobj_s {
  char name[1024];
  byte type;
} stackobj;

typedef struct {
  byte length;
  char name[1024];
  unsigned short lineref;
  byte defined;
  unsigned int numrefs;
} linelabel;

typedef struct {
  char name[32];
  unsigned short lineref;
  byte length;
  byte defined;
  unsigned int numrefs;
} userfunction;

typedef struct symbol_s {
  byte length;
  unsigned short idx;
  short dim1;
  short dim2;
  short dim3;
  char name[1024];
  unsigned int numrefs;
  tbstring string;
  tbfloat numeric;
  struct symbol_s *arraylink; // used for arrays
  struct symbol_s *nextsym;
} symbol;

typedef struct {
  symbol *storeto;
  tbfloat startval;
  tbfloat endval;  
  tbfloat stepval;
  int repeatline;
  int negative;
} forstruct;

struct labelset {
  unsigned short labelnum;
};

typedef struct statement_s {
  byte length;
  unsigned short linenum;
  unsigned short opcode;
  byte errorflag;
  int numlabels;
  struct labelset *labelset[32];
  struct {
    unsigned short operation;
    unsigned short shortarg;
    char stringarg[MAX_STRING_LENGTH];
    tbfloat floatarg;
    byte intarg;
  } metalist[MAX_STMT_METAS];
  int metapos;
  byte endcode;
  struct statement_s *nextstmt;
  struct statement_s *prevstmt;
} statement;

typedef struct program_s {
  unsigned short signature;
  unsigned short numsymbols;
  unsigned short numlabels;
  unsigned short lengthprogram;
  unsigned short lengthsymtab;
  unsigned short lengthlabeltab;
  unsigned short lengthfunctab;
  byte dummy;
  byte numfunctions;
  char *filename;
  statement *firststmt;
  unsigned short symmax;
  symbol *firstsym;
  linelabel labels[MAX_FILE_LABELS];
  userfunction userfunctions[MAX_FILE_FUNCTIONS];
  unsigned int lastexec;
  struct program_s *nextprog;
  struct program_s *prevprog;
} program;

struct {
  int curchannel;
  int curindex;
  int errline;
  int seterrline;
  int eofline;
  int cursize;
  char *curopt;

  byte cursep;
  int curisz;

  byte envlevel;
  byte abort;
  byte allowesc;
  byte allowexec;
  byte allowseterr;
  byte stmttype;  
  byte lineref;

  unsigned int gotoline;
  unsigned int escline;
  unsigned int lastexec;
  unsigned int lasterror;
  unsigned int lasterrorline;
  unsigned int lastseterrline;
  unsigned int argc;
  unsigned int numfunctions;
  filetbl *firstfile;
  struct {
    char *name;
    byte numdays;
  } datestrings[19];
  char *argv[32];
} envinfo;

typedef struct {
  byte type;
  tbstring strarg;
  tbfloat fltarg;
} execelement;

// storage.c
int writeall(program *pgm, char *filename);
int loadall(program *pgm, char *filename);
void listprog(program *pgm, int start, int end);
void storageinit(program *pgm);
void envinit(void);
void buffermeta(statement *stmt, int type, char *c);
void stmtinit(statement *stmt);
int addlabel(program *pgm, char *name, int line);
symbol *addsymbol(program *pgm, char *sym);
void delsymbol(program *pgm, int num);
int addfunction(program *pgm, char *name, unsigned short lineref, int defined);
void insertstmt(program *pgm, statement *stmt);
void deletestmt(program *pgm, unsigned int line);
void runprog(program *pgm, unsigned int line);
void freeprog(program *pgm);
int myhtoi(char c1, char c2);

// token.c
int get_token(void);

// basic.c
void rterror(int code);
void lineerror(statement *stmt, char *file, int line);

// utility.c
int get_userfnc(char *name);
int get_command(char *cmd);
int get_opcode(char *symbol);
int get_cmdindex(char *symbol);
char *get_opname(int code);
int get_opercode(char *symbol);
char *get_symbol(int opcode);
int get_symref(char *symbol);
char *get_symname(int symref);
int get_labelref(char *symbol);
char *get_labelname(int labelref);
int get_fnc(char *name);
char *get_fncname(int fncnum);
int get_fncidx(int opcode);
int get_sysvar(char *name);
char *get_sysvarname(int fncnum);
int get_sysvaridx(int opcode);
int get_mnemonic(char *name);
filetbl *get_chaninfo(int channel);
void ExtractFilename(char *path, char *dest);

// stack.c
void push(char *name, int type);
stackobj *pop(void);
void popstack(statement *stmt, int forced);
void stackinit(void);
int stackempty(void);
int checkprec(char *str, int type);

// exec.c
int execline(program *pgm, statement *stmt);

// byteswap.c
short BigShort(short l);
long BigLong(long l);
float BigFloat(float l);
void WriteByte(FILE *fp, byte c);
byte ReadByte(FILE *fp);
void WriteString(FILE *fp, char *c);
char *ReadString(FILE *fp, int length);
void WriteShort(FILE *fp, short l);
short ReadShort(FILE *fp);
void WriteLong(FILE *fp, long l);
long ReadLong(FILE *fp);

void *SafeMalloc(long size);
void reduce(tbfloat *flt);
tbfloat int2flt(int x);
int flt2int(tbfloat flt);
tbfloat dbl2flt(double dbl);
double flt2dbl(tbfloat flt);
tbfloat asc2flt(char *str);
char *flt2asc(tbfloat flt);

tbfloat fltexecpop(void);
void fltexecpush(tbfloat flt);
tbstring strexecpop(void);
void strexecpush(char *str);
void strexecpush2(char *str, unsigned short len);
void strexecpush3(tbstring str);
execelement execpop(void);
void execpush(execelement elmt);
int compare(tbfloat val1, tbfloat val2, int op);
int strcompare(tbstring val1, tbstring val2, int op);
void flipstack(int stoplevel);
void resetvars(program *pgm);
void resetstacks(void);
int simplify(program *pgm, statement *stmt, int x);

void exec_function(program *pgm, statement *stmt, int pos);
void dumpstmt(statement *stmt, FILE *fp);
void listline(char *var, statement *stmt, byte showline);
void closefiles(void);
void encode_rpn(void);
char readkey(void);

void dbg_step(void);

symbol *idx2sym(program *pgm, unsigned short idx);
symbol *get_arraysym(symbol *top, int array1, int array2, int array3);

char *str;
byte lineref;

program *gprog; // global program

char *input;
char token[TOKEN_LEN], *prog;
int tokenpos, token_type, lasttype, parencount;
unsigned short lastopcode;
byte checkerr; // horrible hack

#endif
