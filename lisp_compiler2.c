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

static type_def * compile_value(c_value * val, value_expr e){
  val->type = C_INLINE_VALUE;
  var_def * vdef;
  switch(e.type){
  case STRING:
    val->raw.value = fmtstr("\"%.*s\"",e.strln,e.value);
    val->raw.type = &char_ptr_def;
    return &char_ptr_def;
  case KEYWORD:
  case SYMBOL:
    val->type = C_SYMBOL;
    val->symbol = fmtstr("%.*s",e.strln, e.value);
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
  default:
    break;
  }
  return &error_def;
}

#define COMPILE_ASSERT(expr) if(!(expr)){ERROR("Compile error");return &error_def;}

type_def * _type_macro(expr typexpr);
bool read_decl(expr dclexpr, decl * out){
  if(dclexpr.type == EXPR){
    sub_expr sexpr = dclexpr.sub_expr;
    if(sexpr.sub_expr_count == 2){
      expr name = sexpr.sub_exprs[0];
      expr type = sexpr.sub_exprs[1];
      if(name.type == VALUE && name.value.type == SYMBOL){
	out->name = fmtstr("%.*s",name.value.strln, name.value.value);
	out->type = _type_macro(type);
	return &error_def != out->type;
      }
    }
  }
  return false;
}

type_def * _type_macro(expr typexpr){
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
      type_def * ret = _type_macro(sexp.sub_exprs[1]);
      COMPILE_ASSERT(&error_def != ret);
      decl args[sexp.sub_expr_count - 2];
      for(int i = 0; i < sexp.sub_expr_count - 2; i++){
	COMPILE_ASSERT(read_decl(sexp.sub_exprs[i + 2], args + i));
      } 
      out.fcn.ret = ret;
      out.fcn.args = clone(args, sizeof(args));
      out.fcn.cnt = array_count(args);
      return get_type_def(out);
    }else if (strncmp(vkind.value,"ptr",vkind.strln) == 0){
      COMPILE_ASSERT(sexp.sub_expr_count == 2);
      type_def out;
      out.kind = POINTER;
      out.ptr.inner = _type_macro(sexp.sub_exprs[1]);
      return get_type_def(out);
    }
  }else{
    value_expr vkind = typexpr.value;
    char tname[vkind.strln + 1];
    strncpy(tname,vkind.value,vkind.strln);
    tname[vkind.strln] = 0;
    type_def * td = get_type_from_string(tname);
    COMPILE_ASSERT(td != NULL);
    return td;
  }
  return &error_def;
}

expr symbol_expr(char * name){
  expr e;
  e.type = VALUE;
  e.value.type = SYMBOL;
  e.value.value = name;
  e.value.strln = strlen(name);
  return e;
}

expr string_expr(char * name){
  expr e = symbol_expr(name);
  e.value.type = STRING;
  return e;
}

static type_def * _compile_expr(c_block * block, c_value * val,  expr e );

