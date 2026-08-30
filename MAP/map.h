#ifndef MAP_H
#define MAP_H

#include <SDL2/SDL.h>

void map_init(SDL_Renderer* renderer);
void map_handle_key(SDL_Keycode key);
void map_update(void);
void map_render(SDL_Renderer* renderer);
void map_shutdown(void);

#endif // MAP_H
