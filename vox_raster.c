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
*/

vec3 idx_to_vec3(u8 idx){
  static vec3 lut[8] = {{.data = {0.0,0.0,0.0}},
			{.data = {1.0,0.0,0.0}},
			{.data = {0.0,1.0,0.0}},
			{.data = {1.0,1.0,0.0}},
			{.data = {0.0,0.0,1.0}},
			{.data = {1.0,0.0,1.0}},
			{.data = {0.0,1.0,1.0}},
			{.data = {1.0,1.0,1.0}},};
  return lut[idx];
}

#include <stdio.h>
void vox_raster_isometric(voxtree * vt, vec3 cam_position, u8 * image, u32 width, u32 height){
  // the viewing vector is (-1, -1, -1)
  static float sqrt2 = sqrtf(2);
  static float sqrt3 = sqrtf(2);
  memset(image, 0, width * height);
  vec3 viewv = vec3_normalize(vec3mk(-1.0, -1.0, -1.0));
  vec2 screen_pos(vec3 p){
    vec3 pr = vec3_sub(p, cam_position);
    float d = vec3_mul_inner(pr, viewv);
    vec3 pc = vec3_sub(p, vec3_scale(viewv, d));
    pc = vec3_sub(pc,cam_position);
    float t = pc.x / sqrt2;
    float s = pc.y / -sqrt2;
    return vec2mk(t,s);
  }
	  
  void blit(voxtree * subtree, vec3 position, float size){
    if(tree_is_leaf(subtree)){
      // since viewing vector is (-1, -1, -1), y is up.
      // this means
      // the furthest corner of any rectangle is 0,0,0. the closest is 1,1,1
      // the diagonals are (1,0,1) and (0,1,0) for y
      // (1, 0, 0) and (0, 0, 1) for x
      // transforming these into view (pixel) space will give
      // extremities.
      for(int i = 0; i < 8; i++){
	vec3 p = vec3mk(1.0,0.0,1.0);//idx_to_vec3(i);//
	vec2 sp = screen_pos(p);
	vec2_print(sp);logd("\n");
      }
      
    }else{
      for(i8 i = 7; i >= 0; i--){
	vec3 offset = idx_to_vec3(i);
	
      }
    }
  }
  blit(vt, vec3mk(1.0,1.0,1.0), 1.0);
  
}


bool vox_raster_test(){
  voxtree_ctx * ctx = voxtree_ctx_make(16);
  voxtree * vt = voxtree_ctx_tree(ctx);
  //vt = tree_leaf_to_node(vt, ctx);
  voxtree * vt2 = vt;//tree_step_into(vt);
  *tree_color(vt2) = 1;
  //vt2 = tree_skip(vt2);
  //*tree_color(vt2) = 1;
  u32 w = 32, h = 32;
  u8 img[w * h];
  vec3 cam_position = vec3mk(1.0,1.0,1.0);
  logd("render iso..\n");
  vox_raster_isometric(vt, cam_position, img, w, h);
  return true;
}
