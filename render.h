#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "pipboy.h"

void render_health_background(SDL_Renderer *renderer);
void render_ap_bar(SDL_Renderer *renderer, PipState *state);
void render_date_time(SDL_Renderer *renderer, TTF_Font *font, PipState *state);

#endif
