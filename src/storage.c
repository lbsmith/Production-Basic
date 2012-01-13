/*
 * $Id: storage.c,v 1.34 2000/06/04 22:35:09 lees Exp $
 */

#include <sys/stat.h>
#include <string.h>
#include "basic.h"

extern tbfloat asc2flt(char *str);
extern int chaninfo;

extern int numargs[];
extern int foundequals;
extern int numlabels;

void resetdate();

void envinit()
{
  int x = 0;

  resetdate();

  str = SafeMalloc(sizeof(char)*14);

  envinfo.abort = 0;
  envinfo.gotoline = 0;
  envinfo.lasterror = 0;
  envinfo.allowesc = 1;
  envinfo.escline = 0;
  envinfo.allowexec = 1;
  envinfo.allowseterr = 1;
  envinfo.lasterrorline = 0;
  envinfo.lastseterrline = 0;

  envinfo.firstfile = SafeMalloc(sizeof(filetbl));
  envinfo.firstfile->type = 0;
  envinfo.firstfile->channel = 0;

  for (x=0; *fnctable[x].name; x++) envinfo.numfunctions++;
}

void storageinit(program *pgm)
{
  int x = 0;

  pgm->lastexec = 0;
  pgm->numfunctions = 0;
  pgm->numsymbols = 0;
  pgm->numlabels = 0;
  pgm->lengthprogram = 16; // filename + 2 end statements
  pgm->lengthsymtab = 1; // end 00 byte
  pgm->lengthlabeltab = 5;
  pgm->lengthfunctab = 5;
  pgm->signature = 0x40;

  pgm->firstsym = SafeMalloc(sizeof(symbol));
  pgm->firstsym->idx = 0;
  pgm->firstsym->name[0] = '\0';
  pgm->firstsym->nextsym = 0;

  pgm->nextprog = 0;

  for (x=0; x<32; x++) {
    pgm->userfunctions[x].lineref = 0;
    pgm->userfunctions[x].defined = 0;
    pgm->userfunctions[x].length = 0;
  }

  pgm->firststmt = SafeMalloc(sizeof(statement));
  pgm->firststmt->length = 5;
  pgm->firststmt->linenum = 0xFFFF; // last available line
  pgm->firststmt->opcode = CMD_END;
  pgm->firststmt->endcode = 0xF2;
}

void resetdate()
{
  envinfo.datestrings[0].name = "JANUARY";
  envinfo.datestrings[0].numdays = 31;
  envinfo.datestrings[1].name = "FEBRUARY";
  envinfo.datestrings[1].numdays = 28;
  envinfo.datestrings[2].name = "MARCH";
  envinfo.datestrings[2].numdays = 31;
  envinfo.datestrings[3].name = "APRIL";
  envinfo.datestrings[3].numdays = 30;
  envinfo.datestrings[4].name = "MAY";
  envinfo.datestrings[4].numdays = 31;
  envinfo.datestrings[5].name = "JUNE";
  envinfo.datestrings[5].numdays = 30;
  envinfo.datestrings[6].name = "JULY";
  envinfo.datestrings[6].numdays = 31;
  envinfo.datestrings[7].name = "AUGUST";
  envinfo.datestrings[7].numdays = 31;
  envinfo.datestrings[8].name = "SEPTEMBER";
  envinfo.datestrings[8].numdays = 30;
  envinfo.datestrings[9].name = "OCTOBER";
  envinfo.datestrings[9].numdays = 31;
  envinfo.datestrings[10].name = "NOVEMBER";
  envinfo.datestrings[10].numdays = 30;
  envinfo.datestrings[11].name = "DECEMBER";
  envinfo.datestrings[11].numdays = 31;
  envinfo.datestrings[12].name = "SUNDAY";
  envinfo.datestrings[13].name = "MONDAY";
  envinfo.datestrings[14].name = "TUESDAY";
  envinfo.datestrings[15].name = "WEDNESDAY";
  envinfo.datestrings[16].name = "THURSDAY";
  envinfo.datestrings[17].name = "FRIDAY";
  envinfo.datestrings[18].name = "SATURDAY";
}

void stmtinit(statement *stmt)
{
  lineref = 0;

  stmt->metapos = 0;
  stmt->linenum = 0;
  stmt->length = 4; // length byte and endbyte
  stmt->errorflag = 0;
  stmt->numlabels = 0;
  stmt->nextstmt = 0;
  stmt->prevstmt = 0;
  stmt->opcode = 0;
  stmt->endcode = 0xF2;
}

