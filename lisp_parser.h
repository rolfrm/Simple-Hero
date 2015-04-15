typedef enum{
  EXPRESSION,
  VALUE,
  COMMENT
}expr_type;

typedef enum{
  // All numbers.
  NUMBER,
  // things starting with ':'
  KEYWORD,
  // "-delimited strings
  STRING,
  // Comments are ignored by the compiler
  COMMENT 
}value_type;

typedef struct{
  char * name;
  expression * sub_expressions;
  int sub_expression_count;  
}sub_expression_expr;

typedef struct{
  value_type value;
  char * value;
}value_expr;

typedef struct{
  expr_type type;
  union{
    sub_expression_expr sub_expression;
    value_expr value;
  };
}expression;

expression lisp_parse(char * code);
