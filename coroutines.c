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

costack * stacks[] = {1, NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL, NULL, NULL};

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

 bool costack_save( costack * cc){
   int stk = setjmp(((costack *)cc)->buf);
  if(stk == 0){
     size_t stacksize =0;
     void * ptr = NULL;
     int ok = pthread_attr_getstack(&((costack *)cc)->attr, &ptr, &stacksize);
    cc->stack = realloc(cc->stack,stacksize);
    cc->stacksize = stacksize;
    memcpy(cc->stack, ptr, stacksize);
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
  longjmp(stk->buf,stk_to_idx(stk));
}

void costack_delete( costack * stk)
{
  stacks[stk_to_idx(stk)] = NULL;
  free(stk->stack);
}

// test //

int yeild_states[100];
costack * stk1;

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
  bool r = costack_save(stk);
  yeild_states[stk_to_idx(stk)]++; 
  int i = yeild_states[stk_to_idx(stk)];
  if(i == 1){
    costack_resume(*(mstk()));
  }
}

void inner_test(int level){
  for(int i = 0; i < level;i++){
    yield();
    printf("a %i %i \n",level, i);
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

static void * do_things(void * _attr){
  static bool done = false;
  static int last = 0;
  static costack * stks[] = {NULL, NULL, NULL, NULL};
  
  pthread_attr_t * attr = (pthread_attr_t *) _attr;
  bool first = true;
  stk1 = costack_create(attr);
  
  costack_save(stk1);
  for(int i = 0 ; i < array_count(stks); i++)
    if(stks[i] == NULL)
      iterate(attr,stks + i, i);

  costack_save(stk1);
  while(true){
    costack * _stk = stks[last++ % array_count(stks)];
    if(_stk != NULL)
      costack_resume(_stk);
  }
  
  printf("final..\n");
  if(done)
    return NULL;
  done = true;
  printf("ok!\n");
  return NULL;
}


void costack_test(){
  pthread_attr_t attr;
  pthread_t thread;
  pthread_attr_init(&attr);
  pthread_attr_setstack(&attr,malloc(0x4000),0x4000);
  int ret = pthread_create( &thread, &attr, do_things, &attr);
  void * _ret;
  pthread_join(thread, &_ret);
}
