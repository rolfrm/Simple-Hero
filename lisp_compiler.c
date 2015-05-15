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



typedef enum {
  SIMPLE = 0,
  FUNCTION = 1,
  POINTER = 2,
  STRUCT = 3,
  UNION = 4,
  ENUM = 5,
  TYPEDEF = 6
} type_def_kind;

struct _type_def;
typedef struct _type_def type_def;
struct _decl;
typedef struct _decl decl;

struct _type_def{
  type_def_kind kind;
  union{
    struct{
      char ** names;
      i64 * values;
      i64 cnt;
      char * enum_name;
    }cenum;

    struct{
      char * name;
      char * cname;
    }simple;
    
    struct{
      type_def * ret;
      decl * args;
      i64 cnt;
    }fcn;

    struct{
      char * name; // NULL if anonymous
      decl * members;
      i64 cnt;
    }cstruct;

    struct{
      char * name; // NULL if anonymous
      decl * members;
      i64 cnt;
    }cunion;

    struct{
      type_def * inner;
    }ptr;

    struct{
      char * name;
      type_def * inner;
    }ctypedef;
  };
};

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

struct _decl{
  char * name;
  type_def type;
};

void print_cdecl(decl idecl);

struct _compiler_state;
typedef struct _compiler_state compiler_state;

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

typedef struct _fcn_def{
  char * name;
  type_def type;
  u8 is_extern;
  void * ptr;
}fcn_def;

bool fcn_def_cmp(fcn_def a, fcn_def b){
  return type_def_cmp(a.type,b.type)
    && strcmp(a.name,b.name) == 0;
}
	  
typedef struct{
  char * name;
  type_def type;
  void * data;
}var_def;

static type_def void_def;
static type_def void_ptr_def;
static type_def char_def;
static type_def i64_def;
static type_def u8_def;
static type_def char_ptr_def;
static type_def char_ptr_ptr_def;
static type_def i64_ptr_def;
static type_def type_def_def;
static type_def type_def_kind_def;
static type_def type_def_ptr_def;
static type_def decl_def;
static type_def decl_ptr_def;
static type_def error_def;
static type_def fcn_def_def;

