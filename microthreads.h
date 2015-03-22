// microthreading

void ccyield();
void ccfork();
void ccend();
typedef struct _ccdispatch ccdispatch;

ccdispatch * ccstart();
void ccthread(ccdispatch * dispatcher, void (* fcn)(void *), void * userdata);
void ccstep(ccdispatch * dispatcher);


// test //
void costack_test();
