#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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

int main(){
  costack_test();
  return 0;
  i32 k = jump_consistent_hash(132132, 57);
  i32 k2 = jump_consistent_hash_str("hello world!!!", 128);
  u64 d1 = -1;
  i32 k3 = jump_consistent_hash_raw(&d1,sizeof(d1),128);
  printf("%i %i %i\n",k, k2,k3);
  player pl1 = {PLAYER, 0, 0};
  grass_leaf leaf = {GRASS, 100, 100};
  
  game_state state;
  state.is_running = true;
  state.items[0].player = pl1;
  state.items[1].grass_leaf = leaf;
  state.item_count = 2;
  game_renderer * renderer = renderer_load();
  while(state.is_running){
    renderer_render_game(renderer,&state);      
    if(faulty)break;
  }
  
  renderer_unload(renderer);
  return 0;
}
