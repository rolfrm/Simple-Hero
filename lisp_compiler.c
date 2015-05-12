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
}  type_def_kind;

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
      //first one is return
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

struct _decl{
  char * name;
  type_def type;
};

struct _compiler_state;
typedef struct _compiler_state compiler_state;

type_def make_simple(char * name, char * cname){
  static type_def def;
  def.kind = SIMPLE;
  def.simple.name = name;
  def.simple.cname = cname;
  return def;
}

typedef struct _basic_defs{
  type_def type_def_def;
  type_def decl_def;
  type_def type_def_kind_def;
}basic_defs;

static type_def void_def;
static type_def char_def;
static type_def i64_def;
static type_def char_ptr_def;
static type_def char_ptr_ptr_def;
static type_def i64_ptr_def;
static type_def type_def_def;
static type_def type_def_kind_def;
static type_def type_def_ptr_def;
static type_def decl_def;
static type_def decl_ptr_def;
  
void load_defs(){
  void_def = make_simple("void", "void");
  char_def = make_simple("char", "char");
  i64_def = make_simple("i64", "i64");
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

  {
    static decl members[2];
    static type_def dclinner;
    members[0].name = "name";
    members[0].type = char_ptr_def;
    members[1].name = "type";
    members[1].type = type_def_ptr_def;
     
    dclinner.kind = STRUCT;
    dclinner.cstruct.name = "_decl";
    dclinner.cstruct.members = members;
    dclinner.cstruct.cnt = array_count(members);
    
    decl_def.kind = TYPEDEF;
    decl_def.ctypedef.name = "decl";
    decl_def.ctypedef.inner = &dclinner;
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
	//printf("SIMPLE: %i\n", type_def_def.cstruct.members[1].type.cstruct.members[1].type.cstruct.cnt);

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
	umembers[4].type.cstruct.name = "cunion";
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
}

typedef struct{
  char * name;
  type_def type;
}fcn_def;

typedef struct{
  char * name;
  type_def type;
}var_def;

struct _compiler_state{
  type_def * type;
  size_t type_cnt;
  
  var_def * vars;
  size_t var_cnt;
};

void print_def(type_def type, int ind, bool is_decl){
  type_def inner;
  switch(type.kind){
  case SIMPLE:
    printf("%*s %s ",ind, " ",type.simple.name);
    break;
  case STRUCT:
    if(is_decl){
      printf("%*s %s ",ind, "  ", type.cstruct.name);
    }else{
      printf("%*s struct %s{\n",ind, "  ", type.cstruct.name == NULL ? "" : type.cstruct.name);
      
      for(i64 i = 0; i < type.cstruct.cnt; i++){
	//printf("STRUCT MEMBER %i\n",type.cstruct.members[i].type.kind);
	print_def(type.cstruct.members[i].type, ind + 1, true);
	if(type.cstruct.members[i].name != NULL)
	  printf("%s;\n",type.cstruct.members[i].name);
      }
      printf("%*s }",ind, "  "); 
    }
    break;
  case POINTER:
    //printf("PTR  %i\n",type.ptr.inner->kind);
    print_def(*(type.ptr.inner), ind, true);
    printf("* ");
    break;
  case ENUM:
    if(is_decl){
      printf("%s ", type.cenum.enum_name);
    }else{
      printf("%*s %s ",ind, "  ",type.cenum.enum_name);
    }
    break;
  case UNION:
    printf("%*s union {\n",ind, "  ");

    for(i64 i = 0; i < type.cunion.cnt; i++){
      print_def(type.cunion.members[i].type, ind + 1, false);
      printf(" %s;\n", type.cunion.members[i].name);
    }
    printf("%*s };",ind, "  ");
    break;
  case TYPEDEF:
    inner = *type.ctypedef.inner;
    //printf("TYPEDEF: %s\n", type.ctypedef.name);
    if(is_decl){
      printf("%s ", type.ctypedef.name);
      //print_def(inner,ind,true);
    }else{
      printf("%*s typedef ", ind, "  ");
      print_def(inner,ind,false);
      printf("%s;\n",type.ctypedef.name);
    }
    break;
  default:
    printf("not implemented %i", type.kind);
  }
}

size_t write_def_to_buffer(type_def def, char * buffer, size_t buffer_len){
  FILE * f = fmemopen(buffer,buffer_len, "w");
  FILE * oldstdout = stdout;
  stdout = f;
  print_def(def,0,false);
  stdout = oldstdout;
  size_t pos = ftell(f);
  fclose(f);
  return pos;
}

void print_compiler_state(compiler_state * c){
  printf("Compiler state: \n printing types:\n");
  for(size_t i = 0; i < c->type_cnt; i++){
    print_def(c->type[i], 0 , false);
    printf("\n");
  }
  printf("printing variables\n");
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

type_def compiler_define_simple_type(compiler_state * c, char * name, char * cname){
  type_def stype;
  stype.kind = SIMPLE;
  stype.simple.name = name;
  stype.simple.cname = cname;
  list_add((void **)&(c->type),&(c->type_cnt), &stype, sizeof(type_def));
  return stype;
}

void tccerror(void * opaque, const char * msg){
  UNUSED(opaque);
  printf("%s\n",msg);
}

typedef struct{
  
}scope_vars;

typedef struct{
  compiler_state * c;
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
  
void compile_expr(expr * e, compiler_state * lisp_state){

  static TCCState * tccs;
  static int exprcnt = 0;
  tccs = tcc_new();
  tcc_set_lib_path(tccs,".");
  tcc_set_error_func(tccs, NULL, tccerror);
  tcc_set_output_type(tccs, TCC_OUTPUT_MEMORY);
  
  exprcnt++;
  char * buf = malloc(1000);
  
  comp_state s = comp_state_make(buf);
  s.c = lisp_state;
  char header[100];
  sprintf(header,"void test%i() {",exprcnt);
  inscribe(&s, header);
  compile_iexpr(&s, *e);
  inscribe(&s, ";");
  inscribe(&s, "}");
  printf("c code: %s\n",s.buffer);

  int ok = tcc_compile_string(tccs,s.buffer);
  if(!ok)
    ERROR("Unable to compile %s\n",s.buffer);
  int size = tcc_relocate(tccs, TCC_RELOCATE_AUTO);
  printf("size: %i\n", size);
  sprintf(header,"test%i",exprcnt);
  void (* fcn) () = tcc_get_symbol(tccs, header);

  if(fcn != NULL)
    fcn();
  tcc_delete(tccs);
  free(buf);
}

void load_cdecl(char * buffer, char * name, type_def decl){
  
  void inner_print(type_def idecl){
    switch(idecl.kind){
    case SIMPLE:
      sprintf(buffer,"%s %s;",idecl.simple.cname, name);
      break;
    case STRUCT:
      sprintf(buffer,"%s %s;",idecl.cstruct.name, name);
      break;
    case TYPEDEF:
      sprintf(buffer,"%s %s;",idecl.ctypedef.name, name);
      break;
    default:
      printf("Not supported\n");
    }
  }
  inner_print(decl);
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
  return 3;
}

void * compiler_define_variable(compiler_state *c, char * name, type_def t){
  // this is a bit complex. I need to run the code which defines and sets that variable
  UNUSED(c);
  type_def required_types[100];
  size_t required_types_cnt = get_required_types(c, t, required_types,array_count(required_types));
  
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
    if(t.kind == TYPEDEF){

      type_def inner = *t.ctypedef.inner;
      if(inner.kind == STRUCT && inner.cstruct.name != NULL){
	
	size_t cnt = sprintf(locbuf, "struct %s;\n", inner.cstruct.name);
	locbuf += cnt;
	restsize -= cnt;	
	cnt = sprintf(locbuf, "typedef struct %s %s;\n", inner.cstruct.name, t.ctypedef.name);
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
    if(required_types[i].kind != TYPEDEF){
      ERROR("ERROR!\n");
      continue;
    }
    if(required_types[i].ctypedef.inner->kind == ENUM) continue;
    size_t cnt = write_def_to_buffer(*required_types[i].ctypedef.inner,locbuf,restsize);
    locbuf += cnt;
    restsize -= cnt;
    cnt = sprintf(locbuf, ";\n");
    locbuf += cnt;
    restsize -= cnt;	

  }

  { // write code to buffer
    char cdecl[100];
    load_cdecl(cdecl, name, t);

    size_t cnt = sprintf(locbuf, "%s\n",cdecl);
    locbuf += cnt;
    restsize -= cnt;	

  }

  printf("buffer: \n******************\n%s\n******************", buf);
  TCCState * tccs = mktccs();

  tcc_compile_string(tccs, buf);
  
  int size = tcc_relocate(tccs, NULL);
  char * codebuf = malloc(size);
  int fail = tcc_relocate(tccs, codebuf);
  printf("RELOCATE: %i\n",!fail);
  void * var = tcc_get_symbol(tccs, name);

  tcc_delete(tccs);

  free(buf);
  return var;
}
void tccs_test2();
compiler_state * cs = NULL;

void defext(char * name, type_def type){
  // defining an external function is close to defining an internal one
  // except you dont have the code for it. I can do this earlier.
  var_def fdef;
  fdef.name = name;
  fdef.type = type;
  list_add((void **)&cs->vars, &cs->var_cnt,&fdef,sizeof(var_def));
}

bool lisp_compiler_test(){
  compiler_state * c = compiler_make();
  cs = c;
  load_defs();
	  
  static type_def defext_def;
  {
    static decl args[2];
    args[0].name = "name";
    args[0].type = char_ptr_def;
    args[1].name = "type";
    args[1].type = type_def_def;
    defext_def.kind = FUNCTION;
    defext_def.fcn.ret = &void_def;
    defext_def.fcn.cnt = array_count(args);
    defext_def.fcn.args = args;
    defext("defext",defext_def);
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
  printf("%s",buf);
 
  type_def * var = (type_def *) compiler_define_variable(c, "type_def_def", def);
  type_def * var2 = (type_def *) compiler_define_variable(c, "decl_def", decl_def);
  *((type_def *) var) = def;
  *((type_def *) var2) = decl_def;
  
  //type_def * extfcn = (type_def *) compiler_define_variable(c, "defext", def);

  printf("variable var: %i %i\n", var, var2);
  return true;
  char * base_code = "(defvar printf (extfcn \"printf\" :str :c-varadic))";
  UNUSED(base_code);
  char * test_code = "(printf \"Hello World\\n\")(printf \"Hello Sailor!\\n\")";

  //type_def def_void = compiler_define_simple_type(c, ":void", "void");
  //type_def def_i32 = compiler_define_simple_type(c,":i32", "int");

  print_compiler_state(c);
  //compiler_define_variable(c, "oscar", def_i32);
  //compiler_define_variable(c, "oscar2", def_void);

  return true;
  expr out_expr[2];
  char * next = test_code;
  while(next != NULL && *next != 0){
    printf("next: %s\n", next);
    int out = 2;
    next = lisp_parse(test_code,out_expr,&out);
    for(int i = 0; i < out; i++){
      compile_expr(out_expr + i, c);
      delete_expr(out_expr + i);
    }
    if(out == 0)
      break;
    //
  }
  return true;
}

void * tccs_compile_and_get(TCCState * tccs, char * code, char * symbol){
  printf("Compiling %s\n", code);
  int fail = tcc_compile_string(tccs,code);
  printf("COMPILE: %i\n",!fail);
  int size = tcc_relocate(tccs, NULL);
  char * codebuf = malloc(size);
  fail = tcc_relocate(tccs, codebuf);
  printf("RELOCATE: %i\n",!fail);
  return tcc_get_symbol(tccs, symbol);
}


void tccs_test2(){
  char * a = "float calc_x(){ return 5.0f;}";
  char * b = "float calc_x(); int calc_y(){ return calc_x() + 10;}";
  char * c = "float cval = 20.0;";
  TCCState * tccs = mktccs();

  float (* fcn)() =  tccs_compile_and_get(tccs, a, "calc_x");
  printf("outp: %f\n", fcn());
 
  tcc_delete(tccs);
  tccs = mktccs();
  tcc_add_symbol(tccs, "calc_x", fcn);
  int (* fcn2)() = tccs_compile_and_get(tccs, b, "calc_y");
  printf("outp: %i\n", fcn2());
  tcc_delete(tccs);

  tccs = mktccs();
  float * var = tccs_compile_and_get(tccs, c, "cval");
  printf("outp: %i\n", var);
  tcc_delete(tccs);
}
