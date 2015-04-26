#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "../bitguy/linmath.h"
#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"
#include "color.h"
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

collision_info circle_collision(circle * a, circle * b, vec2 * moveout){
  vec2 d = vec2_sub(a->xy, b->xy);
  float sql = vec2_sqlen(d);
  float tsize = a->r + b->r;
  float sqmove =  tsize * tsize - sql;
  if(sqmove > 0){
    float l = sqrtf(sql);
    float move = tsize - l;
    // if they are exactly ontop of eachother, we just take an arbitrary moveout.
    vec2 nd = l == 0.0 ? (vec2){.data = {tsize,0}} : vec2_scale(vec2_normalize(d), move);
    *moveout = nd;
    return COLLISION;
  }
  return NO_COLLISION;
}

vec2 vec2turn90(vec2 v){
  return (vec2){.data = {-v.y, v.x}};
}

bool circle_collision_points(circle * a, circle * b, vec2 *p1, vec2 * p2){
  float tsize = a->r + b->r;
  float tsize2 = tsize * tsize;
  vec2 diff = vec2_sub(b->xy, a->xy);
  float d2 = vec2_sqlen(diff);
  if(tsize2 < d2) return false;
  float d = sqrtf(d2);
  // this approximation is slightly wrong.. 
  float midd = a->r - (tsize - d) * (b->r)/(a->r + b->r);

  vec2 middiff = vec2_add(a->xy,vec2_scale(diff, midd / d));
  // calculate distance at x.
  // r ^ 2 = x ^2 + y ^ 2 -> x = midd. r= ra
  // sqrt(r ^ 2 - x ^ 2) = y
  float dy = sqrtf(a->r * a->r - midd * midd);
  vec2 diff_unit = vec2turn90(vec2_scale(diff,dy / d));
  *p1 = vec2_add(middiff,diff_unit);
  *p2 = vec2_sub(middiff,diff_unit);
  return true;
}

bool test_circle_collision_points(){
  circle a,b;
  a.xy = vec2mk(0.0,0.0);
  a.r = 1.0;
  b.xy = vec2mk(4.0,4.0);
  b.r = 5.0;
  vec2 p1,p2;
  bool does_collide = circle_collision_points(&a, &b, &p1, &p2);
  TEST_ASSERT(does_collide);
  printf("collision %i: ", does_collide);vec2_print(p1);vec2_print(p2);printf("\n");
  // Values found by testing
  // Order is not important.
  if(false == feq(p1.x,0.0, 0.01)){
    vec2 tmp = p1;
    p1 = p2;
    p2 = tmp;
  }

  TEST_ASSERT(feq(p1.x, 0.0, 0.01));
  TEST_ASSERT(feq(p1.y, 1.0, 0.01));
  TEST_ASSERT(feq(p2.x, 1.0, 0.01));
  TEST_ASSERT(feq(p2.y, 0.0, 0.01));

  float offx = 0.0;
  float offy = 0.0;

  a.xy.x += offx;
  a.xy.y += offy;
  b.xy.x += offx;
  b.xy.y += offy;
  vec2_print(p1);vec2_print(p2);printf("<p1p2 \n");
  does_collide = circle_collision_points(&b,&a, &p1, &p2);
  TEST_ASSERT(does_collide);

  if(false == feq(p1.x,1.0, 0.01)){
    vec2 tmp = p1;
    p1 = p2;
    p2 = tmp;
  }
  vec2_print(p1);vec2_print(p2);printf("<p1p2 \n");
  TEST_ASSERT(feq(p1.x, offx + 0.0, 0.01));
  TEST_ASSERT(feq(p1.y, offy + 1.0, 0.01));
  TEST_ASSERT(feq(p2.x, offx + 1.0, 0.01));
  TEST_ASSERT(feq(p2.y, offy + 0.0, 0.01));

  return true;
}


vec2 vec2min(vec2 a, vec2 b){
  return vec2_sqlen(a) > vec2_sqlen(b) ? b : a;
}

vec2 vec2max(vec2 a, vec2 b){
  return vec2_sqlen(a) < vec2_sqlen(b) ? b : a;
}

bool circ_tree_collision2(circ_tree * a, circ_tree * b, vec2 * moveout){
  
}

