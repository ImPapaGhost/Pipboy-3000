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

typedef enum {
    SUBTAB_STATUS,
    SUBTAB_SPECIAL,
    SUBTAB_PERKS,
    NUM_SUBTABS
} StatSubTab;

typedef struct {
    PipboyTab current_tab;
    StatSubTab current_subtab; // STAT subtabs
    int selector_position;
    int special_stats[7];
    int level;
    int health;
    int ap;
    int experience;
    char perks[10][50];
    SDL_Texture *special_animations[7][10]; // 10 frames per SPECIAL animation
} GameState;

GameState game_state;
void render_stat_subtabs(SDL_Renderer *renderer, TTF_Font *font, GameState *state);
void render_status_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state);
void render_special_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state);
void render_perks_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state);
void play_sound(const char *file) {
    Mix_Chunk *sound = Mix_LoadWAV(file);
    if (!sound) {
        fprintf(stderr, "Failed to load sound %s: %s\n", file, Mix_GetError());
        return;
    }
    Mix_PlayChannel(-1, sound, 0);
    while (Mix_Playing(-1)) {
        SDL_Delay(10);
    }
    Mix_FreeChunk(sound);
}

void play_animation(SDL_Renderer *renderer, SDL_Texture *frames[], int frame_count) {
    for (int i = 0; i < frame_count; i++) {
        if (frames[i]) {
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, frames[i], NULL, NULL);
            SDL_RenderPresent(renderer);
            SDL_Delay(1000 / FRAME_RATE);
        }
    }
}

void show_boot_animation(SDL_Renderer *renderer) {
    const int NUM_BOOTUP_FRAMES = 119;
    const int NUM_BOOTBOY_FRAMES = 14;

    SDL_Texture *bootup_frames[NUM_BOOTUP_FRAMES];
    SDL_Texture *bootboy_frames[NUM_BOOTBOY_FRAMES];

    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) bootup_frames[i] = NULL;
    for (int i = 0; i < NUM_BOOTBOY_FRAMES; i++) bootboy_frames[i] = NULL;

    char path[256];

    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) {
        snprintf(path, sizeof(path), "BOOT/BOOTUP/%d.jpg", i);
        SDL_Surface *surface = IMG_Load(path);
        if (!surface) continue;
        bootup_frames[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    for (int i = 0; i < NUM_BOOTBOY_FRAMES; i++) {
        snprintf(path, sizeof(path), "BOOT/BootBoy/%d.jpg", i);
        SDL_Surface *surface = IMG_Load(path);
        if (!surface) continue;
        bootboy_frames[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    play_animation(renderer, bootup_frames, NUM_BOOTUP_FRAMES);
    play_animation(renderer, bootboy_frames, NUM_BOOTBOY_FRAMES);

    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) {
        if (bootup_frames[i]) SDL_DestroyTexture(bootup_frames[i]);
    }
    for (int i = 0; i < NUM_BOOTBOY_FRAMES; i++) {
        if (bootboy_frames[i]) SDL_DestroyTexture(bootboy_frames[i]);
    }
}

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
        SDL_Rect dest_rect = {325, 150, 150, 150};
        SDL_RenderCopy(renderer, vaultboy_frames[vaultboy_frame_index], NULL, &dest_rect);
    }
}

void load_special_animations(SDL_Renderer *renderer, GameState *state) {
    const char *special_names[7] = {"strength", "perception", "endurance", "charisma", "intelligence", "agility", "luck"};
    char path[256];

    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 10; j++) {
            snprintf(path, sizeof(path), "STAT/%s/%02d.png", special_names[i], j); // Adjust path as needed
            SDL_Surface *surface = IMG_Load(path);
            if (!surface) continue;
            state->special_animations[i][j] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
        }
    }
}

void render_special_animation(SDL_Renderer *renderer, GameState *state) {
    int current_stat = state->selector_position; // Assume this controls which SPECIAL stat is highlighted
    static int frame_index = 0;
    frame_index = (frame_index + 1) % 10; // Cycle through 10 frames

    SDL_Texture *current_frame = state->special_animations[current_stat][frame_index];
    if (current_frame) {
        SDL_Rect dest_rect = {400, 50, 200, 200}; // Adjust position and size
        SDL_RenderCopy(renderer, current_frame, NULL, &dest_rect);
    }
}


