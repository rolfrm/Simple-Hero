#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lisp_parser.h"
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
  char * end = take_while(code, &is_keyword_char);
  
}

char * parse_symbol(char * code, value_expr * sym){
  char * end = take_while(code,is_keyword_char);
  if(end == code)
    return NULL;
  sym->value = code;
  sym->strln = end - code;
  if(!is_endexpr(*end))
    return NULL;
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
  string->value = code + 1;
  string->strln = end - code;
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
  string->type = STRING;
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
    return code;  
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

expression * lisp_parse(char * code){
  expression out_exprs[10];
  char * c2 = take_while(code, is_whitespace);
  expression out_expr;
  char * nc = parse_expression(code, &out_expr);
  return NULL;
}

void main(){
  lisp_parse("(hej 1.0)");
}
