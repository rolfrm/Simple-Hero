// Requires bitguy.h
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

struct _decl{
  char * name;
  type_def type;
};

// requires lisp_types.h
struct _c_fundef;
typedef struct _c_fundef c_fundef;
struct _c_expr;
typedef struct _c_expr c_expr;

typedef struct{
  c_expr * exprs;
  size_t expr_cnt;
}c_block;

struct _c_fundef{
  decl fdecl;
  c_block block;
};

typedef enum{
  C_VAR = 2,
  C_VALUE = 4,
  C_RETURN = 6,
  C_BLOCK = 7
}c_expr_kind;

typedef enum{
  C_SUB_EXPR,
  C_INLINE_VALUE,
  C_FUNCTION_CALL,
  C_OPERATOR,
  C_DEREF,
  C_SYMBOL
}c_value_kind;

struct _c_value;
typedef struct _c_value c_value;

typedef struct{
  decl var;
  c_value * value; //if NULL no value
}c_var;

typedef struct{
  type_def * type;
  c_expr * sub_expr;
}c_cast;

typedef struct{
  c_value * left;
  c_value * right;
  char operator;
}c_operator;

typedef struct{
  char * value;
  type_def * type;
}c_raw_value;

typedef struct{
  char * name;
  c_value * args;
  size_t arg_cnt;
}c_function_call;

struct _c_value{
  c_value_kind type;
  union{
    c_cast cast;
    c_raw_value raw;
    char * symbol;
    c_value * value;//sub expr, deref
    c_function_call call;
    c_operator operator;
  };  
};

struct _c_expr{
  c_expr_kind type;
  union{
    c_var var;
    // return, value
    c_value value;
    c_block block;
  };
};

typedef enum{
  C_FUNCTION_DEF,
  C_VAR_DEF,
  C_INCLUDE,
  C_INCLUDE_LIB,
  C_DECL,
  C_TYPE_DEF
}c_root_code_kind;

typedef struct{
  c_root_code_kind type;
  union{
    char * include;
    c_fundef fundef;
    c_var var;
    decl decl;
    type_def * type_def;
  };
}c_root_code;

bool type_def_cmp(type_def a, type_def b);
void print_cdecl(decl idecl);

type_def make_simple(char * name, char * cname);
type_def make_ptr(type_def * def);

// simple function to calculate type dependencies.
// writes the dependencies of a type in defs
// descending order, so least dependent comes first.
void make_dependency_graph(type_def * deps, type_def def);
void print_def(type_def type, int ind, bool is_decl);

void print_c_code(c_root_code code);

// test
bool test_print_c_code();
