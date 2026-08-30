#ifndef ANIMATION_H
#define ANIMATION_H

#include <SDL2/SDL.h>
#include "pipboy.h"

void render_vaultboy(SDL_Renderer *renderer, AppResources *resources, double elapsed_seconds);
void render_special_animation(
    SDL_Renderer *renderer,
    AppResources *resources,
    PipState *state,
    double elapsed_seconds
);

#endif
