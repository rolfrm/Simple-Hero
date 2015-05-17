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
