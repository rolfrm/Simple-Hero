// voxels are packed tightly into an array
// [leaf? color (ifleaf)[subleaf subleaf subleaf subleaf subleaf subleaf subleaf subleaf]
#include <iron/full.h>

#include <stdlib.h>

#include "vox.h"
#include "vox_internal.h"

voxtree * tree_step_into(voxtree * vt){
  if(tree_is_leaf(vt)) return NULL;
  return tree_next(vt);
}
voxtree * tree_step_over(voxtree * vt){
  if(tree_is_leaf(vt)) return tree_next(vt);
  vt =  tree_next(vt);
  for(int i = 0; i < 8; i++)
    vt = tree_step_over(vt);
  return vt;
}

int tree_size(voxtree * vt){
  int subsize = 0;
  if(tree_is_node(vt)){
    vt = tree_step_into(vt);
    for(int i = 0; i < 8; i++){
      subsize += tree_size(vt);
      vt = tree_step_over(vt);
    }
  }
  return 1 + subsize;
}

// testing 
bool tree_size_test(){
  voxtree * vt = tree_make_test();
  int treesize = tree_size(vt);
  logd("Tree size: %i\n", treesize);
  TEST_ASSERT(treesize == 17);
  free(vt);
  return true;
}

void print_tree(voxtree_ctx * ctx){
u8 * t = (u8 *) voxtree_ctx_tree(ctx);
  for(int i = 0 ;i < voxtree_ctx_size(ctx); i += 2){
    int i2 = i /2;
    u8 a = t[i];
    u8 b = t[i+1];
    logd("%i: %i %i\n", i2, a, b);
  }
}

bool tree_test(){
  voxtree * vt = tree_make_test();
  voxtree * vt2 = tree_make_test();
  voxtree * node = vt;
  while(tree_is_node(node))
    node = tree_step_into(node);
  *tree_color(node) = 128;
  TEST_ASSERT(tree_is_leaf(node));
  voxtree_ctx * ctx = voxtree_ctx_make_from_ptr(vt, 17 * 2);
  node = tree_leaf_to_node(node, ctx);
  logd("new capacity: %i %i\n",voxtree_ctx_capacity(ctx), voxtree_ctx_size(ctx));
  TEST_ASSERT(voxtree_ctx_capacity(ctx)>= voxtree_ctx_size(ctx));
  int treesize = tree_size(voxtree_ctx_tree(ctx));
  
  // 8 new leaves has been added
  logd("treesize : %i %i\n", treesize, voxtree_ctx_size(ctx));
  TEST_ASSERT(treesize == 17 + 8 && treesize == voxtree_ctx_size(ctx) / 2);
  for(int i = 0; i < 10;i++){
    while(tree_is_node(node))
      node = tree_step_into(node);
    node = tree_insert_node(node, vt2, ctx);
    treesize = tree_size(voxtree_ctx_tree(ctx));
    
    logd("treesize : %i %i\n", treesize, voxtree_ctx_size(ctx));
    TEST_ASSERT(treesize == voxtree_ctx_size(ctx) / 2);
  }
  return true;
}

bool vox_test(){
  TEST(tree_size_test);
  TEST(tree_test);
  return true;
}
