//requires stdbool, pthread.h

typedef struct _costack costack;

costack * costack_create(pthread_attr_t * attr);
bool costack_save( costack * cc);
void costack_resume( costack * stk);
void costack_delete( costack * stk);
void costack_copy(costack * src, costack * dst);
