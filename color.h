// requires bitguy.h
typedef struct{
  union{
    u8 data[4];
    u8 r,g,b,a;
    u32 color;
  };
}color;
