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
#include "lisp_parser.h"
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

bool vexprcmpstr(value_expr val, const char * str){
  if(strlen(str) == (size_t)val.strln){
    if(strncmp(val.value,str,val.strln) == 0){
      return true;
    }
  }
  return false;
}

	  
typedef struct{
  union{
    u8 data[4];
    u8 r,g,b,a;
    int color;
  };
}color;

typedef struct{
  char * id;
  color color;
  circle circle;
  bool is_scenery;
}entity;
	  
typedef struct{
  int typeid;
  union{
    u64 data;
    double data_double;
    char * data_str;
    circle circle;
    color color;
    entity entity;
  };
}lisp_result;

char * array2str(char * data, size_t len){
  void * out_data = malloc(len + 1);
  memcpy(out_data,data,len);
  data[len] = 0;
  return out_data;
}
enum{
  TYPEID_DOUBLE = VALUE_TYPE_LAST,
  TYPEID_CIRCLE,
  TYPEID_COLOR,
  TYPEID_ENTITY,
  TYPEID_ERROR,
  TYPEID_TYPE_OK
};

void eval_expr(expression * expr, bool just_check_types, lisp_result * result){
  result->typeid = TYPEID_ERROR; // In case noone sets it.
  if(expr->type == VALUE){
    value_expr val = expr->value;
    char str[val.strln + 1];
    str[val.strln] = 0;
    memcpy(str,val.value,val.strln);
    switch(val.type){
    case NUMBER:
      result->data_double = strtod(str,NULL);
      result->typeid = TYPEID_DOUBLE;
      break;
    case KEYWORD:
    case SYMBOL:
    case STRING:
      result->typeid = val.type;
      result->data_str = array2str(str,val.strln + 1);
      break;
    default:
      ERROR("Unsupported val type: %i", val.type);
    }
  }else if(expr->type == EXPRESSION){
    
    sub_expression_expr sexpr = expr->sub_expression;
    value_expr name = sexpr.name;
    lisp_result results[sexpr.sub_expression_count];
    for(int i = 0; i < sexpr.sub_expression_count; i++){
      eval_expr(sexpr.sub_expressions + i, just_check_types, results + i);
      if(results[i].typeid == TYPEID_ERROR){
	loge("ERROR matching type at %s arg %i", name.value, i);
      }
    }
    
    if(vexprcmpstr(name, "circle")){
      circle circ;
      if(array_count(results) != 3)
	loge("circle requires three arguments");
      for(int i = 0; i < array_count(results); i++)
	if(results[i].typeid != TYPEID_DOUBLE)
	  loge("Circle only supports DOUBLE args");
      circ.xy = (vec2){results[0].data_double, results[1].data_double, results[2].data_double};;
      result->typeid = TYPEID_CIRCLE;
      result->circle = circ;
      printf("success parsing circle\n");
      
    }else if(vexprcmpstr(name, "entity")){
      // supports optional id. color, scenery.
      // requires a circle
      entity entity;
      for(size_t i = 0; i < array_count(results); i++){
	lisp_result r = results[i];
	if(r.typeid == KEYWORD){
	  if(strcmp(r.data_str, "id") == 0){
	    i++;
	    r = results[i];
	    if(r.typeid != STRING)
	      goto jmperror;
	    
	    entity.id = r.data_str;
	  }else if(strcmp(r.data_str, "color") == 0){
	    
	    i++;
	    r = results[i];
	    if(r.typeid != TYPEID_COLOR)
	      goto jmperror;
	    entity.color = r.color;
	  }else if(strcmp(r.data_str, "scenery") == 0){
	    i++;
	    r = results[i];
	    if(r.typeid != TYPEID_DOUBLE)
	      goto jmperror;
	    int v = (int)r.data_double;
	    entity.is_scenery = (bool)v;
	  
	  }else{
	    goto jmperror;
	  }
	}else if(r.typeid == TYPEID_CIRCLE){
	  entity.circle = r.circle;
	}else{

	  goto jmperror;
	}
      }
      result->typeid = TYPEID_ENTITY;
      result->entity = entity;
      printf("success parsing entity\n");
    }else if(vexprcmpstr(name, "from.rgb")){
      if(array_count(results) != 3)
	goto jmperror;
      color color;
      for(int i = 0; i < 3; i++){
	if(results[i].typeid != TYPEID_DOUBLE)
	  goto jmperror;
	color.data[i] = results[i].data_double;
      }
      result->typeid = TYPEID_COLOR;
      result->color = color;
      printf("success parsing color\n");
    }else{
      loge("Unknown function '%.*s'\n",name.strln,name.value);
    }
  }
  else{
    ERROR("Unsupported action ");
  }
  return;
 jmperror:
  result->typeid = TYPEID_ERROR;
  loge("ERROR matching type\n");  
}


int circle_sg_main(){
  FILE * l1 = fopen("level1.lisp","rb");
  char * l1_data = read_file(l1);
  expression exprs[10];
  int exprs_cnt = array_count(exprs);
  char * end_of_parse = lisp_parse(l1_data, exprs, &exprs_cnt);
  if(end_of_parse == NULL)
    ERROR("Could not parse whole file.");
  log("parsed %i expressions\n", exprs_cnt);
  
  for(int i = 0 ; i < exprs_cnt; i++){
    printf("** Evaluating:\n");
    print_expression(exprs + i);
    lisp_result r;
    eval_expr(exprs + i, false, &r);

  }

  if(true)
    return 0;
  //return test_lisp_parser();
  /*test_circle();
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

  //ccdispatch * dis = ccstart();
  

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
  circle_tree tree2[5];
  tree2[0] = (circle_tree){ISEC,1,2};//,{LEAF,0,0},{ISEC,1,2},{LEAF,1,0},{LEAF,2,0}}; 
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
  return 0;*/
}
