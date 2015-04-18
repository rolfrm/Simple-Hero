//requires event.h
typedef struct{
  //arrows or gamepad
  // -1.0 <-> 1.0
  double x, y;
  // select up/down +/- 1.
  int select_delta;
  // enter pressed
  int select_accept;
}game_controller;

extern game_controller game_controller_blank;

game_controller game_controller_get_dif(game_controller old, game_controller new);

void game_controller_update_kb(game_controller * gc, key_event key);
