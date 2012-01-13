/* 
 * $Id: basic.c,v 1.37 2000/06/04 22:38:00 lees Exp lees $
 */

#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <readline/history.h>
#include <readline/readline.h>
#include "basic.h"

extern void listline(char *var, statement *stmt, byte showline);

jmp_buf mark;

int numargs[32]; // 32 levels of nested functions
int parenstack[32]; // ugly
int chaninfo = 0;

void encode_rpn();
void evalstack(statement *stmt);
int checkoption(int opidx, char *option, int type);
int get_opercode(char *s);

int foundequals = 0;
int firstvar = 1;
int numlabels = 0;
extern int execcounter;

void fpe_handler(int x)
{
  signal(SIGFPE, fpe_handler);
}

void brk_handler(int x)
{
  signal(SIGINT, brk_handler);

  envinfo.lasterror = 127;
  if (envinfo.allowesc && envinfo.escline) envinfo.gotoline = envinfo.escline;
  else { 
    if ((gprog->lastexec < 0xFFFF) && (gprog->lastexec > 0)) {
      printf("\x0D");
      listprog(gprog, gprog->lastexec, gprog->lastexec); 
      printf("\n");
    } 
    fputc('\n', stdin);
    envinfo.allowexec = 0; 
  }
}

// entry point into program
int main(int argc, char *argv[])
{
  int x;
  program *pgm;
  FILE *fp;

  signal(SIGFPE, fpe_handler);
  signal(SIGINT, brk_handler); 

  gprog = SafeMalloc(sizeof(program));
  storageinit(gprog);  

  envinfo.argc = argc;
  for (x=0; x<argc; x++) envinfo.argv[x] = argv[x];

  if ((fp = fopen("LOGIN", "rb")) == NULL) 
    printf("ERROR: Could not open login script\n");
  else {
    fclose(fp);
    pgm = SafeMalloc(sizeof(program));
    envinit();
    loadall(pgm, "LOGIN");
    runprog(pgm, 1);
    GC_free(pgm);
  }

  printf("\x1B[11m");
  rl_inhibit_completion = 1;
  printf("\n\x1B[10mREADY\n"); 
  setjmp(mark);

  do {
     stackinit();
     gprog->nextprog = 0;
     gprog->prevprog = 0;
     input = readline(">");
     prog = input;
     if (*input) { 
       add_history(input); 
       encode_rpn(); 
     }
  } while (1);

  return 0;
}

