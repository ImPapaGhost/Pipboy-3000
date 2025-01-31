#include "render.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#include "pipboy.h"

void render_health_background(SDL_Renderer *renderer) {
    SDL_Texture *background = IMG_LoadTexture(renderer, "STAT/BOXHP1.jpg");
    SDL_Rect background_rect = {110, 430, 135, 30};
    SDL_SetTextureColorMod(background, 0, 255, 0);
    SDL_RenderCopy(renderer, background, NULL, &background_rect);
    SDL_DestroyTexture(background);
}

void render_ap_bar(SDL_Renderer *renderer) {
    SDL_Texture *bar = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");
    SDL_Rect bar_rect = {SCREEN_WIDTH - 245, 430, 145, 30};
    SDL_SetTextureColorMod(bar, 0, 255, 0);
    SDL_RenderCopy(renderer, bar, NULL, &bar_rect);
    SDL_DestroyTexture(bar);
}
