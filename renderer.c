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
#include "circle.h"
#include "game_object.h"
#include "game_state.h"
#include "renderer.h"

struct _game_renderer{
  SDL_Window * window;
  SDL_Renderer * renderer;
  TTF_Font * font;
  SDL_Texture * left_target, * right_target;

//Caching for text rendering.
SDL_Texture * text_textures;
int * text_idx;
int text_idx_count;
};

//returns NULL on fail
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

game_renderer * renderer_load(){
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();
  IMG_Init(IMG_INIT_PNG);
  checkRenderError();
  SDL_Window * win = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1200, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  checkRenderError();
  SDL_Renderer * ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  checkRenderError();
  TTF_Font * font = TTF_OpenFont("/usr/share/cups/fonts/FreeMono.ttf", 18);
  checkRenderError();
  //glEnable(GL_MULTISAMPLE);
  glEnable( GL_LINE_SMOOTH ); 
  glEnable( GL_POLYGON_SMOOTH );
  glHint( GL_LINE_SMOOTH_HINT, GL_NICEST ); 
  glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
  checkRenderError();  
  game_renderer gr = {
    win,ren,font, 
    SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_TARGET,600,600),
    SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_TARGET,600,600)
  };
  checkRenderError();
  game_renderer * res = malloc(sizeof(gr));
  *res = gr;
  return res;
}

void renderer_unload(game_renderer * renderer){
  free (renderer);
  SDL_Quit();
}

int runid = 0;
void renderer_render_game(game_renderer * _renderer, game_state * state){
  runid++;
  game_renderer renderer = *_renderer;
  SDL_Color color = {0,0,0,255};
  static bool first = true;
  static SDL_Surface * surf = NULL; 
  static SDL_Texture * tex = NULL;
  static SDL_Texture * circ = NULL;
  SDL_Rect dst;
  int w = 512;
  static u8 * image;
  static   u8 * image24;

  if(first){
    //surf = TTF_RenderText_Blended_Wrapped(renderer.font, text, color, 600);
    //tex = SDL_CreateTextureFromSurface(renderer.renderer, surf);
    int w = 512;
    image = malloc(w * w);
    image24 = malloc(w * w * 4);

    circ = SDL_CreateTexture(renderer.renderer,SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STATIC,w,w);
    first = false;
  }
  
  circle circles[] = {
    {{0,0 - 50},100}
    ,{{0,0 + 50},100}
    ,{{0,0 + 50 * sin(runid * 0.02)},15}
  };
  
  circle_tree tree[] = {{ISEC,1,2},{LEAF,0,0},{SUB,1,2},{LEAF,1,0},{LEAF,2,0}}; 

  circle circles2[] = {
    {{0,0 - 50},100},
    {{0,0 + 50},100},
    {{0,0 + 50 * sin(runid * 0.02)},15}
  };
  
  mat3 m1 = mat3_2d_translation(80,0);
  mat3 m3 = mat3_2d_translation(-80,0);
  mat3 m2 = mat3_2d_rotation(runid * 0.01);
  mat3 mt = mat3_2d_translation(200,200);
  mt = mat3_mul(mt,m2);
  //circle_tree tree2[] = {{SUB,1,2},{LEAF,0,0},{SUB,1,2},{LEAF,1,0},{LEAF,2,0}}; 
  circle_tree tree2[] = {{ISEC,1,2},{LEAF,0,0},{SUB,1,2},{LEAF,1,0},{LEAF,2,0}}; 
  circle_tform(circles2,array_count(circles2),m1);
  circle_tform(circles,array_count(circles),m3);
  circle_tform(circles,array_count(circles),mt);
  circle_tform(circles2,array_count(circles2),mt);
  circ_tree a = {tree,circles};
  circ_tree b = {tree2,circles2};
  circ_tree * ct = sub_tree(ADD,&a,&b);
  
  draw_circle_system(ct->circles,ct->tree,image,w,w);
  free(ct);
  for(int i = 0; i < w * w; i++){
    int idx2 = i * 4;
    image24[idx2] = image[i];
    image24[idx2 + 1] = image[i];
    image24[idx2 + 2] = image[i];
  }
  SDL_UpdateTexture(circ, NULL, image24, w * 4);
  // render text:
  SDL_SetRenderTarget(renderer.renderer,renderer.right_target);
  SDL_SetRenderDrawColor(renderer.renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer.renderer);

  SDL_Rect rect = {0,0,0,0};
  //SDL_QueryTexture(tex, NULL, NULL, &rect.w, &rect.h);
  //SDL_RenderCopy(renderer.renderer, tex, NULL, &rect);

  /// render text done

  // render graphics

  SDL_SetRenderTarget(renderer.renderer,renderer.left_target);
  SDL_SetRenderDrawColor(renderer.renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer.renderer);
  filledCircleColor(renderer.renderer, 1, 1, 1, 0xFF000000);

  SDL_SetRenderTarget(renderer.renderer,NULL);
  SDL_RenderClear(renderer.renderer);
  
  dst.x = 0;
  dst.y = 0;
  SDL_QueryTexture(renderer.left_target, NULL, NULL, &dst.w, &dst.h);
  SDL_RenderCopy(renderer.renderer, renderer.left_target,NULL,&dst);
  dst.x = 600;
  SDL_QueryTexture(renderer.right_target, NULL, NULL, &dst.w, &dst.h);
  SDL_RenderCopy(renderer.renderer, renderer.right_target,NULL,&dst);

  SDL_QueryTexture(circ, NULL, NULL, &rect.w, &rect.h);
  SDL_RenderCopy(renderer.renderer, circ, NULL, &rect);
  SDL_RenderPresent(renderer.renderer);
  checkRenderError();
  
  SDL_Event evt;
  bool wait = true;
  
  while(wait){
    wait = false;
    while(SDL_PollEvent(&evt)){
      switch(evt.type){
      case SDL_KEYDOWN:
	if(evt.key.keysym.sym == SDLK_ESCAPE)
	  state->is_running = false;
	wait = false;
	break;
	
      case SDL_QUIT:

	state->is_running = false;
      case SDL_MOUSEBUTTONDOWN:
	break;
      }
    }
  }

}
