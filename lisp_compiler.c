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
#include "lisp_types.h"
#include "lisp_std_types.h"
#include "lisp_compiler.h"

static __thread comp_state * compstate = NULL;
static __thread compiler_state * lisp_state = NULL;

bool fcn_def_cmp(fcn_def a, fcn_def b){
  return type_def_cmp(a.type,b.type)
    && strcmp(a.name,b.name) == 0;
}

// todo: delete this.
size_t write_def_to_buffer(type_def def, char * buffer, size_t buffer_len){
  FILE * f = fmemopen(buffer,buffer_len, "w");
  FILE * oldstdout = stdout;
  stdout = f;
  print_def(def,0,false);
  fflush(f);
  stdout = oldstdout;
  size_t pos = ftell(f);
  fclose(f);
  return pos;
}

compiler_state * compiler_make(){
  return calloc(1, sizeof(compiler_state));
}

void tccerror(void * opaque, const char * msg){
  UNUSED(opaque);
  format("%s\n",msg);
}

comp_state comp_state_make(){
  comp_state out;
  out.buffer = NULL;
  //out.fcns = NULL;
  //out.fcn_cnt = 0;
  out.deps = NULL;
  out.dep_cnt = 0;
  return out;
}

var_def * get_variable(char * name, size_t name_len){
  compiler_state * c = lisp_state;
  for(size_t i = 0;i < c->var_cnt; i++){
    for(size_t j = 0; j < name_len; j++)
      if(name[j] != c->vars[i].name[j])
	goto next_item;
    return c->vars + i;
  next_item:
    continue;
  }
  return NULL;
}

var_def * get_variable2(char * name){
  compiler_state * c = lisp_state;
  for(size_t i = 0;i < c->var_cnt; i++){
    if(strcmp(name,c->vars[i].name) != 0)
      goto next_item;
    return c->vars + i;
  next_item:
    continue;
  }
  return NULL;
}

type_def * get_type_def(char * name, size_t len){
  compiler_state * c = lisp_state;
  for(size_t i = 0;i < c->var_cnt; i++){
    var_def * v = c->vars + i;
    printf("loc type\n");
    if(false == type_def_cmp(v->type,type_def_def))
      goto next_item;
    type_def * d = (type_def *) v->data;
    char * tname = NULL;
    if(d->kind == SIMPLE){
      tname = d->simple.name;
    }
    printf("TYPE: %s\n",tname);
    if(tname != NULL){
      if(strncmp(tname,name,len) ==0){
	return d;
      }
    }
  next_item:
    continue;
  }
  return NULL;
}


fcn_def * get_fcn_def(char * name, size_t name_len){
  var_def * var = get_variable(name, name_len);
  if(var == NULL){
    return NULL;
  }
  
  if(false == type_def_cmp(var->type,fcn_def_def)){
    return NULL;
  }
  return (fcn_def *) var->data;
}

cmacro_def * get_cmacro_def(char * name, size_t name_len){
  var_def * var = get_variable(name, name_len);
  if(var == NULL){
    return NULL;
  }
  
  if(false == type_def_cmp(var->type,cmacro_def_def)){
    return NULL;
  }
  return (cmacro_def *) var->data;
}

void add_dep(char * name){
  comp_state * s = compstate;
  for(size_t i = 0; i < s->dep_cnt; i++){
    char * dep = s->deps[i];
    if(strcmp(dep,name) == 0){
      return;
    }
  }
  list_add((void **) &s->deps, &s->dep_cnt, &name, sizeof(name));
}

