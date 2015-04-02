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
#include "sdl_utils.h"
#include "uivector.h"

typedef struct {
  int id;
  SDL_Texture * text;
}id_text_tuple;

struct _game_renderer{
  SDL_Window * window;
  SDL_Renderer * renderer;
  TTF_Font * font;
  SDL_Texture * left_target, * right_target;

  //Caching for text rendering.
  SDL_Texture * text_textures;
  int * text_idx;
  int text_idx_count;

  // Circle render target
  SDL_Texture * circ;
};

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
  
  game_renderer gr;
  memset(&gr,0,sizeof(game_renderer));
  gr.window = win;
  gr.renderer = ren;
  gr.font = font;
  gr.left_target = SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_TARGET,600,600);
  gr.right_target = SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_TARGET,600,600);
  gr.circ = SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STATIC,600,600);
  checkRenderError();
  game_renderer * res = malloc(sizeof(gr));
  *res = gr;
  return res;
}

void renderer_unload(game_renderer * renderer){
  free (renderer);
  SDL_Quit();
}

void renderer_render_game(game_renderer * renderer, game_state * state){

  SDL_Color color = {0,0,0,255};
  SDL_Rect dst;

  // render text:
  SDL_SetRenderTarget(renderer->renderer,renderer->right_target);
  SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer->renderer);

  SDL_Rect rect = {0,0,0,0};
  for(int i = 0; i < state->logitem_count; i++){
    int j = 0;
    for(; j < renderer->text_idx_count; j++){
      if(renderer->text_idx[j] == state->logitems[i].id)
	break;
    }
    if(j == renderer->text_idx_count){
      //create a new texture with the text..
    }
  }

  
  //SDL_RenderCopy(renderer->renderer, tex, NULL, &rect);

  /// render text done

  // render graphics

  SDL_SetRenderTarget(renderer->renderer,renderer->left_target);
  SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer->renderer);
  filledCircleColor(renderer->renderer, 1, 1, 1, 0xFF000000);

  SDL_SetRenderTarget(renderer->renderer,NULL);
  SDL_RenderClear(renderer->renderer);
  
  dst.x = 0;
  dst.y = 0;
  SDL_QueryTexture(renderer->left_target, NULL, NULL, &dst.w, &dst.h);
  SDL_RenderCopy(renderer->renderer, renderer->left_target,NULL,&dst);
  dst.x = 600;
  SDL_QueryTexture(renderer->right_target, NULL, NULL, &dst.w, &dst.h);
  SDL_RenderCopy(renderer->renderer, renderer->right_target,NULL,&dst);

  SDL_QueryTexture(renderer->circ, NULL, NULL, &rect.w, &rect.h);    
  u8 * image = malloc(rect.w * rect.h);
  u8 * image24 = malloc(rect.w * rect.h * 4);
   
  for(int i = 0; i < state->trees_count; i++){
    draw_circle_system(state->trees[i].circles,state->trees[i].tree,image,rect.w,rect.h);
    int cnt = rect.w * rect.h;
    for(int i = 0; i < cnt; i++){
      int idx2 = i * 4;
      image24[idx2] = image[i];
      image24[idx2 + 1] = image[i];
      image24[idx2 + 2] = image[i];
    }
    SDL_QueryTexture(renderer->circ, NULL, NULL, &rect.w, &rect.h);    
    
    SDL_UpdateTexture(renderer->circ, NULL, image24, rect.w * 4);
    SDL_RenderCopy(renderer->renderer, renderer->circ, NULL, &rect);
  }
  free(image);
  free(image24);


  SDL_RenderPresent(renderer->renderer);
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
