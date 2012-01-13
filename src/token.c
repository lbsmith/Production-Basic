/*
 * $Id: token.c,v 1.12 2000/05/29 15:50:44 lees Exp $
 */

#include <string.h>
#include "basic.h"

extern int chaninfo;

int iswhite(char c);
int isoper(char c);
int islongoper(char *c);
int isverb(char *c);
int isreserved(char *s);

// sets token and token_type to reflect next occuring token
int get_token()
{
  char test[3];
  register char *temp;
  int x;

  lasttype = token_type;

  token_type = 0;
  temp = token;

  for (x=0; x<TOKEN_LEN; x++)
    token[x] = '\0';

  while(iswhite(*prog)) { tokenpos++; prog++; }

  if(*prog == '?')
  {
    prog++;
    tokenpos++;
    *temp++ = 'P'; 
    *temp++ = 'R';
    *temp++ = 'I';
    *temp++ = 'N';
    *temp++ = 'T';
    return(token_type = TOK_COMMAND);
  }

  if((*prog == '\'') && (lasttype == 1))
  {
    prog++;
    tokenpos++;
    *temp++ = 'E';
    *temp++ = 'D';
    *temp++ = 'I';
    *temp++ = 'T';
    return(token_type = TOK_COMMAND);
  }

/*  if((*prog == '.') && (lasttype == 1)) 
  {
    prog++;
    tokenpos++;
    *temp++ = 'P';
    *temp++ = 'B';
    *temp++ = 'S';
    *temp++ = 'T';
    *temp++ = 'E';
    *temp++ = 'P';
    return(token_type = TOK_COMMAND);
  }*/

  if(strchr("\n", *prog))
  {
     *temp = 0;
     return(token_type = TOK_DONE);
  }

  if(*prog == '\'')
  {
    prog++;
    tokenpos++;
    while (*prog != '\'' && *prog != '\r') {
      if (strchr("\n", *prog)) return (token_type = TOK_ERROR); 
      *temp++ = *prog++;
    }
    prog++;
    tokenpos++;
    *temp = 0;
    return (token_type = TOK_MNEMONIC);
  }

  if(*prog == '\"')
  {
    prog++;
    tokenpos++;
    while (*prog != '\"' && *prog != '\r') {
      if (strchr("\n", *prog)) return (token_type = TOK_ERROR);
      *temp++ = *prog++;
    }
    prog++;
    tokenpos++;
    *temp = 0;
    return(token_type = TOK_STRING);
  }

  if(isoper(*prog))
  {
    if (*prog == ',') { prog++; *temp = 0; return(token_type = TOK_COMMA); }
    if (*prog == ':') { prog++; *temp = 0; return(token_type = TOK_COLON); }
    if (*prog == ';') { prog++; *temp = 0; return(token_type = TOK_SEMICOLON); }
    if (strchr("*<>=", *prog)) {
      *temp = *prog; prog++; temp++;
      if (strchr("*<>=", *prog)) {
        *temp = *prog; prog++; temp++;
        if (!strcmp(token, "<<") || !strcmp(token, ">>") 
         || !strcmp(token, "*<") || !strcmp(token, "*>") 
         || !strcmp(token, "*=") || !strcmp(token, "<*") 
         || !strcmp(token, ">*") || !strcmp(token, "=*") 
         || !strcmp(token, "==")) {
          return (token_type = TOK_ERROR);
        }
      }
      *temp = 0;
    } else {
      *temp = *prog;
      prog++;
      temp++;
      *temp = 0;
    }
    return(token_type = TOK_OPERATOR);
  }

  if(isdigit(*prog) || *prog == '.')
  {
     while(!isoper(*prog) && !iswhite(*prog)) *temp++ = *prog++;
     *temp = 0;
     return(token_type = TOK_NUMBER);
  }
	
  while(!isoper(*prog) && !iswhite(*prog) && *prog != '"' && *prog != '\'') 
    *temp++ = *prog++;

  if (islongoper(token) && get_fnc(token)) {
    while (iswhite(*prog)) *temp++ = *prog++;
    if (*prog == '(') {
      while (iswhite(*prog)) *temp++ = *prog++;
      prog++;
      if (isdigit(*prog)) goto blah;
      else if ((*prog == '"') || (*prog == '$')) {
        for (x=strlen(token); x>=3; x--) prog--;
        for (x=3; x<strlen(token)+1; x++) token[x] = '\0';
        return (token_type = TOK_FUNCTION);
      } else if (isalpha(*prog)) { 
        while (isalpha(*prog)) *temp++ = *prog++;
        if (*prog == '$') {
          for (x=strlen(token); x>=3; x--) prog--;
          for (x=3; x<strlen(token)+1; x++) token[x] = '\0';
          return (token_type = TOK_FUNCTION); 
        } else if (*prog == '(') {
          // test if valid function or sysvar
          // if succeeded, test function/sysvar return type for strings
          test[0] = toupper(token[strlen(token)-3]);
          test[1] = toupper(token[strlen(token)-2]);
          test[2] = toupper(token[strlen(token)-1]);
          test[3] = '\0';
          if (get_fnc(test) || get_sysvar(test)) {
            for (x=strlen(token); x>=3; x--) prog--;
            for (x=3; x<strlen(token)+1; x++) token[x] = '\0';
            return (token_type = TOK_FUNCTION);
          } else goto blah;
        } else goto blah;
      } else goto blah;
    } else { 
blah:
      for (x=0; !iswhite(token[x]); x++) ;
      for (x=x; x<strlen(token)+1; x++) token[x] = '\0';
      return (token_type = TOK_OPERATOR);
    }
  } else {
    // !!HACK!!
    // will decide if this is a sysvar or operator the next time
    // gettoken is run.  if next token is '=' then it's an operator
    // if so it will change the metacode for this run.
    if (!strcmp(token, "ERR")) {
      checkerr = 2; 
      return(token_type = TOK_VARIABLE); //SYSVAR);
    }
    if (islongoper(token)) return(token_type = TOK_OPERATOR);
    if (isreserved(token)) return(token_type = TOK_RESERVED);
    if (((token[0] == 'F') || (token[0] == 'f')) &&
        ((token[1] == 'N') || (token[1] == 'n')))
       return (token_type = TOK_USERFUNCTION);
    if (get_opcode(token)) return(token_type = TOK_COMMAND);
    if (get_fnc(token)) return (token_type = TOK_FUNCTION);
    else return(token_type = TOK_VARIABLE);
  }
  return 0;
}

int isoper(char c)
{
  if (strchr("@;:,+-/*^=()[]<>", c)) return 1;
  return 0;
}

int isreserved(char *s)
{
  int i;
  char *p;

  p = s;
  while(*p) { *p = toupper(*p); p++; }
  for (i=0; *resvtable[i].name; i++)
    if (!strcmp(resvtable[i].name, s)) return 1; 
  return 0;
}

int islongoper(char *s)
{
  int i;
  char *p;

  p = s;
  while(*p) { *p = toupper(*p); p++; }
  if (chaninfo == 1) i=0; else i=15;
  for (i=i; *optable[i].symbol; i++) 
    if (!strcmp(optable[i].symbol, s)) return 1;
  return 0;
}

int iswhite(char c)
{
  if(c == ' ' || c == '\t') return 1;
  else return 0;
}
