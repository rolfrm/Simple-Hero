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

static type_def * compile_value(compiler_state * c, c_value * val, value_expr e){
  val->type = C_INLINE_VALUE;
  var_def * vdef;
  switch(e.type){
  case STRING:
    val->raw.value = fmtstr("\"%.*s\"",e.strln,e.value);
    val->raw.type = &char_ptr_def;
    return &char_ptr_def;
  case KEYWORD:
  case SYMBOL:
    val->raw.value = fmtstr("%.*s",e.strln, e.value);
    vdef = get_variable(e.value, e.strln);
    if(vdef == NULL){
      ERROR("Unknown variable '%s'", val->raw.value);
      return &error_def;
    }
    return vdef->type;
  case NUMBER:
    val->raw.value = fmtstr("%.*s",e.strln,e.value);
    val->raw.type = &i64_def;
    return &i64_def;
    break;
  }
}
static type_def * _compile_expr(compiler_state * c, c_block * block, c_value * val,  expr e );
static type_def * __compile_expr(compiler_state * c, c_block * block, c_value * value, sub_expr * se){
  type_def * sub_types[se->sub_expr_count];
  c_value val[se->sub_expr_count];
  for(size_t i = 0; i < se->sub_expr_count; i++){
    expr * e = se->sub_exprs + i;
    sub_types[i] = _compile_expr(c, block, val + i, *e);
  }
  return &error_def;
}
	  
static type_def * _compile_expr(compiler_state * c, c_block * block, c_value * val,  expr e ){
  type_def * td;
  c_expr * exprs;  
  size_t expr_cnt;
  switch(e.type){
  case EXPR:
    return __compile_expr(c, block, val, &e.sub_expr);
    break;
  case VALUE:
    td = compile_value(c,val,e.value);
    expr_cnt = 1;
    exprs = alloc(sizeof(c_expr));
    exprs->type = C_VALUE;
    exprs->value =*val;
    break;
  }
  block->exprs = exprs;
  block->expr_cnt = expr_cnt;
	  
  return td;
}

compiled_lisp * compile_lisp_to_c(compiler_state * c, expr * exp, size_t cnt){
  for(size_t i = 0; i < cnt; i++){
    c_value val;
    c_block blk;
    blk.expr_cnt = 0;
    blk.exprs = NULL;
    type_def * t = _compile_expr(c, &blk, &val, exp[i]);
    list_add((void **) &blk.exprs, &blk.expr_cnt, &val, sizeof(val));
  }
  return NULL;
}

type_def _type_macro(expr typexpr);
bool test_lisp2c(){
  //type_def fcn_def =
  char * test_code = "(defun printhello ()(print_string \"hello\\n\"))";
  test_code = "i64_def";
  size_t exprcnt;
  expr * exprs = lisp_parse_all(test_code, &exprcnt);
  load_defs();
  /*  compiler_state * c = compiler_make();
  with_compiler(c,lambda(void, (){
	load_defs();
	compiler_load_types(c);
	logd("parsed %i expr(s).\n", exprcnt);
	compile_lisp_to_c(c,exprs,exprcnt);
	};));*/
  return TEST_FAIL;
}
