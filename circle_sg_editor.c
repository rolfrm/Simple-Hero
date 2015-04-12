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
#include "event.h"
#include "renderer.h"

#include <stdarg.h>

extern bool faulty;

void run_ai(ccdispatch * dispatcher, game_state * gs);
bool test_circle();

bool test_util_hash_table();

int get_logoption_cnt(logitem * items, int count){
  for(int i = 0; i < count; i++)
    if(items[count - i - 1].is_option == false)
      return i;
  return -1;
}

logitem * get_logoption(logitem * items, size_t count, int idx){
  int optcnt = get_logoption_cnt(items, count);
  if(idx >= optcnt || idx < 0)
    return NULL;
  return items + (count - 1 - idx);
}

int circle_sg_main(){
  test_circle();
  if(!test_util_hash_table()){
    printf("ERROR\n");
    //return -1;
  }
  //return 0;
  game_state state;
  state.items = malloc(sizeof(game_obj) * 0);
  state.is_running = true;
  state.item_count = 0;

  void quitfcn (){
    state.is_running = false;
    printf("qqquiiit!\n");
  }
  void printhi(){
    printf("hi..\n");
  }

  logitem item1 = {"first aaaaaaaaaaaaa hmmmm what now?",0,false};
  logitem item2 = {"[quit?]",1,true, quitfcn};
  logitem item3 = {"[really quit?]",2,true, printhi};
  logitem item4 = {"[really really quit?]",3,true, printhi};
  logitem logitems[] = { item1, item2, item3, item4};
  
  state.logitems = logitems;
  state.logitem_count = array_count(logitems);

  state.selected_idx = 1;

  ccdispatch * dis = ccstart();
  

  game_renderer * renderer = renderer_load();
  // .. testing .. //
  circle circles[] = {
    {{0,0 - 50},100}
    ,{{0,0 + 50},100}
    ,{{0,0 + 50},15}
  };
  
  circle_tree tree[] = {{ISEC,1,2},{LEAF,0,0},{ISEC,1,2},{LEAF,1,0},{LEAF,2,0}}; 

  circle circles2[] = {
    {{0,0 - 50},100},
    {{0,0 + 50},100},
    {{0,0 + 50},15}
  };
  
  mat3 m1 = mat3_2d_translation(80,0);
  mat3 m3 = mat3_2d_translation(-80,0);
  mat3 m2 = mat3_2d_rotation(1.0);
  mat3 mt = mat3_2d_translation(200,200);
  mt = mat3_mul(mt,m2);
  //circle_tree tree2[] = {{SUB,1,2},{LEAF,0,0},{SUB,1,2},{LEAF,1,0},{LEAF,2,0}}; 
  circle_tree tree2[] = {{ISEC,1,2},{LEAF,0,0},{ISEC,1,2},{LEAF,1,0},{LEAF,2,0}}; 
  circle_tform(circles2,array_count(circles2),m1);
  circle_tform(circles,array_count(circles),m3);
  circle_tform(circles,array_count(circles),mt);
  circle_tform(circles2,array_count(circles2),mt);
  circ_tree a = {tree,circles};
  circ_tree b = {tree2,circles2};
  circ_tree * ct = sub_tree(ADD,&a,&b);
  circ_tree _trees[] = {*ct};
  state.trees = _trees;
  state.trees_count = array_count(_trees);

  while(state.is_running){
    usleep(10000);
    renderer_render_game(renderer,&state);
    event evt;      
    while(renderer_read_events(&evt,1)){
      switch(evt.type){
      case QUIT:
	state.is_running = false;
	printf("Quit pls!\n");
      case KEY:
	if(evt.key.type == KEYDOWN){
	  switch(evt.key.sym){
	  case KEY_UP:
	    state.selected_idx++;
	    break;
	  case KEY_DOWN:
	    state.selected_idx--;
	    break;
	  case KEY_RETURN:
	    printf("Enter!\n");
	    logitem * itm = get_logoption(logitems,array_count(logitems),state.selected_idx);
	    if(itm != NULL) itm->cb(NULL);
	    break;
	  default:
	    break;
	  }
	}
      default:
	break;
      }
    }
    if(faulty)break;
  }
  free(ct);  
  renderer_unload(renderer);
  return 0;
}
