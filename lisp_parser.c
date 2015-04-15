
char * take_while(char * data, int (* fcn)(char char_code)){
  while(fcn(data[0])) data++;
  return data;
}

int is_keyword_char(char c){
  return isalphanum(c);
}

int is_whitespace(char c){
  return isspace(c);
}

char * parse_keyword(char * code, value_expr * kw){
  if(code[0] != ':')
    return NULL;
  char * end = take_while(code, &is_keyword_char);
  
}

char * parse_value(char * code, value_expr * val){
  
}

expression lisp_parse(char * code){
  char * c2 = take_while(code, is_whitespace);
}