void buffermeta(statement *stmt, int type, char *c)
{
  int x = 0, y = 0, op = 0; 

  if (type == TOK_OPERATOR) {
    if ((chaninfo == 1) || (!strcmp(c, "ERR"))) x=0; else x=15;
    for (x=x; *optable[x].symbol; x++)
      if (!strcmp(c, optable[x].symbol)) op = optable[x].opcode; 
  }

  if ((op == OP_BNK) || (op == OP_DOM) || (op == OP_END) ||
      (op == OP_IND) || (op == OP_IOL) || (op == OP_ISZ) || (op == OP_KEY) ||
      (op == OP_SIZ) || (op == OP_TBL) || (op == OP_TIM) || (op == OP_ERR) ||
      (op == OP_LEN) || (op == OP_OPT) || (op == OP_ATR) || (op == OP_SRT) || (op == OP_SEP)) {
    stmt->metapos--;
    stmt->metalist[stmt->metapos].operation = 0;
  } else if (op == OP_EQUALS) {
    if ((lastopcode != CMD_LET) && (lastopcode != CMD_DEFFN) &&
        (lastopcode != CMD_FOR))
      op = OP_EQUALSCMP;
    else return; 
  } else if (op == OP_ADD) {
    if (stmt->metalist[stmt->metapos-1].operation == GETVAL_STRING ||
        stmt->metalist[stmt->metapos-1].operation==GETVAL_STRINGARRAY ||
        stmt->metalist[stmt->metapos-1].operation == SHORTLITERAL ||
        stmt->metalist[stmt->metapos-1].operation == HEXSTRING ||
        fnctable[get_fncidx(stmt->metalist[stmt->metapos-1].operation)].
        returntype == 2 || sysvartbl[get_sysvaridx(stmt->metalist[stmt->
        metapos-1].operation)].returntype == 2)
      op = OP_STRCAT;
  }

  if ((stmt->metalist[stmt->metapos-1].operation == 0x00F4) ||
      (stmt->metalist[stmt->metapos-1].operation == 0x01F4) ||
      (!strcmp(c, "GOTO")) || (!strcmp(c, "GOSUB")) || 
      (!strcmp(c, "SETERR")) || (!strcmp(c, "SETESC")) ||
      (!strcmp(c, "ON"))) lineref = 1;

  if (c[0] == '@') {
    if (numargs[parencount] == 2) 
      stmt->metalist[stmt->metapos].operation = 0xF7;
    else stmt->metalist[stmt->metapos].operation = 0xF6;
    stmt->metapos++; stmt->length++;
    numlabels = 0;
    return;
  } else if (c[0] == '_') op = OP_NEGATE;

  if (!op) {
    switch(type) {
      case TOK_SYSVAR:
        stmt->metalist[stmt->metapos].operation = sysvartbl[get_sysvar(c)].code;
        stmt->length++;
        if (parencount == 0) {
          if (envinfo.stmttype == 0) 
            envinfo.stmttype = sysvartbl[get_sysvar(c)].returntype;
          else if (envinfo.stmttype != sysvartbl[get_sysvar(c)].returntype)
            lineerror(stmt, __FILE__, __LINE__);
        }
        break;
      case TOK_USERFUNCTION:
        stmt->metalist[stmt->metapos].operation = 0x8DC9;
        stmt->metalist[stmt->metapos].shortarg = get_userfnc(c);
        stmt->length += 4;
        break;
      case TOK_FUNCTION:
        if (fnctable[get_fnc(c)].numparms == 99)
          stmt->metalist[stmt->metapos].intarg = numargs[parencount];
        stmt->metalist[stmt->metapos].operation = fnctable[get_fnc(c)].code;
        stmt->length++;
        break;
      case TOK_SETVAL:
        stmt->metalist[stmt->metapos].shortarg = get_symref(c);
        if (c[strlen(c)-1] == '$') {
          stmt->metalist[stmt->metapos].operation = SETVAL_STRING;
          if (envinfo.stmttype == 0) envinfo.stmttype = 2;
          else if (envinfo.stmttype != 2) lineerror(stmt, __FILE__, __LINE__); 
        } else {
          stmt->metalist[stmt->metapos].operation = SETVAL_NUMERIC;
          if (envinfo.stmttype == 0) envinfo.stmttype = 1;
          else if (envinfo.stmttype != 1) 
            lineerror(stmt, __FILE__, __LINE__); 
          if (numargs[parencount] >= 30) { 
            stmt->metalist[stmt->metapos].operation =
                                                 SETVAL_NUMERICARRAY;
            stmt->metalist[stmt->metapos].shortarg += 0x8000;
            numargs[parencount] -= 30;
          } else if (numargs[parencount] >= 20) {
            stmt->metalist[stmt->metapos].operation =
                                                 SETVAL_NUMERICARRAY;
            numargs[parencount] -= 20;
          } else if (numargs[parencount] >= 10) {
            stmt->metalist[stmt->metapos].shortarg += 0x8000;
            numargs[parencount] -= 10;
          }
          if (numargs[parencount] > 0) 
            lineerror(stmt, __FILE__, __LINE__);
          numargs[parencount] = 0;
        }
        if (stmt->metalist[stmt->metapos].operation == SETVAL_STRING) {
          if (numargs[parencount] >= 30) { 
            stmt->metalist[stmt->metapos].operation =SETVAL_STRINGARRAY;
            stmt->metalist[stmt->metapos].shortarg += 0x8000;
            numargs[parencount] -= 30;
          } else if (numargs[parencount] >= 20) {
            stmt->metalist[stmt->metapos].operation =SETVAL_STRINGARRAY;
            numargs[parencount] -= 20;
          } else if (numargs[parencount] >= 10) {
            stmt->metalist[stmt->metapos].shortarg += 0x8000;
            numargs[parencount] -= 10;
          }
          if (numargs[parencount] == 2) 
            stmt->metalist[stmt->metapos].shortarg += 0x4000;
          else if (numargs[parencount] == 1) 
            stmt->metalist[stmt->metapos].shortarg += 0x2000;     
          numargs[parencount] = 0;
        }
        stmt->length+=3;
        break;
      case TOK_SEMICOLON:
        stmt->metalist[stmt->metapos].operation = 0xE7;
        stmt->length++;
        break;
      case TOK_COMMA:
        if (lastopcode == CMD_PRINT || lastopcode == CMD_INPUT || lastopcode == CMD_IF)
          stmt->metalist[stmt->metapos].operation = 0xF1;
        else stmt->metalist[stmt->metapos].operation = 0xEC;
        stmt->length++;
        break;
      case TOK_COMMAND:
        stmt->metalist[stmt->metapos].operation = get_opcode(c);
        stmt->length++;
        break;
      case TOK_VARIABLE:
        if ((lastopcode == CMD_GOTO) || (lastopcode == CMD_GOSUB) ||
            (lastopcode == CMD_SETERR) || (lastopcode == CMD_SETESC)
            || (lineref == 1)) {
          addlabel(gprog, c, 0);
          stmt->metalist[stmt->metapos].operation = LABELREF;
          stmt->metalist[stmt->metapos].shortarg = get_labelref(c);
        } else if (c[strlen(c)-1] == '$') {
          if (c[0] == '$') {
            stmt->metalist[stmt->metapos].operation = HEXSTRING;
            delsymbol(gprog, get_symref(c));
            if (strlen(c) % 2) lineerror(stmt, __FILE__, __LINE__);
            x = 0;
            for (y=1; y<strlen(c)-1; y+=2) {              
              stmt->metalist[stmt->metapos].stringarg[x] = myhtoi(c[y], c[y+1]);
              x++;
	    }
            stmt->metalist[stmt->metapos].intarg = x;              
            stmt->length += 2+x;
            if (envinfo.stmttype == 0) envinfo.stmttype = 2;
            else if ((envinfo.stmttype != 2) && (chaninfo != 1)) 
              lineerror(stmt, __FILE__, __LINE__);
          } else {
            stmt->metalist[stmt->metapos].operation = GETVAL_STRING;
            stmt->metalist[stmt->metapos].shortarg = get_symref(c);
            if (envinfo.stmttype == 0) envinfo.stmttype = 2;
            else if (envinfo.stmttype != 2) lineerror(stmt, __FILE__, __LINE__);
          }
        } else {         
          stmt->metalist[stmt->metapos].operation = GETVAL_NUMERIC;
          stmt->metalist[stmt->metapos].shortarg  = get_symref(c);
          if (envinfo.stmttype == 0) envinfo.stmttype = 1;
          else if (envinfo.stmttype != 1) lineerror(stmt, __FILE__, __LINE__);
          if (numargs[parencount] >= 30) {
            stmt->metalist[stmt->metapos].operation = 
                                               GETVAL_NUMERICARRAY;
            stmt->metalist[stmt->metapos].shortarg += 0x8000;
            numargs[parencount] -= 30;
          } else if (numargs[parencount] >= 20) {
            stmt->metalist[stmt->metapos].operation = 
                                               GETVAL_NUMERICARRAY;
            numargs[parencount] -= 20;
          } else if (numargs[parencount] >= 10) {
            stmt->metalist[stmt->metapos].shortarg += 0x8000;
            numargs[parencount] -= 10;
          }
//          if (numargs[parencount] > 0) 
//            lineerror(stmt, __FILE__, __LINE__);
          numargs[parencount] = 0;
        }
        if (stmt->metalist[stmt->metapos].operation == GETVAL_STRING) {
          if (numargs[parencount] >= 30) { 
            stmt->metalist[stmt->metapos].operation =GETVAL_STRINGARRAY;
            stmt->metalist[stmt->metapos].shortarg += 0x8000;
            numargs[parencount] -= 30;
          } else if (numargs[parencount] >= 20) {
            stmt->metalist[stmt->metapos].operation =GETVAL_STRINGARRAY;
            numargs[parencount] -= 20;
          } else if (numargs[parencount] >= 10) {
            stmt->metalist[stmt->metapos].shortarg += 0x8000;
            numargs[parencount] -= 10;
          }
          if (numargs[parencount] == 2) 
            stmt->metalist[stmt->metapos].shortarg += 0x4000;
          else if (numargs[parencount] == 1) 
            stmt->metalist[stmt->metapos].shortarg += 0x2000;     
          numargs[parencount] = 0;
        }
        stmt->length+=3;
        break;
      case TOK_NUMBER:
        if (((lastopcode == CMD_GOTO) ||(lastopcode == CMD_GOSUB) ||
            (lastopcode == CMD_SETESC) ||
            (lastopcode == CMD_SETERR)) || (lineref == 1)) {
          stmt->metalist[stmt->metapos].operation = LINEREF;
          stmt->metalist[stmt->metapos].shortarg = atoi(c);
          stmt->length+=3;
        } else {
          stmt->metalist[stmt->metapos].operation = FLOAT;
          stmt->metalist[stmt->metapos].floatarg = asc2flt(c);
          if (envinfo.stmttype == 0) envinfo.stmttype = 1;
          else if (envinfo.stmttype != 1) lineerror(stmt, __FILE__, __LINE__);
        }
        break;
      case TOK_STRING:
        stmt->metalist[stmt->metapos].operation = SHORTLITERAL;
        stmt->metalist[stmt->metapos].intarg = strlen(c);
        stmt->length+=2;
        for (y=0; y<strlen(c); y++)
          stmt->metalist[stmt->metapos].stringarg[y] = c[y];
        if (envinfo.stmttype == 0) envinfo.stmttype = 2;
        else if ((envinfo.stmttype != 2) && (chaninfo != 1)) lineerror(stmt, __FILE__, __LINE__);
        stmt->length+=strlen(c);
        break;
      case TOK_MNEMONIC:
        stmt->metalist[stmt->metapos].operation = MNEMONICREF;
        for (y=0; y<strlen(c); y++)
          stmt->metalist[stmt->metapos].stringarg[y] = c[y];
        if (strlen(c) > 2) 
          { stmt->metalist[stmt->metapos].intarg = strlen(c); stmt->length++; }
        stmt->length+=strlen(c);
        break;
      default:
        stmt->length++;
        break;
    }
  } else {
    stmt->metalist[stmt->metapos].operation = op;
    stmt->length++;
  }
  stmt->metapos++;
}

