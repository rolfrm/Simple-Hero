#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

#include "game_state.h"

char * text = "I said goodbye to everyone and embarked on the journey.\n"\
  "\n" \
  "Some would say more animal than human after what had happened, but they were all conformists anyway.\n"\
 "[go fight some things] [go home]";

void render_text(){
  
}

typedef struct _game_renderer{
  SDL_Window * window;
  SDL_Renderer * renderer;
  TTF_Font * font;
  SDL_Texture * guy, * grass, * campfire;
  SDL_Texture * left_target, * right_target;
} game_renderer;

SDL_Texture* loadTexture(char * path, SDL_Renderer *ren){
	//Initialize to nullptr to avoid dangling pointer issues
	SDL_Texture *texture = NULL;
	//Load the image
	SDL_RWops *rwop = SDL_RWFromFile(path,"rb");
	SDL_Surface *loadedImage = IMG_LoadPNG_RW(rwop);
	rwop->close(rwop);
	//If the loading went ok, convert to texture and return the texture
	if (loadedImage != NULL){
	  texture = SDL_CreateTextureFromSurface(ren, loadedImage);
	  SDL_FreeSurface(loadedImage);
	}
	return texture;
}

game_renderer load_game_renderer(){
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();
  IMG_Init(IMG_INIT_PNG);
  SDL_Window * win = SDL_CreateWindow("Hello World!", 100, 100, 1200, 600, SDL_WINDOW_SHOWN);
  SDL_Renderer * ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  TTF_Font * font = TTF_OpenFont("/usr/share/cups/fonts/FreeMono.ttf", 18);
  
  return (game_renderer) {win,ren,font, 
      loadTexture("guy.png",ren),
      loadTexture("grass.png",ren),
      loadTexture("campfire.png",ren),
      SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_TARGET,600,600),
      SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_TARGET,600,600)
      };
}

void render_game(game_renderer renderer, game_state * state){

  SDL_Color color = {0,0,0,255};
  static bool first = true;
  static SDL_Surface * surf = NULL; 
  static   SDL_Texture * tex = NULL;
  SDL_Rect dst;
  if(first){
    surf = TTF_RenderText_Blended_Wrapped(renderer.font, text, color, 600);
    tex = SDL_CreateTextureFromSurface(renderer.renderer, surf);
    first = false;
  }

  // render text:
  SDL_SetRenderTarget(renderer.renderer,renderer.right_target);
  SDL_SetRenderDrawColor(renderer.renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer.renderer);

  SDL_Rect rect = {0,0,0,0};
  SDL_QueryTexture(tex, NULL, NULL, &rect.w, &rect.h);
  SDL_RenderCopy(renderer.renderer, tex, NULL, &rect);

  /// render text done

  // render graphics

  SDL_SetRenderTarget(renderer.renderer,renderer.left_target);
  SDL_SetRenderDrawColor(renderer.renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer.renderer);

  dst = (SDL_Rect){300,200,100,100};
  SDL_QueryTexture(renderer.guy, NULL, NULL, &dst.w, &dst.h);
  SDL_RenderCopy(renderer.renderer, renderer.guy,NULL,&dst);

  dst.x = 150; 
  SDL_QueryTexture(renderer.campfire, NULL, NULL, &dst.w, &dst.h);
  SDL_RenderCopy(renderer.renderer, renderer.campfire,NULL,&dst);

  SDL_QueryTexture(renderer.grass, NULL, NULL, &dst.w, &dst.h);

  dst.x = 180;
  dst.y = 350;
  SDL_RenderCopy(renderer.renderer, renderer.grass,NULL,&dst);
  dst.x = 171;
  dst.y += 3;
  SDL_RenderCopy(renderer.renderer, renderer.grass,NULL,&dst);
  dst.x = 121;
  dst.y += 10;

  SDL_RenderCopy(renderer.renderer, renderer.grass,NULL,&dst);
  // render graphics done

  SDL_SetRenderTarget(renderer.renderer,NULL);
  
  SDL_RenderClear(renderer.renderer);
  
  dst.x = 0;
  dst.y = 0;
  SDL_QueryTexture(renderer.left_target, NULL, NULL, &dst.w, &dst.h);
  SDL_RenderCopy(renderer.renderer, renderer.left_target,NULL,&dst);
  dst.x = 600;
  SDL_QueryTexture(renderer.right_target, NULL, NULL, &dst.w, &dst.h);
  SDL_RenderCopy(renderer.renderer, renderer.right_target,NULL,&dst);
  SDL_RenderPresent(renderer.renderer);
  SDL_UpdateWindowSurface(renderer.window);
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

typedef struct{
  int deltax;
  int deltay;
}move_state;

int main(){
  game_state state;
  state.is_running = true;
  game_renderer renderer = load_game_renderer();
  while(state.is_running){
    render_game(renderer,&state);      
  }
  
  SDL_Quit();
  return 0;
}