void load_special_stats_from_csv(const char *file_path, GameState *state) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", file_path);
        return;
    }

    char line[256];
    int i = 0;
    while (fgets(line, sizeof(line), file) && i < 7) {
        sscanf(line, "%*[^,],%d", &state->special_stats[i]); // Skip the name, read the stat value
        i++;
    }
    fclose(file);
}
void render_health_bar(SDL_Renderer *renderer, int health) {
    SDL_Texture *bar = IMG_LoadTexture(renderer, "STAT/PIPBAR1.png");
    SDL_Rect bar_rect = {50, 100, 300 * (health / 100.0), 20};  // Adjust width based on health
    SDL_RenderCopy(renderer, bar, NULL, &bar_rect);
    SDL_DestroyTexture(bar);
}

void render_tabs(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    const char *tab_names[] = {"STAT", "INV", "DATA", "MAP", "RADIO"};
    SDL_Color color = {0, 255, 0, 255};

    // Render main tabs
    for (int i = 0; i < NUM_TABS; i++) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, tab_names[i], color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {50 + i * 150, 20, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Highlight active main tab
    SDL_Rect highlight_rect = {50 + state->current_tab * 150, 20, 150, 25};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &highlight_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // Render sub-tabs only if STAT is the active main tab
    if (state->current_tab == TAB_STAT) {
        render_stat_subtabs(renderer, font, state);
    }
}
void render_static_overlays(SDL_Renderer *renderer) {
    SDL_Texture *category_line = IMG_LoadTexture(renderer, "STAT/CATEGORYLINE.png");
    SDL_Rect line_rect = {0, 0, SCREEN_WIDTH, 10};  // Adjust dimensions and position
    SDL_RenderCopy(renderer, category_line, NULL, &line_rect);
    SDL_DestroyTexture(category_line);
}

void render_attribute_description(SDL_Renderer *renderer, TTF_Font *font, int selector_position) {
    const char *descriptions[] = {
        "Strength: Increases carrying capacity and melee damage.",
        "Perception: Affects weapon accuracy and environmental awareness.",
        "Endurance: Increases total health and stamina.",
        "Charisma: Improves dialogue options and social interactions.",
        "Intelligence: Enhances experience gain and hacking.",
        "Agility: Improves sneak and action points regeneration.",
        "Luck: Affects critical hits and loot chances."
    };

    SDL_Color color = {0, 255, 0, 255};
    SDL_Surface *desc_surface = TTF_RenderText_Solid(font, descriptions[selector_position], color);
    SDL_Texture *desc_texture = SDL_CreateTextureFromSurface(renderer, desc_surface);
    SDL_Rect desc_rect = {250, 350, desc_surface->w, desc_surface->h};
    SDL_RenderCopy(renderer, desc_texture, NULL, &desc_rect);
    SDL_FreeSurface(desc_surface);
    SDL_DestroyTexture(desc_texture);
}

void render_stat_tab(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    SDL_Color color = {0, 255, 0, 255};

    // Render the sub-tabs
    render_stat_subtabs(renderer, font, state);

    // Render content based on the active sub-tab
    switch (state->current_subtab) {
        case SUBTAB_STATUS:
            render_status_content(renderer, font, state);
            break;
        case SUBTAB_SPECIAL:
            render_special_content(renderer, font, state);
            break;
        case SUBTAB_PERKS:
            render_perks_content(renderer, font, state);
            break;
    }

    // Render general stats at the bottom
    char stats_text[100];
    snprintf(stats_text, sizeof(stats_text), "Health: %d  Level %d XP: %d AP: %d", state->health, state->level, state->experience, state->ap);
    SDL_Surface *stats_surface = TTF_RenderText_Solid(font, stats_text, color);
    SDL_Texture *stats_texture = SDL_CreateTextureFromSurface(renderer, stats_surface);
    SDL_Rect stats_rect = {50, 400, stats_surface->w, stats_surface->h};
    SDL_RenderCopy(renderer, stats_texture, NULL, &stats_rect);
    SDL_FreeSurface(stats_surface);
    SDL_DestroyTexture(stats_texture);
}
void render_status_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    SDL_Color color = {0, 255, 0, 255};

    // Render title
    const char *status_title = "STATUS: ";
    SDL_Surface *title_surface = TTF_RenderText_Solid(font, status_title, color);
    SDL_Texture *title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
    SDL_Rect title_rect = {50, 80, title_surface->w, title_surface->h};
    SDL_RenderCopy(renderer, title_texture, NULL, &title_rect);
    SDL_FreeSurface(title_surface);
    SDL_DestroyTexture(title_texture);

    // Render Vault Boy animation
    render_vaultboy(renderer);
}