// main entry into parser
void encode_rpn()
{
  statement *stmt, *tempstmt = gprog->firststmt;
  struct labelset *label;
  unsigned int x = 0, y = 0;
  int channum = 0, lastfnc = 0, lastvar = 0, ifcount = 0;
  byte display = 0, watchchannel = 0, special = 0; 
  char *cont;
  symbol *sym;

begin:

  stmt = SafeMalloc(sizeof(statement));
  stmtinit(stmt);

  for (x=0; x<MAX_STMT_METAS; x++) {
    stmt->metalist[x].operation = 0;
    stmt->metalist[x].floatarg.mantisa.i = 0;
    stmt->metalist[x].floatarg.exp = 0;
    stmt->metalist[x].shortarg = 0;
    for (y=0; y<MAX_STRING_LENGTH; y++)
      stmt->metalist[x].stringarg[y] = 0;
  }

  special = 0;
  foundequals = 0;
  firstvar = 1;
  numlabels = 0;
  envinfo.stmttype = 0;
  tokenpos = 0;
  token_type = 1;
  lastopcode = 0;

  for (x=0; x<32; x++) { numargs[x] = 0; parenstack[x] = 0; }

  do {
     get_token();
     tokenpos += strlen(token);
     if(!checkoption(get_cmdindex(get_opname(stmt->opcode)), token, token_type))
       lineerror(stmt, __FILE__, __LINE__);
     // hack to avoid using runtime system
     // using runtime system would change the last executed line info.
//     if ((!strcmp(token, "PBSTEP")) && (token_type == TOK_COMMAND)) 
//       dbg_step();

     switch (token_type) {
       case TOK_ERROR:
         lineerror(stmt, __FILE__, __LINE__);
         break;

       case TOK_COMMAND:
         lastopcode = get_opcode(token);
         if (cmdtable[get_cmdindex(token)].options & IO_CHANNEL)
           watchchannel = 1;
         else watchchannel = 0;
         stmt->opcode = get_opcode(token);
         if (stmt->opcode == CMD_LET) goto loop2;
         else goto loop;

       case TOK_NUMBER:
         stmt->opcode = 0;
         stmt->linenum = atoi(token);
         break;

       default:
         buffermeta(stmt, token_type, token);
         goto loop2; 
     }

     buffermeta(stmt, token_type, token);

loop:
     get_token();
     if (checkerr == 2) checkerr = 1;
     else if (checkerr == 1) {
       if (token[0] == '=') {
         if (stmt->metalist[stmt->metapos-2].operation == 0xEC)
           stmt->metapos-=2;
         else stmt->metapos--;
         push("ERR", TOK_OPERATOR);
       } else if (token[0] == '(') {
         prog--;
         stmt->metapos--;
         strcpy(token, "ERR");
         token_type = TOK_FUNCTION;
       } 
       checkerr = 0;
     }
     if (lastopcode == CMD_SETERR) {
       if (!strcmp(token, "ON")) lastopcode = CMD_SETERRON;
       if (!strcmp(token, "OFF")) lastopcode = CMD_SETERROFF;
     }
     if (!strcmp(token, "RECORD")) {
       if (stmt->opcode == CMD_READ) lastopcode = CMD_READRECORD;
       if (stmt->opcode == CMD_WRITE) lastopcode = CMD_WRITERECORD;
       if (stmt->opcode == CMD_EXTRACT) lastopcode = CMD_EXTRACTRECORD;
       if (stmt->opcode == CMD_FIND) lastopcode = CMD_FINDRECORD;
       if (stmt->opcode == CMD_PRINT) lastopcode = CMD_PRINTRECORD;
       token_type = lasttype;
       goto loop;
     }
     if (lastopcode == CMD_REM) {
       stmt->metalist[1].operation = 0xF5;
       tokenpos -= 2;
       if (input[tokenpos] == '\"') x = tokenpos; else x = tokenpos+2;
       stmt->metalist[1].shortarg = strlen(input) - x;
       y = 0;
       for (x=x; x<strlen(input); x++)
       { stmt->metalist[1].stringarg[y] = input[x]; y++; }
       stmt->metalist[1].stringarg[y] = '\0';
       stmt->metapos++;
       if (stmt->linenum) insertstmt(gprog, stmt);
       return; 
     }
     tokenpos += strlen(token);
     if(!checkoption(get_cmdindex(get_opname(lastopcode)), token, token_type))
       lineerror(stmt, __FILE__, __LINE__);

loop2:
     switch (token_type) {
       case TOK_SEMICOLON:
         popstack(stmt, -1);
         if (!stmt->linenum) { execline(gprog, stmt); stmtinit(stmt); }
         break;

       case TOK_COLON:
         if ((stmt->metalist[stmt->metapos-1].operation == SETVAL_NUMERIC) ||
             (stmt->metalist[stmt->metapos-1].operation == GETVAL_NUMERIC) ||
             (stmt->metalist[stmt->metapos-1].operation == LABELREF)) {
           label = SafeMalloc(sizeof(struct labelset));
           sym = idx2sym(gprog, stmt->metalist[stmt->metapos-1].shortarg); 
           label->labelnum = addlabel(gprog, sym->name, stmt->linenum);
           for (x=0; x<MAX_STRING_LENGTH; x++)
             sym->name[x] = '\0';
           x = 0;
           stmt->metapos--;
           stmt->metalist[stmt->metapos].shortarg = 0;
           stmt->opcode = 0;
           stmt->labelset[stmt->numlabels] = label;
           stmt->numlabels++;
           firstvar = 1;
         } else {
           do {
             if (stmt->linenum == tempstmt->linenum) {
               for (x=0; x<strlen(input); x++) 
                 if (input[x] == ':') break;
               y = x;
               for (x=0; x<=y; x++) input[x] = ' ';
               cont = SafeMalloc(1024*64);
               *cont = 0;
               listline(cont, tempstmt, 1);
               strcat(cont, input);
               *prog = 0; 
               prog = cont;
               display = 1;
               goto begin;
             }
             tempstmt = tempstmt->nextstmt;
           } while (tempstmt != NULL);
           numlabels++;
         }
         goto loop;

       case TOK_COMMA:
         if (parencount > 0) { 
           // comma being delimiter for system functions
           popstack(stmt, -2);
           push("(", TOK_OPERATOR);
           if (chaninfo == 1) {
             if (!channum) {
               channum = 1;
               stmt->metalist[stmt->metapos].operation = 0xE1;
               stmt->metapos++;
             }
           } else { 
             if (special == 1) numargs[parencount-1]+=10;
             else numargs[parencount-1]++; 
             envinfo.stmttype = 0; 
           }
         } else {
           // comma being delimiter for verbs
           popstack(stmt, -1); 
           buffermeta(stmt, TOK_COMMA, token); 
           for (x=0; x<32; x++) numargs[x] = 0;
           envinfo.stmttype = 0;
           foundequals = 0;
           firstvar = 1;
         }
         goto loop;

       case TOK_ERROR:
         lineerror(stmt, __FILE__, __LINE__);
         return;

       case TOK_COMMAND:
         lastopcode = get_opcode(token);
         if (cmdtable[get_cmdindex(token)].options & IO_CHANNEL)
           watchchannel = 1;
         else watchchannel = 0;
         if (!strcmp(token, "IF")) ifcount++;
         if (!stmt->opcode) {
           stmt->opcode = get_opcode(token);
           envinfo.stmttype = 0;
           goto loop;
         } else {
           popstack(stmt, -1);
           envinfo.stmttype = 0;
           if (ifcount > 0) {
             stmt->metalist[stmt->metapos].operation = 0xE7;
             stmt->metalist[stmt->metapos].shortarg = 0;
             stmt->metalist[stmt->metapos].intarg = 0;
             stmt->metapos++;
             stmt->length++;
//             watchchannel = 0;
             chaninfo = 0;
             channum = 0;
             ifcount--;
           }
           if (lastopcode == CMD_ON) {
             special = 1;
             if (!strcmp(token, "GOTO")) 
               stmt->metalist[stmt->metapos].operation = 0x00F4;
             else stmt->metalist[stmt->metapos].operation = 0x01F4;
             stmt->length += 2;
             stmt->metapos++;
           } else
             buffermeta(stmt, TOK_COMMAND, token);
           goto loop;
         }
       break;
       
       case TOK_RESERVED:
         popstack(stmt, -1);
         envinfo.stmttype = 0;
         lineref = 0;
         if (!strcmp(token, "ELSE")) {
           stmt->metalist[stmt->metapos].operation = 0xE7;
           stmt->metalist[stmt->metapos].intarg = 0;
           stmt->metalist[stmt->metapos].shortarg = 0;
           stmt->metapos++;
           stmt->metalist[stmt->metapos].operation = 0xE2;
           stmt->metalist[stmt->metapos].intarg = 0;
           stmt->metalist[stmt->metapos].shortarg = 0;
           stmt->metapos++; 
           stmt->length+=2;
           envinfo.stmttype = 0;
           chaninfo = 0;
           channum = 0;
         }
         goto loop;

       case TOK_DONE:
         if ((stmt->linenum) && (!stmt->opcode)) deletestmt(gprog,stmt->linenum);
         else {
           popstack(stmt, -1);
           if (stmt->opcode == CMD_LET && foundequals == 0) lineerror(stmt, __FILE__, __LINE__);
           if (parencount > 0 || numlabels < 0) lineerror(stmt, __FILE__, __LINE__);
           if (stmt->linenum) { if (!stmt->errorflag) insertstmt(gprog, stmt); }
           else if (stmt->opcode) { if (!stmt->errorflag) 
                  { execline(gprog, stmt); } }
         }
         if (display) {
           GC_realloc(cont, strlen(cont));
           listprog(gprog, stmt->linenum, stmt->linenum);
         }
         return;

       case TOK_USERFUNCTION:
         if (lastopcode == CMD_DEFFN)
           addfunction(gprog, token, stmt->linenum, 1);
         else addfunction(gprog, token, stmt->linenum, 0);
         if (token[strlen(token)-1] == '$') parenstack[parencount] = 8;
           else parenstack[parencount] = 7;
         buffermeta(stmt, TOK_USERFUNCTION, token);
         break;

       case TOK_FUNCTION:
         lastfnc = get_fnc(token);
         numlabels++;
       case TOK_OPERATOR: 
         if (token[0] == '[') {
           special = 1;
           cont = get_symname(lastvar);
           if (cont[strlen(cont)-1] == '$') 
             parenstack[parencount] = 2;
           else parenstack[parencount] = 1;
           numlabels++;
           goto openparen;
         } else if (token[0] == ']') {  
           numargs[parencount-1] += 9;
           popstack(stmt, -2);
           envinfo.stmttype = parenstack[parencount];
           token_type = TOK_ARRAY;
           special = 0;
           goto loop;
         } else if (token[0] == '-') {
           // placeholder
           if ((lasttype == TOK_OPERATOR) || (lasttype == TOK_RESERVED))
             token[0] = '_';
           evalstack(stmt);
           goto loop;
         } else if (token[0] == '(') {
openparen:
           if (lasttype == TOK_COMMAND) {
             if (watchchannel == 1)
               chaninfo = 1;
           } else if (lasttype == TOK_ARRAY) {
             envinfo.stmttype = 0;
             numargs[parencount]++;
             push(token, token_type);
             goto loop;
           } else if (lasttype == TOK_SETVAL) { 
             push(get_symname(lastvar), TOK_SETVAL);
             stmt->metapos--;
             stmt->metalist[stmt->metapos].shortarg = 0; 
             numlabels++;
             cont = get_symname(lastvar);
             if (cont[strlen(cont)-1] == '$') 
               parenstack[parencount] = 2;
             else parenstack[parencount] = 1;
           } else if (lasttype == TOK_VARIABLE) {
             push(get_symname(lastvar), TOK_VARIABLE);
             numlabels++;
             stmt->metapos--;
             stmt->metalist[stmt->metapos].shortarg = 0; 
             cont = get_symname(lastvar);
             if (cont[strlen(cont)-1] == '$') 
               parenstack[parencount] = 2;
             else parenstack[parencount] = 1;
           } else if (lasttype == TOK_USERFUNCTION) {
             stmt->metapos--;
             stmt->metalist[stmt->metapos].operation = 0xF5;
             stmt->metapos++;
             stmt->length++;
           } else if (lasttype == TOK_FUNCTION) {
             if ((envinfo.stmttype != fnctable[lastfnc].returntype) &&
                 (envinfo.stmttype != 0))
               lineerror(stmt, __FILE__, __LINE__);
             else parenstack[parencount] = fnctable[lastfnc].returntype;
           } else parenstack[parencount] = 1;
           envinfo.stmttype = 0;
           numargs[parencount] = 1;
           push(token, token_type);
           goto loop;
         } else if (token[0] == ')') {
           if (parencount == 0) lineerror(stmt, __FILE__, __LINE__); 
           popstack(stmt, -2); 
           if (parenstack[parencount] == 7) {
             stmt->metalist[stmt->metapos].operation = 0xF8;
             stmt->metapos++;
             stmt->length++;
             envinfo.stmttype = 1;
           } else if (parenstack[parencount] == 8) {
             stmt->metalist[stmt->metapos].operation = 0xF8;
             stmt->metapos++;
             stmt->length++;
             envinfo.stmttype = 2;
           } else envinfo.stmttype = parenstack[parencount];
           if (lastopcode == CMD_DEFFN && foundequals == 0) {
             stmt->metapos--;
             stmt->metalist[stmt->metapos].operation = 0xF8;
             stmt->metapos++;
           }
           if (chaninfo == 1) {
             chaninfo = 0;
             if (!channum) {
               stmt->metalist[stmt->metapos].operation = 0xE1;
               stmt->metapos++;
             }
             stmt->metalist[stmt->metapos].operation = 0xF4F1;
             stmt->metapos++;
             numargs[parencount] = 0;
           }
           goto loop;
         } else { 
           evalstack(stmt); 
           goto loop; 
         }
         break;   

       case TOK_VARIABLE:
         if (get_sysvar(token)) { 
           buffermeta(stmt, TOK_SYSVAR, token); 
           numlabels++;
           goto loop;
         } else { addsymbol(gprog, token); lastvar = get_symref(token); }

       case TOK_NUMBER:
       default:
        if(!stmt->opcode) { envinfo.stmttype = 0; lastopcode = stmt->opcode = CMD_LET; }
        if ((firstvar == 1) && (token_type == TOK_VARIABLE) && 
            (lastopcode == CMD_LET || lastopcode == CMD_FOR ||
             lastopcode == CMD_FOR || lastopcode == CMD_NEXT ||
             lastopcode == CMD_DIM || lastopcode == CMD_INPUT)) {
           buffermeta(stmt, TOK_SETVAL, token);
           firstvar = 0;
           token_type = TOK_SETVAL;
        } else { numlabels++; buffermeta(stmt, token_type, token); }
        goto loop;
     }
  } while (1);
}

