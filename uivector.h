// requires stdlib.h
// stolen from lodePNG.
/*dynamic vector of unsigned ints*/
typedef struct uivector
{
  unsigned* data;
  size_t size; /*size in number of unsigned longs*/
  size_t allocsize; /*allocated size in bytes*/
} uivector;

void uivector_cleanup(uivector* p);
/*returns 1 if success, 0 if failure ==> nothing done*/
unsigned uivector_reserve(uivector* p, size_t allocsize);
/*returns 1 if success, 0 if failure ==> nothing done*/
unsigned uivector_resize(uivector* p, size_t size);
/*resize and give all new elements the value*/
unsigned uivector_resizev(uivector* p, size_t size, unsigned value);
// sets everything to 0.
void uivector_init(uivector* p);
/*returns 1 if success, 0 if failure ==> nothing done*/
unsigned uivector_push_back(uivector* p, unsigned c);
/*copy q to p, returns 1 if success, 0 if failure ==> nothing done*/
unsigned uivector_copy(uivector* p, const uivector* q);
