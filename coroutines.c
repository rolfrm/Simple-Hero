#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>

#include "coroutines.h"
#include "microthreads.h"
struct _costack{
  pthread_attr_t attr;
  void * stack;
  size_t stacksize;
  jmp_buf buf;
};

int pthread_attr_getstack (const pthread_attr_t *__restrict __attr,
				  void **__restrict __stackaddr,
			   size_t *__restrict __stacksize);
int pthread_attr_setstack (pthread_attr_t *__attr, void *__stackaddr,
			   size_t __stacksize);

static costack ** stacks = NULL;
static size_t stacks_count = 0;

static size_t stk_to_idx(costack * stk){
  for(size_t i = 0 ; i < stacks_count; i++){
    if(stk == stacks[i]) return i;
  }
  return -1;
}

static costack *idx_to_stk(size_t idx){
  return stacks[idx];
}

costack * costack_create(pthread_attr_t * attr){
  costack * cc = malloc(sizeof(costack));
  memset(cc,0,sizeof(costack));
  cc->attr = *attr;
  int nstk = stk_to_idx(NULL);
  if(nstk == -1){
    size_t ncnt = stacks_count == 0 ? 16 : (stacks_count * 2);
    stacks = realloc(stacks,ncnt * sizeof(costack *));
    for(size_t i = stacks_count; i < ncnt; i++)
      stacks[i] = NULL;
    stacks_count = ncnt;
    nstk = stk_to_idx(NULL);
  }
  stacks[nstk] = cc;
  return cc;
}

void checkbuffer(void * data, size_t size){
  volatile char * d = (char *) data;
  char b = 0;
  for(size_t i =0 ; i < size; i++){
    b += d[i];
  }
}

bool costack_save( costack * cc){
  int stk = setjmp(((costack *)cc)->buf) - 1;
  if(stk == -1){
    size_t stacksize =0;
    void * ptr = NULL;
    pthread_attr_getstack(&((costack *)cc)->attr, &ptr, &stacksize);
    if(cc->stack == NULL) 
      cc->stack = malloc(stacksize);
    cc->stacksize = stacksize;
    void * stack = cc->stack;
    memcpy(stack, ptr, stacksize);
    return false;
  }else{
    cc = idx_to_stk(stk);
    size_t stacksize =0;
    void * ptr = NULL;
    pthread_attr_getstack(&cc->attr, &ptr, &stacksize);
    memcpy(ptr,cc->stack, stacksize);
    return true;
  }
}

void costack_resume( costack * stk){
  longjmp(stk->buf,stk_to_idx(stk) + 1);
}

void costack_delete( costack * stk)
{
  stacks[stk_to_idx(stk)] = NULL;
  free(stk->stack);
}

// CC //
#define CC_INITIALIZED 0
#define CC_YEILDED 1
#define CC_PRE_YEILD 2
#define CC_RESUMED 3
#define CC_ENDED 4
int yeild_states[100] = {CC_INITIALIZED};

costack ** ccstack(){
  static __thread costack * _stk = NULL;
  return &_stk;
}

costack ** mstack(){
  static __thread costack *_stk = NULL;
  return &_stk;
}

void set_current_stack(costack * stk){
  *(ccstack()) = stk;
}

void yield(){
  costack * stk = *ccstack();
  
  yeild_states[stk_to_idx(stk) - 1] = CC_PRE_YEILD;
  costack_save(stk);
  if(yeild_states[stk_to_idx(stk) - 1] == CC_PRE_YEILD ){
    yeild_states[stk_to_idx(stk) - 1] = CC_YEILDED; 
    costack_resume(*(mstack()));
  }else{
    yeild_states[stk_to_idx(stk) - 1] = CC_RESUMED; 
  }
}

struct _ccdispatch{
  pthread_attr_t attr;
  pthread_t thread;
  costack ** stks;
  u32 stks_count;
  costack * main_stack;
  u32 last_stk;
  void (** loadfcn) (void *);
  void **userptrs;
  sem_t sem;
  sem_t running_sem;
};

