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
#include "event.h"
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

int sdl_key_keytable[] = {
  KEY_BACKSPACE,SDLK_BACKSPACE,
  KEY_TAB,SDLK_TAB,
  KEY_CLEAR,SDLK_CLEAR,
  KEY_RETURN,SDLK_RETURN,
  KEY_PAUSE,SDLK_PAUSE,
  KEY_ESCAPE,SDLK_ESCAPE,
  KEY_SPACE,SDLK_SPACE,
  KEY_EXCLAIM,SDLK_EXCLAIM,
  KEY_QUOTEDBL,SDLK_QUOTEDBL,
  KEY_HASH,SDLK_HASH,
  KEY_DOLLAR,SDLK_DOLLAR,
  KEY_AMPERSAND,SDLK_AMPERSAND,
  KEY_QUOTE,SDLK_QUOTE,
  KEY_LEFTPAREN,SDLK_LEFTPAREN,
  KEY_RIGHTPAREN,SDLK_RIGHTPAREN,
  KEY_ASTERISK,SDLK_ASTERISK,
  KEY_PLUS,SDLK_PLUS,
  KEY_COMMA,SDLK_COMMA,
  KEY_MINUS,SDLK_MINUS,
  KEY_PERIOD,SDLK_PERIOD,
  KEY_SLASH,SDLK_SLASH,
  KEY_0,SDLK_0,
  KEY_1,SDLK_1,
  KEY_2,SDLK_2,
  KEY_3,SDLK_3,
  KEY_4,SDLK_4,
  KEY_5,SDLK_5,
  KEY_6,SDLK_6,
  KEY_7,SDLK_7,
  KEY_8,SDLK_8,
  KEY_9,SDLK_9,
  KEY_COLON,SDLK_COLON,
  KEY_SEMICOLON,SDLK_SEMICOLON,
  KEY_LESS,SDLK_LESS,
  KEY_EQUALS,SDLK_EQUALS,
    KEY_GREATER,SDLK_GREATER,
  KEY_QUESTION,SDLK_QUESTION,
  KEY_AT,SDLK_AT,
  KEY_LEFTBRACKET,SDLK_LEFTBRACKET,
  KEY_BACKSLASH,SDLK_BACKSLASH,
  KEY_RIGHTBRACKET,SDLK_RIGHTBRACKET,
  KEY_CARET,SDLK_CARET,
  KEY_UNDERSCORE,SDLK_UNDERSCORE,
  KEY_BACKQUOTE,SDLK_BACKQUOTE,
  KEY_a,SDLK_a,
  KEY_b,SDLK_b,
  KEY_c,SDLK_c,
  KEY_d,SDLK_d,
  KEY_e,SDLK_e,
  KEY_f,SDLK_f,
  KEY_g,SDLK_g,
  KEY_h,SDLK_h,
  KEY_i,SDLK_i,
  KEY_j,SDLK_j,
  KEY_k,SDLK_k,
  KEY_l,SDLK_l,
  KEY_m,SDLK_m,
  KEY_n,SDLK_n,
  KEY_o,SDLK_o,
  KEY_p,SDLK_p,
  KEY_q,SDLK_q,
  KEY_r,SDLK_r,
  KEY_s,SDLK_s,
  KEY_t,SDLK_t,
  KEY_u,SDLK_u,
  KEY_v,SDLK_v,
  KEY_w,SDLK_w,
  KEY_x,SDLK_x,
  KEY_y,SDLK_y,
  KEY_z,SDLK_z,
  KEY_DELETE,SDLK_DELETE,
  //KEY_KP,SDLK_KP,
  //KEY_KP1,SDLK_KP1,
  //KEY_KP2,SDLK_KP2,
  //KEY_KP3,SDLK_KP3,
  //KEY_KP4,SDLK_KP4,
  //KEY_KP5,SDLK_KP5,
  //KEY_KP6,SDLK_KP6,
  //KEY_KP7,SDLK_KP7,
  //KEY_KP8,SDLK_KP8,
  //KEY_KP9,SDLK_KP9,
  KEY_KP_PERIOD,SDLK_KP_PERIOD,
  KEY_KP_DIVIDE,SDLK_KP_DIVIDE,
  KEY_KP_MULTIPLY,SDLK_KP_MULTIPLY,
  KEY_KP_MINUS,SDLK_KP_MINUS,
  KEY_KP_PLUS,SDLK_KP_PLUS,
  KEY_KP_ENTER,SDLK_KP_ENTER,
  KEY_KP_EQUALS,SDLK_KP_EQUALS,
  KEY_UP,SDLK_UP,
  KEY_DOWN,SDLK_DOWN,
  KEY_RIGHT,SDLK_RIGHT,
  KEY_LEFT,SDLK_LEFT,
  KEY_INSERT,SDLK_INSERT,
  KEY_HOME,SDLK_HOME,
  KEY_END,SDLK_END,
  KEY_PAGEUP,SDLK_PAGEUP,
  KEY_PAGEDOWN,SDLK_PAGEDOWN,
  KEY_F1,SDLK_F1,
  KEY_F2,SDLK_F2,
  KEY_F3,SDLK_F3,
  KEY_F4,SDLK_F4,
  KEY_F5,SDLK_F5,
  KEY_F6,SDLK_F6,
  KEY_F7,SDLK_F7,
  KEY_F8,SDLK_F8,
  KEY_F9,SDLK_F9,
  KEY_F10,SDLK_F10,
  KEY_F11,SDLK_F11,
  KEY_F12,SDLK_F12,
  KEY_F13,SDLK_F13,
  KEY_F14,SDLK_F14,
  KEY_F15,SDLK_F15,
  //KEY_NUMLOCK,SDLK_NUMLOCK,
  KEY_CAPSLOCK,SDLK_CAPSLOCK,
  //KEY_SCROLLOCK,SDLK_SCROLLOCK,
  KEY_RSHIFT,SDLK_RSHIFT,
  KEY_LSHIFT,SDLK_LSHIFT,
  KEY_RCTRL,SDLK_RCTRL,
  KEY_LCTRL,SDLK_LCTRL,
  KEY_RALT,SDLK_RALT,
  KEY_LALT,SDLK_LALT,
  //  KEY_RMETA,SDLK_RMETA,
  //KEY_LMETA,SDLK_LMETA,
  //KEY_LSUPER,SDLK_LSUPER,
  //KEY_RSUPER,SDLK_RSUPER,
  KEY_MODE,SDLK_MODE,
  KEY_HELP,SDLK_HELP,
  //KEY_PRINT,SDLK_PRINT,
  KEY_SYSREQ,SDLK_SYSREQ,
  //KEY_BREAK,SDLK_BREAK,
  KEY_MENU,SDLK_MENU,
  KEY_POWER,SDLK_POWER,
  //KEY_EURO,SDLK_EURO
};