bool circ_tree_collision(circ_tree * a, circ_tree * b, vec2 * moveout){
  collision_info check_col(int aidx, int bidx, vec2 * moveout){
    circle_tree * ca = a->tree + aidx;
    circle_tree * cb = b->tree + bidx;
    if(ca->func == LEAF && cb->func == LEAF){
      return circle_collision(a->circles + ca->circle, b->circles + cb->circle, moveout);
    }
    if(ca->func == LEAF){
      vec2 left_moveout = {.data = {0.0,0.0}};
      vec2 right_moveout = {.data = {0.0,0.0}};
      collision_info leftcollides = check_col(aidx,cb->left, &left_moveout);
      collision_info rightcollides = check_col(aidx,cb->right, &right_moveout);
      switch(cb->func){
      case ADD:
	*moveout = vec2max(left_moveout,right_moveout);
	return leftcollides | rightcollides;
      case ISEC:
	*moveout = vec2min(left_moveout,right_moveout);
	return leftcollides & rightcollides;
      case SUB:	
	return leftcollides & !rightcollides;
      default:
	ERROR("unknown func");
	return false;
      }
    }
    if(cb->func == LEAF){
      vec2 left_moveout = {.data = {0.0,0.0}};
      vec2 right_moveout = {.data = {0.0,0.0}};
      collision_info leftcollides = check_col(ca->left, bidx, &left_moveout);
      collision_info rightcollides = check_col(ca->right, bidx, &right_moveout);
      switch(ca->func){
      case ADD:
	*moveout = vec2max(left_moveout, right_moveout);
	return leftcollides | rightcollides;
      case ISEC:
	*moveout = vec2min(left_moveout, right_moveout);
	return leftcollides & rightcollides;
      case SUB:
	return 
	  (leftcollides == 2 && rightcollides != 0)
	  || (leftcollides == 1 && (rightcollides == 0 || rightcollides == 1 || rightcollides == 3));
      default:
	ERROR("unknown func %i",cb->func);
	return false;
      }
    }

    // both are NODEs
    // graph needs to be compared 4 ways
    vec2 mll,mlr,mrl,mrr;
    bool cll = check_col(ca->left, cb->left, &mll);
    bool clr = check_col(ca->left, cb->right, &mlr);
    bool crl = check_col(ca->right, cb->left, &mrl);
    bool crr = check_col(ca->right, cb->right, &mrr);
    printf("%i %i %i %i\n",cll,clr,crl,crr);
    bool cl,cr;
    vec2 ml,mr;
    // calc partial solutions
    switch(cb->func){
    case ADD:
      cl = cll | clr;
      cr = crl | crr;
      ml = vec2max(mll,mlr);
      mr = vec2max(mrl,mrr);
      break;
    case SUB:
      cl = cll & !clr;
      cr = crl & !crr;
      break;
    case ISEC:
      cl = cll & clr;
      cr = crl & crr;
      ml = vec2min(mll,mlr);
      mr = vec2min(mrl,mrr);
      break;
    default:
      ERROR("UNKNOWN FuNC");
      return false;
    }
    switch(ca->func){
    case ADD:
      *moveout = vec2max(ml,mr);
      return cl | cr;
    case ISEC:
      *moveout = vec2min(ml,mr);
      return cl & cr;
    case SUB:
      return cl & !cr;
    default:
      ERROR("unknown func %i",cb->func);
      return false;
    }
    ERROR("Should never be reached");
    return false;
  }
  return check_col(0,0,moveout);
}

#include <stdio.h>
#include <string.h>
void draw_circle_system(circle * circles,
			circle_tree * ctree, 
			u8 * out_image, int width, int height){
  void blit(int offset, u8 * buffer){
    memset(buffer,0,width * height);
    circle_tree nd = *(ctree + offset);
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
      blit(nd.left, buffer);
      blit(nd.right, buf2);
      u128 * buf = (u128 *) buf2;
      u128 * end = buf + width * height / sizeof(u128);
      u128 * bufo = (u128 *) buffer;
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
      default:
	ERROR("not supposed to happen");
      }
      free(buf2);
    }
  }
  blit(0,out_image);
}

int circle_tree_size(circle_tree * tree){
 
  int check_size(int offset){
    circle_tree * tr = tree + offset;
    if(tr->func == LEAF)
      return 1;
    return 1 + check_size(tr->left) 
      + check_size(tr->right);  
  }
  return check_size(0);
}

