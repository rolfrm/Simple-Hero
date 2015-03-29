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
#include "game_object.h"
#include "game_state.h"
#include "renderer.h"
#include "circle.h"


char * text = "I said goodbye to everyone and embarked on the journey.\n"\
  "\n" \
  "Some would say more animal than human after what had happened, but they were all conformists anyway.\n"\
 "[go fight some things] [go home]";

struct _game_renderer{
  SDL_Window * window;
  SDL_Renderer * renderer;
  TTF_Font * font;
  SDL_Texture * guy, * grass, * campfire;
  SDL_Texture * left_target, * right_target;
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
    loadTexture("guy.png",ren),
    loadTexture("grass.png",ren),
    loadTexture("campfire.png",ren),
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

void renderer_render_game(game_renderer * _renderer, game_state * state){
  game_renderer renderer = *_renderer;
  SDL_Color color = {0,0,0,255};
  static bool first = true;
  static SDL_Surface * surf = NULL; 
  static SDL_Texture * tex = NULL;
  static SDL_Texture * circ = NULL;
  SDL_Rect dst;
  if(first){
    surf = TTF_RenderText_Blended_Wrapped(renderer.font, text, color, 600);
    tex = SDL_CreateTextureFromSurface(renderer.renderer, surf);
    int w = 512;
    circle circles[] = {{{160,160},100}
			,{{160,160 + 100},80}
			,{{160,160 + 20},30}};
    //circle_tree tree[] = {{SUB,1,2},{LEAF,0,0},{ADD,3,4},{LEAF,1,0},{LEAF,2,0}};
    circle_tree tree[] = {{ISEC,1,2}, {LEAF,0,0},{LEAF,1,0}};
    u8 * image = malloc(w * w);
    u8 * image24 = malloc(w * w * 4);
    draw_circle_system(circles,array_count(circles),tree,array_count(tree),image,w,w);
    for(int i = 0; i < w * w; i++){
      int idx2 = i * 4;
      image24[idx2] = image[i];
      image24[idx2 + 1] = image[i];
      image24[idx2 + 2] = image[i];
    }

    circ = SDL_CreateTexture(renderer.renderer,SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STATIC,w,w);
    SDL_UpdateTexture(circ, NULL, image24, w * 4);
    free(image);
    free(image24);

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
  filledCircleColor(renderer.renderer, 1, 1, 1, 0xFF000000);
  
  double widths[] = {1.0,1.0,1.0,1.0};
  mat4 matrixes[array_count(widths)];
  vec3 points[array_count(widths) * 2]; //xyz and 3 pts.
  for(u32 i = 0 ; i < array_count(widths);i++)
    matrixes[i] = mat4_translate(0.0,1.0,0.0);
  
  mat4 m = mat4_scale(mat4_translate(0.0,5.0,0.0),30.0);
  for(int i = 0; i < array_count(widths); i++){
    vec3 zero = {-widths[i],0.0,0.0};
    vec3 w = {widths[i],0.0,0.0};
    points[i * 2] = mat4_mul_vec3(m,zero);
    points[i * 2 + 1] = mat4_mul_vec3(m,w);
    m = mat4_mul(m,matrixes[i]);
  }
  
  float xy3[array_count(widths) * 4];
  for(int i = 0; i < array_count(widths); i++){
    xy3[i * 4 + 0] = points[i * 2].x;
    xy3[i * 4 + 1] = points[i * 2 ].y;
    xy3[i * 4 + 2] = points[i * 2 + 1].x;
    xy3[i * 4 + 3] = points[i * 2 + 1].y;
  }
  //lineColor(renderer.renderer, xydata[0] + 350 ,xydata[1] + 200, x + 350, y + 200,0xFF000000);
  glEnable( GL_BLEND );
  glDisable( GL_DEPTH_TEST ); 
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  glEnableClientState(GL_VERTEX_ARRAY);  
  for(int i = 0; i < state->item_count; i++){
    game_obj go = state->items[i];
    glTranslatef(go.player.x,go.player.y,0.0);
    glColor4f(0.6,0.7,0.6,1.0);
    if(go.header == CAMPFIRE)
      glColor4f(0.9, 0.9, 0.6, 1.0);
    if(go.header == GRASS)
      glColor4f(0.2, 0.9, 0.3, 1.0);
    glVertexPointer(2, GL_FLOAT, 0, xy3);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, array_count(xy3)/2 - 2);
    glTranslatef(-go.player.x,-go.player.y,0.0);
  }
  glDisableClientState(GL_VERTEX_ARRAY);
  //aacircleColor(renderer.renderer, 200, 200, 100, 0xFF000000);
  //aapolygonColor(renderer.renderer,xy2, xy2 + array_count(xydata)/2, array_count(xydata)/2, 0xFF0000000);
  //printf("\n");
  

  /*  
  glRotatef(0,0,1.0,1.0);
  glBegin( GL_POLYGON ); 
  glVertex3f( 0.0f, 100.0f, 0.0f );
  glVertex3f( -100.0f, -100.0f, 0.0f );
  glVertex3f( 100.0f, -100.0f, 0.0f ); 
  glEnd( );*/

  //aapolygonColor(renderer.renderer,xy, xy + array_count(xydata)/2, array_count(xydata)/2, 0xFF0000000);
  //aapolygonColor(renderer.renderer,xy,xy + 3, 3, 0xFF0000000);
  /*for(int i = 0 ; i < state->item_count; i++){
    game_obj gobj = state->items[i];
    dst = (SDL_Rect){0, 0, 100,100};
    SDL_Texture * _tex;
    //printf("gobj.header %i\n",gobj.header);
    switch(gobj.header){
    case PLAYER:
      _tex = renderer.guy;
      dst.x = gobj.player.x;
      dst.y = gobj.player.y;
      break;
    case GRASS:
      _tex = renderer.grass;
      dst.x = gobj.grass_leaf.x;
      dst.y = gobj.grass_leaf.y;
      break;
      }
    
    
    //SDL_QueryTexture(_tex, NULL, NULL, &dst.w, &dst.h);
    //SDL_RenderCopy(renderer.renderer, _tex,NULL,&dst);
    }*/
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
