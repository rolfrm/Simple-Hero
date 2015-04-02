#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <GL/gl.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "../bitguy/bitguy.h"
#include "../bitguy/utils.h"
#include "../bitguy/linmath.h"

SDL_Texture* loadTexture(char * path, SDL_Renderer *ren){
  SDL_RWops *rwop = SDL_RWFromFile(path,"rb");
  if(rwop == NULL){
    ERROR("unable to open path");
    ERROR("path: '%s'", path);
    return NULL;
  }
  SDL_Surface *loadedImage = IMG_LoadPNG_RW(rwop);
  rwop->close(rwop);
  if (loadedImage != NULL){
    SDL_Texture * texture = SDL_CreateTextureFromSurface(ren, loadedImage);
    SDL_FreeSurface(loadedImage);
    return texture;
  }else{
    ERROR("unable to load PNG image");
    return NULL;
  }
}

void checkRenderError(){
  const char * err = SDL_GetError();
  if(strlen(err) > 0){
    ERROR("SDL: %s", err);
    SDL_ClearError();
  }
  u32 glerror = glGetError();
  if(glerror != 0)
    ERROR("GL: %i",glerror);
}
