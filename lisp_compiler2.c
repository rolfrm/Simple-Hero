#include <stdio.h>
#include <iron/full.h>
#include "lisp_parser.h"
#include <libtcc.h>
#include "lisp_types.h"
#include "lisp_std_types.h"
#include "lisp_compiler.h"
	

typedef struct{
  c_root_code * c_code;
  size_t code_cnt;
}compiled_lisp;

static type_def compile_value(compiler_state * c, c_value * val, value_expr e){
  val->type = C_INLINE_VALUE;
  var_def * vdef;
  switch(e.type){
  case STRING:
    val->raw.value = fmtstr("\"%.*s\"",e.strln,e.value);
    val->raw.type = &char_ptr_def;
    return char_ptr_def;
  case KEYWORD:
  case SYMBOL:
    vdef = get_variable(e.value, e.strln);
    val->raw.value = fmtstr("%.*s",e.strln, e.value);
    return vdef->type;
  case NUMBER:
    val->raw.value = fmtstr("%.*s",e.strln,e.value);
    val->raw.type = &i64_def;
    return i64_def;
    break;
  }
}
	  
static type_def _compile_expr(compiler_state * c, c_block * block, expr e ){
  type_def td;
  c_expr * exprs;
  size_t expr_cnt;
  c_value val;
  switch(e.type){
  case EXPR:
    break;
  case VALUE:
    td = compile_value(c,&val,e.value);
    expr_cnt = 1;
    exprs = alloc(sizeof(c_expr));
    exprs->type = C_VALUE;
    exprs->value = val;
    break;
  }
  block->exprs = exprs;
  block->expr_cnt = expr_cnt;
	  
  return td;
}

compiled_lisp * compile_lisp_to_c(compiler_state * c, expr * exp, size_t cnt){
  
}
