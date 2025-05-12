#ifndef MAP_H
#define MAP_H

#include <SDL.h>

void map_init(SDL_Renderer* renderer);
void map_handle_event(SDL_Event* e);
void map_update(void);
void map_render(SDL_Renderer* renderer);
void map_shutdown(void);

void map_cleanup(void);

#endif // MAP_H