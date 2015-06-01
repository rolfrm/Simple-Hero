#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "microthreads.h"
#include <iron/full.h>
#include "color.h"
#include "circle.h"
#include "game_object.h"
#include "game_state.h"
#include "event.h"
#include "renderer.h"
#include <stdarg.h>

#include "vox.h"
#include "vox_raster.h"
#include "lisp_parser.h"
#include "lisp_types.h"
#include "lisp_compiler.h"
const char * allowed_errors[] ={
  "Unknown touch device",
  "Invalid renderer"
};
#include <signal.h>
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
  loge("** ERROR at %s:%i **\n",file,line);
  loge("%s", buffer);
  printf("\n");
  printf("** **\n");
  raise(SIGINT);
}

void run_ai(ccdispatch * dispatcher, game_state * gs);
bool test_circle();
bool lisp_compiler_test();
void circle_sg_main();
void ld32_main();
bool test_lisp_parser();
bool test_lisp2c();
int main(){
  compiler_state * c = compiler_make();
  lisp_run_script_file(c,"test.lisp");
  //TEST(test_lisp_parser);
  //TEST(test_lisp2c);
  //  TEST(lisp_compiler_test);
  return 0;
  //TEST(test_circle);
  TEST(test_utils);
  TEST(vox_test);
  TEST(vox_raster_test);
  //  ld32_main();
  //circle_sg_main();
  return 0;
  test_circle();
  
  //return 0;
  player pl1 = {PLAYER, 100, 0};
  grass_leaf leaf = {GRASS, 100, 100};
  grass_leaf leaf2 = {GRASS, 301, 203};
  campfire campfire = {CAMPFIRE, 200, 200, 50};
  game_state state;
  state.items = malloc(sizeof(game_obj) * 4);
  state.is_running = true;
  state.items[0].player = pl1;
  state.items[1].grass_leaf = leaf;
  state.items[2].campfire = campfire;
  state.items[3].grass_leaf = leaf2;

  state.item_count = 4;

  ccdispatch * dis = ccstart();
  
  game_renderer * renderer = renderer_load();

  while(state.is_running){
    usleep(10000);
    renderer_render_game(renderer,&state);      
    run_ai(dis,&state);
    if(faulty)break;
  }
  
  renderer_unload(renderer);
  return 0;
}