void render_special_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    SDL_Color color = {0, 255, 0, 255};

    // Render title
    const char *special_title = "SPECIAL Attributes";
    SDL_Surface *title_surface = TTF_RenderText_Solid(font, special_title, color);
    SDL_Texture *title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
    SDL_Rect title_rect = {50, 80, title_surface->w, title_surface->h};
    SDL_RenderCopy(renderer, title_texture, NULL, &title_rect);
    SDL_FreeSurface(title_surface);
    SDL_DestroyTexture(title_texture);

    // Render SPECIAL attributes list
    const char *attributes[] = {"Strength", "Perception", "Endurance", "Charisma", "Intelligence", "Agility", "Luck"};
    char attribute_text[50];
    for (int i = 0; i < 7; i++) {
        snprintf(attribute_text, sizeof(attribute_text), "%s: %d", attributes[i], state->special_stats[i]);
        SDL_Surface *attr_surface = TTF_RenderText_Solid(font, attribute_text, color);
        SDL_Texture *attr_texture = SDL_CreateTextureFromSurface(renderer, attr_surface);
        SDL_Rect attr_rect = {50, 120 + i * 40, attr_surface->w, attr_surface->h};
        SDL_RenderCopy(renderer, attr_texture, NULL, &attr_rect);
        SDL_FreeSurface(attr_surface);
        SDL_DestroyTexture(attr_texture);

        // Highlight the selected attribute
        if (i == state->selector_position) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_Rect highlight_rect = {45, 120 + i * 40 - 5, 150, 30}; // Adjust dimensions for the highlight
            SDL_RenderDrawRect(renderer, &highlight_rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        }
    }

    // Render attribute description
    render_attribute_description(renderer, font, state->selector_position);
}

void render_perks_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    SDL_Color color = {0, 255, 0, 255};

    // Render title
    const char *perks_title = "PERKS: Placeholder Content";
    SDL_Surface *title_surface = TTF_RenderText_Solid(font, perks_title, color);
    SDL_Texture *title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
    SDL_Rect title_rect = {50, 80, title_surface->w, title_surface->h};
    SDL_RenderCopy(renderer, title_texture, NULL, &title_rect);
    SDL_FreeSurface(title_surface);
    SDL_DestroyTexture(title_texture);

    // Render perks list (if any perks exist, currently empty as placeholder)
    for (int i = 0; i < 10; i++) {
        if (strlen(state->perks[i]) > 0) {
            SDL_Surface *perk_surface = TTF_RenderText_Solid(font, state->perks[i], color);
            SDL_Texture *perk_texture = SDL_CreateTextureFromSurface(renderer, perk_surface);
            SDL_Rect perk_rect = {50, 120 + i * 40, perk_surface->w, perk_surface->h};
            SDL_RenderCopy(renderer, perk_texture, NULL, &perk_rect);
            SDL_FreeSurface(perk_surface);
            SDL_DestroyTexture(perk_texture);
        }
    }
}


void render_stat_subtabs(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    const char *subtab_names[] = {"STATUS", "SPECIAL", "PERKS"};
    SDL_Color color = {0, 255, 0, 255};

    for (int i = 0; i < NUM_SUBTABS; i++) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, subtab_names[i], color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {50 + i * 150, 60, surface->w, surface->h}; // Render below the main STAT tab
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    SDL_Rect highlight_rect = {50 + state->current_subtab * 150, 60, 150, 22};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &highlight_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
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
            // Main Tabs Navigation (Q for left, E for right)
            case SDLK_q:
                state->current_tab = (state->current_tab - 1 + NUM_TABS) % NUM_TABS;
                break;
            case SDLK_e:
                state->current_tab = (state->current_tab + 1) % NUM_TABS;
                break;

            // Sub-tabs Navigation within STAT (A for left, D for right)
            case SDLK_a:
                if (state->current_tab == TAB_STAT) {
                    state->current_subtab = (state->current_subtab - 1 + NUM_SUBTABS) % NUM_SUBTABS;
                }
                break;
            case SDLK_d:
                if (state->current_tab == TAB_STAT) {
                    state->current_subtab = (state->current_subtab + 1) % NUM_SUBTABS;
                }
                break;

            // SPECIAL Attributes Navigation (W for up, S for down)
            case SDLK_w:
                if (state->current_tab == TAB_STAT && state->current_subtab == SUBTAB_SPECIAL) {
                    state->selector_position = (state->selector_position - 1 + 7) % 7; // 7 attributes
                }
                break;
            case SDLK_s:
                if (state->current_tab == TAB_STAT && state->current_subtab == SUBTAB_SPECIAL) {
                    state->selector_position = (state->selector_position + 1) % 7;
                }
                break;

            // Add additional navigation logic if needed
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
    printf("Starting boot animation...\n");
    play_sound("Sounds/On.mp3");
    show_boot_animation(renderer);
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