// Loads a all the types we need.
// call before anything else.
void load_defs(){
  void_def = make_simple("void", "void");
  void_ptr_def = make_ptr(&void_def);
	  
  error_def = make_simple("error","error");
  char_def = make_simple("char", "char");
  i64_def = make_simple("i64", "i64");
  u8_def = make_simple("u8", "u8");
  char_ptr_def.kind = POINTER;
  char_ptr_def.ptr.inner = &char_def;
  char_ptr_ptr_def.kind = POINTER;
  char_ptr_ptr_def.ptr.inner = &char_ptr_def;
  i64_ptr_def.kind = POINTER;
  i64_ptr_def.ptr.inner = &i64_def;  
  type_def_def.kind = TYPEDEF;
  type_def_def.ctypedef.name = "type_def";
  type_def_ptr_def.kind = POINTER;
  type_def_ptr_def.ptr.inner = &type_def_def;  
  decl_ptr_def.kind = POINTER;
  decl_ptr_def.ptr.inner = &decl_def;
  
  { // kind enum
    static type_def type_def_kind_def_inner;
    type_def_kind_def.kind = TYPEDEF;
    type_def_kind_def.ctypedef.inner = &type_def_kind_def_inner;
    type_def_kind_def.ctypedef.name = "type_def_kind";
    static char * kindnames[] = {"SIMPLE", "FUNCTION", "POINTER", "STRUCT", "UNION", "ENUM"};
    static i64 kindvalues[] = {SIMPLE, FUNCTION, POINTER, STRUCT, UNION, ENUM};
    type_def_kind_def_inner.kind = ENUM;
    type_def_kind_def_inner.cenum.cnt = array_count(kindnames);
    type_def_kind_def_inner.cenum.names = kindnames;
    type_def_kind_def_inner.cenum.values = kindvalues;
  }
    
  { //type_def struct members:
    static type_def itype_def_def;
    type_def_def.ctypedef.inner = &itype_def_def;
    
    static decl members[2];
    itype_def_def.kind = STRUCT;
    itype_def_def.cstruct.members = members;
    itype_def_def.cstruct.cnt = array_count(members);
    itype_def_def.cstruct.name = "_type_def";

    members[0].type = type_def_kind_def;
    members[0].name = "kind";
    members[1].type.kind = UNION;
    members[1].name = NULL;
    members[1].type.cunion.name = NULL;
    {// anon union members

      static decl umembers[7];
      members[1].type.cunion.cnt = array_count(umembers);
      members[1].type.cunion.members = umembers;

      {//cenum
	static type_def cenum_def;
	static decl members[4];
	members[0].name = "names";
	members[0].type = char_ptr_ptr_def;
	members[1].name = "values";
	members[1].type = i64_ptr_def;
	members[2].name = "cnt";
	members[2].type = i64_def;
	members[3].name = "enum_name";
	members[3].type = char_ptr_def;

	cenum_def.kind = STRUCT;
	cenum_def.cstruct.members = members;
	cenum_def.cstruct.name = NULL;
	cenum_def.cstruct.cnt = array_count(members);
	
	umembers[0].type = cenum_def;
	umembers[0].name = "cenum";
      }

      {//simple
	static type_def simple_def;
	static decl members[2];
	simple_def.kind = STRUCT;
	simple_def.cstruct.name=NULL;
	simple_def.cstruct.members = members;
	simple_def.cstruct.cnt = array_count(members);

	members[0].name ="name";
	members[0].type = char_ptr_def;
	members[1].name = "cname";
	members[1].type = char_ptr_def;
	
	umembers[1].type = simple_def;
	umembers[1].name = "simple";
      }
      
      {//fcn
	static type_def fcn_def;
	static decl members[3];
	fcn_def.kind = STRUCT;
	fcn_def.cstruct.cnt = array_count(members);
	fcn_def.cstruct.members = members;
	fcn_def.cstruct.name = NULL;
	members[0].name= "ret";
	members[0].type = type_def_ptr_def;
	members[1].name = "args";
	members[1].type = type_def_ptr_def;
	members[2].name = "cnt";
	members[2].type = i64_def;
	
	umembers[2].type = fcn_def;
	umembers[2].name = "fcn";
      }

      {//cstruct/cunion
	static decl members[3];
	members[0].name = "name";
	members[0].type = char_ptr_def;
	members[1].name = "members";
	members[1].type = decl_ptr_def;
	members[2].name = "cnt";
	members[2].type = i64_def;
	static type_def cstruct_def;
	cstruct_def.kind = STRUCT;
	cstruct_def.cstruct.name = NULL;
	cstruct_def.cstruct.members= members;
	cstruct_def.cstruct.cnt = array_count(members);
	umembers[3].type = cstruct_def;
	umembers[3].name = "cstruct";
	umembers[4].type = cstruct_def;
	umembers[4].type.cstruct.name = NULL;
	umembers[4].name = "cunion";
      }

      {//ptr
	static decl members[1];
	members[0].name = "inner";
	members[0].type = type_def_ptr_def;
	umembers[5].type.kind = STRUCT;
	umembers[5].type.cstruct.members = members;
	umembers[5].type.cstruct.cnt = 1;
	umembers[5].type.cstruct.name = NULL;
	umembers[5].name = "ptr";
      }
      {// typedef
	static decl members[2];
	members[0].name = "name";
	members[0].type = char_ptr_def;
	members[1].name = "inner";
	members[1].type = type_def_ptr_def;
	static type_def ctypedef_def;
	ctypedef_def.kind = STRUCT;
	ctypedef_def.cstruct.name = NULL;
	ctypedef_def.cstruct.members = members;
	ctypedef_def.cstruct.cnt = array_count(members);
	umembers[6].type = ctypedef_def;
	umembers[6].name = "ctypedef";
      }
    }
  }
  {
    static decl members[2];
    static type_def dclinner;
    members[0].name = "name";
    members[0].type = char_ptr_def;
    members[1].name = "type";
    members[1].type = type_def_def;
     
    dclinner.kind = STRUCT;
    dclinner.cstruct.name = "_decl";
    dclinner.cstruct.members = members;
    dclinner.cstruct.cnt = array_count(members);
    
    decl_def.kind = TYPEDEF;
    decl_def.ctypedef.name = "decl";
    decl_def.ctypedef.inner = &dclinner;
  }
  { // fcn_def
    fcn_def_def.kind = TYPEDEF;
    static decl members[4];
    static type_def inner;
    fcn_def_def.ctypedef.name = "fcn_def";
    fcn_def_def.ctypedef.inner = &inner;
    inner.kind = STRUCT;
    inner.cstruct.members = members;
    inner.cstruct.cnt = array_count(members);
    inner.cstruct.name = "_fcn_def";
    members[0].name = "name";
    members[0].type = char_ptr_def;
    members[1].name = "type";
    members[1].type = type_def_def;
    members[2].name = "is_extern";
    members[2].type = u8_def;
    members[3].name = "ptr";
    members[3].type = void_ptr_def;
  }
}

