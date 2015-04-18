#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "microthreads.h"
#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"
#include "../bitguy/linmath.h"
#include "color.h"
#include "circle.h"
#include "game_object.h"
#include "game_state.h"
#include "event.h"
#include "renderer.h"
#include "lisp_parser.h"
#include "lisp_interpreter.h"

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

char * read_file(FILE * s){
  fseek(s, 0, SEEK_END);
  i64 p = ftell(s);
  char * str = malloc(p);
  fseek(s,0,SEEK_SET);
  i64 read_p = fread(str, 1, p,s);
  if(read_p != p){
    ERROR("Unexpected EOF %i != %i", p, read_p);
    free(str);
    return NULL;
  }
  return str;
}

void load_level(FILE * level_stream, game_state * state){
  char * l1_data = read_file(level_stream);
  char * end_of_parse = l1_data;

  entity * entities = NULL;

  int entity_count = 0;
  while(end_of_parse != NULL && *end_of_parse != 0){
    expression exprs[64];
    int exprs_cnt = array_count(exprs);
  
    end_of_parse = lisp_parse(end_of_parse, exprs, &exprs_cnt);
    
    lisp_result r[exprs_cnt];
    for(int i = 0 ; i < exprs_cnt; i++){
      eval_expr(exprs + i, false, r + i);
      if(r[i].typeid == TYPEID_ERROR){
	ERROR("Error intrepreting results!");
	return;
      }
      delete_expression(exprs + i);
    }

    // load entities
    int entity_cnt = 0;
    int start_count = entity_count;
    for(int i = 0; i < exprs_cnt; i++){
      if(r[i].typeid == TYPEID_ENTITY){
	entity_cnt++;
	entity_count++;
      }
    }

    entities = realloc(entities, entity_count * sizeof(entity));
    
    entity_cnt = 0;
    for(int i = 0; i < exprs_cnt; i++){
      if(r[i].typeid == TYPEID_ENTITY){
	int j = start_count + entity_cnt++;
	entities[j] = r[i].entity;
      }
      lisp_result_delete(r + i);
    }
  }

  circle * circles =  malloc(entity_count * sizeof(circle));
  circle_tree * circ_trees = malloc(entity_count * sizeof(circle_tree));
  circ_tree *circc_trees = malloc(entity_count * sizeof(circ_tree));
  color * colors = malloc(entity_count * sizeof(color));

  for(int i = 0; i < entity_count; i++){
    circles[i] = entities[i].circle;
    circ_trees[i] = circ_leaf(0);
    circc_trees[i] = (circ_tree){circ_trees + i,circles + i}; 
    colors[i] = entities[i].color;
  }
  
  if(entities != NULL){
    free(entities);
  }
  state->trees = circc_trees;
  state->trees_count = entity_count;
  state->colors = colors;
  free(l1_data);
}

//clear the memory.
void unload_level(game_state * state){
  if(state->colors == NULL) return;
  free(state->trees[0].circles);
  free(state->trees[0].tree);
  free(state->trees);
  free(state->colors);
  state->colors = NULL;
  state->trees = NULL;
  state->trees_count = 0;
}

void ld32_main(){
  logd("start..\n");
  test_lisp_parser();
  
  test_circle();
  if(!test_util_hash_table()){
    printf("ERROR\n");
  }
  
  game_state state;
  
  //unload_level(&state);
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

  logitem item1 = {"first aaaaaaaaaaaaa hmmmm what now?",0,false, NULL};
  logitem item2 = {"[quit?]",1,true, quitfcn};
  logitem item3 = {"[really quit?]",2,true, printhi};
  logitem item4 = {"[really really quit?]",3,true, printhi};
  logitem logitems[] = { item1, item2, item3, item4};
  
  state.logitems = logitems;
  state.logitem_count = array_count(logitems);

  state.selected_idx = 1;
  
  game_renderer * renderer = renderer_load();
  FILE * l1 = fopen("level1.lisp","rb");
  load_level(l1, &state);
  fclose(l1);
  while(state.is_running){
    unload_level(&state);
    FILE * l1 = fopen("level1.lisp","rb");
    load_level(l1, &state);
    fclose(l1);
    usleep(100000);
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
  //  free(ct);  
  renderer_unload(renderer);
  return;
}
