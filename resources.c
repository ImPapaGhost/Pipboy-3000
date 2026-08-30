#include "resources.h"

#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <string.h>

static TTF_Font *load_font(const char *path, int point_size) {
    TTF_Font *font = TTF_OpenFont(path, point_size);
    if (!font) {
        fprintf(stderr, "Failed to load %s at %d pt: %s\n", path, point_size, TTF_GetError());
    }
    return font;
}

static SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path) {
    SDL_Texture *texture = IMG_LoadTexture(renderer, path);
    if (!texture) {
        fprintf(stderr, "Failed to load texture %s: %s\n", path, IMG_GetError());
    }
    return texture;
}

bool resources_init(AppResources *resources, SDL_Renderer *renderer) {
    if (!resources || !renderer) {
        return false;
    }

    memset(resources, 0, sizeof(*resources));

    resources->body_font = load_font("monofonto.ttf", 16);
    resources->detail_font = load_font("monofonto.ttf", 20);
    resources->subtab_font = load_font("monofonto.ttf", 22);
    resources->tab_font = load_font("monofonto.ttf", 28);

    if (!resources->body_font || !resources->detail_font ||
        !resources->subtab_font || !resources->tab_font) {
        resources_destroy(resources);
        return false;
    }

    /* These textures are reused every frame, so load them once. */
    resources->select_line = load_texture(renderer, "STAT/SELECTLINE.jpg");
    resources->category_line = load_texture(renderer, "STAT/CATEGORYLINE.jpg");
    resources->health_background = load_texture(renderer, "STAT/BOXHP1.jpg");
    resources->box_background = load_texture(renderer, "STAT/BOX4.jpg");
    resources->target_icon = load_texture(renderer, "INV/TARGET1.jpg");
    resources->ammo_icon = load_texture(renderer, "INV/ammo.png");

    return true;
}

void resources_destroy(AppResources *resources) {
    if (!resources) {
        return;
    }

    if (resources->ammo_icon) SDL_DestroyTexture(resources->ammo_icon);
    if (resources->target_icon) SDL_DestroyTexture(resources->target_icon);
    if (resources->box_background) SDL_DestroyTexture(resources->box_background);
    if (resources->health_background) SDL_DestroyTexture(resources->health_background);
    if (resources->category_line) SDL_DestroyTexture(resources->category_line);
    if (resources->select_line) SDL_DestroyTexture(resources->select_line);

    if (resources->tab_font) TTF_CloseFont(resources->tab_font);
    if (resources->subtab_font) TTF_CloseFont(resources->subtab_font);
    if (resources->detail_font) TTF_CloseFont(resources->detail_font);
    if (resources->body_font) TTF_CloseFont(resources->body_font);

    memset(resources, 0, sizeof(*resources));
}
