#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include "../bitguy/linmath.h"
#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"
#include "circle.h"
// Two circles a,b. dv is the speed of b relative to a. 
// Out_tenter and out_tleave is time when a and b intersects, 
// if they do at all if not these values will not be set..
// returns true if a and b intersects at some time.
bool circle_sweep(circle a, circle b, vec2 dv, float * out_tenter, float * out_tleave){
  // ** Algorithm ** //   
  // circle of size a.r + b.r at o
  // line:  dv * t - o = 0
  // line pt distance from zero:
  // = d(t) = |dv * t + borig|
  // solve d(t) < totr. 
  // easier: d(t)^2 < totr^2
  // d(t)^2 = (dv * t + borig)^2 = (dv * t)^2 + o^2 + 2 * o * dv * t
  //        = t^2 * (dv.x*dv.x + dv.y * dv.y) 
  //              + 2 * t * (p.x * dv.x + p.y * dv.y) + (o.x * o.x + o.y * o.y) 
  //b = 2 * dv * o
  //a = dv^2
  //c = o^2 - totr  
  // solve  = totr^2
  
  float totr = a.r + b.r;
  vec2 o = vec2_sub(b.xy, a.xy);
  
  // Solution for quadratic function.
  float sb = 2.0 * vec2_mul_inner(dv,o);
  float sa = vec2_mul_inner(dv, dv);
  float sc = vec2_mul_inner(o,o) - totr * totr;
  float desc = sb * sb - 4 * sa * sc;
  // no real roots -> no collision, ever.
  if(desc < 0) return false; 
  
  float b4ac = sqrt(desc);
  float r1 = (-sb + b4ac) / (2.0 * sa);
  float r2 = (-sb - b4ac) / (2.0 * sa);
  if(r2 < r1){
    float t = r1;
    r1 = r2;
    r2 = t;
  }
  *out_tenter = r1;
  *out_tleave = r2;
  return true;
}


void draw_circle_system(circle * circles, int circ_count,
			circle_tree * ctree, int ctree_count, 
			u8 * out_image, int width, int height){
  void blit(int ctree_index, u8 * buffer){
    circle_tree nd = ctree[ctree_index];
    if(nd.func == LEAF){
      circle circ = circles[nd.circle];
      int x = (int)circ.xy.x;
      int y = (int)circ.xy.y;
      int r2 = (int)(circ.r * circ.r);
      for(int j = 0; j < height; j++)
	for(int i = 0; i < width; i++){
	  int dx = x - i;
	  int dy = y - j;
	  int d = (dx * dx) + (dy * dy);
	  buffer[i + j * width] = d < r2 ? 255 : 0;
	}
      
    }else{
      u8 * buf2 = malloc(width * height);
      blit(nd.left, buffer);
      blit(nd.right, buf2);
      switch(nd.func){
      case ADD:
	for(int j = 0; j < height; j++)
	  for(int i = 0; i < width; i++){
	    int idx = i + j * width;
	    buffer[idx] = buffer[idx] | buf2[idx];
	  }
	break;
      case SUB:
	for(int j = 0; j < height; j++)
	  for(int i = 0; i < width; i++){
	    int idx = i + j * width;
	    buffer[idx] = buffer[idx] & ~buf2[idx];
	  }
	break;
      case ISEC:
	for(int j = 0; j < height; j++)
	  for(int i = 0; i < width; i++){
	    int idx = i + j * width;
	    buffer[idx] = buffer[idx] & buf2[idx];
	  }
	break;
      }
      free(buf2);
    }
  }
  blit(0,out_image);
}

bool test_circle_sweep(){
  circle a = {{0.0,0.0},1.0};
  circle b = {{4.0,4.0},1.0};
  vec2 dv = {-1.0,-1.0};
  float enter,leave;
  bool collides = circle_sweep(a,b,dv,&enter,&leave);
  
  if(collides && enter > 0){
    b.xy = vec2_add(b.xy,vec2_scale(dv,enter));
  }
  bool collides2 = circle_sweep(a,b,dv,&enter,&leave);
  return collides && collides2 && enter == 0.0;
}
#include <stdio.h>
void print_image(u8 * img, int w, int h){
  for(int j = 0; j < h; j++){
    for(int i = 0; i < w; i++)
      printf( img[i + j *h]? "#" : " ");
    printf("\n");
  }
}

//(- (circ 16 16 10) (- (circ 16 20 5) (circ 16 16 3)))

bool test_draw_circle_system(){

  int w = 32;
  circle circles[] = {{{16,16},10}
		      ,{{16,16 + 4},5}
		      ,{{16,16},3}};
  circle_tree tree[] = {{SUB,1,2},{LEAF,0,0},{SUB,3,4},{LEAF,1,0},{LEAF,2,0}};
  u8 image[w*w];
  draw_circle_system(circles,array_count(circles),tree,array_count(tree),image,w,w);
  print_image(image,w,w);
  return true;
}

void circle_bench_test(){
  int w = 512 * 8;
  circle circles[] = {{{160,160},100}
		      ,{{160,160 + 100},80}
		      ,{{160,160 + 20},30}};
  circle_tree tree[] = {{SUB,1,2},{LEAF,0,0},{ADD,3,4},{LEAF,1,0},{LEAF,2,0}};
  u8 * image = malloc(w * w);
  draw_circle_system(circles,array_count(circles),tree,array_count(tree),image,w,w);
  free(image);
}

void circle_bench(){
  u64 start = timestamp();
  circle_bench_test();
  u64 stop = timestamp();
  printf("Dt: %f s\n",1e-6 * (stop - start));
}

bool test_circle(){
  circle_bench();
  return test_circle_sweep() 
    & test_draw_circle_system();
}

