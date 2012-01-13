/*
 * $Id: runtime.c,v 1.34 2000/06/04 22:35:31 lees Exp $
 */

#include "basic.h"
#include <sys/stat.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>

extern int execcounter;

jmp_buf abortmark;

void whilepush(int value);
int whilepop(void);

void forpush(forstruct x);
forstruct forpop(void);

void gosubpush(int value);
int gosubpop(void);

tbfloat add(tbfloat flt1, tbfloat flt2);
void exec_dumpstmt(program *pgm, statement *stmt);
void exec_let(program *pgm, statement *stmt);
int exec_print(statement *stmt);
int exec_goto(program *pgm, statement *stmt);
int exec_wait(statement *stmt);
int exec_load(program *pgm, statement *stmt);
int exec_save(program *pgm, statement *stmt);
void exec_list(program *pgm, statement *stmt);
int exec_run(program *pgm, statement *stmt);
void exec_delete(program *pgm, statement *stmt);
int exec_gosub(program *pgm, statement *stmt);
int exec_return(statement *stmt);
int exec_for(program *pgm, statement *stmt);
int exec_next(program *pgm, statement *stmt);
int exec_if(program *pgm, statement *stmt);
int exec_while(program *pgm, statement *stmt);
int exec_wend(statement *stmt);
int exec_input(program *pgm, statement *stmt);
int exec_setesc(statement *stmt);
int exec_erase(statement *stmt);
int exec_open(statement *stmt);
int exec_close(statement *stmt);
int exec_dump(program *pgm, statement *stmt);
int exec_text(statement *stmt);
int exec_indexed(statement *stmt);
int exec_readrecord(program *pgm, statement *stmt, byte update);
int exec_writerecord(statement *stmt);
int exec_system(statement *stmt);
void exec_call(program *pgm, statement *stmt);
void exec_enter(program *pgm, statement *stmt);
void exec_exit(program *pgm, statement *stmt);
void exec_rename(statement *stmt);
int exec_seterr(program *pgm, statement *stmt);
void exec_edit(program *pgm, statement *stmt);
void exec_write(statement *stmt);
void exec_read(program *pgm, statement *stmt);
void exec_on(program *pgm, statement *stmt);
void exec_dim(program *pgm, statement *stmt);

// spit out an error and quit
void rterror(int code)
{
  int x;

  execcounter = 0;

  envinfo.lasterror = code; 
  envinfo.lasterrorline = envinfo.lastexec;

  if ((envinfo.allowseterr == 1) && (envinfo.seterrline != 0)) {
    envinfo.gotoline = envinfo.seterrline;
    envinfo.seterrline = 0;
    envinfo.abort = 2;
  } else if (envinfo.errline != -1) {
    envinfo.gotoline = envinfo.errline;
    envinfo.abort = 2;
  } else if (envinfo.eofline != -1 && envinfo.lasterror == ERR_END_OF_FILE) { 
    envinfo.gotoline = envinfo.eofline; 
    envinfo.abort = 2;
  } else {
    for (x=0; x<65; x++) {
      if (errortable[x].errorcode == code) {        
        printf("\n!ERROR=%d  (%s)\n", errortable[x].errorcode, 
                                      errortable[x].errortext);
        if (gprog->lastexec > 0 && gprog->lastexec < 0xFFFF) {
          listprog(gprog, gprog->lastexec, gprog->lastexec); 
          gprog->lastexec = 0;
        }
      }
    }
    envinfo.abort = 1;
  }
  // hack, code == 24 when function name being defined already exists
  // because DEF FNx stuff is not handled by runtime system, this would
  // crash because abortmark would not have been set.
  if (code != 24) longjmp(abortmark, 1);
  return;
}

