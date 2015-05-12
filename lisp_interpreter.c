#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"
#include "../bitguy/linmath.h"
#include "color.h"
#include "circle.h"
#include "game_object.h"
#include "game_state.h"
#include "lisp_parser.h"
#include "lisp_interpreter.h"

// consider moving to utils.
char * array2str(char * data, size_t len){
  void * out_data = malloc(len + 1);
  memcpy(out_data,data,len);
  data[len] = 0;
  return out_data;
}

bool vexprcmpstr(value_expr val, const char * str){
  if(strlen(str) == (size_t)val.strln){
    if(strncmp(val.value,str,val.strln) == 0){
      return true;
    }
  }
  return false;
}

void lisp_result_delete(lisp_result * r){
  switch(r->typeid){
  case STRING:
  case KEYWORD:
  case SYMBOL:
    free(r->data_str);
    break;
  case TYPEID_ENTITY:
    free(r->entity.id);
    break;
  default:
    break;
  }
}

void eval_expr(expr * expr, bool just_check_types, lisp_result * result){
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
  }else if(expr->type == EXPR){
    
    sub_expr sexpr = expr->sub_expr;
    lisp_result * results;
    lisp_result _results[sexpr.sub_expr_count];
    results = _results;
    for(int i = 0; i < sexpr.sub_expr_count; i++){
      eval_expr(sexpr.sub_exprs + i, just_check_types, results + i);
      if(results[i].typeid == TYPEID_ERROR){
	ERROR("ERROR matching type at arg %i ", i);
      }
    }
    char * name = results[0].data_str;
    
    results++;
    u32 argn = sexpr.sub_expr_count -1;
    

    if(strcmp(name, "circle")){
      circle circ;
      if(argn != 3)
	loge("circle requires three arguments");
      for(u32 i = 0; i < argn; i++)
	if(results[i].typeid != TYPEID_DOUBLE)
	  loge("Circle only supports DOUBLE args");
      circ.xy = (vec2){.data = {results[0].data_double, results[1].data_double}};
      circ.r = results[2].data_double;
      result->typeid = TYPEID_CIRCLE;
      result->circle = (circle_graph){.type = CG_LEAF, .circ = circ};
    }else if(strcmp(name, "add") || strcmp(name, "sub") || strcmp(name, "isec") ){
      if(argn != 2){
	ERROR("unexpected number of args");
	goto jmperror;
      }
      for(u32 i = 0; i < argn; i++){
	if(results[i].typeid != TYPEID_CIRCLE){
	  ERROR("add only accepts CIRCLE graph args (arg %i)", i);
	  goto jmperror;
	}
      }
      circle_graph_node * cg = malloc(sizeof(circle_graph_node));
      if(strcmp(name, "add")){
	cg->func = ADD;
      }else if(strcmp(name, "sub")){
	cg->func = SUB;
      }else if(strcmp(name, "isec")){
	cg->func = ISEC;
      }
      cg->left = results[0].circle;
      cg->right = results[1].circle;
      result->circle.type = CG_NODE;
      result->circle.node =cg;
      result->typeid = TYPEID_CIRCLE;
      
    }else if(strcmp(name, "entity")){
      // supports optional id. color, scenery.
      // requires a circle
      entity entity;
      entity.xy = (vec2){.data = {0.0,0.0}};
      entity.id = NULL;
      entity.color = (color){.color = 0xFFFFFFFF};
      for(size_t i = 0; i < argn; i++){
	lisp_result r = results[i];
	if(r.typeid == KEYWORD){
	  if(strcmp(r.data_str, "id") == 0){
	    i++;
	    r = results[i];
	    if(r.typeid != STRING)
	      goto jmperror;
	    
	    entity.id = r.data_str;
	    entity.type = game_type_from_string(entity.id);
	  }else if(strcmp(r.data_str, "color") == 0){
	    
	    i++;
	    r = results[i];
	    if(r.typeid != TYPEID_COLOR)
	      goto jmperror;
	    entity.color = r.color;
	  }else{
	    goto jmperror;
	  }
	  free(results[i -1].data_str);
	}else if(r.typeid == TYPEID_CIRCLE){
	  entity.circle = r.circle;
	}else{
	  goto jmperror;
	}
      }
      result->typeid = TYPEID_ENTITY;
      result->entity = entity;
    }else if(strcmp(name, "from.rgb")){
      if(argn != 3)
	goto jmperror;
      color color;
      for(int i = 0; i < 3; i++){
	if(results[i].typeid != TYPEID_DOUBLE)
	  goto jmperror;
	color.data[i] = results[i].data_double;
      }
      result->typeid = TYPEID_COLOR;
      result->color = color;
    }else{
      loge("Unknown function '%s'\n",name);
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
