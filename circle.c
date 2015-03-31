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
  // solve  = quadratic eq.
  // ** ** //
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

#include <stdio.h>
#include <string.h>
void draw_circle_system(circle * circles,
			circle_tree * ctree, 
			u8 * out_image, int width, int height){
  
  void blit(circle_tree * ctree, u8 * buffer){
    memset(buffer,0,width * height);
    circle_tree nd = *ctree;
    if(nd.func == LEAF){
      circle circ = circles[nd.circle];
      int x = (int)circ.xy.x;
      int y = (int)circ.xy.y;
      
      void paint_pt(int _x, int _y){
	int dx = x - _x;
	int dy = y - _y;
	float d = MAX(0.0, sqrt(dx * dx + dy * dy) - circ.r);
	
	if(d <= 1.0 && d >= 0)
	  buffer[_x + _y * width] = 255 * (1.0 - d);
      }
      int ystart = MAX(0, y - circ.r);
      int yend = MIN(height-1, y + circ.r + 1);
      float r2 = circ.r * circ.r;
      for(int j = ystart; j < yend; j++){
	int dy = y - j;
	float diff = sqrt(MAX(0, (circ.r + 1) * (circ.r + 1) - dy * dy));
	float diff_small = sqrt(MAX(0, (circ.r - 1) * (circ.r - 1) - dy * dy));
	int xstartaa = MAX(0, x - diff);
	int xstart = MAX(0,x - diff_small);
	int xendaa = MIN(width-1, x + diff + 1);
	int xend = MIN(width-1, x + diff_small + 1);
	if(xstart > xend) continue;
	for(int i = xstartaa ; i < xstart; i++)
	  paint_pt(i,j);
	for(int i = xend ; i < xendaa ; i++)
	  paint_pt(i,j);
	memset(buffer + xstart + j * width,255, xend - xstart);
      }	  
    }else{
      u8 * buf2 = malloc(width * height);
      blit(ctree + nd.left, buffer);
      blit(ctree + nd.right, buf2);
      int size = width * height;
      u128 * buf = buf2;
      u128 * end = buf + width * height / sizeof(u128);
      u128 * bufo = buffer;
      switch(nd.func){
      case ADD:
	for(;buf < end;bufo++, buf++)
	  *bufo |= *buf;
	break;
      case SUB:
	for(;buf < end;bufo++, buf++)
	  *bufo &= ~(*buf);
	break;
      case ISEC:
	for(;buf < end;bufo++, buf++)
	  *bufo &= *buf;
	break;
      }
      free(buf2);
    }
  }
  blit(ctree,out_image);
}

int circle_tree_size(circle_tree * tr){
  if(tr->func == LEAF)
    return 1;
  return 1 + circle_tree_size(tr + tr->left) 
    + circle_tree_size(tr + tr->right);  
}

int circle_tree_max_leaf(circle_tree * tr){
  if(tr->func == LEAF){
    return tr->circle;
  }
  return MAX(tr->left,tr->right);
}

circ_tree * sub_tree(circle_func fcn, circ_tree * a, circ_tree * b){
  int leftsize = circle_tree_size(a->tree);
  int leftleafs = circle_tree_max_leaf(a->tree) + 1;
  int rightsize = circle_tree_size(b->tree);
  int rightleafs = circle_tree_max_leaf(b->tree) + 1;
  
  int circlesize = sizeof(circle) * (leftleafs  + rightleafs);
  int treesize =  sizeof(circle_tree) * (1 + leftsize + rightsize);
  int totsize = sizeof(circ_tree) + circlesize + treesize;
    
  printf("%i %i %i\n",leftsize,rightsize,totsize);
  void * memblock = malloc(totsize);
  
  circ_tree * c = memblock;
  circle * circs =  memblock + sizeof(circ_tree);
  circle_tree * tree = memblock + sizeof(circ_tree) + circlesize;
  c->tree =tree;
  c->circles =circs;
  memcpy(tree + 1 , a->tree, sizeof(circle_tree) * leftsize);
  memcpy(tree + 1 + leftsize, b->tree, sizeof(circle_tree) * rightsize);
  memcpy(circs, a->circles, leftleafs * sizeof(circle));
  memcpy(circs + leftleafs, b->circles, rightleafs * sizeof(circle));
  for(int i = 0 ; i <rightsize;i++){
    circle_tree * ct2 = tree + i + leftsize + 1;
    if(ct2->func == LEAF){
      ct2->circle += leftleafs;
      printf("LEAF:\n");
    }
  }
  tree->func = fcn;
  tree->left = 1;
  tree->right = leftsize + 1;
  return c;
} 
	  
void circle_tform(circle * c, int count, mat3 t){
  for(int i = 0 ; i < count; i++)
    c[i].xy = mat3_mul_vec2(t, c[i].xy);
}


void circle_move(circle * c, int count, vec2 xy){
  for(int i = 0; i < count; i++){
    c[i].xy.x += xy.x;
    c[i].xy.y += xy.y;
  }
}

// testing //

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

void print_image(u8 * img, int w, int h){
  for(int j = 0; j < h; j++){
    for(int i = 0; i < w; i++)
      printf( img[i + j *h]? "#" : " ");
    printf("\n");
  }
}

bool test_draw_circle_system(){
  int w = 32;
  circle circles[] = {{{0/2,0},10}
		      ,{{0/2,0 + 4},5}
		      ,{{0/2,0},3}};
  circle_tree tree[] = {{SUB,1,2},{LEAF,0,0},{SUB,1,2},{LEAF,1,0},{LEAF,2,0}};
  circle circles2[] = {{{0/2,0},10}
		      ,{{0/2,0 + 4},5}
		      ,{{0/2,0},3}};
  circle_tree tree2[] = {{SUB,1,2},{LEAF,0,0},{SUB,1,2},{LEAF,1,0},{LEAF,2,0}};

  mat3 m = mat3_2d_rotation(2.14);
  circle_tform(circles2,array_count(circles2), m);
  circle_tform(circles,array_count(circles), m);
  circle_tform(circles,array_count(circles), mat3_2d_translation(20,20));
  circle_move(circles2,array_count(circles2), (vec2){50,50});
  circ_tree ct = {tree, circles};
  circ_tree ct2 = {tree2, circles2};
  ct2.circles = circles2;
  ct2.tree = tree2;

  circ_tree * nct = sub_tree(ADD,&ct,&ct2);

  u8 image[w*w];
  //draw_circle_system(circles,tree,image,w,w);
  draw_circle_system(circles,tree,image,w,w);
  free(nct);
  print_image(image,w,w);
  return true;
}

void circle_bench_test(){
  int w = 32;
  circle circles[] = {{{w/2,w/2},3}
		      ,{{w/2,w/2 },3}
		      ,{{w/2,w/2 },3}};
  circle_tree tree[] = {{ISEC,1,2},{LEAF,0,0},{ADD,1,2},{LEAF,1,0},{LEAF,2,0}};
  int size = circle_tree_size(tree);
  int leafs = circle_tree_max_leaf(tree);
  printf("size: %i\n",size);
  printf("leaf: %i\n",leafs);
  u8 * image = malloc(w * w);
  draw_circle_system(circles,tree,image,w,w);
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


