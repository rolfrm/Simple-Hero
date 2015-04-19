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
  return 
    circle_graph_count_circles(graph.node->left)
    + circle_graph_count_circles(graph.node->right);
}

void circle_graph_count_stats(circle_graph graph, u32 * leafs, u32 * nodes){
  printf("GRAPH TYPE: %i\n",graph.type);
  if(graph.type == CG_LEAF){
    *leafs += 1;
    return;
  }
  *nodes += 1;
  circle_graph_count_stats(graph.node->left,leafs,nodes);
  circle_graph_count_stats(graph.node->right,leafs,nodes);
}

void write_circles_to_array(circle_graph graph, circle * array){
  circle * next(circle_graph * g, circle * arr){
    if(g->type == CG_LEAF){
      *arr = g->circ;
      return arr + 1;
    }else{
      arr = next(&g->node->left,arr);
      arr = next(&g->node->right,arr);
      return arr;
    }
  }
  next(&graph,array);
}

circ_tree * make_circ_tree(entity * entities, int count){
  circ_tree * circ_trees = malloc(sizeof(circ_tree) * count);
  u32 circle_cnt = 0;
  u32 node_cnt = 0;
  for(int i = 0; i < count ; i++){
    circle_graph_count_stats(entities[i].circle, &circle_cnt,&node_cnt);	       
    printf("allocating.. %i %i\n",node_cnt, circle_cnt);
  }
  node_cnt += circle_cnt; //need leaf nodes as well
  
  circle_tree * circle_trees = malloc(node_cnt * sizeof(circle_tree));
  circle * circles = malloc(circle_cnt * sizeof(circle));
  circle_cnt = 0;
  node_cnt = 0;
  for(int i = 0; i < count; i++){
    circ_tree * ct = circ_trees + i;
    ct->circles = circles + circle_cnt;
    ct->tree = circle_trees + node_cnt;
    int node = 0;
    int circ = 0;
    int next_node(circle_graph * g){
      if(g->type == CG_LEAF){
	int circ_idx = circ++;
	ct->circles[circ_idx] = g->circ;
	ct->tree[node].func = LEAF;
	ct->tree[node].circle = circ_idx;
	return node++;

      }else{
	int thisnode = node++;
	ct->tree[thisnode].func = g->node->func;
	int next = next_node(&g->node->left);
	int next2 = next_node(&g->node->right);
	ct->tree[thisnode].left = next;
	ct->tree[thisnode].right = next2;
	return thisnode;
	
      }
      return 0;
    }
    next_node(&entities[i].circle);
    circle_cnt += circ;
    node_cnt += node;
    logd(" it: %i %i max: %i %i\n",circ, node, circle_cnt, node_cnt);
  }
  for(u32 i = 0; i < node_cnt; i++){
    log("%i %i %i\n",circle_trees[i].func,circle_trees[i].left, circle_trees[i].right);
  }
  
  // quick sanity test.. 
  for(int i = 0 ; i < count; i++){
    log("item %i\n",i );
    int size = circle_tree_size(circ_trees[i].tree);
    int leafs = circle_tree_max_leaf(circ_trees[i].tree);
    printf("size: %i, leafs: %i\n",size,leafs);
  }

  return circ_trees;
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