// Currently the compiler just contains variables.
struct _compiler_state{
  var_def * vars;
  size_t var_cnt;
};

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
    //format("PTR  %i\n",type.ptr.inner->kind);
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
  default:
    ERROR("not implemented %i", type.kind);
  }
}

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

void list_add(void ** dst, size_t * cnt, void * src, size_t item_size){
  size_t next_size = ++(*cnt);
  void * ptr = *dst;
  ptr =  realloc(ptr, next_size * item_size);
  *dst = ptr;
  memcpy(ptr + (next_size - 1) * item_size, src, item_size);
}

void tccerror(void * opaque, const char * msg){
  UNUSED(opaque);
  format("%s\n",msg);
}

typedef struct{
  compiler_state * c;
  char * buffer;
  // required functions
  fcn_def * fcns;
  size_t fcn_cnt;
}comp_state;

comp_state comp_state_make(){
  comp_state out;
  out.buffer = NULL;
  out.fcns = NULL;
  out.fcn_cnt = 0;
  return out;
}

fcn_def * get_fcn_def(compiler_state * c, char * name, size_t name_len){
  for(size_t i = 0;i < c->var_cnt; i++){
    for(size_t j = 0; j < name_len; j++)
      if(name[j] != c->vars[i].name[j])
	goto next_item;
    return (fcn_def *) c->vars[i].data;
  next_item:
    continue;
  }
  return NULL;
}

