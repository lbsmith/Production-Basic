/*
 * $Id: stack.c,v 1.9 2000/05/29 15:50:36 lees Exp $
 */

#include "basic.h"

int getprec(char *str);

int stackcount = 0;
stackobj *stack[STACK_DEPTH];

extern int chaninfo;
extern int foundequals;
extern int numlabels;
extern int argctr;
extern int numargs[];

void stackinit()
{
  stackcount = 0;
}

int stackempty()
{
  if (stackcount == 0) return 1;
  else return 0;
}

// push operator on stack
void push(char *name, int type)
{
  stackobj *obj;
  int x;

  obj = SafeMalloc(sizeof(stackobj));
  for (x=0; x<strlen(name); x++)
    obj->name[x] = name[x];
  obj->name[strlen(name)] = '\0';
  obj->type = type;

  if ((obj->name[0] == '(') || (obj->name[0] == '[')) parencount++;

  if ((obj->name[0] == '=') && chaninfo == 0)
  { numargs[parencount+1] = 0; foundequals = 1; } 

  stack[stackcount] = obj;

  stackcount++;
}

// pop operator off stack
stackobj *pop()
{
  if (stackcount == 0) return 0;
  stackcount--;
  if ((stack[stackcount]->name[0] == '(') ||
      (stack[stackcount]->name[0] == '[')) parencount--;
    if ((stack[stackcount]->name[0] != '(') && 
        (stack[stackcount]->name[0] != '[') &&
        (stack[stackcount]->name[0] != '=') &&
        (stack[stackcount]->name[0] != '_')) {
    if (numlabels >= 2) numlabels--;
    else numlabels -= 100;
  }
  return stack[stackcount];
}

// pop remainder of stack
// poplevel=-2 to pop until an open paren is found
void popstack(statement *stmt, int poplevel)
{
  int x;
  stackobj *obj;

  if (poplevel == -2) {
    do {
      obj = pop();
      if ((obj->name[0] == '(') || (obj->name[0] == '[')) break;
      else buffermeta(stmt, obj->type, obj->name);
      GC_free(obj);
    } while (1);
  } else for (x=stackcount; stackcount > 0; x--) {
    obj = pop();
    buffermeta(stmt, obj->type, obj->name);
    GC_free(obj);
  }
}

// return precedence of passed operator
int getprec(char *str)
{
  int x;

  if (!get_opercode(str)) return 1;

  for (x=0; *optable[x].symbol; x++) {
    if (!strcmp(str, optable[x].symbol)) {
      return optable[x].precedence;
    }
  }

  return OP_ERROR;
}

// valid only for operators for obvious reasons
int checkprec(char *str, int type)
{
  if ((stack[stackcount-1]->name[0] == '(') ||
      (stack[stackcount-1]->name[0] == '[')) return 1;
  if (stack[stackcount-1]->type != TOK_OPERATOR) return 0;
  if (type != TOK_OPERATOR) return 1;
  if (getprec(str) >= getprec(stack[stackcount-1]->name)) return 0;
  else return 1; 
}
