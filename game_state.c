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

int circle_graph_count_circles(circle_graph graph){
  if(graph.type == CG_LEAF)
    return 1;
  return circle_graph_count_circles(graph.node->left)
    + circle_graph_count_circles(graph.node->right);
}


typedef struct{
  game_type type;
  const char * name;
}game_type_str_pair;
game_type_str_pair game_type2str[] ={
  {UNKNOWN, "unknown"},
  {DEAD,"dead"},
  {WEAPON,"weapon"},
  {PLAYER, "player"},
  {ENEMY,"enemy"},
  {SCENERY, "scenery"},
  {GOBLIN, "goblin"},
  {GRASS, "grass"},
  {WALL, "wall"},
  {CAMPFIRE, "campfire"}
};


game_type game_type_from_string(char * str){
  for(u32 i = 0; i < array_count(game_type2str);i++)
    if(strcmp(game_type2str[i].name,str) == 0)
      return game_type2str[i].type;
  return UNKNOWN;
}

