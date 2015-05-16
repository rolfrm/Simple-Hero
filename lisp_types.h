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


bool type_def_cmp(type_def a, type_def b);
void print_cdecl(decl idecl);

type_def make_simple(char * name, char * cname);
type_def make_ptr(type_def * def);

// simple function to calculate type dependencies.
// writes the dependencies of a type in defs
// descending order, so least dependent comes first.
void make_dependency_graph(type_def * deps, type_def def);
void print_def(type_def type, int ind, bool is_decl);