int addfunction(program *pgm, char *name, unsigned short lineref, int defined)
{
  int x;

  for (x=1; x<pgm->numfunctions+1; x++) {
    if (!strcmp(name, pgm->userfunctions[x].name)) {
      if ((defined == 1) && (pgm->userfunctions[x].defined == 1)) 
        rterror(ERR_FUNC_NAME_ALREADY_EXISTS);
      else if (defined == 1) {
        pgm->userfunctions[x].lineref = lineref;
        pgm->userfunctions[x].defined = 1; 
      }
      return x;
    }
  }

  pgm->numfunctions++;

  for (x=0; x<strlen(name); x++)    
    pgm->userfunctions[pgm->numfunctions].name[x] = name[x];
  for (x=x; x<MAX_STRING_LENGTH; x++)
    pgm->userfunctions[pgm->numfunctions].name[x] = '\0';
  pgm->userfunctions[pgm->numfunctions].length = strlen(name);
  pgm->userfunctions[pgm->numfunctions].lineref = lineref;
  pgm->lengthfunctab += strlen(name)+3;
  if (defined) pgm->userfunctions[pgm->numfunctions].defined = 1;

  return 0;
}

void delfunction(program *pgm, int num)
{
  unsigned int x;
  statement *tempstmt = pgm->firststmt;

  do {
    for (x=0; x<tempstmt->metapos; x++) {
      if (tempstmt->metalist[x].operation == 0x8DC9) {
        if (tempstmt->metalist[x].shortarg > num)
          tempstmt->metalist[x].shortarg--;
      }
    } 
    tempstmt = tempstmt->nextstmt;
  } while (tempstmt != NULL);

  for (x=num; x<pgm->numfunctions; x++)
    pgm->userfunctions[x] = pgm->userfunctions[x+1];
  pgm->numfunctions--;
}

