#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <GL/gl.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <iron/full.h>
#include "color.h"
#include "circle.h"
#include "game_object.h"
#include "game_state.h"
#include "event.h"

#include "renderer.h"
#include "sdl_utils.h"
#include "uivector.h"
#include "sdl_event.h"
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
  hash_table * texts;

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
  res->texts = ht_create(1024,sizeof(i32),sizeof(id_text_tuple));
  return res;
}

void renderer_unload(game_renderer * renderer){
  free (renderer);
  SDL_Quit();
}

void renderer_render_game(game_renderer * renderer, game_state * state){

  SDL_Rect dst;

  // render text:
  SDL_SetRenderTarget(renderer->renderer,renderer->right_target);
  SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer->renderer);

  SDL_Rect rect = {0,0,0,0};
  int offsety = 0;

  for(int i = 0; i < state->logitem_count; i++){
    id_text_tuple * tp = ht_lookup(renderer->texts,&state->logitems[i].id);
    if(tp == NULL){
      id_text_tuple new_item;
      new_item.id = state->logitems[i].id;
      SDL_Surface * texsurf = TTF_RenderText_Blended_Wrapped(renderer->font, state->logitems[i].text, (SDL_Color){0,0,0,1}, 200);
      new_item.text = SDL_CreateTextureFromSurface(renderer->renderer, texsurf);
      ht_insert(renderer->texts, &state->logitems[i].id,&new_item);
    }
  }

  int option_idx = 0;
  int option_cnt = 1;
  
  for(int i = 0; i < state->logitem_count; i++){
    if(state->logitems[i].is_option){
      option_cnt++;
    }else{
      option_idx = i + 1;
      option_cnt = 1;
    }
  }

  for(int i = 0; i < state->logitem_count; i++){
    id_text_tuple * tp = ht_lookup(renderer->texts,&state->logitems[i].id);
    SDL_QueryTexture(tp->text, NULL, NULL, &rect.w, &rect.h);
    rect.x = 0;
    rect.y = offsety;
    if(state->logitems[i].is_option && i >= option_idx){
      SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
      rectangleColor(renderer->renderer,rect.x,rect.y,rect.x + rect.w, rect.y + rect.h,0xFF000000);
      if(state->selected_idx == (i - option_idx)){
	boxColor(renderer->renderer,rect.x,rect.y,rect.x + rect.w, rect.y + rect.h,0xFFFFFF00);
      }
    }
    
    SDL_RenderCopy(renderer->renderer, tp->text, NULL, &rect);
    offsety += rect.h + 10;
  }
  
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
  u8 * image_fin = malloc(rect.w * rect.h);
  u8 * image32 = malloc(rect.w * rect.h * 4);
  memset(image32,0,rect.w * rect.h * 4);
  for(int i = 0; i < state->trees_count; i++){
    u32 basecolor = state->colors[i].color;
    memset(image,0,rect.w * rect.h);
    draw_circle_system(state->trees[i].circles,state->trees[i].tree,image,rect.w,rect.h);
    u32 * image322 = (u32 *) image32;

    int cnt = rect.w * rect.h;
    for(int j = 0; j < cnt; j++){
      u8 col = image[j];
      u32 colormask = col | (col << 8) | (col << 16) | (0xFF << 24);
      u32 fcolor = colormask & basecolor;
      image322[j] |= fcolor;
    }
  }
  
  SDL_QueryTexture(renderer->circ, NULL, NULL, &rect.w, &rect.h);    
  
  SDL_UpdateTexture(renderer->circ, NULL, image32, rect.w * 4);
  SDL_Point pt = {0,0};
  SDL_RenderCopyEx(renderer->renderer, renderer->circ, NULL, &rect,0, &pt, SDL_FLIP_VERTICAL);
  
  free(image);
  free(image32);
  free(image_fin);

  SDL_RenderPresent(renderer->renderer);
  checkRenderError(); 
}

u32 renderer_read_events(event * buffer, u32 count){
  SDL_Event evt; 
  u32 cnt = 0;
  for(; cnt<count;cnt++){
    if(SDL_PollEvent(&evt)){
      buffer[cnt] = sdl_event_to_event(evt);
    }else{
      return cnt;
    }
  }
  return cnt;
}
