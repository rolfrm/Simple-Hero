#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"
#include "../bitguy/linmath.h"
#include "lisp_parser.h"
#include <libtcc.h>

void tccerror(void * opaque, const char * msg){
  printf("%s\n",msg);
}


typedef struct{
  
}scope_vars;

typedef struct{
  char * buffer;
  char * buffer_ptr;
  char * end_of_buffer;
}comp_state;

comp_state comp_state_make(char * buffer){
  comp_state out;
  out.buffer = buffer;
  out.buffer_ptr = buffer;
  return out;
}

void comp_state_delete(comp_state s){
  free(s.buffer);
}

static void inscriben(comp_state * state, char * data, size_t len){
  strncpy(state->buffer_ptr,data,len);
  state->buffer_ptr += len;
  state->buffer_ptr[0] = 0;
}

static void inscribe(comp_state * state, char * data){
  size_t len = strlen(data);
  inscriben(state,data,len);
}
static void compile_iexpr(comp_state * s, expr expr1);
static void compile_sexpr(comp_state * s, sub_expr sexpr){
  inscriben(s,sexpr.name.value,sexpr.name.strln);
  inscribe(s,"( ");
  for(int i = 0; i < sexpr.sub_expr_count; i++){
    compile_iexpr(s, sexpr.sub_exprs[i]);
    if(i != sexpr.sub_expr_count -1)
      inscribe(s,",");
  }
  inscribe(s,")");
}

static void compile_value(comp_state * state,value_expr vexpr){
  if(vexpr.type == STRING){
    inscribe(state,"\"");
    inscriben(state,vexpr.value,vexpr.strln);
    inscribe(state,"\"");
  }
}

static void compile_iexpr(comp_state * s, expr expr1){
  if(expr1.type == VALUE){
    compile_value(s, expr1.value);
  }else{
    compile_sexpr(s, expr1.sub_expr);
  }
}
  

void compile_expr(expr * e, void * lisp_state){
  static bool compiler_loaded = false;
  static TCCState * tccs;
  static int exprcnt = 0;
  if(!compiler_loaded){
    tccs = tcc_new();
    tcc_set_lib_path(tccs,".");
    tcc_set_error_func(tccs, NULL, tccerror);
    tcc_set_output_type(tccs, TCC_OUTPUT_MEMORY);
    compiler_loaded = true;
  }
  exprcnt++;
  char * buf = malloc(1000);
  
  comp_state s = comp_state_make(buf);
  char header[100];
  sprintf(header,"void test%i() {",exprcnt);
  inscribe(&s, header);
  compile_iexpr(&s, *e);
  inscribe(&s, ";");
  inscribe(&s, "}");
  printf("c code: %s\n",s.buffer);

  
  int ok = tcc_compile_string(tccs,s.buffer);
  int size = tcc_relocate(tccs, TCC_RELOCATE_AUTO);
  printf("size: %i\n", size);
  sprintf(header,"test%i",exprcnt);
  void (* fcn) () = tcc_get_symbol(tccs, header);

  if(fcn != NULL)
    fcn();
  //tcc_delete(tccs);
  free(buf);
}

bool lisp_compiler_test(){
  
  char * test_code = "(printf \"Hello World\\n\")(printf \"Hello Sailor!\\n\")";
  expr out_expr[2];
  char * next = test_code;
  while(next != NULL && *next != 0){
    printf("next: %s\n", next);
    int out = 2;
    next = lisp_parse(test_code,out_expr,&out);
    for(int i = 0; i < out; i++){
      compile_expr(out_expr + i, NULL);
      delete_expr(out_expr + i);
    }
    if(out == 0)
      break;
    //
  }
  return true;
}
