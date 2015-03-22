#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "microthreads.h"
#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"
#include "../bitguy/linmath.h"
#include "game_object.h"
#include "game_state.h"
#include "renderer.h"
#include <stdarg.h>

const char * allowed_errors[] ={
  "Unknown touch device",
  "Invalid renderer"
};

bool faulty = false;
void _error(const char * file, int line, const char * str, ...){
  char buffer[1000];  
  va_list arglist;
  va_start (arglist, str);
  vsprintf(buffer,str,arglist);
  va_end(arglist);
  bool noncritical = false;
  for(u32 i = 0; i < array_count(allowed_errors);i++)
    if(strstr(buffer, allowed_errors[i]) != NULL)
      noncritical = true;
  if(!noncritical)
    faulty = true;
  if(noncritical) return; //skip noncritical errors
  printf("** ERROR at %s:%i **\n",file,line);
  printf(buffer);
  printf("\n");
  printf("** **\n");
}

void run_ai(ccdispatch * dispatcher, game_state * gs);

int main(){
  player pl1 = {PLAYER, 0, 0};
  grass_leaf leaf = {GRASS, 100, 100};
  campfire campfire = {CAMPFIRE, 200, 200, 50};
  game_state state;
  state.items = malloc(sizeof(game_obj) * 3);
  state.is_running = true;
  state.items[0].player = pl1;
  state.items[1].grass_leaf = leaf;
  state.items[2].campfire = campfire;
  state.item_count = 3;

  ccdispatch * dis = ccstart();
  for(int i = 0; i < 100;i++)
    run_ai(dis,&state);
  //ccstep(dis);
  return 0;

  game_renderer * renderer = renderer_load();

  while(state.is_running){
    renderer_render_game(renderer,&state);      
    if(faulty)break;
  }
  
  renderer_unload(renderer);
  return 0;
}
