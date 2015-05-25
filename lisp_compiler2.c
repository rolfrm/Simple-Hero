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
  UNUSED(c);
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
static type_def * _compile_expr(compiler_state * c, c_block * block, c_value * val,  expr e );

type_def * type_macro(compiler_state * c, c_block * block, c_value * value, expr e){
  static int typevarid = 0;
  UNUSED(c);
  UNUSED(block);
  //UNUSED(value);
  UNUSED(e);
  type_def * t =_type_macro(e);
  char * varname = fmtstr("type_%i", typevarid++);
  compiler_define_variable_ptr(c,varname,&type_def_ptr_def, t);
  value->type = C_INLINE_VALUE;
  value->raw.value = "NULL";
  value->raw.type = &type_def_ptr_def;
  return _compile_expr(c, block, value, symbol_expr(varname));
}


static type_def * __compile_expr(compiler_state * c, c_block * block, c_value * value, sub_expr * se){
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
    logd("C MACRO '%s'\n",macro->name);
    if(macro->arg_cnt != argcnt)
      ERROR("Unsupported number of arguments %i for %s",argcnt, macro->name);
    
    switch(macro->arg_cnt){
    case 0:
      return ((type_def *(*)(compiler_state * c, c_block * block, c_value * value)) macro->fcn)(c,block,value);
    case 1:
      return ((type_def *(*)(compiler_state * c, c_block * block, c_value * value,expr)) macro->fcn)(c,block,value,args[0]);
    case 2:
      return ((type_def *(*)(compiler_state * c, c_block * block, c_value * value,expr,expr)) macro->fcn)(c,block,value,args[0],args[1]);
    case 3:
      return ((type_def *(*)(compiler_state * c, c_block * block, c_value * value, expr,expr,expr)) macro->fcn)(c,block,value,args[0],args[1],args[2]);
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
      farg_types[i] = _compile_expr(c, block, fargs + i, args[i]);
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
    logd("*** function end ***\n");
    return td->fcn.ret;
  }else{
    logd("Not supported..\n");
  }
  logd("Found variable");
  type_def * sub_types[se->sub_expr_count];
  UNUSED(sub_types);
  c_value val[se->sub_expr_count];
  for(i64 i = 0; i < se->sub_expr_count; i++){
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
  c_block blk;
  blk.expr_cnt = 0;
  blk.exprs = NULL; 
  
  for(size_t i = 0; i < cnt; i++){
    c_value val;
    type_def * t = _compile_expr(c, &blk, &val, exp[i]);

    logd("gets here..\n");
    print_def(t, false);
    c_expr expr;
    expr.type = C_VALUE;
    expr.value = val;
    list_add((void **) &blk.exprs, &blk.expr_cnt, &expr, sizeof(c_expr));
  }

  c_fundef fundef;
  fundef.fdecl.name = "eval";
  fundef.block = blk;
  logd("BLOCK: %i exprs\n",blk.expr_cnt);
  type_def ftype;
  ftype.kind = FUNCTION;
  ftype.fcn.cnt = 0;
  ftype.fcn.ret = &void_def;
  fundef.fdecl.type = get_type_def(ftype);

  c_root_code root_code;
  root_code.type = C_FUNCTION_DEF;  
  root_code.fundef = fundef;
  print_c_code(root_code);
  return NULL;
}

void compiler_define_variable_ptr(compiler_state * c, char * name, type_def * t, void * ptr){
  var_def vdef;
  vdef.name = name;
  vdef.type = t;
  vdef.data = ptr;
  list_add((void **)&c->vars, &c->var_cnt, &vdef, sizeof(var_def));
}

type_def * str2type(char * str){
  return _type_macro(lisp_parse1(str));
}

void print_type(type_def * def){
  print_def(def,true);
}

type_def * _type_macro(expr typexpr);
bool test_lisp2c(){
  //type_def fcn_def =
  char * test_code = "(defun printhello ()(print_string \"hello\\n\"))";
  test_code = "(print_type (type i64))";
  size_t exprcnt;
  expr * exprs = lisp_parse_all(test_code, &exprcnt);
  load_defs();
  compiler_state * c = compiler_make();
  with_compiler(c,lambda(void, (){
	load_defs();

	{ //type_def type_macro(expr typexpr)
	  static cmacro_def cast_def;
	  cast_def.arg_cnt = 1;
	  cast_def.fcn = &type_macro;
	  cast_def.name = "type";
	  compiler_define_variable_ptr(c, "type", &cmacro_def_def, &cast_def);
	}
	{
	  //static fcn_def printtype_def;
	  type_def * type = str2type("(fcn void (a (ptr type_def)))");
	  type_def * type2 = str2type("(fcn void (a (ptr type_def)))");
	  type_def * type3 = str2type("(fcn void (a (ptr void)))");
	  compiler_define_variable_ptr(c, "print_type", type, print_type);
	  if(type != type2 && type != type3)
	    ERROR("types does not match");
	}

	logd("parsed %i expr(s).\n", exprcnt);
	compile_lisp_to_c(c, exprs, exprcnt);
	};));
  return TEST_FAIL;
}
