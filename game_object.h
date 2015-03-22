typedef int game_obj_header;

#define PLAYER 1
#define GRASS 2
#define CAMPFIRE 3
typedef struct{
  game_obj_header header;
  int x;
  int y;
}player;

typedef struct{
  game_obj_header header;
  int x;
  int y;
}grass_leaf;

typedef struct{
  game_obj_header header;
  int x, y;
  int fuel;
}campfire;

typedef struct{
  union{
    game_obj_header header;
    grass_leaf grass_leaf;
    player player;
    campfire campfire;
  };
}game_obj;

