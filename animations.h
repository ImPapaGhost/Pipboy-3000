#ifndef ANIMATION_H
#define ANIMATION_H

#include <SDL2/SDL.h>
#include "pipboy.h"

extern SDL_Texture *vaultboy_frames[NUM_VAULTBOY_FRAMES];
extern int vaultboy_frame_index;

void load_vaultboy_frames(SDL_Renderer *renderer);
void free_vaultboy_frames(void);
void render_vaultboy(SDL_Renderer *renderer);
void render_special_animation(SDL_Renderer *renderer, PipState *state);

#endif
