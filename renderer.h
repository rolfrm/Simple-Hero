//requires game_state.h
typedef struct _game_renderer game_renderer;


game_renderer * renderer_load();
void renderer_render_game(game_renderer * _renderer, game_state * state);
void renderer_unload(game_renderer * renderer);