static type_def compile_sexpr(sub_expr sexpr){
  fcn_def * fcn;
  for(int i = 0; i < sexpr.sub_expr_count; i++){
    if(i == 0){ 
      // first arg must be a symbol.
      if(sexpr.sub_exprs[0].type == EXPR){
	ERROR("First arg must be a symbol");
	return error_def;
      }
      
      value_expr sexpr2 = sexpr.sub_exprs[0].value;
      fcn = get_fcn_def(sexpr2.value,sexpr2.strln);
      if(fcn != NULL){
	add_dep(fcn->name);
	format("%s(",fcn->name);	
      }else{
	cmacro_def * cmac = get_cmacro_def(sexpr2.value,sexpr2.strln);
	if(cmac == NULL){
	  ERROR("cannot handle %.*s",sexpr2.strln,sexpr2.value);
	  return error_def;
	}
	int argcnt = sexpr.sub_expr_count -1;
	if(argcnt != cmac->arg_cnt){
	  ERROR("Macro '%s' cannot handle %s arguments\n", cmac->name, argcnt);
	  return error_def;
	}
	expr * se = sexpr.sub_exprs;
	switch(argcnt){
	case 0:
	  return ((type_def (*)())cmac->fcn)();
	case 1:
	  return ((type_def (*)(expr))cmac->fcn)(se[1]);
	case 2:
	  return ((type_def (*)(expr,expr))cmac->fcn)(se[1],se[2]);
	case 3:
	  return ((type_def (*)(expr,expr,expr))cmac->fcn)(se[1],se[2],se[3]);
	case 4:
	  return ((type_def (*)(expr,expr,expr,expr))cmac->fcn)(se[1],se[2],se[3],se[4]);
	default:
	  ERROR("%i arguments are not supported", argcnt);
	  return error_def;
	}
      }

    }else{
      expr exp = sexpr.sub_exprs[i];
      type_def def = compile_iexpr(exp);
      if(type_def_cmp(def,error_def)){
	return def;
      }
      
      if(false == type_def_cmp(def, fcn->type.fcn.args[i -1].type)){
	ERROR("Invalid argument %i for '%s'", i -1, fcn->name);
	print_cdecl(fcn->type.fcn.args[i-1]);
	decl arg;
	arg.name = "arg";
	arg.type = def;
	print_cdecl(arg);
	return error_def;
      }
      if(i != sexpr.sub_expr_count -1)
	format(", ");
    }
  }
  format(")");
  return *(fcn->type.fcn.ret);
}

type_def compile_value(value_expr vexpr){
  if(vexpr.type == STRING){
    format("\"%.*s\"",vexpr.strln, vexpr.value);
    return char_ptr_def;
  }else if(vexpr.type == KEYWORD || vexpr.type == SYMBOL){
    format("%.*s ",vexpr.strln,vexpr.value);
    var_def * var = get_variable(vexpr.value,vexpr.strln);
    add_dep(var->name);
    return var->type;
  }else if(vexpr.type == NUMBER){
    format("%.*s ",vexpr.strln,vexpr.value);
    return i64_def;
  }
  ERROR("Unhandled type '%i'", vexpr.type);
  return error_def;
}

type_def compile_iexpr(expr expr1){
  if(expr1.type == VALUE){
    return compile_value(expr1.value);
  }else{
    return compile_sexpr(expr1.sub_expr);
  }
  return error_def;
}
	  
void compiler_set_state(compiler_state * ls){
  lisp_state = ls;
}

