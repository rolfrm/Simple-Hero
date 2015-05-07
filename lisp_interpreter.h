// requires bitguy.h, circle.h, lisp_parser.h, color.h, game_state.h

typedef struct{
  int typeid;
  union{
    u64 data;
    double data_double;
    char * data_str;
    circle_graph circle;
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

// results has to be cleaned manually using lisp_result_delete;
void eval_expr(expr * expr, bool just_check_types, lisp_result * result);
void lisp_result_delete(lisp_result * r);