static type_def * __compile_expr(c_block * block, c_value * value, sub_expr * se){
  UNUSED(value);
  if(se->sub_expr_count == 0)
    ERROR("sub expressio count 0");
  expr name_expr = se->sub_exprs[0];
  if(name_expr.type != VALUE && name_expr.value.type != SYMBOL) ERROR("need symbol for first car");
  
  expr * args = se->sub_exprs + 1;
  i64 argcnt = se->sub_expr_count - 1;

  char name[name_expr.value.strln + 1];
  sprintf(name, "%.*s", name_expr.value.strln, name_expr.value.value);
  var_def * fvar = get_variable(name_expr.value.value, name_expr.value.strln);
  if(fvar == NULL) ERROR("unknown symbol '%s'", name);
  if(fvar->type == &cmacro_def_def){
    cmacro_def * macro = fvar->data;

    if(macro->arg_cnt != argcnt)
      ERROR("Unsupported number of arguments %i for %s",argcnt, macro->name);

    switch(argcnt){
    case 0:
      return ((type_def *(*)(c_block * block, c_value * value)) macro->fcn)(block,value);
    case 1:
      return ((type_def *(*)(c_block * block, c_value * value,expr)) macro->fcn)(block,value,args[0]);
    case 2:
      return ((type_def *(*)(c_block * block, c_value * value,expr,expr)) 
	      macro->fcn)(block,value,args[0],args[1]);
    case 3:
      return ((type_def *(*)(c_block * block, c_value * value, expr,expr,expr)) 
	      macro->fcn)(block,value,args[0],args[1],args[2]);
    default:
      ERROR("Number of macro arguments not supported: %i", argcnt);
    }
  }else if(fvar->type->kind == FUNCTION){
    type_def * td = fvar->type;
    COMPILE_ASSERT(td->fcn.cnt == argcnt);

    c_function_call call;
    call.type = td;
    call.name = fvar->name;
    c_value fargs[argcnt];
    type_def * farg_types[argcnt];
    for(i64 i = 0; i < argcnt; i++){
      farg_types[i] = _compile_expr(block, fargs + i, args[i]);
      // check types works
      if(farg_types[i] != td->fcn.args[i].type){
	logd("Types not matching:\n 1: %i\n", farg_types[i]);
	print_def(farg_types[i],true);
	logd("\n2: %i\n",td->fcn.args[i].type);	
	print_def(td->fcn.args[i].type,true);
	logd("\n");	
	ERROR("Types not matching");
      }
    }
    call.args = clone(fargs,sizeof(fargs));
    value->type = C_FUNCTION_CALL;
    value->call = call;
    value->call.arg_cnt = argcnt;
    format("hello\n");
    return td->fcn.ret;
  }else{
    ERROR("Not supported..\n");
  }
  type_def * sub_types[se->sub_expr_count];
  UNUSED(sub_types);
  c_value val[se->sub_expr_count];
  for(i64 i = 0; i < se->sub_expr_count; i++){
    expr * e = se->sub_exprs + i;
    
    sub_types[i] = _compile_expr(block, val + i, *e);
  }
  return &error_def;
}
	  
static type_def * _compile_expr(c_block * block, c_value * val,  expr e ){
  type_def * td;
  switch(e.type){
  case EXPR:
    return __compile_expr(block, val, &e.sub_expr);
    break;
  case VALUE:
    td = compile_value(val,e.value);
    break;
  }	  
  return td;
}

c_root_code compile_lisp_to_eval(expr exp){
  c_root_code r;
  c_fcndef * f = &r.fcndef;
  r.type = C_FUNCTION_DEF;

  f->block.expr_cnt = 0;
  f->block.exprs = NULL; 
  c_value val;
  type_def * t = _compile_expr(&f->block, &val, exp);
  type_def td;
  td.kind = FUNCTION;
  td.fcn.ret = t;
  td.fcn.args = NULL;
  td.fcn.cnt = 0;

  f->fdecl.name = "eval";
  f->fdecl.type = get_type_def(td);

  c_expr expr;
  expr.type = C_VALUE;
  if(t != &void_def) expr.type = C_RETURN;
  expr.value = val;
  list_add((void **) &f->block.exprs, &f->block.expr_cnt, &expr, sizeof(c_expr));


  
  return r;
}

type_def * str2type(char * str){
  return _type_macro(lisp_parse1(str));
}

void print_type(type_def * def){
  logd("type: '")print_def(def,true); logd("'\n");
}
void write_line(char * str){
  logd("%s\n", str);
}

#include <libtcc.h>
#include <stdlib.h>
void tccerror(void * opaque, const char * msg){
  UNUSED(opaque);
  format("%s\n",msg);
}

TCCState * mktccs(){
  TCCState * tccs = tcc_new();
  tcc_set_lib_path(tccs,".");
  tcc_set_error_func(tccs, NULL, tccerror);
  tcc_set_output_type(tccs, TCC_OUTPUT_MEMORY);
  return tccs;
}


