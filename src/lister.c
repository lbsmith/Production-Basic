/*
 * $Id: lister.c,v 1.23 2000/06/01 01:15:46 lees Exp $
 */

#include <stdarg.h>
#include <string.h>
#include "basic.h"

extern char *get_opname(int);
extern char *get_symname(int);
extern char *get_labelname(int);
extern int getprec(char *name);
extern int isopcode(int);

void listline(char *var, statement *stmt, byte showline);
void list_for(char *var, statement *stmt);
void list_print(char *var, statement *stmt);
void list_let(char *var, statement *stmt);
void list_if(char *var, statement *stmt);
void list_deffn(char *var, statement *stmt);
void list_on(char *var, statement *stmt);
void simplifylist(char *var, statement *stmt, int start, int end);

int opctr = 0;
byte opstack[STACK_DEPTH];

char outstack[STACK_DEPTH][MAX_STRING_LENGTH];
int outcount = 0;

void listpush(char *str)
{
  int x;

  for (x=0; x<strlen(str); x++)
    outstack[outcount][x] = str[x];
  outstack[outcount][x] = '\0';
  outcount++;
}

char *listpop()
{
  outcount--;
  return outstack[outcount];
}

void oppush(byte op)
{
  opstack[opctr] = op;
  opctr++;
}

byte oppop()
{
  opctr--;
  return opstack[opctr];
}

void mysprintf(char *var, char *fmt, ...)
{
  va_list v;
  int pos = 0;

  pos = strlen(var);

  va_start(v, fmt);

  vsprintf(var+pos, fmt, v);
}

void listprog(program *pgm, int start, int end)
{
  statement *tempstmt = pgm->firststmt;
  char *listout;

  do {
    if ((tempstmt->linenum >= start) && (tempstmt->linenum <= end)) {
      listout = SafeMalloc(1024*64);
      *listout = 0;
      listline(listout, tempstmt, 1);
      printf("%s\n", listout);
      GC_free(listout);
    }
    tempstmt = tempstmt->nextstmt;    
  } while (tempstmt != NULL);
}

void listline(char *var, statement *stmt, byte showline)
{
  int x;

  outcount = 0;

  if (stmt->errorflag) {
    mysprintf(var, "*ERR ");
    for (x=0; x<tokenpos; x++) mysprintf(var, " ");
    mysprintf(var, "V\n");
  }
  if (showline) mysprintf(var, "%.5d ", stmt->linenum);
  if (stmt->errorflag) {
    mysprintf(var, "%s", stmt->metalist[1].stringarg);
    return;
  }
  if (stmt->numlabels) {
    for (x=0; x<stmt->numlabels; x++)         
      mysprintf(var, "%s: ", get_labelname(stmt->labelset[x]->labelnum));
  } 
  mysprintf(var, "%s ", get_opname(stmt->opcode));
  switch(stmt->opcode) {
    case CMD_DEFFN:
      list_deffn(var, stmt);
      break;
    case CMD_REM:
      mysprintf(var, "%s", stmt->metalist[1].stringarg);
      break;
    case CMD_IF:
      list_if(var, stmt);
      break;
    case CMD_LET:
      list_let(var, stmt);
      break;
    case CMD_SETESC:
    case CMD_SETERR:
    case CMD_GOTO:
    case CMD_GOSUB:
      if (stmt->metalist[1].operation == LABELREF)
        mysprintf(var, "%s", get_labelname(stmt->metalist[1].shortarg));
      else mysprintf(var, "%.5d", stmt->metalist[1].shortarg);
      break;
    case CMD_FOR:
      list_for(var, stmt);
      break;
    case CMD_NEXT:
      mysprintf(var, "%s", get_symname(stmt->metalist[1].shortarg));
      break;
    case CMD_LIST:
    case CMD_DELETE:          
      if (stmt->metapos == 1)  
        mysprintf(var, "00001, 65534");
      else if (stmt->metapos == 2)
        mysprintf(var, "%.5d", flt2int(stmt->metalist[1].floatarg));
      else mysprintf(var, "%.5d, %.5d", flt2int(stmt->metalist[1].floatarg),
                                flt2int(stmt->metalist[3].floatarg));
      break;
    case CMD_RETURN:
    case CMD_WEND:
    case CMD_RETRY:
    case CMD_SETERRON:
    case CMD_SETERROFF:
      break;
    case CMD_ON:
      list_on(var, stmt);
      break;
    default:
      if (stmt->metapos == 1) break;
      simplifylist(var, stmt, 0, stmt->metapos);
      mysprintf(var, "%s", listpop());
      break;
  }
}

