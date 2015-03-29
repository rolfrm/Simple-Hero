//requires game_object.h
struct _game_state;

typedef struct _game_state{
  bool is_running;
  game_obj * items;
  int item_count;
  
}game_state;