void compile_as_c(c_root_code * codes, size_t code_cnt){
  type_def * deps[100];
  char * vdeps[100];
  memset(deps, 0, sizeof(deps));
  memset(vdeps, 0, sizeof(vdeps));
  for(size_t i = 0; i < code_cnt; i++){
    c_root_code_dep(deps, vdeps, codes[i]);
  }
  
  void go_write(){

    write_dependencies(deps);
    for(size_t i = 0; i < array_count(deps) && deps[i] != NULL; i++){
      if(deps[i]->kind == TYPEDEF){
	continue;
	print_def(deps[i]->ctypedef.inner,false);
      }else{
	print_def(deps[i],false);
      }
      format(";\n");
    }
    
    for(size_t i = 0; i < array_count(vdeps) && vdeps[i] != NULL; i++){
      var_def * var = get_variable2(vdeps[i]);
      ASSERT(var != NULL);
      decl dcl;
      dcl.name = var->name;
      dcl.type = var->type;
      format("extern ");
      print_cdecl(dcl);format(";\n");
    }
  
    for(size_t i = 0; i < code_cnt; i++){
      c_root_code_dep(deps, vdeps, codes[i]);
      print_c_code(codes[i]);
    }
  }
  char * data;
  size_t cnt;
  FILE * f = open_memstream(&data, &cnt);
  with_format_out(f, lambda( void, (){go_write();}));
  fclose(f);
  dump_buffer_to_file(data,cnt,"compile_out.c");
  TCCState * tccs = mktccs();
  for(size_t i = 0; i < array_count(vdeps) && vdeps[i] != NULL; i++){
    var_def * var = get_variable2(vdeps[i]);
    if(var->type->kind == FUNCTION){
      int fail = tcc_add_symbol(tccs,var->name,var->data);
      ASSERT(!fail);
    }else{
      int fail = tcc_add_symbol(tccs,var->name,&var->data);
      ASSERT(!fail);
    }
  }
  int fail = tcc_compile_string(tccs, data);
  free(data);
  ASSERT(!fail);
  data = NULL;
  int size = tcc_relocate(tccs, NULL);
  fail = tcc_relocate(tccs, alloc(size));
  ASSERT(!fail);
  
  for(size_t i = 0; i < code_cnt; i++){
    c_root_code r = codes[i];
    if(r.type == C_FUNCTION_DEF){
      decl fdecl = r.fcndef.fdecl;
      void * ptr = tcc_get_symbol(tccs, fdecl.name);
      ASSERT(ptr != NULL);
      compiler_define_variable_ptr(fdecl.name, fdecl.type, ptr);
    }
  }

  tcc_delete(tccs);
}

type_def * type_macro(c_block * block, c_value * value, expr e){
  UNUSED(block);
  static int typevarid = 0;
  type_def * t =_type_macro(e);
  char * varname = fmtstr("type_%i", typevarid++);
  compiler_define_variable_ptr(varname,&type_def_ptr_def, t);
  value->type = C_INLINE_VALUE;
  value->raw.value = "NULL";
  value->raw.type = &type_def_ptr_def;
  type_def * rt = _compile_expr(block, value, symbol_expr(varname));
  //dealloc(varname);
  return rt;
}

expr mk_sub_expr(expr * exprs, size_t cnt){
  expr e;
  e.type = EXPR;
  e.sub_expr.sub_exprs = exprs;
  e.sub_expr.sub_expr_count = cnt;
  return e;
}

