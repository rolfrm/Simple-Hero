typedef enum{
  EXPRESSION,
  VALUE
}expr_type;

typedef enum{
  // All numbers.
  NUMBER,
  // things starting with ':'
  KEYWORD,
  // "-delimited strings
  STRING,
  // Comments are ignored by the compiler
  COMMENT,
  // Symbols are used first in functions.
  SYMBOL
}value_type;

typedef struct{
  value_type type;
  char * value;
  int strln;
}value_expr;

typedef struct _expression expression;

typedef struct{
  value_expr name;
  expression * sub_expressions;
  int sub_expression_count;  
}sub_expression_expr;

struct _expression{
  expr_type type;
  union{
    sub_expression_expr sub_expression;
    value_expr value;
  };
};

expression * lisp_parse(char * code);
