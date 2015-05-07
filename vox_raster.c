#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"
#include "../bitguy/linmath.h"

#include "color.h"
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

bool chkrng(int min, int mid, int max){
  return min <= mid && mid <= max;
}
	  
#include <stdio.h>
void vox_raster_isometric(voxtree * vt, vec3 cam_position, u8 * image, u32 width, u32 height){
  // the viewing vector is (-1, -1, -1)
  float sqrt2 = sqrtf(2);
  float sqrt3 = sqrtf(2);
  float sqrt23 = sqrt2 * sqrt3;
  float sqrt3d2 = powf(2,3.0/2.0);
  memset(image, 0, width * height);
  vec3 c = cam_position;
  vec3 viewv = vec3_normalize(vec3mk(-1.0, -1.0, -1.0));
  vec3 viewup = vec3mk(sqrt2/sqrt3, - 1.0/(sqrt2 * sqrt3), - 1.0/(sqrt2 * sqrt3));
  vec3 viewright = vec3mk(0, - 1.0 / sqrt2, 1.0 / sqrt2);
  vec2 screen_pos(vec3 p){
    vec3 pr = vec3_sub(p, cam_position);
    float d = vec3_mul_inner(pr, viewv);
    vec3 pc = vec3_sub(p, vec3_scale(viewv, d));
    pc = vec3_sub(pc,cam_position);

    float t = sqrt23 * (p.x + p.z - c.z - c.x) + sqrt3d2 * sqrt3 * (- p.y + c.y);
    t = -t / 6;
    float s = sqrt2 * (p.z - p.x - c.z + c.x)/2;
    return vec2mk((s * 0.5 + 0.5) * width,(t * 0.5 + 0.5) * height);
  }
	  
  void blit(voxtree * subtree, vec3 position, float size){
    u8 coloridx = *tree_color(subtree);
    if(tree_is_leaf(subtree)){
      // since viewing vector is (-1, -1, -1), y is up.
      // this means
      // the furthest corner of any rectangle is 0,0,0. the closest is 1,1,1
      // the diagonals are (1,0,1) and (0,1,0) for y
      // (1, 0, 0) and (0, 0, 1) for x
      // transforming these into view (pixel) space will give
      // extremities.
      //for(int i = 0; i < 8; i++){
      vec3 p = position;//idx_to_vec3(i);//
      vec2 sp = screen_pos(p);
      int x = (int)sp.x;
      int y = (int)sp.y;
      if(coloridx != 0 && chkrng(0,x,width - 1) && chkrng(0,y,height-1)){ 
	image[x + y * height] = coloridx;
	vec3_print(p);logd(" => ");vec2_print(sp);logd("\n");
	
      }
	//}
      
    }else{
      subtree = tree_step_into(subtree);
      for(i8 i = 0; i < 8; i++){
	vec3 offset = idx_to_vec3(i);
	blit(subtree,vec3_add(vec3_scale(offset, size), position), size * 0.5);
	subtree = tree_step_over(subtree);
      }
    }
  }
  blit(vt, vec3mk(0.0,0.0,0.0), 1.0);
  
}

void print_img(u8 * data, int width, int height){
  for(int i = 0; i < height; i++){
    for( int j = 0; j < width; j++){
      printf("%i",data[i * height + j]);
    }
    printf("\n");
  }
}

bool vox_raster_test(){
  voxtree_ctx * ctx = voxtree_ctx_make(16);
  voxtree * vt = voxtree_ctx_tree(ctx);
  
  voxtree * vt2 = vt2;
  for(int j = 0; j < 2;j++){
    vt2 = tree_leaf_to_node(vt2, ctx);
    
    vt2 = tree_step_into(vt2);
    for(int i = 1; i <= 7;i++){
      *tree_color(vt2) = i;
      vt2 = tree_skip(vt2);
    }
  }
  u32 w = 16, h = 16;
  u8 img[w * h];
  vec3 cam_position = vec3mk(1.0,1.0,1.0);
  logd("render iso..\n");
  vox_raster_isometric(vt, cam_position, img, w, h);
  print_img(img, w, h);
  return true;
}