void list_if(char *var, statement *stmt)
{
  int x = 0, y = 0, z = 0, ctr = 0;
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

  simplifylist(var, stmt, 0, y);  
  mysprintf(var, "%s THEN ", listpop());

  ctr = 1;

  tmpstmt->opcode = stmt->metalist[y+1].operation;
  for (x=y+2; x<z; x++) {  
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
  listline(var, tmpstmt, 0);

  if (z != stmt->metapos) {
    stmtinit(tmpstmt);
    ctr = 0;
    mysprintf(var, " ELSE ");
    tmpstmt->opcode = stmt->metalist[z+1].operation;
    for (x=z+2; x<stmt->metapos; x++) {  
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
    listline(var, tmpstmt, 0);
  }
}

void list_for(char *var, statement *stmt)
{
  char *tmp1 = 0, *tmp2 = 0, *tmp3 = 0;

  mysprintf(var, "%s=", get_symname(stmt->metalist[1].shortarg));
  simplifylist(var, stmt, 2, stmt->metapos);

  if (outcount == 3) {
    tmp3 = listpop();
    tmp2 = listpop();
    tmp1 = listpop();
  } else {
    tmp2 = listpop();
    tmp1 = listpop();
  }    
  mysprintf(var, "%s TO %s", tmp1, tmp2);
  if (tmp3) mysprintf(var, " STEP %s", tmp3);
}

void list_let(char *var, statement *stmt)
{
  int x = 0, start = 0;

  for (x=1; x<stmt->metapos; x++)
  {
    if (!start) start = x;
    if (stmt->metalist[x].operation == 0xEC)
    {
      simplifylist(var, stmt, start, x); start = 0;
      mysprintf(var, "%s%s, ", listpop(), listpop());
    }
  } 
  simplifylist(var, stmt, start, stmt->metapos);
  mysprintf(var, "%s%s", listpop(), listpop());
}

void list_on(char *var, statement *stmt)
{
  int x = 0;

  for (x=0; x<=stmt->metapos; x++)
    if ((stmt->metalist[x].operation == 0x00F4) ||
        (stmt->metalist[x].operation == 0x01F4)) break;

  simplifylist(var, stmt, 0, x);
  mysprintf(var, "%s", listpop());

  if (stmt->metalist[x].operation == 0x00F4)
    mysprintf(var, " GOTO ");
  else mysprintf(var, " GOSUB ");
 
  simplifylist(var, stmt, x+1, stmt->metapos);
  mysprintf(var, "%s", listpop());
}

void list_deffn(char *var, statement *stmt)
{
  int x = 0, y = 0, z = 0, ctr = 0;
  char *reorder[32];

  // find bounds for param list
  for (x=0; x<=stmt->metapos; x++)   
    if (stmt->metalist[x].operation == 0xF5) break; 

  for (y=x; y<=stmt->metapos; y++)
    if (stmt->metalist[y].operation == 0xF8) break; 

  simplifylist(var, stmt, x-1, x);
  mysprintf(var, "%s(", listpop());

  simplifylist(var, stmt, x, y);
  z = outcount;
  for (ctr = 0; ctr < z; ctr++) {
    reorder[ctr] = SafeMalloc(MAX_STRING_LENGTH);
    reorder[ctr] = listpop();
  }
  for (ctr = ctr-1; ctr >= 0; ctr--) {
    mysprintf(var, "%s", reorder[ctr]);
    if (ctr) mysprintf(var, ",");
  }

  simplifylist(var, stmt, y, stmt->metapos);
  mysprintf(var, ")=%s", listpop());
}

void simplifylist(char *var, statement *stmt, int start, int end)
{
  int y = 0, channels = 0;
  char tmp[MAX_STRING_LENGTH], *hex;
  char *tmp1, *tmp2, *tmp3;
  char *reorder[32]; int ctr = 0;

  for (y=start; y<=end; y++) {            
    oppush(0);
    if (get_sysvarname(stmt->metalist[y].operation)) {
      sprintf(tmp, "%s", get_sysvarname(stmt->metalist[y].operation));
      listpush(tmp);
    } else if (get_fncname(stmt->metalist[y].operation)) {
      if ((stmt->metalist[y].operation == OP_AND) ||
          (stmt->metalist[y].operation == OP_OR) ||
          (stmt->metalist[y].operation == OP_XOR)) goto blah;
      sprintf(tmp, "%s(", get_fncname(stmt->metalist[y].operation));
      if (stmt->metalist[y].intarg) {
        for (ctr=0; ctr<stmt->metalist[y].intarg; ctr++) {
          reorder[ctr] = SafeMalloc(MAX_STRING_LENGTH);
          reorder[ctr] = listpop();
        }
      } else {
        for (ctr=0; ctr<fnctable[get_fnc(get_fncname(stmt->metalist[y].
                operation))].numparms; ctr++) { 
          reorder[ctr] = SafeMalloc(MAX_STRING_LENGTH); 
          reorder[ctr] = listpop(); 
        }
      }     
     for (ctr=ctr-1; ctr>=0; ctr--) { 
        strcat(tmp, reorder[ctr]); 
        if (ctr) strcat(tmp, ",");
      }
      strcat(tmp, ")");
      listpush(tmp);
    } else  
blah:
      switch(stmt->metalist[y].operation) {
      case HEXSTRING:
        hex = SafeMalloc(2+(stmt->metalist[y].intarg*2));
        *hex = 0;
        sprintf(tmp, "$");
        for (ctr = 0; ctr < stmt->metalist[y].intarg; ctr++) {
          sprintf(hex, "%X", stmt->metalist[y].stringarg[ctr]);
          sprintf(hex, "%c%c", hex[strlen(hex)-2], hex[strlen(hex)-1]);
          strcat(tmp, hex);
        }
        strcat(tmp, "$");
        GC_free(hex);
        listpush(tmp);
        break;
      case 0xF5:
        if (stmt->opcode != CMD_DEFFN) listpush("<BEGINLIST>");
        break;
      case 0xF8: 
        if (stmt->opcode != CMD_DEFFN) {
          ctr = 0;
          do {
            reorder[ctr] = SafeMalloc(MAX_STRING_LENGTH);
            reorder[ctr] = listpop(); 
            if (!strcmp(reorder[ctr], "<BEGINLIST>")) break;
            ctr++; 
          } while(1);
          sprintf(tmp, "%s(", listpop());
          for (ctr = ctr-1; ctr >= 0; ctr--) {
            strcat(tmp, reorder[ctr]);
            if (ctr) strcat(tmp, ",");
          }         
          strcat(tmp, ")");
          listpush(tmp);
        }
        break;
      case 0x8DC9:
        listpush(gprog->userfunctions[stmt->metalist[y].shortarg].name);
        break;
      case 0xF1:
      case 0xEC:
        // workaround, LET handles its own commas
        if (stmt->opcode != CMD_LET) {
          sprintf(tmp, "%s,", listpop());
          listpush(tmp);
          for (ctr = 0; ctr < outcount; ctr++)
            mysprintf(var, "%s", listpop());
        }
        break;
      case 0xF4F1:
        if (channels) mysprintf(var, ") ");
        break;
      case 0xE1:
        channels = 1;
        mysprintf(var, "(%s", listpop());
        break;
      case OP_OPT:
      case OP_SEP:
      case OP_SRT:
      case OP_BNK:
      case OP_DOM:
      case OP_END:
      case OP_IND:
      case OP_IOL:
      case OP_ISZ: 
      case OP_KEY:
      case OP_SIZ:
      case OP_TBL:
      case OP_TIM:
      case OP_ERR:
      case OP_LEN:
      case OP_PWD:
      case OP_ATR:
        if (channels)
          mysprintf(var, ", %s=%s", get_symbol(stmt->metalist[y].operation), listpop());
        else {
          sprintf(tmp, "%s, %s=%s", listpop(), get_symbol(stmt->metalist[y].operation), listpop());
          listpush(tmp);
        }
        break;
      case LABELREF:
        sprintf(tmp, "%s", get_labelname(stmt->metalist[y].shortarg));
        listpush(tmp);
        break;
      case LINEREF:
        sprintf(tmp, "%.5d", stmt->metalist[y].shortarg);
        listpush(tmp);
        break;
      case MNEMONICREF:
        sprintf(tmp, "\'%s\'", stmt->metalist[y].stringarg);
        listpush(tmp);
        break;
      case 0xF7:
        tmp2 = listpop();
        sprintf(tmp, "@(%s,%s)", listpop(), tmp2);
        listpush(tmp);
        break;
      case 0xF6:
        sprintf(tmp, "@(%s)", listpop());
        listpush(tmp);
        break;
      case FLOAT:
        sprintf(tmp, "%s", flt2asc(stmt->metalist[y].floatarg));
        listpush(tmp);
        break;
      case GETVAL_STRINGARRAY:
      case SETVAL_STRINGARRAY:
      case GETVAL_STRING:
      case SETVAL_STRING:
        ctr = stmt->metalist[y].shortarg;
        tmp3 = SafeMalloc(MAX_STRING_LENGTH);
        if (stmt->metalist[y].shortarg & 0x2000) {
          ctr -= 0x2000;
          sprintf(tmp3, "(%s)", listpop());
        } else if (stmt->metalist[y].shortarg & 0x4000) {
          ctr -= 0x4000;
          tmp2 = listpop();
          sprintf(tmp3, "(%s,%s)", listpop(), tmp2);
        } else tmp3 = 0;
        if (stmt->metalist[y].shortarg & 0x8000) {
          ctr -= 0x8000;
          if (stmt->metalist[y].operation == GETVAL_STRINGARRAY ||
              stmt->metalist[y].operation == SETVAL_STRINGARRAY) {
            tmp1 = listpop(); 
            tmp2 = listpop();
            sprintf(tmp, "[%s,%s,%s]", listpop(), tmp2, tmp1);
            listpush(tmp);
          } else {
            sprintf(tmp, "[%s]", listpop());
            listpush(tmp);
          }
        } else if (stmt->metalist[y].operation == GETVAL_STRINGARRAY ||
                   stmt->metalist[y].operation == SETVAL_STRINGARRAY) {
          tmp1 = listpop();
          sprintf(tmp, "[%s,%s]", listpop(), tmp1);
          listpush(tmp);
        } else listpush(""); // noarrayref
        if (tmp3)
          sprintf(tmp, "%s%s%s", get_symname(ctr), listpop(), tmp3);
        else sprintf(tmp, "%s%s", get_symname(ctr), listpop());
        GC_free(tmp3);
        if ((stmt->metalist[y].operation == SETVAL_STRING) ||
            (stmt->metalist[y].operation == SETVAL_STRINGARRAY)) {
          if ((stmt->opcode == CMD_LET) || (stmt->opcode == CMD_FOR))
            strcat(tmp, "=");
        }
        listpush(tmp);
        break;
      case GETVAL_NUMERIC:
      case SETVAL_NUMERIC:        
      case GETVAL_NUMERICARRAY:
      case SETVAL_NUMERICARRAY:
        ctr = stmt->metalist[y].shortarg;
        if (stmt->metalist[y].shortarg & 0x8000) {
          ctr -= 0x8000;
          if (stmt->metalist[y].operation == GETVAL_NUMERICARRAY ||
              stmt->metalist[y].operation == SETVAL_NUMERICARRAY) {
            tmp1 = listpop(); 
            tmp2 = listpop();
            sprintf(tmp, "[%s,%s,%s]", listpop(), tmp2, tmp1);
            listpush(tmp);
          } else {
            sprintf(tmp, "[%s]", listpop());
            listpush(tmp);
          }
        } else if (stmt->metalist[y].operation == GETVAL_NUMERICARRAY ||
                   stmt->metalist[y].operation == SETVAL_NUMERICARRAY) {
          tmp1 = listpop();
          sprintf(tmp, "[%s,%s]", listpop(), tmp1);
          listpush(tmp);
        } else listpush(""); // noarrayref
        sprintf(tmp, "%s%s", get_symname(ctr), listpop());
        if (stmt->metalist[y].operation == SETVAL_NUMERIC ||
            stmt->metalist[y].operation == SETVAL_NUMERICARRAY) 
         if ((stmt->opcode == CMD_LET) || (stmt->opcode == CMD_FOR))
           strcat(tmp, "=");
        listpush(tmp);
        break;
      case SHORTLITERAL:
        sprintf(tmp, "\"%s\"", stmt->metalist[y].stringarg);
        listpush(tmp);
        break;
      case OP_NEGATE:
        sprintf(tmp, "(-%s)", listpop());
        listpush(tmp);
        break;
      case OP_STRCAT:
        tmp2 = listpop(); 
        sprintf(tmp, "%s+%s", listpop(), tmp2);
        listpush(tmp); 
        break;
      case OP_AND:
      case OP_XOR:
      case OP_OR:
      case OP_LESSTHAN:
      case OP_GREATTHAN:
      case OP_NOTEQUAL:
      case OP_EQUALSCMP:
      case OP_LTEQ:
      case OP_GTEQ:
      case OP_ADD:
      case OP_SUBTRACT:
      case OP_MULTIPLY:
      case OP_DIVIDE:
      case OP_EXPONENT:
        oppop();
        if (oppop() >= getprec(get_symbol(stmt->metalist[y].operation))) {
          tmp3 = listpop();
          tmp1 = SafeMalloc(strlen(tmp3)+2);
          sprintf(tmp1, "(%s)", tmp3);
        } else tmp1 = listpop();
        if (oppop() >= getprec(get_symbol(stmt->metalist[y].operation))) {
          tmp3 = listpop();
          tmp2 = SafeMalloc(strlen(tmp3)+2);
          sprintf(tmp2, "(%s)", tmp3);
        } else tmp2 = listpop();
        oppush(getprec(get_symbol(stmt->metalist[y].operation)));
        sprintf(tmp, "%s%s%s", tmp2, get_symbol(stmt->metalist[y].operation), tmp1);
        listpush(tmp);
        break;
    }
  }
}
