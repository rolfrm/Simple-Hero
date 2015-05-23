
#include "lisp_compiler.h"

typedef struct{
  c_root_code * c_code;
  size_t code_cnt;
}compiled_lisp;


static type_def compile_expr(compiler_state * c, c_block * block, expr e ){
  switch(e.type){
  case EXPR:
    break;
  case VALUE:
    break;
  }
}

compiled_lisp * compile_lisp_to_c(compiler_state * c, expr * exp, size_t cnt){
  
}
