#ifndef ANIMATION_H
#define ANIMATION_H

#include <SDL2/SDL.h>

void play_animation(SDL_Renderer *renderer, SDL_Texture *frames[], int frame_count, int frame_delay);

#endif