int execline(program *pgm, statement *stmt)
{
  int x;

  execcounter = 0;

  envinfo.curisz = -1;
  envinfo.cursize = -1;
  envinfo.curchannel = -1;
  envinfo.curindex = -1;
  envinfo.cursep = 0x8A;
  envinfo.errline = -1;
  envinfo.eofline = -1;
  
  envinfo.lastexec = stmt->linenum;

  if (setjmp(abortmark) != 0) { 
    if (envinfo.abort == 2) return 1;  // handled, resume operations
    else if (envinfo.abort == 1) return 0; // not handled
  }

  if (stmt->opcode != CMD_IF && stmt->opcode != CMD_ENTER && 
      stmt->opcode != CMD_DEFFN) {    
    if (stmt->linenum) {
      for (x=1; x<stmt->metapos+1; x++) {
        x = x + simplify(pgm, stmt, x);
      }
    } else {
      for (x=0; x<stmt->metapos+1; x++) {               
        x = x + simplify(pgm, stmt, x);
      }
    }
  } 
//  printf("(%s)\n", get_opname(stmt->opcode));   
  switch(stmt->opcode) 
  {
    case CMD_DUMPSTMT:
      exec_dumpstmt(pgm, stmt);
      break;
    case CMD_RENAME:
      exec_rename(stmt);
      break;
    case CMD_EXIT:
      exec_exit(pgm, stmt);
      envinfo.abort = 1;
      longjmp(abortmark, 1);
      break;
    case CMD_ENTER:
      exec_enter(pgm, stmt);
      break;
    case CMD_CALL:
      exec_call(pgm, stmt);
      break;
    case 0xF314:
      rterror(ERR_SYNTAX);
      envinfo.abort = 1;
      break;
    case CMD_SYSTEM:
      exec_system(stmt);
      break;
    case CMD_EDIT:
      exec_edit(pgm, stmt);
      break;
    case CMD_READRECORD:
      exec_readrecord(pgm, stmt, 1);
      break;
    case CMD_READ:
      exec_read(pgm, stmt);
      break;
    case CMD_WRITERECORD:
      exec_writerecord(stmt);
      break;
    case CMD_WRITE:
      exec_write(stmt);
      break;
    case CMD_EXTRACTRECORD:
      exec_readrecord(pgm, stmt, 0);
      break;
    case CMD_INDEXED:
      exec_indexed(stmt);
      break;
    case CMD_TEXT:
      exec_text(stmt);
      break;
    case CMD_CLOSE:
      exec_close(stmt);
      break;
    case CMD_OPEN:
      exec_open(stmt);
      break;
    case CMD_ESCAPE: 
      raise(SIGINT);
      break;
    case CMD_DUMP:
      exec_dump(pgm, stmt);
      break;
    case CMD_ERASE:
      exec_erase(stmt);
      break;
    case CMD_ESCON:
      envinfo.allowesc = 1;
      break;
    case CMD_ESCOFF:
      envinfo.allowesc = 0;
      break;
    case CMD_SETESC:
      exec_setesc(stmt);
      break;
    case CMD_RETRY:
      if (!stmt->linenum) rterror(ERR_STATEMENT_USAGE);
      if (!envinfo.lasterrorline) rterror(ERR_RETURN_WITHOUT_GOSUB);
      envinfo.seterrline = envinfo.lastseterrline;
      envinfo.lastseterrline = 0;
      envinfo.gotoline = envinfo.lasterrorline;
      envinfo.lasterrorline = 0;
      envinfo.abort = 2;
      longjmp(abortmark, 1);
      break;
    case CMD_SETERRON:
      envinfo.allowseterr = 1;
      break;
    case CMD_SETERROFF:
      envinfo.allowseterr = 0;
      break;
    case CMD_SETERR:
      exec_seterr(pgm, stmt);
      break;
    case CMD_INPUT:
      exec_input(pgm, stmt);
      break;
    case CMD_WHILE:
      exec_while(pgm, stmt);
      break;
    case CMD_WEND:
      exec_wend(stmt);
      break;
    case CMD_ON:
      exec_on(pgm, stmt);
      break;
    case CMD_DIM:
      exec_dim(pgm, stmt);
      break;
    case CMD_IF:
      exec_if(pgm, stmt);
      break;
    case CMD_FOR:
      exec_for(pgm, stmt);
      break;
    case CMD_NEXT:
      exec_next(pgm, stmt);
      break;
    case CMD_GOSUB:
      exec_gosub(pgm, stmt);
      break;
    case CMD_RETURN:
      exec_return(stmt);
      break;
    case CMD_RELEASE:
      exit(1);
      break;
    case CMD_LET:
      exec_let(pgm, stmt); 
      break;
    case CMD_PRINT:
      exec_print(stmt);
      break;
    case CMD_GOTO:
      exec_goto(pgm, stmt);
      break;
    case CMD_WAIT:
      exec_wait(stmt);
      break;
    case CMD_RUN:
      envinfo.seterrline = 0;
      envinfo.escline = 0;
      exec_run(pgm, stmt);
      break;
    case CMD_DELETE:
      exec_delete(pgm, stmt);
      break;
    case CMD_LOAD:
      envinfo.seterrline = 0;
      envinfo.escline = 0;
      exec_load(pgm, stmt);
      break;
    case CMD_SAVE:
      exec_save(pgm, stmt);
      break;
    case CMD_LIST:
      exec_list(pgm, stmt);
      break;
    case CMD_RESET:
      envinfo.escline = 0;
      envinfo.seterrline = 0;
      resetstacks();
      break;
    case CMD_BEGIN:
      envinfo.seterrline = 0;
      envinfo.firstfile = 0;
      closefiles();
    case CMD_CLEAR:
      envinfo.seterrline = 0;
      envinfo.escline = 0;
      resetstacks();
      break;
    case CMD_STOP:
    case CMD_END: 
      envinfo.seterrline = 0;
      envinfo.firstfile = 0;
      closefiles();
      envinfo.gotoline = 1;
      envinfo.escline = 0;
      resetstacks();
      envinfo.abort = 1;
      longjmp(abortmark, 1);
      break;
    case CMD_REM:      
    default:
      break;
  }

  return 1;
}

void exec_dumpstmt(program *pgm, statement *stmt)
{
  int y = 0;
  filetbl *tempfile;
  statement *tempstmt = pgm->firststmt;
  FILE *fp;

  y = flt2int(fltexecpop()); 

  if (envinfo.curchannel > 0) {
    tempfile = get_chaninfo(envinfo.curchannel);
    if (!tempfile) rterror(ERR_FILE_DEVICE_USAGE);
    fp = tempfile->fp;
  } else fp = stdout;

  do {
    if (y == tempstmt->linenum) dumpstmt(tempstmt, fp);
    tempstmt = tempstmt->nextstmt;
  } while (tempstmt != NULL);
}

void exec_let(program *pgm, statement *stmt)
{
  unsigned int x, y = 0, storeto = 0;
  int start = 0, end = 0, count = 0;
  tbstring tmp;
  execelement elmt;
  symbol *tmpsym;

  flipstack(0);

  if (stmt->linenum) y = 1; else { y = 0; execcounter--; }
  for (x=y; x<stmt->metapos+1; x++)
  { 
    if ((stmt->metalist[x].operation == 0xEC) || (x == stmt->metapos)) {
      elmt = execpop();
      if (elmt.type == 2) {
        if (!end) end = tmpsym->string.len; 
        if (start == 0 && end == tmpsym->string.len) tmp = elmt.strarg;
        else {
          tmp.str = SafeMalloc(tmpsym->string.len);
          tmp.len = tmpsym->string.len;
          count = 0;
          for (y = 0; y < tmpsym->string.len; y++) {
            if (y >= start && y < end) {
              if (elmt.strarg.len == 0) tmp.str[y] = ' ';
              else tmp.str[y] = elmt.strarg.str[count];
              count++;
            } else tmp.str[y] = tmpsym->string.str[y];
          }
        }
        tmpsym->string = tmp;
      } else tmpsym->numeric = elmt.fltarg;
      if (x == stmt->metapos) break;
    }
    if (stmt->metalist[x].operation == SETVAL_STRING ||
        stmt->metalist[x].operation == SETVAL_STRINGARRAY ||
        stmt->metalist[x].operation == SETVAL_NUMERIC ||
        stmt->metalist[x].operation == SETVAL_NUMERICARRAY) {
      storeto = stmt->metalist[x].shortarg;
      if (stmt->metalist[x].shortarg & 0x2000) storeto -= 0x2000;
      if (stmt->metalist[x].shortarg & 0x4000) storeto -= 0x4000;
      if ((stmt->metalist[x].shortarg & 0x8000)) { 
        storeto -= 0x8000;
        if (stmt->metalist[x].operation == SETVAL_STRINGARRAY ||
            stmt->metalist[x].operation == SETVAL_NUMERICARRAY) {
          tmpsym = get_arraysym(idx2sym(pgm,storeto), 
                   flt2int(fltexecpop()), flt2int(fltexecpop()),
                   flt2int(fltexecpop()));
        } else {
          tmpsym = get_arraysym(idx2sym(pgm, storeto), -1, -1, 
                                  flt2int(fltexecpop()));
        }
      } else if (stmt->metalist[x].operation == SETVAL_STRINGARRAY ||
                 stmt->metalist[x].operation == SETVAL_NUMERICARRAY) {
        tmpsym = get_arraysym(idx2sym(pgm, storeto), -1, 
                         flt2int(fltexecpop()), flt2int(fltexecpop()));
      } else {
        tmpsym = idx2sym(pgm, storeto);
      }
      if (stmt->metalist[x].shortarg & 0x2000) {
        start = flt2int(fltexecpop())-1;
      } else if (stmt->metalist[x].shortarg & 0x4000) {
        start = flt2int(fltexecpop())-1;
        end = flt2int(fltexecpop());
      } else {
        start = 0;
      }
    }
  }
}