void dellabel(program *pgm, int num)
{
  unsigned int x;
  statement *tempstmt = pgm->firststmt;

  do {
    for (x=0; x<tempstmt->metapos; x++) {
      if (tempstmt->metalist[x].operation == LABELREF) {
        if (tempstmt->metalist[x].shortarg > num)
          tempstmt->metalist[x].shortarg--;
      }
    }
    tempstmt = tempstmt->nextstmt;
  } while (tempstmt != NULL);

  for (x=num; x<pgm->numlabels; x++)
    pgm->labels[x] = pgm->labels[x+1];
  pgm->numlabels--;
}

symbol *addsymbol(program *pgm, char *sym)
{
  int x = 0;
  symbol *tmplast, *tmp;
  symbol *newsym;

  tmplast = tmp = pgm->firstsym;

  do {
    if (!strcmp(sym, tmp->name)) return tmp;
    tmplast = tmp;
    tmp = tmp->nextsym;
    x++;
  } while (tmp != NULL);

  newsym = SafeMalloc(sizeof(symbol));

  pgm->numsymbols++;
  newsym->idx = x;
  for (x=0; x<strlen(sym); x++)
    newsym->name[x] = sym[x];
  newsym->name[x] = '\0';

  newsym->length = strlen(sym);
  pgm->lengthsymtab += strlen(sym)+1; // length byte
 
  newsym->nextsym = NULL;
  tmplast->nextsym = newsym;

  return newsym;
}

void delsymbol(program *pgm, int num)
{
  unsigned int x;
  statement *tempstmt = pgm->firststmt;
  symbol *tempsym, *templast;

  tempsym = templast = pgm->firstsym;
 
  do {
    for (x=0; x<tempstmt->metapos; x++) {
      if (tempstmt->metalist[x].operation == GETVAL_STRING ||
          tempstmt->metalist[x].operation == GETVAL_NUMERIC ||
          tempstmt->metalist[x].operation == GETVAL_NUMERICARRAY ||
          tempstmt->metalist[x].operation == SETVAL_STRING ||
          tempstmt->metalist[x].operation == SETVAL_NUMERIC ||
          tempstmt->metalist[x].operation == SETVAL_NUMERICARRAY ||
          tempstmt->metalist[x].operation == GETVAL_STRINGARRAY ||
          tempstmt->metalist[x].operation == SETVAL_STRINGARRAY) {
        if (tempstmt->metalist[x].shortarg > num)
          tempstmt->metalist[x].shortarg--;
      }
    }
    tempstmt = tempstmt->nextstmt;
  } while (tempstmt != NULL);

  for (x=0; x<num; x++) {
    templast = tempsym;
    tempsym = tempsym->nextsym;
  }

  templast->nextsym = tempsym->nextsym;
 
  pgm->numsymbols--;
}

int addlabel(program *pgm, char *sym, int lineref)
{
  int x;
  linelabel label;

  if (lineref == 0) {
    x = get_labelref(sym);
    if (x) { pgm->labels[x].numrefs++; return x; }
  }

  for (x=1; x<pgm->numlabels+1; x++) {
    if (!strcmp(sym, pgm->labels[x].name)) {
      pgm->labels[x].lineref = lineref;
      pgm->labels[x].defined = 1;
      pgm->labels[x].numrefs++;
      return x;
    }
  }
  
  pgm->numlabels++;

  for (x=0; x<strlen(sym); x++)
    label.name[x] = sym[x];

  label.name[x] = '\0';
  label.length = strlen(sym);
  label.lineref = lineref;
  if (lineref == 0) label.defined = 0; else label.defined = 1;
  label.numrefs = 1; 
 
  pgm->labels[pgm->numlabels] = label;
  pgm->lengthlabeltab += strlen(sym)+5;
  
  return pgm->numlabels;
}

