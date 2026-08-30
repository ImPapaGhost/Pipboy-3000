#include "map.h"
#include <SDL2/SDL.h>
#include <stdio.h>

static SDL_Texture* map_texture = NULL;
static int map_offset_x = 0;
static int map_offset_y = 0;

#define MAP_SCROLL_SPEED 8
#define MAP_TEXTURE_WIDTH 1024
#define MAP_TEXTURE_HEIGHT 1024
#define MAP_VIEW_WIDTH 700
#define MAP_VIEW_HEIGHT 370

static int clamp(int value, int minimum, int maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void map_init(SDL_Renderer* renderer) {
    // Create a simple grid-like texture (for demo purposes)
    SDL_Surface* surface = SDL_CreateRGBSurface(0, MAP_TEXTURE_WIDTH, MAP_TEXTURE_HEIGHT, 32,
                                                0x00FF0000,
                                                0x0000FF00,
                                                0x000000FF,
                                                0xFF000000);
    if (!surface) {
        printf("Failed to create surface: %s\n", SDL_GetError());
        return;
    }

    const Uint32 background = SDL_MapRGBA(surface->format, 0, 0, 0, 255);
    const Uint32 grid_color = SDL_MapRGBA(surface->format, 100, 255, 100, 255);
    SDL_FillRect(surface, NULL, background);

    // Drawing 32 narrow rectangles is much cheaper than mapping every pixel.
    for (int x = 0; x < MAP_TEXTURE_WIDTH; x += 64) {
        SDL_Rect line = {x, 0, 1, MAP_TEXTURE_HEIGHT};
        SDL_FillRect(surface, &line, grid_color);
    }
    for (int y = 0; y < MAP_TEXTURE_HEIGHT; y += 64) {
        SDL_Rect line = {0, y, MAP_TEXTURE_WIDTH, 1};
        SDL_FillRect(surface, &line, grid_color);
    }

    map_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!map_texture) {
        printf("Failed to create map texture: %s\n", SDL_GetError());
    }
}

void map_handle_key(SDL_Keycode key) {
    switch (key) {
        case SDLK_UP:    map_offset_y -= MAP_SCROLL_SPEED; break;
        case SDLK_DOWN:  map_offset_y += MAP_SCROLL_SPEED; break;
        case SDLK_LEFT:  map_offset_x -= MAP_SCROLL_SPEED; break;
        case SDLK_RIGHT: map_offset_x += MAP_SCROLL_SPEED; break;
        default: return;
    }

    map_offset_x = clamp(map_offset_x, 0, MAP_TEXTURE_WIDTH - MAP_VIEW_WIDTH);
    map_offset_y = clamp(map_offset_y, 0, MAP_TEXTURE_HEIGHT - MAP_VIEW_HEIGHT);
}

void map_update() {
    // Logic updates would go here
}

void map_render(SDL_Renderer* renderer) {
    if (!map_texture) return;

    SDL_Rect map_area = {50, 60, MAP_VIEW_WIDTH, MAP_VIEW_HEIGHT};

    SDL_Rect src_rect = { map_offset_x, map_offset_y, map_area.w, map_area.h };

    SDL_RenderCopy(renderer, map_texture, &src_rect, &map_area);
}

static void map_cleanup(void) {
    if (map_texture) {
        SDL_DestroyTexture(map_texture);
        map_texture = NULL;
    }
}
void map_shutdown(void) {
    map_cleanup();
}