int exec_print(statement *stmt)
{
  filetbl *tempfile = 0;
  execelement elmt;
  FILE *fp = 0;
  int x = 0;

  if (envinfo.curchannel > 0) {
    tempfile = get_chaninfo(envinfo.curchannel);
    if (!tempfile) rterror(ERR_FILE_DEVICE_USAGE);
    fp = tempfile->fp;
  } else fp = stdout;

  flipstack(0);

  do {
    elmt = execpop();
    if (elmt.type == 2) {
      for (x = 0; x <= elmt.strarg.len; x++)
        fputc(elmt.strarg.str[x], fp);
    } else {
      fprintf(fp, " %s", flt2asc(elmt.fltarg));
    }
  } while (execcounter > 0);

  if (stmt->metalist[stmt->metapos-1].operation != 0xF1) fprintf(fp, "\n");

  return 1;
}

int exec_goto(program *pgm, statement *stmt)
{
  int tmp = 0;

  if (stmt->linenum) tmp = 1; else tmp = 0;

  switch (stmt->metalist[tmp].operation)
  {
    case LABELREF:
      envinfo.gotoline = pgm->labels[stmt->metalist[tmp].shortarg].lineref;
      break;
    case LINEREF:
      envinfo.gotoline = stmt->metalist[tmp].shortarg;
      break;
    default:
      rterror(ERR_STATEMENT_NUMBER);
      return 0;
  }
  return 1;
}

int exec_seterr(program *pgm, statement *stmt)
{
  int tmp = 0;

  if (stmt->linenum) tmp = 1; else tmp = 0;

  switch (stmt->metalist[tmp].operation)
  {
    case LABELREF:
      envinfo.seterrline = pgm->labels[stmt->metalist[tmp].shortarg].lineref;
      break;
    case LINEREF:
      envinfo.seterrline = stmt->metalist[tmp].shortarg;
      break;
    default:
      rterror(ERR_STATEMENT_NUMBER);
      return 0;
  }

  envinfo.lastseterrline = stmt->linenum;

  return 1;
}

int exec_wait(statement *stmt)
{
  if (execcounter) {
    sleep(flt2int(fltexecpop()));
    return 1;
  } else rterror(ERR_SYNTAX);

  return 1;
}

int exec_save(program *pgm, statement *stmt)
{
  tbstring s;

  if (execcounter) {
    s = strexecpop();
    if (!writeall(pgm, s.str)) return 0;
  } else if (pgm->filename) {
    if (!writeall(pgm, pgm->filename)) return 0;
  } else rterror(ERR_SYNTAX);

  return 1;
}

int exec_load(program *pgm, statement *stmt)
{
  tbstring s;
 
  if (execcounter) {
    s = strexecpop();
    if (!loadall(pgm, s.str)) return 0;
  } else rterror(ERR_SYNTAX);

  return 1;
}

void exec_list(program *pgm, statement *stmt)
{
  int x = 0, y = 0;

  if (execcounter) {   
    if (execcounter == 2) {
      y = flt2int(fltexecpop());
      x = flt2int(fltexecpop());
    } else { 
      x = y = flt2int(fltexecpop());
    }
  } else { x = 1; y = 65534; }
  listprog(pgm, x, y);
}

void exec_delete(program *pgm, statement *stmt)
{
  int x = 0, y = 0;

  if (execcounter) {
    if (execcounter == 2) {
      y = flt2int(fltexecpop());
      for (x=flt2int(fltexecpop()); x<=y; x++) deletestmt(pgm, x);
    } else deletestmt(pgm, flt2int(fltexecpop()));
  } else {
    for (x=1; x<0xFFFF; x++) deletestmt(pgm, x);
    pgm->firststmt = SafeMalloc(sizeof(statement));
    pgm->firststmt->linenum = 0xFFFF;
    pgm->firststmt->opcode = CMD_END;
    pgm->firststmt->endcode = 0xF2;
    pgm->firststmt->length = 5;
    pgm->firststmt->nextstmt = NULL;
  }
}

int exec_run(program *pgm, statement *stmt)
{
  tbstring s;

  if (stmt->linenum) rterror(ERR_STATEMENT_USAGE);

  if (execcounter) {
    s = strexecpop();
    if (!loadall(pgm, s.str)) return 0;
  }

  envinfo.allowexec = 1;
  if (envinfo.gotoline) runprog(pgm, envinfo.gotoline);
  else runprog(pgm, 1);

  envinfo.gotoline = 1;

  return 1;
}

int exec_gosub(program *pgm, statement *stmt)
{
  if (!stmt->linenum) rterror(ERR_STATEMENT_USAGE);

  switch (stmt->metalist[1].operation)
  {
    case LABELREF:
      envinfo.gotoline = pgm->labels[stmt->metalist[1].shortarg].lineref;
      break;
    case LINEREF:
      envinfo.gotoline = stmt->metalist[1].shortarg;
      break;
    default:
      rterror(ERR_STATEMENT_USAGE);
      return 0;
  }
  gosubpush(stmt->linenum);
  return 1;
}

int exec_return(statement *stmt)
{
  int x = gosubpop();

  envinfo.gotoline = x+1;
  return 1;
}

int isopcode(int possible)
{
  int x;

  for (x=0; *cmdtable[x].command; x++)
    if (possible == cmdtable[x].code) return 1;
  return 0;
}

