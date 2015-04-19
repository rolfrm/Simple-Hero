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
#include "game_controller.h"
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
  return items + count - optcnt + idx;
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
      }else{
	lisp_result_delete(r + i);
      }
    }
  }

  int circle_cnt =0;
  for(int i = 0 ; i < entity_count;i++){

  }

  circle * circles =  malloc(entity_count * sizeof(circle));
  circle_tree * circ_trees = malloc(entity_count * sizeof(circle_tree));
  circ_tree *circc_trees = malloc(entity_count * sizeof(circ_tree));
  color * colors = malloc(entity_count * sizeof(color));
  
  for(int i = 0; i < entity_count; i++){
    circles[i] = entities[i].circle.circ;
    circ_trees[i] = circ_leaf(0);
    circc_trees[i] = (circ_tree){circ_trees + i,circles + i}; 
    colors[i] = entities[i].color;
  }
  
  state->entities = entities;
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
  free(state->circles);
  for(int i = 0 ; i < state->trees_count; i++)
    if(state->entities[i].id != NULL)
      free(state->entities[i].id);
  free(state->entities);
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
  
  game_renderer * renderer = renderer_load();
  
  game_controller gc = game_controller_blank;
  
  entity * player_ent = NULL;
  entity * weapon_ent = NULL;
  vec2 weapon_offset = {.data = {0.0, 0.0}};
  void load_game(){
    FILE * l1 = fopen("level1.lisp","rb");
    load_level(l1, &state);
    fclose(l1);
    player_ent = NULL;
    weapon_ent = NULL;
    for(int i = 0 ; i < state.trees_count; i++){
      game_type type = state.entities[i].type;
      switch(type){
      case PLAYER:
      	player_ent = state.entities + i;
	break;
      case WEAPON:
	weapon_ent = state.entities + i;
	break;
      default:
	break;
      }
    }
    if(player_ent != NULL && weapon_ent != NULL){
      weapon_offset = vec2_sub(weapon_ent->xy, player_ent->xy);
    }
  }

  void reload_game(){
    printf("reloading..\n");
    unload_level(&state);
    load_game();
  }

  logitem item1 = {"rock on..",0,false, NULL};
  logitem item2 = {"[reload?]",2,true, reload_game};
  logitem item3 = {"[really really quit?]",3,true, quitfcn};
  logitem logitems[] = { item1, item2, item3};
  
  state.logitems = logitems;
  state.logitem_count = array_count(logitems);

  state.selected_idx = 0;
  
  load_game();
  while(state.is_running){
    for(int i = 0 ; i < state.trees_count; i++){
      entity * ent = state.entities + i;
      if(ent->type & ENEMY && !(ent->type & DEAD)){
	vec2 v2 = {.data = {0.0,0.0}};
	if(player_ent != NULL)
	  v2 = vec2_sub(player_ent->xy, ent->xy);
	v2 = vec2_normalize(v2);
	ent->xy.x += rand() % 5 - 2;
	ent->xy.y += rand() % 5 - 2;
	ent->xy = vec2_add(ent->xy, vec2_scale(v2,0.4));
      }
    }
    for(int i = 0 ; i < state.trees_count; i++){
      for(int j = i + 1; j < state.trees_count; j++){
	/*entity * enta = state.entities + i;
	entity * entb = state.entities + j;
	game_type ta = enta->type, tb = entb->type;
	if((ta | tb) & DEAD)
	  continue;
	bool one_weapon = (ta | tb) & WEAPON;
	bool one_enemy =  (ta | tb) & ENEMY;
	bool one_player = (ta | tb) & PLAYER;
	bool dontresolve = one_weapon;
	float move_ratio = (enta->type & SCENERY) ? 0.0 : 0.5;
	if(ta & SCENERY){
	  if(tb & SCENERY) continue;
	  move_ratio = 1.0;
	}
	vec2 moveout;
	bool collides = false;//circle_collision(&enta->circle,&entb->circle,&moveout);
	if(collides && !dontresolve){
	  // handle collision
	}
	if(collides && one_enemy && one_weapon){
	  entity * victim = (ta & ENEMY) ? enta : entb;
	  victim->type |= DEAD;
	  victim->color = (color){.data = {68,68,68,255}}; 
	}
	if(collides && one_enemy && one_player){
	  entity * victim = (ta & PLAYER) ? enta : entb;
	  victim->type |= DEAD;
	  victim->color = (color){.data = {68,68,68,255}}; 
	  weapon_ent->type |= DEAD;
	  weapon_ent->color = (color){.data = {68,68,68,255}}; 
	  }*/
      }
    }
    for(int i = 0 ; i < state.trees_count; i++){
      state.colors[i] = state.entities[i].color;
    }
    usleep(10000);
    renderer_render_game(renderer,&state);
    event evt;      
    game_controller gc_old = gc;
    while(renderer_read_events(&evt,1)){
      switch(evt.type){
      case QUIT:
	state.is_running = false;
	printf("Quit pls!\n");
      case KEY:
	game_controller_update_kb(&gc,evt.key);
	break;
      default:
	  break;
      }
    }
    
    game_controller gcdif = game_controller_get_dif(gc_old,gc);
    state.selected_idx += gcdif.select_delta != 0 ? gc.select_delta : 0;
    if(gcdif.select_accept == 1){
      
      printf("Enter!\n");
      logitem * itm = get_logoption(logitems,array_count(logitems),state.selected_idx);
      if(itm != NULL) itm->cb(NULL);
    }
    if(player_ent != NULL && false == (player_ent->type & DEAD)){
      player_ent->xy = vec2_add(player_ent->xy,(vec2){.data = {gc.x,gc.y}});
      
      if(weapon_ent != NULL)
	weapon_ent->xy = vec2_add(player_ent->xy,weapon_offset);
    }
    
    if(faulty) break;
  }
  renderer_unload(renderer);
}