int circle_tree_max_leaf(circle_tree * tree){
  int check_size(int offset){
    circle_tree * tr = tree + offset;
    if(tr->func == LEAF){
      return tr->circle;
    }
    int left = check_size(tr->left);
    int right = check_size(tr->right);
    return MAX(left,right);
  }
  return check_size(0);
}

circ_tree * sub_tree(circle_func fcn, circ_tree * a, circ_tree * b){

  int leftsize = circle_tree_size(a->tree);
  int leftleafs = circle_tree_max_leaf(a->tree) + 1;
  int rightsize = circle_tree_size(b->tree);
  int rightleafs = circle_tree_max_leaf(b->tree) + 1;

  int circlesize = sizeof(circle) * (leftleafs  + rightleafs);
  int treesize =  sizeof(circle_tree) * (1 + leftsize + rightsize);
  int totsize = sizeof(circ_tree) + circlesize + treesize;
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
    if(ct2->func == LEAF)
      ct2->circle += leftleafs;
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

circle_tree circ_leaf(int circ_idx){
  circle_tree out;
  out.func = LEAF;
  out.circle = circ_idx;
  return out;
}
circle_tree circ_func(circle_func func, int left_tree, int right_tree){
  circle_tree out;
  out.func = func;
  out.left =left_tree;
  out.right = right_tree;
  return out;
}

// testing //

bool test_circle_sweep(){
  circle a = {{.data = {0.0,0.0}},1.0};
  circle b = {{.data = {4.0,4.0}},1.0};
  vec2 dv = {.data = {-1.0,-1.0}};
  float enter,leave;
  bool collides = circle_sweep(a,b,dv,&enter,&leave);
  
  if(collides && enter > 0){
    b.xy = vec2_add(b.xy,vec2_scale(dv,enter));
  }
  bool collides2 = circle_sweep(a,b,dv,&enter,&leave);
  TEST_ASSERT(collides);
  TEST_ASSERT(collides2);
  TEST_ASSERT(feq(enter,0.0,0.0001));
  return true;
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
  circle circles[] = {{{.data = {0/2,0}},10}
		      ,{{.data = {0/2,0+ 4} },5}
		      ,{{.data = {0/2,0}},3}};
  circle_tree tree[] = {circ_func(SUB,1,2),circ_leaf(0),circ_func(SUB,3,4),circ_leaf(1),circ_leaf(2)};
  circle circles2[] = {{{.data = {0/2,0}},10}
		       ,{{.data = {0/2,0 + 4}},5}
		       ,{{.data = {0/2,0}},3}};
  circle_tree tree2[] = {circ_func(SUB,1,2),circ_leaf(0),circ_func(SUB,3,4),circ_leaf(1),circ_leaf(2)};
  mat3 m = mat3_2d_rotation(2.14);
  circle_tform(circles2,array_count(circles2), m);
  circle_tform(circles,array_count(circles), m);
  circle_tform(circles,array_count(circles), mat3_2d_translation(20,20));
  circle_move(circles2,array_count(circles2), (vec2){.data = {50,50}});
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
  circle circles[] = {{.xy = {.data ={w/2,w/2}}, .r = 3}
		      ,{.xy = {.data = {w/2,w/2 }}, .r = 3}
		      ,{.xy = {.data = {w/2,w/2 }}, .r = 3}};
  circle_tree tree[] = {circ_func(ISEC,1,2), circ_leaf(0),circ_func(ADD,3,4), circ_leaf(1), circ_leaf(2)};
  int size = circle_tree_size(tree);
  int leafs = circle_tree_max_leaf(tree);
  printf("size: %i\n",size);
  printf("leaf: %i\n",leafs);
  u8 * image = malloc(w * w);
  draw_circle_system(circles,tree,image,w,w);
  free(image);
}

bool circle_bench(){
  u64 start = timestamp();
  circle_bench_test();
  u64 stop = timestamp();
  printf("Dt: %f s\n",1e-6 * (stop - start));
  return true;
}

bool test_circle(){
  TEST(test_circle_collision_points);
  TEST(circle_bench);
  TEST(test_circle_sweep);
  TEST(test_draw_circle_system);
  return true;
}