compiled_expr compile_expr(expr * e){

  compiled_expr err;
  err.fcn = NULL;
  err.result_type = error_def;

  size_t cdecl_size = 0;
  comp_state * olds = compstate;
  comp_state s = comp_state_make();
  compstate = &s;

  s.c = lisp_state;
  FILE * stream = open_memstream(&s.buffer, &cdecl_size);
  type_def td;
  with_format_out(stream,lambda(void,(){ td = compile_iexpr(*e); }));
  fclose(stream);
  if(type_def_cmp(error_def,td)){
    ERROR("COULD NO COMPILE!\n");
    return err;
  }
  char * prebuffer = NULL;
  size_t pre_size = 0;
  type_def dep_graph[100];
  for(size_t i = 0; i < array_count(dep_graph); i++)
    dep_graph[i] = void_def;
  
  struct{
    char * name;
    void * data;
  }symbols[50];
  size_t symcnt = 0;
  void addsym(char * name, void * ptr){
    symbols[symcnt].name = name;
    symbols[symcnt].data = ptr;
    symcnt++;
  }
  
  stream = open_memstream(&prebuffer,&pre_size);

  void expand_deps(){
    for(size_t i = 0; i < s.dep_cnt; i++){
      var_def * var = get_variable2(s.deps[i]);
      if(var == NULL){
	ERROR("Undefined variable '%s'",s.deps[i]);
	return;
      }
      
      if(type_def_cmp(var->type,fcn_def_def)){
	fcn_def * fcn = var->data;
	if(fcn->is_extern == false){
	  addsym(fcn->name,fcn->ptr);
	}
	decl dcl = {fcn->name, fcn->type};
	make_dependency_graph(dep_graph,dcl.type);
      }else{
	addsym(var->name,var->data);
	make_dependency_graph(dep_graph,var->type);
      }
    }
    write_dependencies(dep_graph);
    for(size_t i = 0; i < s.dep_cnt; i++){
      var_def * var = get_variable2(s.deps[i]);
      if(type_def_cmp(var->type,fcn_def_def)){
	fcn_def * fcn = var->data;
	decl dcl = {fcn->name, fcn->type};
	print_cdecl(dcl);
      }else{
	decl dcl = {var->name, var->type};
	format("extern ");
	print_cdecl(dcl);
      }
    }
    print_def(td,0,true);
    format(" __eval(){\n %s %s;\n}", type_def_cmp(void_def,td) ? "" : "return", s.buffer); 
  }

  with_format_out(stream,expand_deps);
  compstate = olds;
  fclose(stream);
  printf("Compiling:\n--------------\n\n%s\n\n ------------\n\n", prebuffer);

  TCCState * tccs = tcc_new();
  tcc_set_lib_path(tccs,".");
  tcc_add_library_path(tccs,"/usr/lib/x86_64-linux-gnu/");
  tcc_add_library(tccs, "glfw");
  tcc_set_error_func(tccs, NULL, tccerror);
  tcc_set_output_type(tccs, TCC_OUTPUT_MEMORY);
  for(size_t i = 0; i < symcnt; i++){
    tcc_add_symbol(tccs, symbols[i].name, symbols[i].data);
  }
  int ok = tcc_compile_string(tccs,prebuffer);

  if(ok != 0){
    ERROR("Unable to compile %s\n error: %i",prebuffer, ok);
    return err;
  }
  int size = tcc_relocate(tccs, NULL);
  tcc_relocate(tccs,malloc(size));
  if(size == -1){
    ERROR("Unable to link %s\n",s.buffer);
    return err;
  }
  
  void * fcn = tcc_get_symbol(tccs, "__eval");
  tcc_delete(tccs);
  free(prebuffer);
  free(s.buffer);
  
  if(fcn == NULL){
    ERROR("Unable to create function");
    return err;
  }
  compiled_expr fdef = {td, fcn};
  return fdef;
}

#define COMPILE_ASSERT(expr) if(!(expr)){ERROR("Compile error");return error_def;}

type_def cast_macro(expr arg1, expr typearg){
  compiled_expr typexpr = compile_expr(&typearg);
  if(type_def_cmp(typexpr.result_type,type_def_def) == false){
    ERROR("Unexpected type");
    return error_def;
  }
  type_def (* fcn)() = typexpr.fcn;
  type_def result = fcn();
  with_format_out(stdout, lambda(void,(){print_def(result,0,true);}));
  if(result.kind == POINTER || result.kind == SIMPLE){
    // only pointers and simple types can be casted
    format("((");
    print_def(result,0,true);
    format(") ");
    compile_iexpr(arg1);
    format(")");
    return result;
  }else{
    ERROR("Cannot cast invalid type");
    return error_def;
  }
}

type_def _type_macro(expr typexpr);
bool read_decl(expr dclexpr, decl * out){
    if(dclexpr.type == EXPR){
      sub_expr sexpr = dclexpr.sub_expr;
      if(sexpr.sub_expr_count == 2){
	expr name = sexpr.sub_exprs[0];
	expr type = sexpr.sub_exprs[1];
	if(name.type == VALUE && name.value.type == SYMBOL){
	  out->name = malloc(name.value.strln + 1);
	  strncpy(out->name,name.value.value,name.value.strln);
	  out->type = _type_macro(type);
	  return false == type_def_cmp(error_def,out->type);
	}
      }
    }
    return false;
  }