int exec_if(program *pgm, statement *stmt)
{
  int x = 0, y = 0, z = 0, ctr = 0;
  int start = 0, end = 0;
  tbfloat flt;
  statement *tmpstmt;

  tmpstmt = SafeMalloc(sizeof(statement));
  stmtinit(tmpstmt);

  for (x=0; x<stmt->metapos; x++) 
    if (stmt->metalist[x].operation == 0xE7) break;

  y = x;

  for (x=y; x<stmt->metapos; x++) {
    if (stmt->metalist[x].operation == 0xE7) ctr++;
    if (stmt->metalist[x].operation == 0xE2) ctr -= 2;
    if (ctr == 0) break;
  }

  z = x;
  
  for (x=0; x<y; x++) x = x + simplify(pgm, stmt, x);

  ctr = 1;
  tmpstmt->linenum = stmt->linenum+1;

  flt = fltexecpop();
 
  if (flt2int(flt) == 1) { start = y; end = z; }
  else { start = z; end = stmt->metapos; }
  
  tmpstmt->opcode = stmt->metalist[start+1].operation;
  for (x=start+2; x<end; x++) {  
    tmpstmt->metalist[ctr].operation = stmt->metalist[x].operation;
    if (stmt->metalist[x].intarg)
      tmpstmt->metalist[ctr].intarg = stmt->metalist[x].intarg;
    if (stmt->metalist[x].floatarg.mantisa.i)
      tmpstmt->metalist[ctr].floatarg = stmt->metalist[x].floatarg;
    if (stmt->metalist[x].shortarg)
      tmpstmt->metalist[ctr].shortarg = stmt->metalist[x].shortarg;
    if (stmt->metalist[x].stringarg) {
      if (stmt->metalist[x].operation == MNEMONICREF && 
          !stmt->metalist[x].intarg) { 
        tmpstmt->metalist[ctr].stringarg[0] = stmt->metalist[x].stringarg[0];
        tmpstmt->metalist[ctr].stringarg[1] = stmt->metalist[x].stringarg[1];
      } else { 
        for (y=0; y<stmt->metalist[x].intarg; y++) 
          tmpstmt->metalist[ctr].stringarg[y] = stmt->metalist[x].stringarg[y]; 
      }
    }
    ctr++;
  }
  tmpstmt->metapos = ctr;
  execline(pgm, tmpstmt);
 
  return 1;
}

int exec_for(program *pgm, statement *stmt)
{
  forstruct tmp;

  if (!stmt->linenum) rterror(ERR_STATEMENT_USAGE);
 
  tmp.storeto = idx2sym(pgm, stmt->metalist[1].shortarg);
  tmp.stepval = int2flt(1);
  tmp.repeatline = stmt->linenum+1;

  if (execcounter == 2) {
    tmp.endval = fltexecpop();
    tmp.startval = fltexecpop();
  } else {
    tmp.stepval = fltexecpop();
    tmp.endval = fltexecpop();
    tmp.startval = fltexecpop();
  }
  tmp.storeto->numeric = tmp.startval;

  if (flt2dbl(tmp.startval) > flt2dbl(tmp.endval)) tmp.negative = 1;
  else tmp.negative = 0;

  forpush(tmp);

  return 1;
}

int exec_next(program *pgm, statement *stmt)
{
  forstruct tmp;

  if (!stmt->linenum) rterror(ERR_STATEMENT_USAGE);

  tmp = forpop();

  if (stmt->metalist[1].operation == SETVAL_NUMERIC) {
    if (stmt->metalist[1].shortarg == tmp.storeto->idx) {
      tmp.storeto->numeric = add(tmp.storeto->numeric, tmp.stepval);
      if (tmp.negative) {
        if (atof(flt2asc(tmp.storeto->numeric)) < atof(flt2asc(tmp.endval)))
        { envinfo.gotoline = stmt->linenum+1; return 1; }
        else { forpush(tmp); envinfo.gotoline = tmp.repeatline; return 1; }
      } else {
        if (flt2dbl(tmp.storeto->numeric) > flt2dbl(tmp.endval))
        { envinfo.gotoline = stmt->linenum+1; return 1; }
        else { forpush(tmp); envinfo.gotoline = tmp.repeatline; return 1; }
      }
    } 
  } else rterror(ERR_SYNTAX);
  return 1;
}

int exec_while(program *pgm, statement *stmt)
{
  statement *tempstmt = pgm->firststmt;
  tbfloat flt;

  if (!stmt->linenum) rterror(ERR_STATEMENT_USAGE);

  flt = fltexecpop();
  if (flt2int(flt) == 1) { whilepush(stmt->linenum); return 1; }

  do {
    if (tempstmt->linenum == stmt->linenum) {
      do {
        if (tempstmt->opcode == CMD_WEND) {
          envinfo.gotoline = tempstmt->linenum+1;
          return 1;
        }
        tempstmt = tempstmt->nextstmt;
      } while (tempstmt != NULL);
    }
    tempstmt = tempstmt->nextstmt;
  } while (tempstmt != NULL);
  return 1;
}

int exec_wend(statement *stmt)
{
  int x;

  if (!stmt->linenum) rterror(ERR_STATEMENT_USAGE);

  x = whilepop();
  whilepush(x);
  envinfo.gotoline = x;

  return 1;
}

int exec_input(program *pgm, statement *stmt)
{
  int tmp, x = 0, y = 0, inctr = 0, storeto = 0;
  char strtmp[80];
  symbol *tmpsym;
  byte ch = 0;
  execelement elmt;

  if (stmt->linenum) tmp = 1; else tmp = 0;

  for (x=tmp; x<stmt->metapos; x++)
  {
    ch = 0; 
    inctr = 0;

    memset(strtmp, '\0', 80);
    flipstack(0);

    if (stmt->metalist[x].operation == SETVAL_NUMERIC ||
        stmt->metalist[x].operation == SETVAL_NUMERICARRAY ||
        stmt->metalist[x].operation == SETVAL_STRINGARRAY ||
        stmt->metalist[x].operation == SETVAL_STRING) {
      storeto = stmt->metalist[x].shortarg;
      if (stmt->metalist[x].shortarg & 0x2000) storeto -= 0x2000;
      if (stmt->metalist[x].shortarg & 0x4000) storeto -= 0x4000;
      if ((stmt->metalist[x].shortarg & 0x8000)) { 
        storeto -= 0x8000;
        if (stmt->metalist[x].operation == SETVAL_STRINGARRAY ||
            stmt->metalist[x].operation == SETVAL_NUMERICARRAY) {
          tmpsym = get_arraysym(idx2sym(pgm,storeto), 
                   flt2int(fltexecpop()), flt2int(fltexecpop()),
                   flt2int(fltexecpop()));
        } else {
          tmpsym = get_arraysym(idx2sym(pgm, storeto), -1, -1, 
                                  flt2int(fltexecpop()));
        }
      } else if (stmt->metalist[x].operation == SETVAL_STRINGARRAY ||
                 stmt->metalist[x].operation == SETVAL_NUMERICARRAY) {
        tmpsym = get_arraysym(idx2sym(pgm, storeto), -1, 
                    flt2int(fltexecpop()), flt2int(fltexecpop()));
      } else {
        tmpsym = idx2sym(pgm, storeto);
      }
      tmpsym->string.str = SafeMalloc(MAX_STRING_LENGTH);
      do {
again:
        ch = readkey();
        if (ch == 0x7F) {
          if ((inctr-1) != -1) { inctr--; printf("\b \b"); }
          goto again;
        }
        if (stmt->metalist[x].operation == SETVAL_STRING ||
            stmt->metalist[x].operation == SETVAL_STRINGARRAY) 
          tmpsym->string.str[inctr] = ch;
        else strtmp[inctr] = ch;
        inctr++;
        printf("%c", ch);
        if (ch == 0xA) break;
      } while (1);
      if (stmt->metalist[x].operation == SETVAL_STRING ||
          stmt->metalist[x].operation == SETVAL_STRINGARRAY) {
        tmpsym->string.str[inctr-1] = '\0';
        tmpsym->string.len = inctr-1;
      } else {
        tmpsym = idx2sym(pgm, stmt->metalist[x].shortarg);
        tmpsym->numeric = asc2flt(strtmp);
      }
      x++; // skip comma immediately after variable
    } else if (stmt->metalist[x].operation == 0xF1 ||
               stmt->metalist[x].operation == 0xF2) {
      elmt = execpop();
      if (elmt.type == 2) {
        for (y = 0; y <= elmt.strarg.len; y++)
          fputc(elmt.strarg.str[y], stdout);
      } else {
        fprintf(stdout, " %s", flt2asc(elmt.fltarg));
      }
    }
  }
  return 1;
}

