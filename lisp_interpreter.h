// requires bitgui.h, circle.h, lisp_parser.h

typedef struct{
  union{
    u8 data[4];
    u8 r,g,b,a;
    int color;
  };
}color;

typedef struct{
  char * id;
  color color;
  circle circle;
  bool is_scenery;
}entity;
	  
typedef struct{
  int typeid;
  union{
    u64 data;
    double data_double;
    char * data_str;
    circle circle;
    color color;
    entity entity;
  };
}lisp_result;

enum{
  TYPEID_DOUBLE = VALUE_TYPE_LAST,
  TYPEID_CIRCLE,
  TYPEID_COLOR,
  TYPEID_ENTITY,
  TYPEID_ERROR,
  TYPEID_TYPE_OK
};

void eval_expr(expression * expr, bool just_check_types, lisp_result * result);