type_def _type_macro(expr typexpr){
    
  if(typexpr.type == EXPR){
    sub_expr sexp = typexpr.sub_expr;
    COMPILE_ASSERT(sexp.sub_expr_count > 0);
    expr kind = sexp.sub_exprs[0];
    COMPILE_ASSERT(kind.type == VALUE && kind.value.type == SYMBOL);
    value_expr vkind = kind.value;
    if(strncmp(vkind.value,"fcn",vkind.strln) == 0){
      type_def out;
      out.kind = FUNCTION;
      COMPILE_ASSERT(sexp.sub_expr_count > 1);
      type_def ret = _type_macro(sexp.sub_exprs[1]);
      COMPILE_ASSERT(false == type_def_cmp(error_def,ret));
      decl args[sexp.sub_expr_count - 2];
      for(int i = 0; i < sexp.sub_expr_count - 2; i++){
	COMPILE_ASSERT(read_decl(sexp.sub_exprs[i + 2], args + i));
      } 
      out.fcn.ret = malloc(sizeof(type_def));
      *out.fcn.ret = ret;
      out.fcn.args = malloc(sizeof(args));
      memcpy(out.fcn.args,args,sizeof(args));
      out.fcn.cnt = array_count(args);
      return out;
    }
  }else{
    value_expr vexp = typexpr.value;
    COMPILE_ASSERT(vexp.type == SYMBOL);
    type_def * td = get_type_def(vexp.value,vexp.strln);
    COMPILE_ASSERT(td != NULL);
    return *td;
  }
}
void * compiler_define_variable(compiler_state *c, char * name, type_def t);

type_def type_macro(expr typexpr){
  static int _typeid = 0;
  _typeid++;
  char * typeid = malloc(10);
  sprintf(typeid, "type%i",_typeid);
  type_def td = _type_macro(typexpr);

  type_def * typevar = (type_def *) compiler_define_variable(lisp_state,typeid,type_def_def);
  *typevar = td;
  add_dep(typeid);
  format(typeid);
  return *typevar;
  
}

type_def new_macro(expr typexpr, expr body){
  type_def td = _type_macro(typexpr);
  if(td.kind == FUNCTION){
    format("NULL");
  }
  return td;
}

type_def defun_macro(expr types, expr body){
  COMPILE_ASSERT(types.type == EXPR);
  sub_expr types2 = types.sub_expr;
  int exprcnt = types2.sub_expr_count;
  COMPILE_ASSERT(exprcnt > 0);
  expr retexpr = types2.sub_exprs[0];
  COMPILE_ASSERT(retexpr.type == VALUE && retexpr.value.type == SYMBOL);
  
  type_def def;
  def.kind = FUNCTION;
}

void print_cdecl(decl idecl){
  void inner_print(decl idecl){
    
    type_def def = idecl.type;
    switch(idecl.type.kind){
    case TYPEDEF:
    case STRUCT:
    case SIMPLE:
    case POINTER:
      print_def(def,0,true);
      format("%s",idecl.name);
      break;
    case FUNCTION:
      
      print_def(*def.fcn.ret,0,true);
      format("%s( ",idecl.name);
      for(i64 i = 0;i<def.fcn.cnt; i++){
	inner_print(def.fcn.args[i]);
	if(i + 1 != def.fcn.cnt)
	  format(", ");
      }
      format(")");
      break;
    default:
      ERROR("Not supported: '%i'\n", def.kind);
    }
  }

  inner_print(idecl);
  format(";\n");
}

size_t load_cdecl(char * buffer, size_t buffer_len, decl idecl){
  
  FILE * f = fmemopen(buffer,buffer_len, "w");
  void go(){
    print_cdecl(idecl);	  
  }
  with_format_out(f,go);
  size_t pos = ftell(f);  
  fclose(f);
  return pos;  
}

TCCState * mktccs(){
  TCCState * tccs = tcc_new();
  tcc_set_lib_path(tccs,".");
  tcc_set_error_func(tccs, NULL, tccerror);
  tcc_set_output_type(tccs, TCC_OUTPUT_MEMORY);
  return tccs;
}

void write_dependencies(type_def * deps){
  size_t dep_cnt = 0;
  for(; type_def_cmp(deps[dep_cnt], void_def) == false; dep_cnt++);
  format("#include \"cstd_header.h\"\n");
  for(size_t i = 0; i < dep_cnt; i++){
    type_def t = deps[i];
    if(t.kind == STRUCT){
      char * name = t.cstruct.name;
      if(name != NULL){
	format("struct %s;\n", name);
      }
    }
    if(t.kind == TYPEDEF){
      type_def inner = *t.ctypedef.inner;
      if(inner.kind == STRUCT){
	char * name = inner.cstruct.name;
	char _namebuf[100];
	if(name == NULL){
	  sprintf(_namebuf, "_%s_", t.ctypedef.name);
	  name = _namebuf;
	}	
	format("typedef struct %s %s;\n", name, t.ctypedef.name);
      }
      if(inner.kind == ENUM){
	format("typedef enum {\n");
	for(int j = 0; j < inner.cenum.cnt; j++){
	  char * comma = (j !=(inner.cenum.cnt-1) ? "," : "");
	  format("   %s = %i%s\n", inner.cenum.names[j], inner.cenum.values[j], comma);
	}
	format("}%s;\n",t.ctypedef.name);
      }
    }
  }

  for(size_t i = 0; i < dep_cnt; i++){
    type_def t = deps[i];
    if(t.kind != STRUCT) continue;
    print_def(t,0,false);
    format(";\n");
  }
}

