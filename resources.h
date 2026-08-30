#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "video.h"

typedef struct {
    TTF_Font *body_font;
    TTF_Font *detail_font;
    TTF_Font *subtab_font;
    TTF_Font *tab_font;

    SDL_Texture *select_line;
    SDL_Texture *category_line;
    SDL_Texture *health_background;
    SDL_Texture *box_background;
    SDL_Texture *target_icon;
    SDL_Texture *ammo_icon;

    VideoPlayer *vaultboy_video;
    VideoPlayer *special_videos[7];
    VideoPlayer *radio_video;
} AppResources;

bool resources_init(AppResources *resources, SDL_Renderer *renderer);
void resources_destroy(AppResources *resources);

#endif
