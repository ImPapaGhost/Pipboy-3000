#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "pipboy.h"

// Function Prototypes
void render_stat_subtabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_tabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_current_tab(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_stat_tab(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_inv(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_data_tab(SDL_Renderer *renderer, TTF_Font *font);
void render_map_tab(SDL_Renderer *renderer, TTF_Font *font);
void render_radio_tab(SDL_Renderer *renderer, TTF_Font *font);
void render_special_animation(SDL_Renderer *renderer, PipState *state);
void render_damage_bar(SDL_Renderer *renderer, int x, int y, int width, int height, int health);
void render_vaultboy(SDL_Renderer *renderer);
void render_inv_subtabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_attribute_description(SDL_Renderer *renderer, TTF_Font *font, int selector_position);

// ✅ **Missing function declarations added**
void render_health_background(SDL_Renderer *renderer);
void render_ap_bar(SDL_Renderer *renderer);
void render_level_xp_background(SDL_Renderer *renderer, PipState *state);
void render_status_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_perks_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state);

// ✅ **Animation utility function (needed for SPECIAL animations)**
float ease_out_cubic(float t);

// Extern variables (defined in render.c)
extern SDL_Texture *categoryline_texture;
extern SDL_Texture *selectline_texture;
extern SDL_Texture *vaultboy_frames[NUM_VAULTBOY_FRAMES];
extern int vaultboy_frame_index;
extern DamageBars damage_bars;

#endif // RENDER_H