void insertstmt(program *pgm, statement *stmt)
{
  statement *tempstmt, *templast;
  symbol *tempsym;
  unsigned int x, y;

  for (x=0; x<stmt->metapos; x++) {
    if (stmt->metalist[x].operation == GETVAL_NUMERIC ||
        stmt->metalist[x].operation == GETVAL_NUMERICARRAY ||
        stmt->metalist[x].operation == SETVAL_NUMERIC ||
        stmt->metalist[x].operation == SETVAL_NUMERICARRAY ||
        stmt->metalist[x].operation == GETVAL_STRINGARRAY ||
        stmt->metalist[x].operation == SETVAL_STRINGARRAY ||
        stmt->metalist[x].operation == GETVAL_STRING ||
        stmt->metalist[x].operation == SETVAL_STRING) {
      y = stmt->metalist[x].shortarg;
      if (stmt->metalist[x].shortarg & 0x2000) 
        y -= 0x2000;
      if (stmt->metalist[x].shortarg & 0x4000) 
        y -= 0x4000;
      if (stmt->metalist[x].shortarg & 0x8000) 
        y -= 0x8000;
      tempsym = idx2sym(gprog, y);
      tempsym->numrefs++;
    } else if (stmt->metalist[x].operation == 0x8DC9) {
      pgm->userfunctions[stmt->metalist[x].shortarg].numrefs++;
    }
  }
 tempstmt = templast = pgm->firststmt;

 if (stmt->linenum < pgm->firststmt->linenum) {
    stmt->nextstmt = pgm->firststmt;
    stmt->prevstmt = 0;
    pgm->firststmt = stmt;
    return;               
  }
  if (stmt->linenum == pgm->firststmt->linenum) {
    stmt->nextstmt = pgm->firststmt->nextstmt;
    stmt->prevstmt = 0;
    deletestmt(pgm, stmt->linenum);
    pgm->firststmt = stmt;  
    return;
  }
  do {
    if (tempstmt->linenum > stmt->linenum) {
      templast->nextstmt = stmt;
      stmt->prevstmt = templast;
      stmt->nextstmt = tempstmt;
      return;
    }
    if (tempstmt->linenum == stmt->linenum) {
      deletestmt(pgm, stmt->linenum);
      templast->nextstmt = stmt;
      stmt->prevstmt = templast->prevstmt;
      stmt->nextstmt = tempstmt->nextstmt;
      return;
    }
    templast = tempstmt;
    tempstmt = tempstmt->nextstmt;
  } while (tempstmt != NULL);
  templast->nextstmt = stmt;
}

void deletestmt(program *pgm, unsigned int line)
{
  statement *stmttemp, *stmtlast;
  symbol *tempsym;
  unsigned int x, y;
 
  stmttemp = pgm->firststmt;
  stmtlast = pgm->firststmt;

  do {
    if (stmttemp->linenum == line) {
      if (stmttemp->numlabels) {
        for (x=0; x<stmttemp->numlabels; x++) {
          pgm->labels[stmttemp->labelset[x]->labelnum].numrefs--;
        }
      }
      for (x=0; x<stmttemp->metapos; x++) {        
        if (stmttemp->metalist[x].operation == GETVAL_NUMERIC ||
            stmttemp->metalist[x].operation == GETVAL_NUMERICARRAY ||
            stmttemp->metalist[x].operation == SETVAL_NUMERIC ||
            stmttemp->metalist[x].operation == SETVAL_NUMERICARRAY ||
            stmttemp->metalist[x].operation == GETVAL_STRING ||
            stmttemp->metalist[x].operation == SETVAL_STRING ||
            stmttemp->metalist[x].operation == GETVAL_STRINGARRAY ||
            stmttemp->metalist[x].operation == SETVAL_STRINGARRAY) {
          y = stmttemp->metalist[x].shortarg;
          if (stmttemp->metalist[x].shortarg & 0x2000) 
            y -= 0x2000;
          if (stmttemp->metalist[x].shortarg & 0x4000) 
            y -= 0x4000;
          if (stmttemp->metalist[x].shortarg & 0x8000) 
            y -= 0x8000;
          tempsym = idx2sym(gprog, y);
          tempsym->numrefs--;
        } else if (stmttemp->metalist[x].operation == 0x8DC9) {
          pgm->userfunctions[stmttemp->metalist[x].shortarg].numrefs--;
        } else if (stmttemp->metalist[x].operation == LABELREF) {
          pgm->labels[stmttemp->metalist[x].shortarg].numrefs--;
        }
        if ((stmttemp->opcode == CMD_DEFFN) &&
            (stmttemp->metalist[x].operation == 0x8DC9))
          pgm->userfunctions[stmttemp->metalist[x].shortarg].defined = 0;
      }
      if (stmttemp == pgm->firststmt)
        pgm->firststmt = stmttemp->nextstmt;
      else stmtlast->nextstmt = stmttemp->nextstmt;
      GC_free(stmttemp);
      return;
    }
    stmtlast = stmttemp;
    stmttemp = stmttemp->nextstmt;
  } while (stmttemp != NULL);
}

void runprog(program *pgm, unsigned int startline)
{
  statement *curstmt = pgm->firststmt;

  do {
skip:
    if (curstmt->linenum >= startline) {
      pgm->lastexec = curstmt->linenum;
      envinfo.gotoline = 0;
      if (!execline(pgm, curstmt)) goto done;
      if (envinfo.allowexec == 0)
       { envinfo.gotoline = pgm->lastexec; envinfo.allowexec = 1; return; }
      if (envinfo.gotoline > 0) { 
         startline = envinfo.gotoline; 
         envinfo.gotoline = 0;
         curstmt = pgm->firststmt; 
         goto skip;
      } 
    }
    curstmt = curstmt->nextstmt;
  } while (curstmt != NULL);
done:
  pgm->lastexec = 0;
  if (pgm == gprog) printf("\nREADY\n");
}