static void * run_dispatcher(void * _attr){
  ccdispatch * dis = (ccdispatch *) _attr;
  sem_wait(&dis->sem);
  dis->main_stack = costack_create(&dis->attr);
  *(mstack()) = dis->main_stack;
  costack_save(dis->main_stack);
  
  while(true){

    if(dis->stks_count == 0){
      continue;
    }

    if(dis->stks_count > dis->last_stk 
       && (yeild_states[dis->last_stk] == CC_RESUMED || yeild_states[dis->last_stk] == CC_ENDED)
       && dis->loadfcn[dis->last_stk] != NULL){
      yeild_states[dis->last_stk] = CC_ENDED;
      dis->loadfcn[dis->last_stk] = NULL;
      dis->stks[dis->last_stk] = NULL;
    }
    
    dis->last_stk++;
    if(dis->last_stk == dis->stks_count){
      sem_post(&dis->running_sem);
      sem_wait(&dis->sem);
      dis->last_stk = 0;
    }
    
    costack * _stk = dis->stks[dis->last_stk];
	
    if(_stk == NULL){
      if(dis->loadfcn[dis->last_stk] == NULL) continue;

      _stk = costack_create(&dis->attr);
      dis->stks[dis->last_stk] = _stk;
      set_current_stack(_stk);
      dis->loadfcn[dis->last_stk](dis->userptrs[dis->last_stk]);
    }else{
      set_current_stack(_stk);
      yeild_states[stk_to_idx(_stk) - 1] = CC_ENDED;
      costack_resume(_stk);
    }
   }
  return NULL;
}

void ccthread(ccdispatch * dis, void (*fcn) (void *), void * userdata){
  int nidx = dis->stks_count;
  int ncnt = dis->stks_count + 1;
  for(int i = 0; i < dis->stks_count;i++){
    if(dis->loadfcn[i] == NULL){
      printf("reuse..\n");
      nidx = i;
      break;
    }
  }
  if(nidx == dis->stks_count){

    dis->stks = realloc(dis->stks, ncnt * sizeof(costack *));
    dis->loadfcn = realloc(dis->loadfcn, ncnt * sizeof(void *));
    dis->userptrs = realloc(dis->userptrs, ncnt * sizeof(void *));
  }
  dis->stks[nidx] = NULL;
  yeild_states[dis->last_stk] = CC_INITIALIZED;
  dis->loadfcn[nidx] = fcn;
  dis->userptrs[nidx] = userdata;
  dis->stks_count += 1;
}

ccdispatch * ccstart(){
  ccdispatch * dis = malloc(sizeof(ccdispatch));

  memset(dis,0,sizeof(ccdispatch));
  sem_init(&dis->sem,0, -1);
  sem_init(&dis->running_sem,0, 1);
  int stacksize = 0x4000;
  void * stack = malloc(stacksize);
  memset(stack,0,stacksize);
  pthread_attr_init(&dis->attr);
  pthread_attr_setstack(&dis->attr,stack,stacksize);
  pthread_create( &dis->thread, &dis->attr, run_dispatcher, dis);
  return dis;
}

void ccstep(ccdispatch * dis){
  sem_wait(&dis->running_sem);
  sem_post(&dis->sem);
}

// test //
int tclock = 0;
int time_of_day(){
  return tclock % 24;
}

int day(){
  return (tclock / 24) * 24;
}

int next_day(){
  return day() + 24;
}

bool is_day(){
  return time_of_day() > 8 && time_of_day() < 18;
}

void wait_until(int day_time){
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
}

void test(void * data){
  char * name = (char *) data;
  void status(char * status){
    printf("%s: %s..\n",name,status);
  }
  while(true){
    int end_of_day = day() + 17;
      while(tclock < end_of_day) {
      status("Chopping wood");
      yield();
      if(!(rand() % 20)){
	status("Spider! :( ");
	int t = tclock + rand() % 10;
	while(tclock < t){
	  yield();
	  status("Running around.. :( ");
	}
	status("getting back to work..\n");
      }
    }
    status("Time to go home..");
    while(wait_until2(18)) status("go home");
    while(wait_until2(22)) status("eat dinner");
    status("go to sleep");
    while(wait_until2(6))  {
      if(!(rand() %5)) status("Zzz");
      if(!(rand() %5)) status("zhmn");
    }
    status("Wake up");
    while(wait_until2(8)) status("go to forest");
  }
}

void costack_test(){
 ccdispatch * d = ccstart();
 ccthread(d,test,"kirk"); 
 ccthread(d,test,"johnson"); 
 ccthread(d,test,"rita"); 
 ccthread(d,test,"steve"); 
 for(int i = 8; i < 300; i++){
   usleep(100000);
   tclock = i;
   printf("** time of day: %i:00 **\n", time_of_day());
   ccstep(d);
   printf("** ** ** **\n\n");
 }
}
