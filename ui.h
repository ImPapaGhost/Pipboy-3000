#ifndef UI_H
#define UI_H

#include <SDL2/SDL_ttf.h>
#include "pipboy.h"

// UI rendering functions
void render_stat_subtabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_special_animation(SDL_Renderer *renderer, PipState *state);
void render_inv_subtabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_mid_background(SDL_Renderer *renderer, PipState *state);
void render_damage_bar(SDL_Renderer *renderer, int x, int y, int width, int height, int health);
void render_item_attribute(SDL_Renderer *renderer, TTF_Font *font, const char *label, const char *value, int x, int y);
void render_data_subtabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_data_tab(SDL_Renderer *renderer, TTF_Font *font, PipState *state);

#endif