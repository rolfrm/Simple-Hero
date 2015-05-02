#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"
#include "../bitguy/linmath.h"

#include "vox.h"
#include "vox_internal.h"

struct _voxtree{
  u8 is_node;
  u8 color;
  // this is just to get a pointer to the next tree
  u8 next;
};

static const size_t node_size = 2;

voxtree * tree_next(voxtree * vt){
  return (voxtree *) &vt->next;
}

u8 * tree_color(voxtree * vt){
  return &vt->color;
}

bool tree_is_node(voxtree * vt){
  return vt->is_node;
}

bool tree_is_leaf(voxtree * vt){
  return !vt->is_node;
}

voxtree * tree_skip(voxtree * vt){
  if(tree_is_leaf(vt)) return tree_next(vt);
  vt = tree_next(vt);
  for(int i = 0; i < 8; i++){
    vt = tree_skip(vt);
  }
  return vt;
}

struct _voxtree_ctx{
  // Tree data. Might change with realloc when capacity changes.
  voxtree * tree;
  // Size in bytes
  size_t size;
  // Capacity in bytes
  size_t capacity;
};

voxtree_ctx * voxtree_ctx_make(size_t capacity){
  voxtree_ctx * ctx = malloc(sizeof(ctx));
  ctx->tree = malloc(capacity);
  ctx->tree->is_node = 0;
  ctx->tree->color = 0;
  ctx->size = 2;
  ctx->capacity = capacity;
  return ctx;
}

voxtree_ctx * voxtree_ctx_make_from_ptr(voxtree * tree, size_t size){
  voxtree_ctx * outctx = malloc(sizeof(voxtree_ctx));
  outctx->tree = tree;
  outctx->size =size;
  outctx->capacity = size;
  return outctx;
}

voxtree * voxtree_ctx_tree(voxtree_ctx * ctx){
  return ctx->tree;
}

size_t voxtree_ctx_capacity(voxtree_ctx * ctx){
  return ctx->capacity;
}

size_t voxtree_ctx_size(voxtree_ctx * ctx){
  return ctx->size;
}

static size_t tree_dist(voxtree * a, voxtree * b){
  return ((u8 *) a - (u8 *)b);
}

voxtree * resize_ctx(voxtree * vt, voxtree_ctx * ctx, size_t new_size){
  size_t offset = tree_dist(vt, ctx->tree);
  ctx->tree = realloc(ctx->tree, new_size);
  ctx->capacity = new_size;
  return (voxtree *) (((u8 *)ctx->tree) + offset);
}

voxtree * tree_resize_node(voxtree * vt, size_t new_size, voxtree_ctx * ctx){
  voxtree * next = tree_skip(vt);  
  size_t current_size = tree_dist(next, vt);
  i64 size_change = new_size - current_size;

  // double capacity until it can contain the new size
  // mostly if size_change > 0
  while(ctx->size + size_change > ctx->capacity)
    vt = resize_ctx(vt, ctx, ctx->capacity * 2);

  // move things back or forth depending on size_change.
  // if size is reduced, move things back
  // otherwise move things forward
  u8 * start = ((u8 *) vt) + current_size;
  u8 * begin = ((u8 *) vt);
  size_t cnt = ctx->size - (start - begin);

  memmove(start + size_change, start, cnt);
  ctx->size += size_change;
  return vt;
}

voxtree * tree_leaf_to_node(voxtree * vt, voxtree_ctx * ctx){
  if(tree_is_node(vt)) return vt;
  u8 color = *tree_color(vt);
  u8 subtree_data[] = {1,color,
		       0,color,
		       0,color,
		       0,color,
		       0,color,
		       
		       0,color,
		       0,color,
		       0,color,
		       0,color};
  
  vt = tree_resize_node(vt, sizeof(subtree_data), ctx);
  memcpy(vt,subtree_data,sizeof(subtree_data));
  vt->is_node = true;
  return vt;
}

voxtree * tree_node_to_leaf(voxtree * vt, voxtree_ctx * ctx){
  if(tree_is_leaf(vt)) return vt;
  vt = tree_resize_node(vt,node_size,ctx);
  vt->is_node = false;
  return vt;
}

voxtree * tree_insert_node(voxtree * node, voxtree * node_data, voxtree_ctx * ctx){
  voxtree * next_data = tree_skip(node_data);
  size_t new_size = tree_dist(next_data, node_data);
  node = tree_resize_node(node, new_size, ctx);
  memmove(node, node_data, new_size);
  return node;
}

// test
voxtree * tree_make_test(){
  u8 tree_template[] = {1, 0,  
			0, 1,  
			0, 2,  
			0, 3,  
			0, 4,  
			1, 5,  
			0, 6,  
			0, 7,  
			0, 8,  
			0, 9,  
			0, 10,  
			0, 11, 
			0, 12, 
			0, 13,
			0, 14, 
			0, 15, 
			0, 16};
  u8 * tree_out = malloc(sizeof(tree_template));
  memcpy(tree_out,tree_template,sizeof(tree_template));
  return (voxtree *) tree_out;
}


