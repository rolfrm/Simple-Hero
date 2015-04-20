//regures bitguy.h. linmath.h
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

circle_tree circ_leaf(int circ_idx);
circle_tree circ_func(circle_func, int left_tree, int right_tree);


void draw_circle_system(circle * circles, circle_tree * ctree,
			u8 * out_image, int width, int height);

typedef struct{
  circle_tree * tree;
  circle * circles;
}circ_tree;

circ_tree * sub_tree(circle_func fcn, circ_tree * a, circ_tree * b);

void circle_move(circle * c, int count, vec2 offset);
void circle_tform(circle * c, int count, mat3 t);

int circle_tree_size(circle_tree * tr);
int circle_tree_max_leaf(circle_tree * tr);

typedef enum{
  NO_COLLISION = 0,
  COLLISION = 1,
  A_HALF_INSIDE_B = 2,
  A_INSIDE_B = 3,
  B_HALF_INSIDE_A = 4,
  B_INSIDE_A = 5
}collision_info;

// returns true in case of collision
collision_info circle_collision(circle * a, circle * b, vec2 * moveout);
bool circ_tree_collision(circ_tree * a, circ_tree * b, vec2 * moveout);
bool test_circle();

