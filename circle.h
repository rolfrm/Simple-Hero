//regures bitguy.h
typedef struct _circle{
  vec2 xy;
  float r;
}circle;

bool circle_sweep(circle a, circle b, vec2 dv, float * out_tenter, float * out_tleave);

typedef enum { LEAF, ADD, SUB, ISEC} circle_func;

typedef struct _circle_tree{
  circle_func func;
  //if func == LEAF refers to circle index
  // else tree index for left hand side
  union{
    int left;
    int circle;
  };
  //unused if func == LEAF
  int right; 
}circle_tree;


void draw_circle_system(circle * circles, int circ_count,
			circle_tree * ctree, int ctree_count, 
			u8 * out_image, int width, int height);


typedef struct{
  circle_tree * tree;
  circle * circles;
}circ_tree;




bool test_circle();