int writeall(program *pgm, char *filename)
{
  FILE *fp;
  unsigned int x, y;
  statement *tempstmt;
  symbol *tempsym;
  char *tmp;

  tempstmt = pgm->firststmt;
 
  pgm->filename = filename;
 
  if ((fp = fopen(filename, "wb")) == NULL) 
  { rterror(ERR_OUT_OF_DISK_SPACE); return 0; }

  // scan through all symbol table entries, deleting unreferenced entries
restart:
  for (x=1; x<pgm->numsymbols+1; x++) 
  { 
    tempsym = idx2sym(pgm, x); 
    if (!tempsym) { pgm->numsymbols = x; break; }
    if (tempsym->numrefs == 0) { 
      delsymbol(pgm, x); 
      goto restart; 
    } 
  }
restart2:
  for (x=1; x<pgm->numfunctions+1; x++)
  { if (pgm->userfunctions[x].numrefs == 0) { delfunction(pgm, x); goto restart2; } }
restart3:
  for (x=1; x<pgm->numlabels+1; x++)
  { if (pgm->labels[x].numrefs == 0) { dellabel(pgm, x); goto restart3; } }
  //  TBred program header
  for (x=0; x<26; x++) WriteByte(fp, 0x00);
  fwrite(&pgm->signature, sizeof(short), 1, fp);
  WriteShort(fp, pgm->numsymbols);
  WriteShort(fp, pgm->dummy);
  WriteShort(fp, pgm->lengthprogram);
  if (pgm->numlabels) WriteShort(fp, 0x8000);
  else WriteShort(fp, 0x0000);
  WriteShort(fp, pgm->lengthsymtab);
  WriteByte(fp, pgm->dummy);
  WriteShort(fp, pgm->lengthprogram + pgm->numsymbols);
  tmp = SafeMalloc(strlen(filename));
  ExtractFilename(filename, tmp);
  while (strlen(tmp) < 8) strcat(tmp, " "); // pad name with spaces
  fwrite(tmp, 8, 1, fp);
  GC_free(tmp);
  WriteByte(fp, pgm->numfunctions);
  
  do {
    if (tempstmt->opcode == 0xF314) {
      WriteShort(fp, 0xF314);
      WriteShort(fp, tempstmt->linenum);
      WriteShort(fp, tempstmt->metalist[1].operation);
      fwrite(&tempstmt->metalist[1].stringarg, 
        strlen(tempstmt->metalist[1].stringarg), 1, fp);
      WriteByte(fp, 0x00);
      goto getnext;
    }
    WriteByte(fp, tempstmt->length);
    WriteShort(fp, tempstmt->linenum);
    for (y=0; y<tempstmt->numlabels; y++) {
      WriteShort(fp, 0x8E6B);
      WriteShort(fp, 0xF4A1);
      WriteShort(fp, tempstmt->labelset[y]->labelnum);
    }      
    if (tempstmt->opcode < 0x8E00)
      WriteByte(fp, tempstmt->opcode);
    else WriteShort(fp, tempstmt->opcode);
    for (y=1; y<tempstmt->metapos; y++) {
      if ((tempstmt->metalist[y].operation > 0x8D00 &&
           tempstmt->metalist[y].operation < 0x8F00) ||
           tempstmt->metalist[y].operation == 0xF4F1 ||
           tempstmt->metalist[y].operation == 0x00F4 ||
           tempstmt->metalist[y].operation == 0x01F4)
        WriteShort(fp, tempstmt->metalist[y].operation);
      else WriteByte(fp, tempstmt->metalist[y].operation);
      if (tempstmt->metalist[y].intarg)
        WriteByte(fp, tempstmt->metalist[y].intarg);
      if (tempstmt->metalist[y].operation == FLOAT) {
        if (!tempstmt->metalist[y].floatarg.exp) {
          if (tempstmt->metalist[y].floatarg.mantisa.i <= 255) {
            fseek(fp, -1, SEEK_CUR); WriteByte(fp, INTEGER);
            WriteByte(fp, tempstmt->metalist[y].floatarg.mantisa.i);
          } else {
            fseek(fp, -1, SEEK_CUR); WriteByte(fp, LONGINTEGER);
            WriteLong(fp, tempstmt->metalist[y].floatarg.mantisa.i);
          }
        } else if (tempstmt->metalist[y].floatarg.exp == -2) {
          fseek(fp, -1, SEEK_CUR); WriteByte(fp, FLOATIMPLIED);
          WriteLong(fp, tempstmt->metalist[y].floatarg.mantisa.i);
        } else {
          WriteByte(fp, tempstmt->metalist[y].floatarg.exp);
          WriteShort(fp, 0x0000);
          WriteLong(fp, tempstmt->metalist[y].floatarg.mantisa.i);
        }
      }
      if (tempstmt->metalist[y].shortarg)
        WriteShort(fp, tempstmt->metalist[y].shortarg);
      if (tempstmt->metalist[y].stringarg[0]) {
        if (tempstmt->metalist[y].operation == MNEMONICREF && 
           (strlen(tempstmt->metalist[y].stringarg) <= 2))
          fwrite(&tempstmt->metalist[y].stringarg, 2, 1, fp);
        else if(tempstmt->opcode == CMD_REM)
          fwrite(&tempstmt->metalist[y].stringarg, 
             tempstmt->metalist[y].shortarg, 1, fp);
        else fwrite(&tempstmt->metalist[y].stringarg,
             tempstmt->metalist[y].intarg, 1, fp);     
      }
    }
    WriteByte(fp, tempstmt->endcode);
getnext:
    tempstmt = tempstmt->nextstmt;
  } while (tempstmt != NULL);

  // TBred always appends this at the end of all statements
  WriteByte(fp, 0x03);
  WriteShort(fp, 0x00);

  if (pgm->numsymbols) {
    tempsym = pgm->firstsym;
    do {
     if (tempsym->length > 0) {
       WriteByte(fp, tempsym->length);
       fwrite(&tempsym->name, tempsym->length, 1, fp);
     }
     tempsym = tempsym->nextsym;
    } while (tempsym != NULL);
    WriteByte(fp, 0x00);
  }

  if (pgm->numlabels) {
    WriteShort(fp, 0xFFFF);
    WriteShort(fp, pgm->lengthlabeltab);
    for (x=1; x<pgm->numlabels+1; x++) {
      WriteByte(fp, pgm->labels[x].length);
      fwrite(&pgm->labels[x].name, pgm->labels[x].length, 1, fp);
      if (pgm->labels[x].defined == 1)
        WriteShort(fp, pgm->labels[x].lineref);
      else WriteShort(fp, 0x0000);
      WriteShort(fp, x);
    }
    WriteByte(fp, 0x00);
  }
 
  if (pgm->numfunctions) {
    WriteShort(fp, 0xFFFE);
    WriteShort(fp, pgm->lengthfunctab);
    for (x=1; x<pgm->numfunctions+1; x++) {
      WriteByte(fp, pgm->userfunctions[x].length);
      fwrite(pgm->userfunctions[x].name, pgm->userfunctions[x].length, 1, fp);
      if (pgm->userfunctions[x].defined == 1)
        WriteShort(fp, pgm->userfunctions[x].lineref);     
      else WriteShort(fp, 0x0000);
    }
    WriteByte(fp, 0x00);
  }

  fclose(fp);

  chmod(filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

  return 1;
}

int loadall(program *pgm, char *filename)
{ 
  FILE *fp;
  unsigned int laststmt = 0, x = 0, y = 0, labeltab = 0;
  char tmp[9], c, *test;
  statement *tempstmt = 0, *templast = 0;
  struct labelset *label = 0;
  symbol *tempsym;

  if ((fp = fopen(filename, "rb")) == NULL) 
  { rterror(ERR_BAD_FILEID); return 0; }

  fseek(fp, 41, SEEK_SET);
  for (x=0; x<9; x++) tmp[x] = '\0';
  fread(&tmp, 8, 1, fp);
 
  test = SafeMalloc(strlen(filename));
  ExtractFilename(filename, test);
  while (strlen(test) < 8) strcat(test, " ");

  if (strncmp(test, tmp, 8)) {
    GC_free(test); 
    rterror(ERR_PROGRAM_FORMAT_INCORRECT);
  }
  GC_free(test);

  storageinit(pgm);

  pgm->filename = filename;
  pgm->firststmt = NULL;

  fseek(fp, 26, SEEK_SET);
  fread(&pgm->signature, sizeof(short), 1, fp);
  pgm->numsymbols = ReadShort(fp);

  ReadShort(fp);
  pgm->lengthprogram = ReadShort(fp);
  labeltab = ReadShort(fp);
  pgm->lengthsymtab = ReadShort(fp);
  ReadByte(fp);
  ReadShort(fp);

  ReadShort(fp); // these 4 are to read the filename
  ReadShort(fp); // but the filename has already been handled
  ReadShort(fp); // so just skip over them
  ReadShort(fp);

  pgm->numfunctions = ReadByte(fp);

  do {
    tempstmt = SafeMalloc(sizeof(statement));
    stmtinit(tempstmt);

    tempstmt->length = ReadByte(fp);
    // statement in error, no length byte or endbyte
    if (tempstmt->length == 0xF3) {
      fseek(fp, -1, SEEK_CUR);
      tempstmt->length = ReadShort(fp);
      if (tempstmt->length == 0x14) { 
        tempstmt->opcode = 0xF314;
        tempstmt->errorflag = 1;
        tempstmt->linenum = ReadShort(fp);
        tempstmt->metalist[1].operation = ReadShort(fp);
        x = 0;
        do {
          c = ReadByte(fp);
          tempstmt->metalist[1].stringarg[x] = c;
          x++;
        } while (c != 0x00);
        goto getnext;
      } else { fseek(fp, -1, SEEK_CUR); tempstmt->length = 0xF3; }
    }
    tempstmt->linenum = ReadShort(fp);
    if (tempstmt->linenum == 0xFFFF) laststmt = 1;
getopcode:
    tempstmt->opcode = ReadByte(fp);
    if (tempstmt->opcode == 0x8E) {
      fseek(fp, -1, SEEK_CUR);
      tempstmt->opcode = ReadShort(fp);
      if (tempstmt->opcode == 0x8E6B) {
        label = SafeMalloc(sizeof(struct labelset)); 
        ReadShort(fp);
        label->labelnum = ReadShort(fp);
        tempstmt->labelset[tempstmt->numlabels] = label;
        tempstmt->numlabels++;
        goto getopcode;
      }
    }
    for (y=1; y<MAX_STMT_METAS; y++) {
      tempstmt->metalist[y].operation = ReadByte(fp);
      tempstmt->metapos++;
      if (tempstmt->metalist[y].operation == 0xF2) break;
      if (tempstmt->metalist[y].operation == 0x8D ||
          tempstmt->metalist[y].operation == 0x8E ||
          tempstmt->metalist[y].operation == 0xF4 ||
          tempstmt->metalist[y].operation == 0x00 ||
          tempstmt->metalist[y].operation == 0x01) {
        fseek(fp, -1, SEEK_CUR);
        tempstmt->metalist[y].operation = ReadShort(fp);
      }
      if (get_fncname(tempstmt->metalist[y].operation)) {
        if (fnctable[get_fnc(get_fncname(tempstmt->metalist[y].operation))].
           numparms == 99) tempstmt->metalist[y].intarg = ReadByte(fp);
      } else switch(tempstmt->metalist[y].operation) {
        case 0x8DC9:
          tempstmt->metalist[y].shortarg = ReadShort(fp);
          break;
        case 0xF5: 
          if (tempstmt->metalist[y-1].operation != 0x8DC9) {
            tempstmt->metalist[y].shortarg = ReadShort(fp);
            fread(&tempstmt->metalist[y].stringarg, 
              tempstmt->metalist[y].shortarg, 1, fp); 
          }
          break;
        case MNEMONICREF:
          if (isgraph(ReadByte(fp))) {
            fseek(fp, -1, SEEK_CUR);
            fread(&tempstmt->metalist[y].stringarg, 2, 1, fp);
          } else {
            fseek(fp, -1, SEEK_CUR);
            tempstmt->metalist[y].intarg = ReadByte(fp);
            fread(&tempstmt->metalist[y].stringarg, 
                   tempstmt->metalist[y].intarg, 1, fp);
          }
          break;
        case LABELREF:
        case LINEREF:
          tempstmt->metalist[y].shortarg = ReadShort(fp);
          break;
        case INTEGER:
          tempstmt->metalist[y].operation = FLOAT;
          tempstmt->metalist[y].floatarg.exp = 0;
          tempstmt->metalist[y].floatarg.mantisa.i = (int)ReadByte(fp);
          break;
        case LONGINTEGER:
          tempstmt->metalist[y].operation = FLOAT;
          tempstmt->metalist[y].floatarg.exp = 0;
          tempstmt->metalist[y].floatarg.mantisa.i = ReadLong(fp);
          break;
        case FLOAT:
          tempstmt->metalist[y].floatarg.exp = ReadByte(fp);
          ReadShort(fp);
          tempstmt->metalist[y].floatarg.mantisa.i = ReadLong(fp);
          break;
        case FLOATIMPLIED:
          tempstmt->metalist[y].operation = FLOAT;
          tempstmt->metalist[y].floatarg.exp = -2;
          tempstmt->metalist[y].floatarg.mantisa.i = ReadLong(fp);
          break;
        case SHORTLITERAL:
        case HEXSTRING:
          tempstmt->metalist[y].intarg = ReadByte(fp);
          fread(&tempstmt->metalist[y].stringarg, 
                 tempstmt->metalist[y].intarg, 1, fp);
          break;
        case GETVAL_NUMERIC:
        case SETVAL_NUMERIC:
        case GETVAL_NUMERICARRAY:
        case SETVAL_NUMERICARRAY:
        case GETVAL_STRING:
        case SETVAL_STRING:
        case GETVAL_STRINGARRAY:
        case SETVAL_STRINGARRAY:
          tempstmt->metalist[y].shortarg = ReadShort(fp);
          break;
      }
    }
getnext:
    if (!pgm->firststmt) pgm->firststmt = tempstmt;
    else templast->nextstmt = tempstmt;
    templast = tempstmt;
    if (laststmt) { ReadByte(fp); ReadShort(fp); break; }
  } while (1);

  if (pgm->numsymbols) {
    for (x=1; x<pgm->numsymbols+1; x++) {
      y = ReadByte(fp);
      test = SafeMalloc(y); 
      fread(test, y, 1, fp);
      addsymbol(pgm, test);
    }
    pgm->numsymbols = (pgm->numsymbols * 2) - pgm->numsymbols;
    ReadByte(fp);
  }

  if (labeltab) {
    ReadShort(fp);
    pgm->lengthlabeltab = ReadShort(fp);    
    do {
      pgm->numlabels++;
      pgm->labels[pgm->numlabels].length = ReadByte(fp);
      if (pgm->labels[pgm->numlabels].length == 0x00) {
        pgm->numlabels--;
        break;
      }
      fread(&pgm->labels[pgm->numlabels].name, 
             pgm->labels[pgm->numlabels].length, 1, fp); 
      pgm->labels[pgm->numlabels].name[pgm->labels[pgm->numlabels].length+1] = '\0';
      pgm->labels[pgm->numlabels].lineref = ReadShort(fp);
      if (pgm->labels[pgm->numlabels].lineref != 0) 
        pgm->labels[pgm->numlabels].defined = 1;
      else pgm->labels[pgm->numlabels].defined = 0;
      ReadShort(fp);
      pgm->labels[pgm->numlabels].numrefs = 1;
    } while (1); 
  }

  if (pgm->numfunctions) {
    ReadByte(fp);
    pgm->lengthfunctab = ReadShort(fp);    
    for (x=1; x<pgm->numfunctions+1; x++) {
      pgm->userfunctions[x].length = ReadByte(fp);
      fread(&pgm->userfunctions[x].name, pgm->userfunctions[x].length, 1,  fp);
      pgm->userfunctions[x].lineref = ReadShort(fp);
      if (pgm->userfunctions[x].lineref != 0) pgm->userfunctions[x].defined = 1;
    }
    ReadByte(fp);
  }

  tempstmt = pgm->firststmt;
  do {
    for (x=0; x<tempstmt->metapos; x++) {
      if (tempstmt->metalist[x].operation == LABELREF) 
        pgm->labels[tempstmt->metalist[x].shortarg].numrefs++;
      if (tempstmt->metalist[x].operation == 0x8DC9)
        pgm->userfunctions[tempstmt->metalist[x].shortarg].numrefs++;
      if (tempstmt->metalist[x].operation == GETVAL_STRING ||
          tempstmt->metalist[x].operation == SETVAL_STRING ||
          tempstmt->metalist[x].operation == GETVAL_STRINGARRAY ||
          tempstmt->metalist[x].operation == SETVAL_STRINGARRAY ||
          tempstmt->metalist[x].operation == GETVAL_NUMERIC ||
          tempstmt->metalist[x].operation == SETVAL_NUMERIC) {
        tempsym = idx2sym(pgm, tempstmt->metalist[x].shortarg);
        tempsym->numrefs++;
      }
    }
    tempstmt=tempstmt->nextstmt;
  } while (tempstmt != NULL);

  fclose(fp);

  return 1;
}

void closefiles()
{
  filetbl *tmp;

  if (!envinfo.firstfile) return;

  tmp = envinfo.firstfile;

  do {
    fclose(tmp->fp);
    tmp = tmp->nextfile;
  } while (tmp != NULL);
}