// recursively decides what to do based on status of stack
void evalstack(statement *stmt)
{
  stackobj *obj;

  if (stackempty()) 
    push(token, token_type);
  else {
     if (checkprec(token, token_type))
        push(token, token_type);
     else {
        obj = pop();
        buffermeta(stmt, obj->type, obj->name);
        evalstack(stmt);
        GC_free(obj);
     }
  }
}

void lineerror(statement *stmt, char *file, int line)
{
  int x, y = 0;

  parencount = 0;
  execcounter = 0;

  for (x=0; x<strlen(input); x++)
    if (!isdigit(input[x])) break;
  stmt->errorflag = 1;
  stmt->opcode = 0xF314;
  stmt->metalist[1].operation = 0x000F;
  if (stmt->linenum) {
    for (x=x+1; x<strlen(input); x++)
    { stmt->metalist[1].stringarg[y] = input[x]; y++; }
    insertstmt(gprog, stmt);
    listprog(gprog, stmt->linenum, stmt->linenum);
  } else {
    printf("*ERR ");
    for (x=0; x<tokenpos; x++) printf(" ");
    printf(" V\n");
    printf("00000 %s\n", input);
  }
  printf("(%s : %d)\n", file, line);
  longjmp(mark, 1);
}

int checkoption(int opidx, char *option, int type)
{
  int oplist = cmdtable[opidx].options;
  byte pass = 0;

  if (type != TOK_OPERATOR) pass = 1;

  if (!strcmp(option, "BNK")) {
    if (oplist & IO_BNK) pass = 1;
  } else if (!strcmp(option, "DOM")) {
    if (oplist & IO_DOM) pass = 1;
  } else if (!strcmp(option, "END")) {
    if (oplist & IO_END) pass = 1;
  } else if (!strcmp(option, "IND")) {
    if (oplist & IO_IND) pass = 1;
  } else if (!strcmp(option, "IOL")) {
    if (oplist & IO_IOL) pass = 1;
  } else if (!strcmp(option, "ISZ")) {
    if (oplist & IO_ISZ) pass = 1;
  } else if (!strcmp(option, "KEY")) {
    if (oplist & IO_KEY) pass = 1;
  } else if (!strcmp(option, "SIZ")) { 
    if (oplist & IO_SIZ) pass = 1;
  } else if (!strcmp(option, "TBL")) { 
    if (oplist & IO_TBL) pass = 1;
  } else if (!strcmp(option, "TIM")) {
    if (oplist & IO_TIM) pass = 1;
  } else if (!strcmp(option, "ERR")) {
    if (oplist & IO_ERR) pass = 1;
  } else if (!strcmp(option, "LEN")) {
    if (oplist & IO_LEN) pass = 1;
  } else if (!strcmp(option, "ATR")) {
    if (oplist & IO_ATR) pass = 1;
  } else if (!strcmp(option, "OPT")) {
    if (oplist & IO_OPT) pass = 1;
  } else if (!strcmp(option, "SEP")) {
    if (oplist & IO_SEP) pass = 1;
  } else if (!strcmp(option, "SRT")) {
    if (oplist & IO_SRT) pass = 1;
  } else pass = 1;

  if (pass == 0) return 0;

  return 1;
}
