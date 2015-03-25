#include <stdbool.h>
#include "../bitguy/linmath.h"

typedef struct _circle{
  vec2 xy;
  float r;
}circle;

float circle_dist(circle a, circle b){
  return vec2_len(vec2_sub(a.xy,b.xy)) - a.r - b.r;
}

//circle: (x - x1)^2 + (y - y1)^2 - r = 0 
//intersection: (x - x1)^2 +( y - y1)^2 - r1 = (x - x2)^2 + (y - y2)^2 - r2

bool circle_collision(circle a, circle b){
  
  
}


#include <stdio.h>
#include <math.h>
bool circle_sweep(circle a, circle b, vec2 dv, float * out_tenter, float * out_tleave){
  
  // circle of size a.r + b.r at origo
  // line:  dv * t - borig = 0
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
  // figure out the collision times.
  // (x - x1)^2 +(y - y1)^2 - r1 = 
  //{ax,ay} * {bx,by} = ax * bx + ay * by
  float totr = a.r + b.r;
  vec2 o = vec2_sub(b.xy, a.xy);
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
  
  printf("%f %f %f -> %f %f\n", sa, desc, b4ac, r1, r2);
  
  return false;
}

void test_circle_sweep(){
  circle a = {{0.0,0.0},1.0};
  circle b = {{4.0,0.0},1.0};
  vec2 dv = {-0.5,0.0};
  float enter,leave;
  circle_sweep(a,b,dv,&enter,&leave);
}
