#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"

#include "lisp_types.h"
#include "lisp_std_types.h"

bool type_def_cmp(type_def a, type_def b){
  if(a.kind != b.kind)
    return false;
  switch(a.kind){
  case ENUM:
    return a.cenum.enum_name == b.cenum.enum_name;
  case SIMPLE:
    return a.simple.name == b.simple.name;
  case FUNCTION:
    return 
      a.fcn.args == b.fcn.args
      && a.fcn.ret == b.fcn.ret;
  case STRUCT:
    return 
      a.cstruct.name == b.cstruct.name &&
      a.cstruct.members == b.cstruct.members;
  case UNION:
    return 
      a.cunion.name == b.cunion.name &&
      a.cunion.members == b.cunion.members;
  case POINTER:
    return a.ptr.inner == b.ptr.inner;
  case TYPEDEF:
    return a.ctypedef.inner == b.ctypedef.inner;
  }
  ERROR("Should not happen!\n");
  return false;
}

type_def make_simple(char * name, char * cname){
  static type_def def;
  def.kind = SIMPLE;
  def.simple.name = name;
  def.simple.cname = cname;
  return def;
}

type_def make_ptr(type_def * def){
  type_def out;
  out.kind = POINTER;
  out.ptr.inner = def;
  return out;
}

void print_def(type_def type, int ind, bool is_decl){
  type_def inner;
  switch(type.kind){
  case SIMPLE:
    format("%*s%s ",ind, "",type.simple.name);
    break;
  case STRUCT:
    if(is_decl){
      format("%*s %s ",ind, "  ", type.cstruct.name);
    }else{
      format("%*s struct %s{\n",ind, "  ", type.cstruct.name == NULL ? "" : type.cstruct.name);
      
      for(i64 i = 0; i < type.cstruct.cnt; i++){
	print_def(type.cstruct.members[i].type, ind + 1, true);
	if(type.cstruct.members[i].name != NULL)
	  format("%s;\n",type.cstruct.members[i].name);
      }
      format("%*s }",ind, "  "); 
    }
    break;
  case POINTER:
    print_def(*(type.ptr.inner), ind, true);
    format("* ");
    break;
  case ENUM:
    if(is_decl){
      format("%s ", type.cenum.enum_name);
    }else{
      format("%*s %s ",ind, "  ",type.cenum.enum_name);
    }
    break;
  case UNION:
    format("%*s union {\n",ind, "  ");

    for(i64 i = 0; i < type.cunion.cnt; i++){
      print_def(type.cunion.members[i].type, ind + 1, false);
      format(" %s;\n", type.cunion.members[i].name);
    }
    format("%*s };",ind, "  ");
    break;
 
 case TYPEDEF:
    inner = *type.ctypedef.inner;
    char struct_name[20];

    if(inner.kind == STRUCT && inner.cstruct.name == NULL){
      sprintf(struct_name, "_%s_",type.ctypedef.name);
      inner.cstruct.name = struct_name;
    }
    if(is_decl){
      format("%s ", type.ctypedef.name);
      //print_def(inner,ind,true);
    }else{
      format("%*s typedef ", ind, "  ");
      print_def(inner,ind,false);
      format("%s;\n",type.ctypedef.name);
    }
    
    break;
  case FUNCTION:
    // this is an error.
    print_cdecl((decl){"anon", type});
    break;
  default:
    ERROR("not implemented %i", type.kind);
  }
}

void make_dependency_graph(type_def * defs, type_def def){
	  
  if(type_def_cmp(void_def,def)) return;
   
  switch(def.kind){
  case UNION:
    for(int i = 0; i < def.cunion.cnt; i++){
      type_def sdef = def.cunion.members[i].type;
      make_dependency_graph(defs,sdef);
    }	  
    if(def.cunion.name == NULL) return;
    break;
  case STRUCT:
    for(int i = 0; i < def.cstruct.cnt; i++){
      type_def sdef = def.cstruct.members[i].type;
      make_dependency_graph(defs,sdef);
    }

    if(def.cstruct.name == NULL) return;
    break;
  case POINTER:
    def = *def.ptr.inner;
    break;
  case TYPEDEF:
    make_dependency_graph(defs,*def.ctypedef.inner);
    break;
  case ENUM:
    return;
  case FUNCTION:
    make_dependency_graph(defs, *def.fcn.ret);
    for(int i = 0; i < def.fcn.cnt; i++)
      make_dependency_graph(defs, def.fcn.args[i].type);
    break;
  case SIMPLE:
  default:
    break;
  }
	  
  while(type_def_cmp(void_def,*defs) == false){
    if(type_def_cmp(def,*defs)) return;
    defs++;
  }
  *defs = def;	  
}

