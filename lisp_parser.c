#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "lisp_parser.h"

#include <stdio.h>

char * take_while(char * data, bool (* fcn)(char char_code)){
  while(fcn(data[0])) data++;
  return data;
}

bool is_alphanum(char c){
  return isdigit(c) || isalpha(c);
}

bool is_keyword_char(char c){
  return is_alphanum(c);
}

bool is_whitespace(char c){
  return isspace(c);
}

bool is_endexpr(char c){
  return c == ')' || is_whitespace(c) || c == 0;
}

char * parse_keyword(char * code, value_expr * kw){
  if(code[0] != ':')
    return NULL;
  code++;
  char * end = take_while(code, &is_keyword_char);
  if(!is_endexpr(*end)){
    return NULL;
  }
  kw->type = KEYWORD;
  kw->value = code;
  kw->strln = end - code;
  return end;
}

char * parse_symbol(char * code, value_expr * sym){
  char * end = take_while(code,is_keyword_char);
  if(end == code)
    return NULL;
  if(!is_endexpr(*end))
    return NULL;
  sym->value = code;
  sym->strln = end - code;
  sym->type = SYMBOL;
  return end;
}
char * read_to_end_of_string(char * code){
  while(true){
    code++;
    if(*code == '"')
      return code + 1;
    if(*code == 0)
      return code;
    if(*code == '\\')
      code++;//next item is escaped. skip it.
  }
  //this is an error.
  return NULL;
}

char * parse_string(char * code, value_expr * string){
  if(*code != '"') return NULL;
  code++;
  char * end = read_to_end_of_string(code);
  string->value = code;
  string->strln = end - code - 1; //-1: last "
  string->type = STRING;
  return end;
}

char * parse_number(char * code, value_expr * string){
  int decimal_reached = 0;
  char * it = code;
  for(; false == is_endexpr(*it); it++){
    if(*it == '.'){
      if(decimal_reached)
	return NULL;
      decimal_reached = 1;
    }else if(!isdigit(*code)){
      return NULL;
    }
  }
  string->value = code;
  string->type = NUMBER;
  string->strln = it - code;
  return it;
}

char * parse_value(char * code, value_expr * val){
  char * next;
  next = parse_string(code, val);
  if(next != NULL) return next;
  next = parse_keyword(code, val);
  if(next != NULL) return next;
  next = parse_number(code, val);
  if(next != NULL) return next;

  return NULL;
}
char * parse_expression(char * code, expression * out_expr);

char * parse_subexpression(char * code, sub_expression_expr * subexpr){
  if(code[0] != '(')
    return NULL;
  code++;
  code = take_while(code,is_whitespace);
  value_expr sym;
  code = parse_symbol(code,&sym);
  if(code == NULL)
    return NULL;
  subexpr->name = sym;
  subexpr->sub_expressions = NULL;
  subexpr->sub_expression_count = 0;
  expression exprs[10];
  int len = 0;
 next_part:
  code = take_while(code,is_whitespace);
  if(*code == ')'){
    subexpr->sub_expression_count = len;
    subexpr->sub_expressions = malloc(len * sizeof(expression));
    memcpy(subexpr->sub_expressions, exprs, len * sizeof(expression));
    
    return code + 1;  
  }
  code = parse_expression(code, exprs + len);
  if(code == NULL)
    return NULL;
  len++;
  goto next_part;

}

char * parse_expression(char * code, expression * out_expr){
  code = take_while(code,is_whitespace);
  {// parse subexpression.
    sub_expression_expr subexpr;
    char * next = parse_subexpression(code,&subexpr);
    if(next != NULL){
      out_expr->type = EXPRESSION;
      out_expr->sub_expression = subexpr;
      return next;
    }
  }
  
  value_expr value;
  { // parse value. can be many things.
    char * next = parse_value(code, &value);
    if(next != NULL){
      out_expr->value = value;
      out_expr->type = VALUE;
      return next;
    }
  }
  return NULL;
}

void delete_expression(expression * expr){
  
}

void print_expression(expression * expr){
    value_expr value = expr->value;
    sub_expression_expr subexpr = expr->sub_expression;
    
  switch(expr->type){
  case EXPRESSION:
    printf("name: %.*s \n", value.strln, subexpr.name.value);
    for(int i = 0 ; i < subexpr.sub_expression_count; i++){
      print_expression(subexpr.sub_expressions + i);
    }
    break;
  case VALUE:
    printf("Value: %i %.*s\n",value.type, value.strln,value.value);
    break;
  }
}


char * lisp_parse(char * code, expression * out_exprs, int out_exprs_count){
  for(int i = 0 ; i < out_exprs_count; i++){
    code = take_while(code, is_whitespace);
    char * cn = parse_expression(code, out_exprs + i);
    printf("parse.. %s\n", code);
    if(cn == NULL){
      return NULL;
    }
    print_expression(out_exprs + i);

    code = cn;
    if(*code == 0) return code;
  }
  return code;
}

int main(){
  lisp_parse("(hej 1.0312)(add (sub 1 :a 5  \"hello\") 2)");
  return 0;
}
