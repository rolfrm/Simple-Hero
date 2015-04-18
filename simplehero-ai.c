#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "../bitguy/bitguy.h"
#include "../bitguy/linmath.h"

#include "microthreads.h"
#include "color.h"
#include "circle.h"
#include "game_object.h"
#include "game_state.h"

// nanoseconds;
typedef u64 timestamp;
timestamp tclock = 0;

u64 clock_ns(timestamp time){
  return time;
}

timestamp ns_clock(u64 nanoseconds){
  return nanoseconds;
}

u64 clock_us(timestamp time){
  return clock_ns(time) / 1000;
}

timestamp us_clock(u64 us){
  return ns_clock(us * 1000);
}

u64 clock_ms(timestamp time){
  return clock_us(time) / 1000;
}

timestamp ms_clock(u64 ms){
  return us_clock(ms * 1000);
}

u64 clock_sec(timestamp time){
  return clock_ms(time) / 1000;
}

timestamp sec_clock(u64 s){
  return ms_clock(s * 1000);
}

u64 clock_min(timestamp time){
  return clock_sec(time) / 60;
}

timestamp min_clock(u64 m){
  return sec_clock(m * 60);
}

int clock_hour(timestamp time){
  return clock_min(time) / 60;
}

timestamp hour_clock(u64 h){
  return min_clock(h * 60);
}

int clock_day(timestamp time){
  return clock_hour(time) / 24;
}

timestamp day_clock(u64 d){
  return hour_clock(d * 24);
}

int clock_week(timestamp time){
  return clock_day(time) / 7;
}

int week_clock(u64 week){
  return day_clock(week * 7);
}
/*void wait_until(int day_time){
  int target = day() + day_time + (time_of_day() > day_time ? 24 : 0);
  while(tclock < target) yield();
}

bool wait_until2(int day_time){
  int target;
  if(time_of_day() > day_time){
    target = day() + 24 + day_time;
  }else{
    target = day() + day_time;
  }
  yield();
  return tclock < target;
  }*/

timestamp today(timestamp timestamp){
  return day_clock(clock_day(timestamp));
}

typedef struct {
  game_obj * thisobj;
  game_state * gamestate;
}gamedata;
#include <stdio.h>
void campfire_ai(gamedata * gd){
  while(true){
    while(gd->thisobj->campfire.fuel > 0){
      printf("burn.. %i\n",gd->thisobj->campfire.fuel);
      gd->thisobj->campfire.fuel -= 10;
      ccyield(); 
    }
    printf("the fire died..\n");
    while(gd->thisobj->campfire.fuel <= 0){
      //printf("no more fuel..\n");
      ccyield();
    }
  }
}

void player_ai(gamedata * gd){
  float x = gd->thisobj->player.x;
  float y = gd->thisobj->player.y;
  while(true){
    
    while(gd->thisobj->player.y-- - y > 0) ccyield();
    while(gd->thisobj->player.y++ - y < 10) ccyield();
    while(gd->thisobj->grass_leaf.x-- - x > 0) ccyield();
    while(gd->thisobj->grass_leaf.y++ - y< 10) ccyield();
    while(gd->thisobj->grass_leaf.x++ - x< 10) ccyield();
    while(gd->thisobj->grass_leaf.y-- - y> 0) ccyield();
  }
}
#include <stdlib.h>
void grass_ai(gamedata * gd){
  float x = gd->thisobj->player.x + (rand() % 200);
  float y = gd->thisobj->player.y + (rand() % 200);
  while(true){
    printf("Im just grass..\n");
    while(gd->thisobj->grass_leaf.x-- - x> 0 && (rand() % 15)) ccyield();
    while(gd->thisobj->grass_leaf.y++ - y< 41  && (rand() % 15)) ccyield();
    while(gd->thisobj->grass_leaf.x++ - x< 43  && (rand() % 15)) ccyield();
    while(gd->thisobj->grass_leaf.y-- - y> 0  && (rand() % 15)) ccyield();
  }
}

gamedata gamedatas[100] = {{NULL,NULL}};

void run_ai( ccdispatch * dispatcher, game_state * gs){
  for(int i = 0; i < gs->item_count;i++){
    if(gamedatas[i].thisobj == NULL)
      switch(gs->items[i].header){
      case CAMPFIRE:
	gamedatas[i].thisobj = gs->items + i;
	gamedatas[i].gamestate = gs;
	ccthread(dispatcher, campfire_ai,gamedatas + i);
	break;
      case PLAYER:
	gamedatas[i].thisobj = gs->items + i;
	gamedatas[i].gamestate = gs;
	ccthread(dispatcher, player_ai, gamedatas + i);
	break;
      case GRASS:
	gamedatas[i].thisobj = gs->items + i;
	gamedatas[i].gamestate = gs;
	ccthread(dispatcher, grass_ai, gamedatas + i);
	break;
      default:
	break;
      }
  }
  ccstep(dispatcher);
}