type_def * defun_macro(c_block * block, c_value * value, expr name, expr args, expr body){

  // This function is rather complicated.
  // it handles turning something this: (defun funname (void (a i64) (b i64)) (+ a b)) 
  // into a function that can be called from througout the system.

  // there is really no simple way of doing this. <100 lines of code is ok for this task.
  // it generates a new c AST for defining the function and compiles it runtime.
  // it then registers the new function as a variable and returns the name of it.

  UNUSED(block);
  COMPILE_ASSERT(name.type == VALUE && name.value.type == SYMBOL);
  COMPILE_ASSERT(args.type == EXPR && args.sub_expr.sub_expr_count > 0);

  char * fcnname = fmtstr("%.*s",name.value.strln,name.value.value);
  logd("defining function: '%s'\n", fcnname);
  c_root_code newfcn_root;
  newfcn_root.type = C_FUNCTION_DEF;
  c_fcndef * f = &newfcn_root.fcndef;
  c_block * blk = &f->block;
  
  // ** get function decleration **
  decl *fdecl = &f->fdecl;
  fdecl->name = fcnname;
  
  expr subexpr[args.sub_expr.sub_expr_count + 1];
  subexpr[0] = symbol_expr("fcn");
  for(int i = 1; i < args.sub_expr.sub_expr_count + 1; i++){
    subexpr[i] = args.sub_expr.sub_exprs[i-1];
  }
  
  expr typexpr = mk_sub_expr(subexpr, array_count(subexpr));

  type_def * typeid = _type_macro(typexpr);
  fdecl->type = typeid;
  // ** register arguments as symbols ** //
  size_t varcnt = typeid->fcn.cnt;
  var_def _vars[typeid->fcn.cnt];
  var_def * vars = _vars;
  for(size_t i = 0; i < varcnt; i++){
    vars[i].data = NULL;
    vars[i].name = typeid->fcn.args[i].name;
    vars[i].type = typeid->fcn.args[i].type;
  }
  
  // ** Compile body with symbols registered ** //
  c_value val;
  with_symbols(&vars, &varcnt, lambda(void, (){
	type_def * td = _compile_expr(blk,&val, body);
	ASSERT(td == typeid->fcn.ret);
      }));
  c_expr expr;
  if(typeid->fcn.ret == &void_def){
    expr.type = C_VALUE;
  }else{
    expr.type = C_RETURN;
  }
  expr.value = val;

  blk->exprs = NULL;
  blk->expr_cnt = 0;
  list_add((void **) &blk->exprs, &blk->expr_cnt, &expr, sizeof(c_expr));
  compile_as_c(&newfcn_root,1);
  // ** Just return the function name ** //
  return compile_value(value, string_expr(fcnname).value);
}
	  
bool test_lisp2c(){
  char * test_code = "(defun printhello ()(print_string \"hello\\n\"))";
  test_code = "(type (ptr (ptr (ptr (ptr (ptr (ptr (ptr char))))))))";
  test_code = "\"hello sailor!\"";
  test_code = "(defun fst (i64 (a i64) (b i64)) a)";
  size_t exprcnt;
  expr * exprs = lisp_parse_all(test_code, &exprcnt);
  load_defs();

  compiler_state * c = compiler_make();
  with_compiler(c,lambda(void, (){
	load_defs();
	define_macro("type",1,&type_macro);
	define_macro("defun",3,&defun_macro);
	{
	  type_def * type = str2type("(fcn void (a (ptr type_def)))");
	  type_def * type2 = str2type("(fcn void (a (ptr type_def)))");
	  type_def * type3 = str2type("(fcn void (a (ptr void)))");
	  compiler_define_variable_ptr("print_type", type, print_type);
	  ASSERT(type == type2 && type != type3);
	  compiler_define_variable_ptr("write_line", str2type("(fcn void (a (ptr char)))"), &write_line);
	}

	logd("parsed %i expr(s).\n", exprcnt);
	for(size_t i = 0; i < exprcnt; i++){
	  c_root_code cl = compile_lisp_to_eval(exprs[i]);
	  compile_as_c(&cl,1);
	  var_def * evaldef = get_variable2("eval");
	  print_def(evaldef->type,false); logd(" :: ");
	  if(evaldef->type->fcn.ret == &void_def){
	    void (* fcn)() = evaldef->data;
	    fcn();
	  }else if(evaldef->type->fcn.ret == str2type("(ptr type_def)")){
	    type_def * (* fcn)() = evaldef->data;
	    fcn();
	    logd("type\n");
	  }else if(evaldef->type->fcn.ret == &char_ptr_def){
	    char * (* fcn)() = evaldef->data;
	    char * str = fcn();
	    logd("\"%s\"\n",str);
	      
	  }else if(evaldef->type->fcn.ret->kind == POINTER){
	    void * (* fcn)() = evaldef->data;
	    void * ptr = fcn();
	    logd("ptr: %x\n", ptr);
	  }else{
	    logd("\n");
	    loge("Unable to eval function of this type\n");
	  }
	}

      };));
  return TEST_FAIL;
}
