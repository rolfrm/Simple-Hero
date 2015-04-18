// requires nothing.
typedef enum{
  EXPRESSION = 5,
  VALUE = 6
}expr_type;

typedef enum{
  // All numbers.
  NUMBER = 1,
  // things starting with ':'
  KEYWORD = 2,
  // "-delimited strings
  STRING = 3,
  // Comments are ignored by the compiler
  COMMENT = 4,
  // Symbols are used first in functions.
  SYMBOL = 5,
  VALUE_TYPE_LAST
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

char * lisp_parse(char * code, expression * out_exprs, int * out_exprs_count);
void print_expression(expression * expr);
int test_lisp_parser();
