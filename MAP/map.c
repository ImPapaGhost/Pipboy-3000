#include "map.h"
#include <SDL.h>
#include <stdio.h>

static SDL_Texture* map_texture = NULL;
static int map_offset_x = 0;
static int map_offset_y = 0;

#define MAP_SCROLL_SPEED 8

void map_init(SDL_Renderer* renderer) {
    // Create a simple grid-like texture (for demo purposes)
    SDL_Surface* surface = SDL_CreateRGBSurface(0, 1024, 1024, 32,
                                                0x00FF0000,
                                                0x0000FF00,
                                                0x000000FF,
                                                0xFF000000);
    if (!surface) {
        printf("Failed to create surface: %s\n", SDL_GetError());
        return;
    }

    // Draw vertical and horizontal grid lines
    SDL_LockSurface(surface);
    Uint32* pixels = (Uint32*)surface->pixels;
    for (int y = 0; y < 1024; y++) {
        for (int x = 0; x < 1024; x++) {
            if (x % 64 == 0 || y % 64 == 0) {
                pixels[y * 1024 + x] = SDL_MapRGBA(surface->format, 100, 255, 100, 255);
            } else {
                pixels[y * 1024 + x] = SDL_MapRGBA(surface->format, 0, 0, 0, 255);
            }
        }
    }
    SDL_UnlockSurface(surface);

    map_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!map_texture) {
        printf("Failed to create map texture: %s\n", SDL_GetError());
    }
}

void map_handle_event(SDL_Event* e) {
    if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
            case SDLK_UP:    map_offset_y -= MAP_SCROLL_SPEED; break;
            case SDLK_DOWN:  map_offset_y += MAP_SCROLL_SPEED; break;
            case SDLK_LEFT:  map_offset_x -= MAP_SCROLL_SPEED; break;
            case SDLK_RIGHT: map_offset_x += MAP_SCROLL_SPEED; break;
            default: break;
        }
    }
}

void map_update() {
    // Logic updates would go here
}

void map_render(SDL_Renderer* renderer) {
    if (!map_texture) return;

    SDL_Rect map_area = { 50, 60, 700, 370 };

    SDL_Rect src_rect = { map_offset_x, map_offset_y, map_area.w, map_area.h };

    SDL_RenderCopy(renderer, map_texture, &src_rect, &map_area);
}

void map_cleanup() {
    if (map_texture) {
        SDL_DestroyTexture(map_texture);
        map_texture = NULL;
    }
}
void map_shutdown(void) {
    map_cleanup();
}

