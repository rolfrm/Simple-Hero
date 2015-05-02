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


typedef struct {
  bool is_leaf;
  u8 color;
  u8 next;
}voxtree;

int tree_size(voxtree * tree){
  int subsize = 0;
  if(tree->is_leaf){
    subsize = tree_size((voxtree *) &tree->next);
  }
  return 1 + subsize;
}

bool tree_size_test(){
  u8 tree = [1, 0,  
	     0, 255,  
	     0, 0,  
	     0, 0,  
	     0, 0,  
	     0, 255,  
	     0, 255, 
	     0, 255, 
	     0, 255];
  TEST_ASSERT(tree_size((voxtree *) tree) == 5);
  return true;
}

bool voxtree_test(){
  TEST(tree_size_test);
  return true;
}