void * compiler_define_variable(compiler_state *c, char * name, type_def t){
  // this is a bit complex. I need to run the code which defines and sets that variable
  // todo: It is probably not nessesary to invoke tcc just to figure out the size of t.

  type_def required_types[100];
  for(size_t i = 0; i < array_count(required_types); i++)
    required_types[i] = void_def;
  make_dependency_graph(required_types,t);
  
  char * cdecl;
  { // write code to buffer
    size_t cdecl_size = 0;
    FILE * stream = open_memstream(&cdecl, &cdecl_size);
    decl dcl;
    dcl.name = name;
    dcl.type = t;
    void go(){
      write_dependencies(required_types);
      print_cdecl(dcl);
    }
    with_format_out(stream, go);
    fclose(stream);
    //locbuf += cnt;
    //restsize -= cnt;	
  }

  TCCState * tccs = mktccs();

  tcc_compile_string(tccs, cdecl);
  
  int size = tcc_relocate(tccs, NULL);
  char * codebuf = malloc(size);
  tcc_relocate(tccs, codebuf);
  void * var = tcc_get_symbol(tccs, name);
  var_def vdef;
  vdef.name = name;
  vdef.type = t;
  vdef.data = var;
  list_add((void **)&c->vars, &c->var_cnt,&vdef,sizeof(var_def));
  tcc_delete(tccs);
  free(cdecl);
  return var;
}
void tccs_test2();
compiler_state * cs = NULL;
fcn_def defext(char * name, type_def type){
  // defining an external function is close to defining an internal one
  // except you dont have the code for it. I can do this earlier.
  fcn_def fdef;
  fdef.name = name;
  fdef.type = type;
  fdef.is_extern = true;
  fdef.ptr = NULL;
  //
  return fdef;
}

void print_string(char * string){
  format("%s", string);
}
	  
void print_dep_graph(type_def * defs){
  for(int i = 0; type_def_cmp(void_def,defs[i]) == false; i++){
    format("%i ",defs[i].kind);
    if(defs[i].kind == SIMPLE)
      format("%s ", defs[i].simple.cname);
    if(defs[i].kind == STRUCT){
      if(defs[i].cstruct.name != NULL)
	format("%s ", defs[i].cstruct.name);
    }
    if(defs[i].kind == TYPEDEF){
      if(defs[i].ctypedef.name != NULL)
	format("%s ", defs[i].ctypedef.name);
    }
    format(" \n");
  }	  
}	  

