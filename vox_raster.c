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

/*void vox_raster_render(voxtree * vt, mat4 camera, u8 * image, u32 width, u32 height){

}

void vox_raster_isometric(voxtree * vt, u8 * image, u32 width, u32 height){
  memset(image, 0, width * height);
  vec3 position = vec3mk(1.0,1.0,1.0);
  }*/


bool vox_raster_test(){
  voxtree_ctx * ctx = voxtree_ctx_make(16);
  voxtree * vt = voxtree_ctx_tree(ctx);
  vt = tree_leaf_to_node(vt, ctx);

  return true;
}
