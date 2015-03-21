#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"

#include <pthread.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>

#include "coroutines.h"

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

costack * stacks[] = {NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL};

int stk_to_idx(costack * stk){
  for(u32 i = 0 ; i < array_count(stacks); i++){
    if(stk == stacks[i]) return i;
  }
  return -1;
}

costack *idx_to_stk(int idx){
  return stacks[idx];
}

costack * costack_create(pthread_attr_t * attr){
  costack * cc = malloc(sizeof(costack));
  memset(cc,0,sizeof(costack));
  cc->attr = *attr;
  stacks[stk_to_idx(NULL)] = cc;
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
     if(cc->stack == NULL){
       cc->stack = malloc(stacksize);
     }
     
     cc->stacksize = stacksize;
     void * stack = cc->stack;
     //checkbuffer(ptr,stacksize);
     memcpy(stack, ptr, stacksize);
     
     return false;
  }else{
     cc = idx_to_stk(stk);
     //resume stack
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

// test //

int yeild_states[100];

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
  
  yeild_states[stk_to_idx(stk)] = 0;
  costack_save(stk);

  yeild_states[stk_to_idx(stk)]++; 

  int i = yeild_states[stk_to_idx(stk)];
  if(i == 1){
    costack_resume(*(mstack()));
  }
}

void inner_test(int level){
  for(int i = 0; i < level;i++){
    yield();
  }
  yield();
  if(level < 10)
    inner_test(level + 1);
}

void iterate(int x){
  printf("a\n");
  yield();
  printf("b %i\n",x);
  yield();
  inner_test(0);
  printf("c\n");
  yield();
  printf("d\n");
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
};

static void * run_dispatcher(void * _attr){
  ccdispatch * dis = (ccdispatch *) _attr;
  dis->main_stack = costack_create(&dis->attr);
  *(mstack()) = dis->main_stack;
  printf("Saving.. %i\n",dis->main_stack);
  costack_save(dis->main_stack);
  
  while(true){
    
    //usleep(1);
    
    if(dis->stks_count == 0){
      continue;
    }
    dis->last_stk++;
    dis->last_stk %= dis->stks_count;
    
    costack * _stk = dis->stks[dis->last_stk];
    if(_stk == NULL){
      _stk = costack_create(&dis->attr);
      dis->stks[dis->last_stk] = _stk;
      set_current_stack(_stk);
      dis->loadfcn[dis->last_stk](dis->userptrs[dis->last_stk]);
    }else{
      set_current_stack(_stk);
      costack_resume(_stk);
    }
  }
  return NULL;
}

void ccthread(ccdispatch * dis, void (*fcn) (void *), void * userdata){
  dis->stks = realloc(dis->stks, dis->stks_count + 1);
  dis->loadfcn = realloc(dis->loadfcn, dis->stks_count + 1);
  dis->userptrs = realloc(dis->userptrs, dis->stks_count + 1);
  dis->stks[dis->stks_count] = NULL;
  dis->loadfcn[dis->stks_count] = fcn;
  dis->userptrs[dis->stks_count] = userdata;
  dis->stks_count += 1;
}

ccdispatch * ccstart(){
  ccdispatch * dis = malloc(sizeof(ccdispatch));
  memset(dis,0,sizeof(ccdispatch));
  int stacksize = 0x4000;
  void * stack = malloc(stacksize);
  memset(stack,0,stacksize);
  pthread_attr_init(&dis->attr);
  pthread_attr_setstack(&dis->attr,stack,stacksize);
  pthread_create( &dis->thread, &dis->attr, run_dispatcher, dis);
  return dis;
}

void test(void * data){
  for(int i = 0; i < 10000;i++){
    printf("%i %i __\n",i, data);
    yield();
  }
}

void costack_test(){
 ccdispatch * d = ccstart();
 ccthread(d,test,(void *)1);
 usleep(100000);
 ccthread(d,test,(void *)2);
 usleep(200000);
 ccthread(d,test,(void *)0);
 usleep(3500000);
 ccthread(d,test,(void *)4);
 usleep(400000);
}
