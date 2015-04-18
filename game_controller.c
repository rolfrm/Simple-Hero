#include <stdint.h>
#include "../bitguy/bitguy.h"
#include "event.h"
#include "game_controller.h"

game_controller game_controller_blank = {0.0,0.0,0,0};

game_controller game_controller_get_dif(game_controller old, game_controller new){
  game_controller gc;
  gc.x = new.x - old.x;
  gc.y = new.y - old.y;
  gc.select_delta = new.select_delta - old.select_delta;
  gc.select_accept = new.select_accept - old.select_accept;
  return gc;
}

void game_controller_update_kb(game_controller * gc, key_event key){
  int state = key.type == KEYDOWN ? 1 : 0;
  switch(key.sym){
  case KEY_w:
    gc->y = state;
    break;
  case KEY_s:
    gc->y = -state;
    break;
  case KEY_a:
    gc->x = -state;
    break;
  case KEY_d:
    gc->x = state;
    break;
  case KEY_UP:
    gc->select_delta = state;
    break;
  case KEY_DOWN:
    gc->select_delta = -state;
    break;
  case KEY_RETURN:
    gc->select_accept = state;
    break;
  default:
    break;
  }
}