bool lisp_compiler_test(){

  compiler_state * c = compiler_make();
  compiler_set_state(c);
  load_defs();
  print_def(void_def,0,false);
	  
  decl dcl;
  dcl.name = "test";
  dcl.type = void_ptr_def;
  char * testd = calloc(1,10000);
  print_cdecl(dcl);
  load_cdecl(testd, 10000, dcl);
  type_def defs[1000];
  for(size_t i = 0; i < array_count(defs);i++)
    defs[i] = void_def;
  make_dependency_graph(defs,type_def_def);
  make_dependency_graph(defs,decl_def);

  { // print_string definition
    static type_def print_string_def;
    static decl args[1];
    args[0].name = "string";
    args[0].type = char_ptr_def;    
    print_string_def.kind = FUNCTION;
    print_string_def.fcn.ret = &void_def;
    print_string_def.fcn.cnt = array_count(args);
    print_string_def.fcn.args = args;
    fcn_def fdef = defext("print_string", print_string_def);
    fdef.is_extern = false;
    fdef.ptr = &print_string;
    fcn_def * var = (fcn_def *) compiler_define_variable(c, "print_string", fcn_def_def);
    format("print string: %i\n", var);
    TEST_ASSERT(var != NULL);
    *var = fdef;
  }

  { // define the function to define external functions.
    static type_def defext_def;
    static decl args[2];
    args[0].name = "name";
    args[0].type = char_ptr_def;
    args[1].name = "type";
    args[1].type = type_def_def;
    defext_def.kind = FUNCTION;
    defext_def.fcn.ret = &void_def;
    defext_def.fcn.cnt = array_count(args);
    defext_def.fcn.args = args;
    
    fcn_def fdef = defext("defext",defext_def);
    fcn_def * var = (fcn_def *) compiler_define_variable(c, "defext", fcn_def_def);
    format("defext: %i\n", var);
    *var = fdef;
  }

  {// the cast macro
    cmacro_def * var = (cmacro_def *) compiler_define_variable(c, "cast", cmacro_def_def);
    static cmacro_def cast_def;
    cast_def.arg_cnt = 2;
    cast_def.fcn = &cast_macro;
    cast_def.name = "cast";
    *var = cast_def;
  }

  {// the lol macro
    type_def lol(expr exp){
      printf("LOL: Compiling stuff..\n");
      return compile_iexpr(exp);
    }
    cmacro_def * var = (cmacro_def *) compiler_define_variable(c, "lol", cmacro_def_def);
    static cmacro_def cast_def;
    cast_def.arg_cnt = 1;
    cast_def.fcn = &lol;
    cast_def.name = "lol";
    *var = cast_def;
  }

  {//type_def type_macro(expr typexpr)
    cmacro_def * var = (cmacro_def *) compiler_define_variable(c, "type", cmacro_def_def);
    static cmacro_def cast_def;
    cast_def.arg_cnt = 1;
    cast_def.fcn = &type_macro;
    cast_def.name = "type";
    *var = cast_def;
  }
  {//type_def new_macro(expr typexpr)
    cmacro_def * var = (cmacro_def *) compiler_define_variable(c, "new", cmacro_def_def);
    static cmacro_def cast_def;
    cast_def.arg_cnt = 1;
    cast_def.fcn = &new_macro;
    cast_def.name = "new";
    *var = cast_def;
  }	  
  
  {
    static type_def voidstr_def;
    voidstr_def.kind = FUNCTION;
    voidstr_def.fcn.cnt = 0;
    voidstr_def.fcn.ret = &char_ptr_def;
    voidstr_def.fcn.args = NULL;
    fcn_def fdef = defext("glfwGetVersionString",voidstr_def);
    
    fcn_def * var = (fcn_def *) compiler_define_variable(c, "glfwGetVersionString", fcn_def_def);
    *var = fdef;
  }
  {
    static type_def voidstr_def;
    voidstr_def.kind = FUNCTION;
    voidstr_def.fcn.cnt = 0;
    voidstr_def.fcn.ret = &void_def;
    voidstr_def.fcn.args = NULL;
    fcn_def fdef = defext("glfwInit",voidstr_def);
    
    fcn_def * var = (fcn_def *) compiler_define_variable(c, "glfwInit", fcn_def_def);
    *var = fdef;
  }

  int * list = NULL;
  size_t list_cnt = 0;
  for(int i = 0; i < 5;i++)
    list_add((void **)&list,&list_cnt,&i, sizeof(int));
  TEST_ASSERT(list_cnt == 5);
  TEST_ASSERT(list[4] = 4);
  tccs_test2();
  type_def * var = (type_def *) compiler_define_variable(c, "type_def_def", type_def_def);
  type_def * var2 = (type_def *) compiler_define_variable(c, "decl_def", decl_def);
  *((type_def *) var) = type_def_def;
  *((type_def *) var2) = decl_def;
  type_def * var3 = (type_def *) compiler_define_variable(c, "char_ptr_def", type_def_def);
  *var3 = char_ptr_def;
  type_def * i64var = (type_def *) compiler_define_variable(c, "i64_def", type_def_def);
  *i64var = i64_def;
  char * test_code = "(print_string \"Hello World\\n\")(glfwInit)(print_string (glfwGetVersionString)) (print_string \"\\nhello sailor!\\n\") (lol (glfwGetVersionString)) (cast 100 i64_def)";
  test_code = "(cast 10 i64_def)";
  test_code = "(new i64)";//(fcn i64 (a i64)))";
  expr out_expr[2];
  char * next = test_code;
  while(next != NULL && *next != 0){
    int out = 2;
    char * prev = next;
    next = lisp_parse(next, out_expr, &out);
    TEST_ASSERT(prev != next);
    for(int i = 0; i < out; i++){
      compiled_expr expr = compile_expr(out_expr + i);
      void eval_print(compiled_expr expr);
      eval_print(expr);
      TEST_ASSERT(NULL != expr.fcn);
      delete_expr(out_expr + i);
    }
    if(out == 0)
      break;
  }
  bool start_read_eval_print_loop();
  //return start_read_eval_print_loop();
  return true;
}