void make_dependency_graph(type_def * defs, type_def def){
	  
  if(type_def_cmp(void_def,def))
    return;
  
  
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

void add_required_fcn(comp_state * s, fcn_def fdef){
  for(size_t i = 0; i < s->fcn_cnt; i++){
    if(fcn_def_cmp(fdef,s->fcns[i]) == true)
      return;
  }
  list_add((void **) &s->fcns, &s->fcn_cnt, &fdef, sizeof(fcn_def));
}
	  
type_def compile_cast(expr arg1, expr arg2){
  UNUSED(arg1);UNUSED(arg2);
  return error_def;
}

static type_def compile_iexpr(comp_state * s, expr expr1);

static type_def compile_sexpr(comp_state * s, sub_expr sexpr){

  fcn_def * fcn;
  for(int i = 0; i < sexpr.sub_expr_count; i++){

    type_def def = compile_iexpr(s, sexpr.sub_exprs[i]);
    if(i == 0){ 
      value_expr sexpr2 = sexpr.sub_exprs[i].value;
      fcn = get_fcn_def(s->c,sexpr2.value,sexpr2.strln);
      add_required_fcn(s, *fcn);
      if(fcn == NULL){
	ERROR("Unknown function '%.*s'",sexpr2.strln,sexpr2.value);
	return error_def;
      }
      
      format("(");
    }else{
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

static type_def compile_value(comp_state * state, value_expr vexpr){
  UNUSED(state);
  if(vexpr.type == STRING){
    format("\"%.*s\"",vexpr.strln, vexpr.value);
    return char_ptr_def;
  }else if(vexpr.type == KEYWORD || vexpr.type == SYMBOL){
    format("%.*s ",vexpr.strln,vexpr.value);
    return void_def;
  }else if(vexpr.type == NUMBER){
    format("%.*s ",vexpr.strln,vexpr.value);
    return i64_def;
  }
  ERROR("Unhandled type '%i'", vexpr.type);
  return error_def;
}

static type_def compile_iexpr(comp_state * s, expr expr1){
  if(expr1.type == VALUE){
    return compile_value(s, expr1.value);
  }else{
    return compile_sexpr(s, expr1.sub_expr);
  }
  return error_def;
}
  
typedef struct{
  type_def result_type;
  void * fcn;
}compiled_expr;

void print_string(char * buf);
compiled_expr compile_expr(expr * e, compiler_state * lisp_state){

  static TCCState * tccs;
  compiled_expr err;
  err.fcn = NULL;
  err.result_type = error_def;
  tccs = tcc_new();
  tcc_set_lib_path(tccs,".");
  tcc_add_library_path(tccs,"/usr/lib/x86_64-linux-gnu/");
  tcc_add_library(tccs, "glfw");
  tcc_set_error_func(tccs, NULL, tccerror);
  tcc_set_output_type(tccs, TCC_OUTPUT_MEMORY);

  size_t cdecl_size = 0;
  comp_state s = comp_state_make();
  s.c = lisp_state;
  FILE * stream = open_memstream(&s.buffer, &cdecl_size);
  type_def td;
  with_format_out(stream,lambda(void,(){
	td = compile_iexpr(&s, *e);
      }));
  fclose(stream);

  char * prebuffer = NULL;
  size_t pre_size = 0;
  stream = open_memstream(&prebuffer,&pre_size);
  with_format_out(stream,lambda(void,(){
	format("%s", "#include \"cstd_header.h\"\n");
	for(size_t i = 0; i < s.fcn_cnt; i++){
	  fcn_def fcn = s.fcns[i];
	  if(fcn.is_extern == false){
	    tcc_add_symbol(tccs, fcn.name, fcn.ptr);
	  }
	  decl dcl;
	  dcl.name = fcn.name;
	  dcl.type = fcn.type;
	  print_cdecl(dcl);
	}
	print_def(td,0,true);
	format(" __eval(){\n %s %s;\n}", type_def_cmp(void_def,td) ? "" : "return", s.buffer); 
      }));

  fclose(stream);
  printf("compiling |\n%s\n",prebuffer);
  int ok = tcc_compile_string(tccs,prebuffer);

  if(ok != 0){
    ERROR("Unable to compile %s\n error: %i",s.buffer, ok);
    return err;
  }
  int size = tcc_relocate(tccs, TCC_RELOCATE_AUTO);
  if(size == -1){
    ERROR("Unable to link %s\n",s.buffer);
    return err;
  }
  
  void * fcn = tcc_get_symbol(tccs, "__eval");
  
  //((void (*)())fcn)();
  //printf("DID IT! %i\n",fcn);

  //tcc_delete(tccs);
  free(prebuffer);
  free(s.buffer);
  
  if(fcn == NULL){
    ERROR("Unable to create function");
    return err;
  }
  compiled_expr fdef = {td, fcn};
  return fdef;
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

size_t get_required_types(compiler_state * c, type_def main_def, type_def * ts, size_t cnt){
  UNUSED(cnt);
  UNUSED(main_def);
  UNUSED(c);
  ts[0] = decl_def;
  ts[1] = type_def_def;
  ts[2] = type_def_kind_def;
  ts[3] = fcn_def_def;
  return 4;
}

void * compiler_define_variable(compiler_state *c, char * name, type_def t){
  // this is a bit complex. I need to run the code which defines and sets that variable
  //UNUSED(c);
  type_def required_types[100];
  for(size_t i = 0; i < array_count(required_types); i++)
    required_types[i] = void_def;
  make_dependency_graph(required_types,t);
  size_t required_types_cnt = 0;
  for(; type_def_cmp(required_types[required_types_cnt], void_def) == false; required_types_cnt++);
  
  size_t bufsize = 10000;
  char * buf = malloc(bufsize);
  
  char * locbuf = buf;
  size_t restsize = bufsize;


  { // insert header
    size_t cnt = sprintf(locbuf, "#include \"cstd_header.h\"\n");
    locbuf += cnt;
    restsize -= cnt;	
  }
  for(size_t i = 0; i < required_types_cnt; i++){
    type_def t = required_types[i];
    if(t.kind == STRUCT){
      char * name = t.cstruct.name;
      if(name != NULL){
	size_t cnt;
	cnt = sprintf(locbuf, "struct %s;\n", name);
	locbuf += cnt;
	restsize -= cnt;
      }
    }
    if(t.kind == TYPEDEF){
      type_def inner = *t.ctypedef.inner;
      if(inner.kind == STRUCT){
	char * name = inner.cstruct.name;
	size_t cnt;
	char _namebuf[100];
	if(name == NULL){
	  sprintf(_namebuf, "_%s_", t.ctypedef.name);
	  name = _namebuf;
	}	
	cnt = sprintf(locbuf, "typedef struct %s %s;\n", name, t.ctypedef.name);
	locbuf += cnt;
	restsize -= cnt;	
	
      }
      if(inner.kind == ENUM){
	size_t cnt = sprintf(locbuf, "typedef enum {\n");
	locbuf += cnt;
	restsize -= cnt;	
	
	for(int j = 0; j < inner.cenum.cnt; j++){
	  char * comma = (j !=(inner.cenum.cnt-1) ? "," : "");
	  size_t cnt = sprintf(locbuf, "   %s = %i%s\n", inner.cenum.names[j], inner.cenum.values[j], comma);
	  locbuf += cnt;
	  restsize -= cnt;	
	
	}
	cnt = sprintf(locbuf, "}%s;\n",t.ctypedef.name);
	locbuf += cnt;
	restsize -= cnt;	

      }
    }
  }

  for(size_t i = 0; i < required_types_cnt; i++){
    type_def t = required_types[i];
    if(t.kind != STRUCT) continue;
    size_t cnt = write_def_to_buffer(t,locbuf,restsize);
    locbuf += cnt;
    restsize -= cnt;
    cnt = sprintf(locbuf, ";\n");
    locbuf += cnt;
    restsize -= cnt;	
    
  }

  { // write code to buffer
    char * cdecl;
    size_t cdecl_size = 0;
    FILE * stream = open_memstream(&cdecl, &cdecl_size);
    decl dcl;
    dcl.name = name;
    dcl.type = t;
    with_format_out(stream, lambda(void,(){print_cdecl(dcl);}));
    fclose(stream);
    size_t cnt = sprintf(locbuf, "%s\n", cdecl);
    free(cdecl);
    locbuf += cnt;
    restsize -= cnt;	
  }

  TCCState * tccs = mktccs();

  tcc_compile_string(tccs, buf);
  
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

  free(buf);
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
  cs = c;
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
    format("VAR: %i\n", var);
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
    UNUSED(fdef);
    fcn_def * var = (fcn_def *) compiler_define_variable(c, "defext", fcn_def_def);
    *var = fdef;
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
  type_def def = type_def_def;
  char buf[2000];
  format("%s",buf);
 
  type_def * var = (type_def *) compiler_define_variable(c, "type_def_def", def);
  type_def * var2 = (type_def *) compiler_define_variable(c, "decl_def", decl_def);
  *((type_def *) var) = def;
  *((type_def *) var2) = decl_def;
  
  //type_def * extfcn = (type_def *) compiler_define_variable(c, "defext", def);

  char * base_code = "(defvar format (extfcn \"format\" :str :c-varadic))";
  UNUSED(base_code);
  char * test_code = "(print_string \"Hello World\\n\")(glfwInit)(print_string (glfwGetVersionString)) (print_string \"\\nhello sailor!\\n\")";
  
  expr out_expr[2];
  char * next = test_code;
  while(next != NULL && *next != 0){
    int out = 2;
    char * prev = next;
    next = lisp_parse(next, out_expr, &out);
    TEST_ASSERT(prev != next);
    for(int i = 0; i < out; i++){
      compiled_expr expr = compile_expr(out_expr + i, c);
      TEST_ASSERT(NULL != expr.fcn);
      delete_expr(out_expr + i);
    }
    if(out == 0)
      break;
  }
  bool start_read_eval_print_loop(compiler_state * c);
  return start_read_eval_print_loop(c);
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

bool start_read_eval_print_loop(compiler_state * c){
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
	compiled_expr cexpr = compile_expr(out_expr + i, c);
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
	}
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