int exec_setesc(statement *stmt)
{
  int tmp;

  if (stmt->linenum) tmp = 1; else tmp = 0;
  
  if (stmt->metalist[tmp].operation == LINEREF)
    envinfo.escline = stmt->metalist[tmp].shortarg;
  else rterror(ERR_SYNTAX); 

  return 1;
}

int exec_erase(statement *stmt)
{
  tbstring tmp;
  
  tmp = strexecpop();

  if (unlink(tmp.str) == -1) rterror(ERR_BAD_FILEID);

  return 1;
}

int exec_open(statement *stmt)
{
  filetbl *newfile, *tempfile, *templast;
  int x;
  tbstring tmp;

  if (envinfo.curchannel == -1) rterror(ERR_SYNTAX);

  if (get_chaninfo(envinfo.curchannel) != NULL) rterror(ERR_FILE_DEVICE_USAGE);

  tempfile = templast = envinfo.firstfile;

  tmp = strexecpop();

  newfile = SafeMalloc(sizeof(filetbl));
 
  newfile->channel = envinfo.curchannel;
  if((newfile->fp = fopen(tmp.str, "r+")) == NULL) 
    rterror(ERR_BAD_FILEID);
  for (x=0; x<8; x++) newfile->filename[x] = '\0';
  for (x=0; x<tmp.len; x++) newfile->filename[x] = tmp.str[x];
  newfile->nextfile = 0;

  if (envinfo.curopt) {
    if (!strcmp(envinfo.curopt, "TEXT")) goto textfile;
  }

  newfile->cursep = envinfo.cursep;
  if (envinfo.curisz != -1) newfile->curisz = envinfo.curisz;

  for (x=0; x<7; x++) tmp.str[x] = '\0';

  fread(tmp.str, 8, 1, newfile->fp);
  if (!strcmp(tmp.str, newfile->filename)) {
    if (envinfo.curisz != -1) { 
      newfile->recordsize = envinfo.curisz;
      ReadLong(newfile->fp); 
    } else newfile->recordsize = ReadLong(newfile->fp);
    newfile->numrecords = ReadLong(newfile->fp);
    newfile->nextrecord = 0;
    newfile->type = FILE_INDEXED;
    ReadByte(newfile->fp);
  } else {
textfile:
    if (envinfo.curisz > 0) newfile->indexsize = envinfo.curisz;
    else newfile->indexsize = 0;
    fseek(newfile->fp, 0, SEEK_SET);
    newfile->type = FILE_TEXT;
  }
  if (envinfo.firstfile == 0) {
    envinfo.firstfile = newfile;
  } else if (newfile->channel < envinfo.firstfile->channel) {
    newfile->nextfile = envinfo.firstfile;
    envinfo.firstfile = newfile;
  } else {
    do {
      if (tempfile->channel > newfile->channel) {
        templast->nextfile = newfile;
        newfile->nextfile = tempfile;
        break;
      }
      templast = tempfile;
      tempfile = tempfile->nextfile;
    } while (tempfile != NULL);
    templast->nextfile = newfile;
  }

  return 1;
}

int exec_close(statement *stmt)
{
  filetbl *oldfile, *tempfile, *templast;

  if (envinfo.curchannel == -1) rterror(ERR_SYNTAX);

  if (envinfo.curchannel == 0) return 0;

  oldfile = get_chaninfo(envinfo.curchannel);
  if (!oldfile) return 1;

  fclose(oldfile->fp);

  tempfile = templast = envinfo.firstfile;

  do {
    if (tempfile->channel == envinfo.curchannel) {
      if (envinfo.curchannel == envinfo.firstfile->channel)
        envinfo.firstfile = tempfile->nextfile;
      else templast->nextfile = tempfile->nextfile;
      break;
    }      
    templast = tempfile;
    tempfile = tempfile->nextfile;
  } while (tempfile != NULL);

  GC_free(oldfile);

  return 1;
}

