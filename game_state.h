//requires game_object.h, circle.h
struct _game_state;

typedef struct {
  char * text;
  int id;
  bool is_option;
}logitem;

typedef struct _game_state{
  bool is_running;

  game_obj * items;
  int item_count;

  logitem * logitems;
  int logitem_count;

  circ_tree * trees;
  int trees_count;
  
  int selected_idx;
}game_state;