int check_expression(char * buffer){
  int status = 0;
  while(*buffer){
    if(*buffer == '(')
      status += 1;
    if(*buffer == ')')
      status -= 1;
    if(*buffer == '\\')
      buffer++;
    buffer++;
  }
  return status;
}

void eval_print(compiled_expr cexpr){
  if(NULL == cexpr.fcn){
    ERROR("Unable to compile..\n");
  }else if(type_def_cmp(cexpr.result_type, char_ptr_def)){
    char * (*eval) () = cexpr.fcn;
    printf("'%s' : char*\n", eval());
  }
  else if(type_def_cmp(cexpr.result_type, void_def)){
    void (* __eval) () = cexpr.fcn;
    __eval();
    printf("() : unit\n");
  }
  else if(type_def_cmp(cexpr.result_type, i64_def)){
    i64 (*eval) () = cexpr.fcn;
    eval();
    printf("%i : i64\n",eval());
  }else if(type_def_cmp(cexpr.result_type, type_def_def)){

    type_def (*eval) () = cexpr.fcn;
    type_def d = eval();
    print_def(d,0,false);
    printf(" : type_def\n");
  }
}

bool start_read_eval_print_loop(){
  format("C-LISP REPL\n");

  char * expr_reader = NULL;
  size_t cnt;
  FILE * mem = NULL;
  while(true){
    if(expr_reader == NULL){
      cnt = 0;
      mem = open_memstream(&expr_reader,&cnt);
      format(">");
    }else{
      format(" ");
    }

    char * data = NULL;
    size_t cnt2;
    getline(&data,&cnt2,stdin);
    with_format_out(mem,lambda(void,(){format("%s",data);}));
    fflush(mem);
    int expr_state = check_expression(expr_reader);
    if(expr_state < 0){
      ERROR("Invalid paren matching");
      goto reset;
    }
    if(expr_state > 0)
      continue;
    printf("Expr state: %i %s\n", expr_state, expr_reader);
    char * next = expr_reader;
    while(next != NULL && *next != 0){
      expr out_expr[2];
      int out = array_count(out_expr);
      char * prev = next;
      next = lisp_parse(next, out_expr, &out);
      if(prev == next){
	ERROR("Unable to parse..\n");
	continue;
      }
      for(int i = 0; i < out; i++){
	compiled_expr cexpr = compile_expr(out_expr + i);
	eval_print(cexpr);
	delete_expr(out_expr + i);
      }
      if(out == 0)
	break;
    }
  reset:
    fclose(mem);
    free(expr_reader);
    expr_reader = NULL;
    cnt = 0;
  } 
  return true;
}

void * tccs_compile_and_get(TCCState * tccs, char * code, char * symbol){
  format("Compiling %s\n", code);
  int fail = tcc_compile_string(tccs,code);
  format("COMPILE: %i\n",!fail);
  int size = tcc_relocate(tccs, NULL);
  char * codebuf = malloc(size);
  fail = tcc_relocate(tccs, codebuf);
  format("RELOCATE: %i\n",!fail);
  return tcc_get_symbol(tccs, symbol);
}

void tccs_test2(){
  char * a = "float calc_x(){ return 5.0f;}";
  char * b = "float calc_x(); int calc_y(){ return calc_x() + 10;}";
  char * c = "float cval = 20.0;";
  TCCState * tccs = mktccs();

  float (* fcn)() =  tccs_compile_and_get(tccs, a, "calc_x");
  format("outp: %f\n", fcn());
 
  tcc_delete(tccs);
  tccs = mktccs();
  tcc_add_symbol(tccs, "calc_x", fcn);
  int (* fcn2)() = tccs_compile_and_get(tccs, b, "calc_y");
  format("outp: %i\n", fcn2());
  tcc_delete(tccs);

  tccs = mktccs();
  float * var = tccs_compile_and_get(tccs, c, "cval");
  format("outp: %i\n", var);
  tcc_delete(tccs);
}
