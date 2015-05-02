// voxels are packed tightly into an array
// [leaf? color (ifleaf)[subleaf subleaf subleaf subleaf subleaf subleaf subleaf subleaf]

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"
#include "../bitguy/linmath.h"

#include "vox.h"

typedef struct {
  bool is_leaf;
  u8 color;
  u8 next;
}voxtree;

voxtree * tree_step_into(voxtree * vt){
  if(vt->is_leaf) return NULL;
  return (voxtree *) &vt->next;
}
voxtree * tree_step_over(voxtree * vt){
  if(vt->is_leaf) return &vt->next;
  
  for(int i = 0; i < 8; i++)
    vt = tree_step_over((voxtree *) &vt->next);
  return vt;
}

int tree_size(voxtree * tree){
  int subsize = 0;
  if(tree->is_leaf){
    subsize = tree_size((voxtree *) &tree->next);
  }
  return 1 + subsize;
}
// testing 
bool tree_size_test(){
  u8 tree[] = {1, 0,  
	       0, 255,  
	       0, 0,  
	       0, 0,  
	       0, 0,  
	       0, 255,  
	       0, 255, 
	       0, 255, 
	       0, 255};
  voxtree * vt = (voxtree *) tree;
  voxtree * end = tree_step_over(vt);
  printf("vt %i end %i %i\n", vt, end, end - vt);
  int treesize = tree_size((voxtree *) tree);
  logd("Tree size: %i\n", treesize);
  TEST_ASSERT(tree_size == 9);
  return true;
}

bool vox_test(){
  TEST(tree_size_test);
  return true;
}