SDL_Keycode key_to_sdl_keycode(keysym sym){
  for(int i = 0; i < array_count(sdl_key_keytable); i += 2){
    if((int)sdl_key_keytable[i] == (int)sym){
      return (SDL_Keycode) sdl_key_keytable[i + 1];
    }
  }
  return (SDL_Keycode)0;
}

keysym sdl_keycode_to_key(SDL_Keycode sym){
  for(int i = 0; i < array_count(sdl_key_keytable); i += 2){
    if((int)sdl_key_keytable[i + 1] == (int)sym){
      return (keysym) sdl_key_keytable[i];
    }
  }
  return (keysym)0;
}


event sdl_event_to_event(SDL_Event sdlevt){
  event evt;
  key_event_type ktp = KEYDOWN;
  switch(sdlevt.type){
  case SDL_KEYUP:
    ktp = KEYUP;
  case SDL_KEYDOWN:
    evt.type = KEY;
    evt.key.type = ktp;
    evt.key.sym = sdl_keycode_to_key(sdlevt.key.keysym.sym);
    break;
  default:
    evt.type = UNKNOWN;
  }
  return evt;
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

  
  //

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
      event evt2 = sdl_event_to_event(evt);
      switch(evt.type){
      case SDL_KEYDOWN:
	//printf(evt2.key
	//if(evt.key.keysym.sym == SDLK_ESAPE)
	//  state->is_running = false;
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