void print_value(c_value val){
  switch(val.type){
  case C_DEREF:
    format("*");
  case C_SUB_EXPR:
    format("( ");
    print_value(*val.value);
    format(")");
    break;
  case C_INLINE_VALUE:
    format("%s ", val.raw.value);
    break;
  case C_FUNCTION_CALL:
    format("%s(", val.call.name);
    for(size_t i = 0; i < val.call.arg_cnt; i++){
      print_value(val.call.args[i]);
      if(i != val.call.arg_cnt -1)
	format(", ");
    }
    format(") ");
    break;
  case C_OPERATOR:
    print_value(*val.operator.left);
    format("%c ",val.operator);
    print_value(*val.operator.right);
    break;
  case C_SYMBOL:
    format("%s ", val.symbol);
    break;
  }
}

void print_c_var(c_var var){
    print_cdecl(var.var);
    if(var.value != NULL){
      format(" = ");
      print_value(*var.value);
    }
    format(";\n");
}

static void print_expr(c_expr expr){
  switch(expr.type){
  case C_VAR:
    print_c_var(expr.var);
    break;
  case C_RETURN:
    format("return ");
  case C_VALUE:
    print_value(expr.value);
    format(";\n");
    break;
  case C_BLOCK:
    format("{\n");
    for(size_t i = 0; i < expr.block.expr_cnt; i++){
      print_expr(expr.block.exprs[i]);
    }
    format("}\n");
  }
}

void print_block(c_block blk){
  format("{\n");
  for(size_t i = 0; i < blk.expr_cnt; i++){
    print_expr(blk.exprs[i]);
  }
  format("}\n");
}

void print_fcn_code(c_fundef fundef){
  print_cdecl(fundef.fdecl);
  print_block(fundef.block);
}

void print_c_code(c_root_code code){
  switch(code.type){
  case C_INCLUDE:
    format("#include \"%s\"\n",code.include);
    break;
  case C_INCLUDE_LIB:
    format("#include <%s>\n",code.include);
    break;
  case C_FUNCTION_DEF:
    print_fcn_code(code.fundef);
    break;
  case C_VAR_DEF:
    print_c_var(code.var);
    break;
  case C_TYPE_DEF:
    print_def(*code.type_def,0,false);
    break;
  case C_DECL:
    print_cdecl(code.decl);
    format(";\n");
    break;
  }
}


bool test_print_c_code(){
  { // Simple include
    c_root_code c1;
    c1.type = C_INCLUDE_LIB;
    c1.include = "stdio.h";
    print_c_code(c1);
  }
  
  { // Complex, function definition
    c_value cv1a1;
    cv1a1.type = C_INLINE_VALUE;
    cv1a1.raw.value = "\"hello world!\"";
    cv1a1.raw.type = &char_ptr_def;

    c_value a_sym;
    a_sym.type = C_SYMBOL;
    a_sym.symbol = "a";

    c_value cv1;
    cv1.type = C_FUNCTION_CALL;
    cv1.call.name = "printf";

    cv1.call.arg_cnt = 1;
    cv1.call.args = &a_sym;

    c_expr expr;
    expr.type = C_VALUE;
    expr.value = cv1;
    
    c_fundef fundef;
    type_def ftype;
    ftype.kind = FUNCTION;
    ftype.fcn.cnt = 0;
    ftype.fcn.ret = &char_ptr_def;
    
    c_expr var;
    decl v;
    v.name = "a";
    v.type = char_ptr_def;
    var.type = C_VAR;
    var.var.var = v;
    var.var.value = &cv1a1;
    
    c_expr ret;
    ret.type = C_RETURN;
    ret.value = a_sym;

    c_expr exprs2[] = {var, expr, ret};
    
    c_block block;
    block.exprs = &exprs2;
    block.expr_cnt = array_count(exprs2);
    
    c_expr expr3;
    expr3.type = C_BLOCK;
    expr3.block = block;
      
    fundef.block.exprs = &expr3;
    fundef.block.expr_cnt = 1;

    decl fdecl;
    fdecl.type = ftype;
    fdecl.name = "print_test";
    fundef.fdecl = fdecl;
    
    c_root_code c2;
    c2.type = C_FUNCTION_DEF;
    c2.fundef = fundef;
    print_c_code(c2);
  }

  { // Complex type expansion
    c_root_code c3;
    c3.type = C_TYPE_DEF;
    c3.type_def = &type_def_def;
    print_c_code(c3);
  }

  return TEST_SUCCESS;
}
