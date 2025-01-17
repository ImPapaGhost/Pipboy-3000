#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define FRAME_RATE 60
#define NUM_VAULTBOY_FRAMES 8 // Total frames for VaultBoy animation

SDL_Texture *vaultboy_frames[NUM_VAULTBOY_FRAMES];
int vaultboy_frame_index = 0;
Uint32 last_vaultboy_update = 0; // Time tracker for animation updates

typedef struct {
    SDL_Texture *texture;
    SDL_Rect rect;
} Sprite;

typedef enum {
    TAB_STAT,
    TAB_INV,
    TAB_DATA,
    TAB_MAP,
    TAB_RADIO,
    NUM_TABS
} PipboyTab;

typedef struct {
    PipboyTab current_tab;
    int selector_position;
    int special_stats[7];
    int level;
    int health;
    int ap;
    int experience;
    char perks[10][50];
} GameState;

GameState game_state;

void load_vaultboy_frames(SDL_Renderer *renderer) {
    char path[256];
    for (int i = 0; i < NUM_VAULTBOY_FRAMES; i++) {
        snprintf(path, sizeof(path), "STAT/VaultBoy/%02d.png", i);
        SDL_Surface *surface = IMG_Load(path);
        if (!surface) {
            fprintf(stderr, "Failed to load VaultBoy frame %d: %s\n", i, path);
            vaultboy_frames[i] = NULL;
            continue;
        }
        vaultboy_frames[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
}

void free_vaultboy_frames() {
    for (int i = 0; i < NUM_VAULTBOY_FRAMES; i++) {
        if (vaultboy_frames[i]) {
            SDL_DestroyTexture(vaultboy_frames[i]);
            vaultboy_frames[i] = NULL;
        }
    }
}

void render_vaultboy(SDL_Renderer *renderer) {
    if (vaultboy_frames[vaultboy_frame_index]) {
        SDL_Rect dest_rect = {325, 300, 150, 150};
        SDL_RenderCopy(renderer, vaultboy_frames[vaultboy_frame_index], NULL, &dest_rect);
    }
}

void render_tabs(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    const char *tab_names[] = {"STAT", "INV", "DATA", "MAP", "RADIO"};
    SDL_Color color = {0, 255, 0, 255};

    for (int i = 0; i < NUM_TABS; i++) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, tab_names[i], color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {50 + i * 150, 20, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    SDL_Rect highlight_rect = {50 + state->current_tab * 150, 20, 150, 30};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &highlight_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}

void render_stat_tab(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    SDL_Color color = {0, 255, 0, 255};
    const char *stats[] = {"Strength", "Perception", "Endurance", "Charisma", "Intelligence", "Agility", "Luck"};
    char stats_text[100];
    snprintf(stats_text, sizeof(stats_text), "Health: %d  AP: %d  XP: %d", state->health, state->ap, state->experience);

    SDL_Surface *stats_surface = TTF_RenderText_Solid(font, stats_text, color);
    SDL_Texture *stats_texture = SDL_CreateTextureFromSurface(renderer, stats_surface);
    SDL_Rect stats_rect = {75, 50, stats_surface->w, stats_surface->h};
    SDL_RenderCopy(renderer, stats_texture, NULL, &stats_rect);
    SDL_FreeSurface(stats_surface);
    SDL_DestroyTexture(stats_texture);

    for (int i = 0; i < 7; i++) {
        char stat_text[50];
        snprintf(stat_text, sizeof(stat_text), "%s: %d", stats[i], state->special_stats[i]);
        SDL_Surface *surface = TTF_RenderText_Solid(font, stat_text, color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {75, 100 + i * 40, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    render_vaultboy(renderer);
}

void render_inv_tab(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Color color = {0, 255, 0, 255};
    const char *placeholder = "Inventory Tab Placeholder";
    SDL_Surface *surface = TTF_RenderText_Solid(font, placeholder, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {75, 140, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_data_tab(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Color color = {0, 255, 0, 255};
    const char *placeholder = "Data Tab Placeholder";
    SDL_Surface *surface = TTF_RenderText_Solid(font, placeholder, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {75, 140, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_map_tab(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Color color = {0, 255, 0, 255};
    const char *placeholder = "Map Tab Placeholder";
    SDL_Surface *surface = TTF_RenderText_Solid(font, placeholder, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {75, 140, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_radio_tab(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Color color = {0, 255, 0, 255};
    const char *placeholder = "Radio Tab Placeholder";
    SDL_Surface *surface = TTF_RenderText_Solid(font, placeholder, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {75, 140, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_current_tab(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    switch (state->current_tab) {
        case TAB_STAT:
            render_stat_tab(renderer, font, state);
            break;
        case TAB_INV:
            render_inv_tab(renderer, font);
            break;
        case TAB_DATA:
            render_data_tab(renderer, font);
            break;
        case TAB_MAP:
            render_map_tab(renderer, font);
            break;
        case TAB_RADIO:
            render_radio_tab(renderer, font);
            break;
    }
}

void handle_navigation(SDL_Event *event, GameState *state) {
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case SDLK_RIGHT:
                state->current_tab = (state->current_tab + 1) % NUM_TABS;
                break;
            case SDLK_LEFT:
                state->current_tab = (state->current_tab - 1 + NUM_TABS) % NUM_TABS;
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Pip-Boy 3000", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("monofonto.ttf", 16);
    if (!font) {
        fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError());
        return 1;
    }

    load_vaultboy_frames(renderer);

    bool running = true;
    SDL_Event event;
    Uint32 last_frame_time = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else {
                handle_navigation(&event, &game_state);
            }
        }

        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_vaultboy_update > 100) {
            vaultboy_frame_index = (vaultboy_frame_index + 1) % NUM_VAULTBOY_FRAMES;
            last_vaultboy_update = current_time;
        }

        SDL_RenderClear(renderer);

        render_tabs(renderer, font, &game_state);
        render_current_tab(renderer, font, &game_state);

        SDL_RenderPresent(renderer);

        SDL_Delay(1000 / FRAME_RATE);
    }

    free_vaultboy_frames();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
