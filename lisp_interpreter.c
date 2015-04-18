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
      circ.xy = (vec2){results[0].data_double, results[1].data_double};
      circ.r = results[2].data_double;
      result->typeid = TYPEID_CIRCLE;
      result->circle = circ;
      
    }else if(vexprcmpstr(name, "entity")){
      // supports optional id. color, scenery.
      // requires a circle
      entity entity;
      entity.id = "noid";
      entity.color = (color){.color = 0xFFFFFFFF};
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
	  free(results[i -1].data_str);
	}else if(r.typeid == TYPEID_CIRCLE){
	  entity.circle = r.circle;
	}else{
	  goto jmperror;
	}
      }
      result->typeid = TYPEID_ENTITY;
      result->entity = entity;
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
