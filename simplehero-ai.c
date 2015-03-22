#include <stdint.h>
#include <stdbool.h>
#include "../bitguy/bitguy.h"
#include "microthreads.h"
// nanoseconds;
typedef u64 _timestamp_;
_timestamp_ tclock = 0;

u64 clock_ns(_timestamp_ time){
  return time;
}

_timestamp_ ns_clock(u64 nanoseconds){
  return nanoseconds;
}

u64 clock_us(_timestamp_ time){
  return clock_ns(time) / 1000;
}

_timestamp_ us_clock(u64 us){
  return ns_clock(us * 1000);
}

u64 clock_ms(_timestamp_ time){
  return clock_us(time) / 1000;
}

_timestamp_ ms_clock(u64 ms){
  return us_clock(ms * 1000);
}

u64 clock_sec(_timestamp_ time){
  return clock_ms(time) / 1000;
}

_timestamp_ sec_clock(u64 s){
  return ms_clock(s * 1000);
}

u64 clock_min(_timestamp_ time){
  return clock_sec(time) / 60;
}

_timestamp_ min_clock(u64 m){
  return sec_clock(m * 60);
}

int clock_hour(_timestamp_ time){
  return clock_min(time) / 60;
}

_timestamp_ hour_clock(u64 h){
  return min_clock(h * 60);
}

int clock_day(_timestamp_ time){
  return clock_hour(time) / 24;
}

_timestamp_ day_clock(u64 d){
  return hour_clock(d * 24);
}

int clock_week(_timestamp_ time){
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

_timestamp_ today(_timestamp_ timestamp){
  return day_clock(clock_day(timestamp));
}

void test(void * data){
  char * name = (char *) data;
  void status(char * status){
    //printf("%s: %s..\n",name,status);
  }
  while(true){
    _timestamp_ end_of_day = today(tclock) + hour_clock(17);
    _timestamp_ start_of_day = today(tclock) + hour_clock(8);
    while(start_of_day < tclock && tclock < end_of_day){
      status("working");
      ccyield();
    }
    status("end of day");
    ccyield();
  }
}
