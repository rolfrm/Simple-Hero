

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "microthreads.h"
#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"
#include "../bitguy/linmath.h"
#include "circle.h"
#include "game_object.h"
#include "game_state.h"
#include "renderer.h"
#include <stdarg.h>

extern bool faulty;

void run_ai(ccdispatch * dispatcher, game_state * gs);
bool test_circle();
int circle_sg_main(){
  test_circle();
  game_state state;
  state.items = malloc(sizeof(game_obj) * 0);
  state.is_running = true;
  state.item_count = 0;

  logitem item1 = {"first message",0,false};
  logitem item2 = {"quit?",1,true};
  logitem logitems[2] = {item1,item2};

  ccdispatch * dis = ccstart();
  
  game_renderer * renderer = renderer_load();

  while(state.is_running){
    usleep(10000);
    renderer_render_game(renderer,&state);      
    if(faulty)break;
  }
  
  renderer_unload(renderer);
  return 0;
}