int exec_text(statement *stmt)
{
  FILE *fp;
  tbstring tmp;

  tmp = strexecpop();

  if ((fp = fopen(tmp.str, "r+")) != NULL) rterror(ERR_BAD_FILEID);

  if ((fp = fopen(tmp.str, "w")) == NULL) rterror(ERR_OUT_OF_DISK_SPACE);

  fclose(fp);

  chmod(tmp.str, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

  return 1;
}

int exec_indexed(statement *stmt)
{
  FILE *fp;
  tbstring tmp;

  flipstack(0);

  tmp = strexecpop();
  if ((fp = fopen(tmp.str, "r+")) != NULL) rterror(ERR_BAD_FILEID);

  if ((fp = fopen(tmp.str, "w")) ==NULL) rterror(ERR_OUT_OF_DISK_SPACE);

  fwrite(tmp.str, 8, 1, fp);
  WriteLong(fp, flt2int(fltexecpop()));
  WriteLong(fp, flt2int(fltexecpop()));
  WriteByte(fp, '@');

  fclose(fp);
  chmod(tmp.str, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

  return 1;
}

void exec_read(program *pgm, statement *stmt)
{
  FILE *fp = 0;
  filetbl *tmpfile = 0;
  unsigned int x = 0, recordnum = 0, recordlen = 0, ctr = 0;
  char *out;
  byte ch, sep;
  symbol *tmpsym;
 
  if (envinfo.curchannel > 0) { 
    tmpfile = get_chaninfo(envinfo.curchannel);
    if (!tmpfile) rterror(ERR_FILE_DEVICE_USAGE);
    else fp = tmpfile->fp;
    ch = tmpfile->cursep;
  } else fp = stdin;

  if (fp == stdin) { ch = 0x8A; goto raw; }
  else if (tmpfile->type == FILE_INDEXED) {
    if (envinfo.curindex == -1) recordnum = tmpfile->nextrecord;
    else recordnum = envinfo.curindex;

    recordlen = tmpfile->recordsize;
        
    if (fseek(fp, 17+(recordnum*tmpfile->recordsize), SEEK_SET) != 0) 
      rterror(ERR_END_OF_FILE);
    tmpfile->nextrecord = recordnum + 1;
  } else if (tmpfile->type == FILE_TEXT) {
    if (envinfo.curindex != -1) {
      if (fseek(fp, envinfo.curindex, SEEK_SET) != 0)
        rterror(ERR_END_OF_FILE);
    }
    recordlen = -1;
  }
raw:
  for (x=0; x<stmt->metapos; x++) {
    if ((stmt->metalist[x].operation == GETVAL_STRING) ||
        (stmt->metalist[x].operation == GETVAL_NUMERIC)) {
      out = SafeMalloc(1024 * 64);
      do {
        if (fread(&ch, sizeof(byte), 1, fp) == 0) 
          rterror(ERR_END_OF_FILE);
        if ((ch == sep) || (ch == 0x0A) || (ch == 0x0D)) break; 
        out[ctr] = ch;
        ctr++;
        if ((recordlen > 0) && (ctr > recordlen)) rterror(ERR_END_OF_RECORD);
      } while (1);
      
      GC_realloc(out, ctr);
      if (stmt->metalist[x].operation == GETVAL_STRING) {
        tmpsym = idx2sym(pgm, stmt->metalist[x].shortarg);
        tmpsym->string.str = out;
        tmpsym->string.len = ctr;
      } else {
        tmpsym = idx2sym(pgm, stmt->metalist[x].shortarg);
        tmpsym->numeric = asc2flt(out);
      }
    }
  }
}

int exec_readrecord(program *pgm, statement *stmt, byte update)
{
  unsigned int x, recordnum = 0;
  FILE *fp = 0;
  filetbl *tmpfile = 0;
  symbol *tmpsym;

  if (envinfo.curchannel > 0) { 
    tmpfile = get_chaninfo(envinfo.curchannel);
    if (!tmpfile) rterror(ERR_FILE_DEVICE_USAGE);
    else fp = tmpfile->fp;
  } else fp = stdin;

  for (x=0; x<stmt->metapos; x++) 
    if (stmt->metalist[x].operation == GETVAL_STRING) break;
  if (fp == stdin) goto raw;
  else if (tmpfile->type == FILE_INDEXED) {
    if (envinfo.curindex == -1) recordnum = tmpfile->nextrecord;
    else recordnum = envinfo.curindex;

    if (fseek(fp, 17+(recordnum*tmpfile->recordsize), SEEK_SET) != 0) 
      rterror(ERR_END_OF_FILE);

    tmpsym = idx2sym(pgm, stmt->metalist[x].shortarg);
    tmpsym->string.str = SafeMalloc(tmpfile->recordsize);
    tmpsym->string.len = tmpfile->recordsize;
    if (fread(tmpsym->string.str, tmpfile->recordsize, 1, fp) == 0)
      rterror(ERR_END_OF_FILE);
    GC_realloc(tmpsym->string.str, tmpsym->string.len);
    if (update) tmpfile->nextrecord = recordnum+1;
  } else if (tmpfile->type == FILE_TEXT) {
    if (envinfo.curindex != -1) {
      if (fseek(fp, envinfo.curindex, SEEK_SET) != 0)
        rterror(ERR_END_OF_FILE);
    }
    if (envinfo.cursize != -1) {
raw:
      tmpsym = idx2sym(pgm, stmt->metalist[x].shortarg);
      tmpsym->string.str = SafeMalloc(envinfo.cursize);
      tmpsym->string.len = envinfo.cursize;

      if (fread(tmpsym->string.str, envinfo.cursize, 1, fp) == 0)
        rterror(ERR_END_OF_FILE);
      GC_realloc(tmpsym->string.str, tmpsym->string.len);
    } else rterror (ERR_END_OF_FILE);
  }

  return 1;
}

void exec_write(statement *stmt)
{
  FILE *fp = 0;
  filetbl *tmpfile = 0;
  tbstring s;
  int y = 0, x = 0, offset = 0;
  unsigned recordnum = 0;
  execelement elmt;

  if (envinfo.curchannel > 0) { 
    tmpfile = get_chaninfo(envinfo.curchannel);
    if (!tmpfile) rterror(ERR_FILE_DEVICE_USAGE);
    else fp = tmpfile->fp;
  } else fp = stdout; 

  flipstack(0);

  y = execcounter;
  if (fp == stdout) goto raw;
  else if (tmpfile->type == FILE_INDEXED) {
    if (envinfo.curindex == -1) recordnum = tmpfile->nextrecord;
    else recordnum = envinfo.curindex;

    if (recordnum+1 > tmpfile->numrecords) rterror(ERR_END_OF_FILE);
    fseek(fp, 17+(recordnum*tmpfile->recordsize), SEEK_SET);

    for (x = 0; x < y; x++) {
      elmt = execpop();
      if (elmt.type == 1) s.str = flt2asc(elmt.fltarg);
      else s = elmt.strarg;

      if (offset+s.len> tmpfile->recordsize) rterror(ERR_END_OF_RECORD);

      offset += s.len;

      fwrite(s.str, s.len, 1, fp);
      fwrite(&envinfo.cursep, 1, 1, fp);
    }
    for (x = offset; x<tmpfile->recordsize; x++)
      fwrite("\0", 1, 1, fp);
    tmpfile->nextrecord = recordnum+1;
 } else if (tmpfile->type == FILE_TEXT) {
    if (envinfo.curindex != -1)
      fseek(fp, envinfo.curindex, SEEK_SET);
raw:
    for (x = 0; x < y; x++) {
      elmt = execpop();
      if (elmt.type == 1) s.str = flt2asc(elmt.fltarg);
      else s = elmt.strarg;

      fwrite(s.str, s.len, 1, fp);
      fwrite(&envinfo.cursep, 1, 1, fp);
    }
  }
}

int exec_writerecord(statement *stmt)
{
  unsigned recordnum = 0;
  filetbl *tmpfile = 0;
  tbstring s;
  int x;
  FILE *fp = 0;

  if (envinfo.curchannel > 0) { 
    tmpfile = get_chaninfo(envinfo.curchannel);
    if (!tmpfile) rterror(ERR_FILE_DEVICE_USAGE);
    else fp = tmpfile->fp;
  } else fp = stdout;

  if (fp == stdout) goto raw;
  else if (tmpfile->type == FILE_INDEXED) {
    if (envinfo.curindex == -1) recordnum = tmpfile->nextrecord;
    else recordnum = envinfo.curindex;

    if (recordnum+1 > tmpfile->numrecords) rterror(ERR_END_OF_FILE);

    fseek(fp, 17+(recordnum*tmpfile->recordsize), SEEK_SET);
  
    s = strexecpop();

    if (s.len > tmpfile->recordsize) rterror(ERR_END_OF_RECORD);

    fwrite(s.str, s.len, 1, fp);
    for (x = s.len; x<tmpfile->recordsize; x++)
      fwrite("\0", 1, 1, fp);

    tmpfile->nextrecord = recordnum+1;
  } else if (tmpfile->type == FILE_TEXT) {
    if (envinfo.curindex != -1)
      fseek(fp, envinfo.curindex, SEEK_SET);

raw:

    s = strexecpop();

    fwrite(s.str, s.len, 1, fp);
  }

  return 1;
}

int exec_system(statement *stmt)
{
  struct passwd *pwd;
  tbstring s;
 
  if (execcounter) {
    s = strexecpop();
    system(s.str);
  } else {
    pwd = getpwnam(getlogin());
    system(pwd->pw_shell);
  }
  return 1;
}

void exec_call(program *pgm, statement *stmt)
{
  program *newpgm, *pgmbackup;
  tbstring s;

  if (stmt->linenum == 0)
    rterror(ERR_STATEMENT_USAGE);

  // suicidal tendencies
  if (envinfo.envlevel >= 127) {
    pgmbackup = SafeMalloc(sizeof(program));
    newpgm = pgm;
    do {
      pgmbackup = newpgm->prevprog;
      GC_free(newpgm);
      newpgm = pgmbackup;
      envinfo.envlevel--;
    } while (envinfo.envlevel > 1);
    GC_free(pgmbackup);
    rterror(ERR_MEMORY_CAPACITY);
  }

  flipstack(0);
  
  s = strexecpop();

  newpgm = SafeMalloc(sizeof(program));
//  storageinit(newpgm);
  newpgm->prevprog = pgm;
  envinfo.envlevel++;
  loadall(newpgm, s.str);
  runprog(newpgm, 1);
  envinfo.envlevel--;
  GC_free(newpgm);
  pgm->nextprog = 0;
}

void exec_enter(program *pgm, statement *stmt)
{
  int x = 0;
  statement *tmpstmt;
  symbol *newsym, *tempsym;

  if (pgm == gprog) rterror(ERR_CALL_ENTER_MISMATCH);
 
  tmpstmt = pgm->prevprog->firststmt;

  if (stmt->metapos == 1) {
    tempsym = pgm->prevprog->firstsym;
    do { 
      newsym = addsymbol(pgm, tempsym->name);
      if (newsym->name[newsym->length-1] == '$')
        newsym->string = tempsym->string;
      else 
        newsym->numeric = tempsym->numeric;
      tempsym = tempsym->nextsym;
    } while (tempsym != NULL);
  } else {
    tmpstmt = pgm->prevprog->firststmt;
    do {
      if (tmpstmt->linenum == pgm->prevprog->lastexec) break;
      tmpstmt = tmpstmt->nextstmt;
    } while (tmpstmt != NULL);
    // stmt is this ENTER stmt, tmpstmt is the last CALL stmt
    for (x=1; x<tmpstmt->metapos+1; x++) 
      x = x + simplify(pgm->prevprog, tmpstmt, x);
    flipstack(0);
    strexecpop(); // program name
    
    for (x=1; x<stmt->metapos+1; x++) {
      if (stmt->metalist[x].operation == GETVAL_NUMERIC) {
        tempsym = idx2sym(pgm, stmt->metalist[x].shortarg);
        tempsym->numeric = fltexecpop();
      } else if (stmt->metalist[x].operation == GETVAL_STRING) {
        tempsym = idx2sym(pgm, stmt->metalist[x].shortarg);
        tempsym->string = strexecpop();
      }
    }
  }
}

void exec_exit(program *pgm, statement *stmt)
{
  statement *tmpstmt = pgm->firststmt;
  statement *tmpstmt2 = pgm->prevprog->firststmt;
  int x = 0;
  symbol *newsym, *tempsym;

  if (pgm == gprog) rterror(ERR_RETURN_WITHOUT_GOSUB);

  do {
    if (tmpstmt->opcode == CMD_ENTER) break;
    tmpstmt = tmpstmt->nextstmt;
  } while (tmpstmt != NULL);

  if (tmpstmt->metapos == 1) {
    tempsym = pgm->firstsym;
    do { 
      newsym = addsymbol(pgm->prevprog, tempsym->name);
      if (newsym->name[newsym->length-1] == '$')
        newsym->string = tempsym->string;
      else 
        newsym->numeric = tempsym->numeric;
      tempsym = tempsym->nextsym;
    } while (tempsym != NULL);
  } else {
    do {
      if (tmpstmt2->linenum == pgm->prevprog->lastexec) break;
      tmpstmt2 = tmpstmt2->nextstmt;
    } while (tmpstmt2 != NULL);
    // tmpstmt = ENTER stmt, tmpstmt2 = last CALL stmt
    for (x=1; x<tmpstmt->metapos+1; x++) 
      x = x + simplify(pgm, tmpstmt, x);

    if (envinfo.envlevel == 1)
      flipstack(0);
     
    for (x=1; x<tmpstmt2->metapos+1; x++) {
      if (tmpstmt2->metalist[x].operation == 0xEC ||
          tmpstmt2->metalist[x].operation == 0xF2) {
        if (tmpstmt2->metalist[x-1].operation == GETVAL_NUMERIC) {
          tempsym = idx2sym(pgm->prevprog, 
                           tmpstmt2->metalist[x-1].shortarg);
          tempsym->numeric = fltexecpop();
        } else if (tmpstmt2->metalist[x-1].operation == GETVAL_STRING) {
          tempsym = idx2sym(pgm->prevprog, 
                           tmpstmt2->metalist[x-1].shortarg);
          tempsym->string = strexecpop();
        }
      }
    }
  }
}

void exec_rename(statement *stmt)
{
  program *newpgm;
  tbstring a, b;

  newpgm = SafeMalloc(sizeof(program));
  storageinit(newpgm);

  a = strexecpop();
  b = strexecpop();

  loadall(newpgm, b.str);

  writeall(newpgm, a.str);

  GC_free(newpgm);
 
  if (unlink(b.str) == -1) rterror(ERR_BAD_FILEID);
}

void exec_edit(program *pgm, statement *stmt)
{
  char *line;
  statement *tmpstmt = pgm->firststmt;
  unsigned short x = 0;

  if (stmt->linenum > 0) rterror(ERR_STATEMENT_USAGE);
 
  x = flt2int(fltexecpop());

  do {
    if (tmpstmt->linenum == x) break;
    tmpstmt = tmpstmt->nextstmt;
  } while (tmpstmt != NULL);

  if (!tmpstmt) return;

  line = SafeMalloc(1024*64);
  *line = 0;
  listline(line, tmpstmt, 1);
  for (x = 0; x < strlen(line); x++)
    rl_stuff_char(line[x]);
  input = readline("");
  input[strlen(input)] = '\0';
  prog = input;
  if (*input) {
    add_history(input);
    encode_rpn();
  }
  GC_free(line);
}

void exec_on(program *pgm, statement *stmt)
{
  int x = 0, y = 0, line = 0;
  statement *tmpstmt;

  flipstack(0);

  x = flt2int(fltexecpop());

  if (x >= execcounter-1) {
    for (y = 0; y < execcounter; y++)
      fltexecpop();
    line = flt2int(fltexecpop());
  } else if (x <= 0) {
    line = flt2int(fltexecpop());
  } else {
    for (y = 0; y < x; y++)
      fltexecpop();
    line = flt2int(fltexecpop());
  }

  for (y = 0; y <= stmt->metapos; y++) 
    if ((stmt->metalist[y].operation == 0x00F4)  ||
        (stmt->metalist[y].operation == 0x01F4)) break;

  tmpstmt = SafeMalloc(sizeof(statement));
  stmtinit(tmpstmt);  
  
  tmpstmt->linenum = stmt->linenum;
  if (stmt->metalist[y].operation == 0x00F4)
    tmpstmt->opcode = CMD_GOTO;
  else tmpstmt->opcode = CMD_GOSUB;
  tmpstmt->metalist[1].shortarg = line;
  tmpstmt->metalist[1].operation = LINEREF;
  execline(pgm, tmpstmt);  
  GC_free(tmpstmt);
}

void exec_dim(program *pgm, statement *stmt)
{
  int a = 0, b = 0, c = 0, x = 0, index = 0;
  int idx1 = -1, idx2 = -1, idx3 = -1, length = -1;
  byte fillchr = ' ';
  symbol *tmpsym, *tmpsym2, *tmpsym3;
  tbstring str;

  flipstack(0);

  if (stmt->linenum) x = 1; else x = 0;
  for (x=x; x<stmt->metapos+1; x++) {
    if (stmt->metalist[x].operation == SETVAL_STRING ||
        stmt->metalist[x].operation == SETVAL_STRINGARRAY ||
        stmt->metalist[x].operation == SETVAL_NUMERIC ||
        stmt->metalist[x].operation == SETVAL_NUMERICARRAY) { 
      // get index
      length = idx1 = idx2 = idx3 = -1;
      fillchr = ' ';
      index = stmt->metalist[x].shortarg;
      if (stmt->metalist[x].shortarg & 0x8000) {
        index -= 0x8000;
        idx1 = flt2int(fltexecpop())+1;
        if (stmt->metalist[x].operation == SETVAL_STRINGARRAY ||
            stmt->metalist[x].operation == SETVAL_NUMERICARRAY) {
          idx2 = flt2int(fltexecpop())+1;
          idx3 = flt2int(fltexecpop())+1;
        }
      } else if (stmt->metalist[x].operation == SETVAL_STRINGARRAY ||
                 stmt->metalist[x].operation == SETVAL_NUMERICARRAY) {
        idx1 = flt2int(fltexecpop())+1;
        idx2 = flt2int(fltexecpop())+1;
      } else return;

      if (stmt->metalist[x].shortarg & 0x4000) {
        index -= 0x4000;
        length = flt2int(fltexecpop());
        str = strexecpop();
        fillchr = str.str[0];
      } else if (stmt->metalist[x].shortarg & 0x2000) {
        index -= 0x2000; 
        length = flt2int(fltexecpop());
      }

      if (abs(idx1 * idx2 * idx3) > 50000) rterror(ERR_INTEGER_RANGE);

      tmpsym = idx2sym(pgm, index);
      tmpsym->dim1 = idx1;
      tmpsym->dim2 = idx2;
      tmpsym->dim3 = idx3;
      tmpsym->arraylink = SafeMalloc(sizeof(symbol));
      tmpsym = tmpsym->arraylink;

      for (a = 0; a <= idx1; a++) {
        tmpsym->nextsym = SafeMalloc(sizeof(symbol));
        if (idx2 != -1) {
          tmpsym->arraylink = SafeMalloc(sizeof(symbol));
          tmpsym2 = tmpsym->arraylink;
          for (b = 0; b <= idx2; b++) {
            tmpsym2->nextsym = SafeMalloc(sizeof(symbol));
            if (idx3 != -1) {
              tmpsym2->arraylink = SafeMalloc(sizeof(symbol));
              tmpsym3 = tmpsym2->arraylink;
              for (c = 0; c <= idx3; c++) {
                tmpsym3->nextsym = SafeMalloc(sizeof(symbol));     
                if (length != -1) {
                  tmpsym3->string.str = SafeMalloc(length);
                  tmpsym3->string.len = length;
                  memset(tmpsym3->string.str, fillchr, length);
                }
                tmpsym3 = tmpsym3->nextsym; 
              }
            }
            if (length != -1) {
              tmpsym2->string.str = SafeMalloc(length);
              tmpsym2->string.len = length;
              memset(tmpsym2->string.str, fillchr, length);
            }
            tmpsym2 = tmpsym2->nextsym;
          }
        }
        if (length != -1) {
          tmpsym->string.str = SafeMalloc(length);
          tmpsym->string.len = length;
          memset(tmpsym->string.str, fillchr, length);
        }
        tmpsym = tmpsym->nextsym;
      }
    }
  }
}
