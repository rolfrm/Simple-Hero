//requires game_object.h, circle.h
struct _game_state;

// If changed here, please change game_state.c
typedef enum{
  //FLAGS:
  UNKNOWN = 0,
  DEAD =   0b10000000000,
  WEAPON = 0b01000000000,
  PLAYER = 0b00100000000,
  ENEMY =  0b00010000000,
  SCENERY= 0b00001000000,
  //specific types
  DELETED = 1,
  GOBLIN = ENEMY | 1,
  GRASS =  SCENERY | 2,
  WALL =   SCENERY | 3,
  CAMPFIRE = SCENERY | 4
}game_type;

game_type game_type_from_string(char * str);

typedef enum{
  CG_LEAF = 7,
  CG_NODE = 8
}circle_graph_type;

typedef struct _circle_graph_node circle_graph_node;

typedef struct _circle_graph{
  circle_graph_type type;
  union{
    circle circ;
    circle_graph_node * node;
  };
}circle_graph;

struct _circle_graph_node
{
  circle_func func;
  circle_graph left, right;
};


int circle_graph_count_circles(circle_graph graph);
void write_circles_to_array(circle_graph graph, circle * array);
typedef struct{
  char * id;
  game_type type;
  color color;
  circle_graph circle;
  vec2 xy;
  float rotation;
}entity;

circ_tree * make_circ_tree(entity * entities, int count);

typedef struct {
  char * text;
  int id;
  bool is_option;
  void (* cb) (void * ptr);
}logitem;



typedef struct _game_state{
  bool is_running;

  game_obj * items;
  int item_count;

  logitem * logitems;
  int logitem_count;

  entity * entities;
  circ_tree * trees;
  color * colors;
  int trees_count;
  
  int selected_idx;
}game_state;
