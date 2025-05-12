#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h> // Required for checking file existence
#include <unistd.h>
#include <math.h>
#include "pipboy.h"
#include "inventory.h"
#include "render.h"
#include "animations.h"
#include "input.h"
#include "ui.h"
#include "events.h"
#include "MAP/map.h"


#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define FRAME_RATE 60
#define NUM_VAULTBOY_FRAMES 8 // Total frames for VaultBoy animation
#define SUBTAB_SPACING 80

// Utility functions
SDL_Texture *vaultboy_frames[NUM_VAULTBOY_FRAMES];
Uint32 last_vaultboy_update = 0; // Time tracker for animation updates
int file_exists(const char *path);
SDL_Texture *vaultboy_frames[NUM_VAULTBOY_FRAMES]; // Define the vaultboy frames
int vaultboy_frame_index = 0;  // Define the frame index for animations

typedef struct {
    SDL_Texture *texture;
    SDL_Rect rect;
} Sprite;

void render_status_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_special_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_perks_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void play_sound(const char *file) {
    Mix_Chunk *sound = Mix_LoadWAV(file);
    Mix_PlayChannel(-1, sound, 0);
    while (Mix_Playing(-1)) {
        SDL_Delay(10);
    }
    Mix_FreeChunk(sound);
}


// Initialize damage bars with full health
DamageBars damage_bars = {100, 100, 100, 100, 100, 100};

void show_boot_animation(SDL_Renderer *renderer) {
    const int NUM_BOOTUP_FRAMES = 119;  // Number of bootup frames
    const int NUM_BOOTBOY_FRAMES = 14; // Number of bootboy frames

    SDL_Texture *bootup_frames[NUM_BOOTUP_FRAMES];
    SDL_Texture *bootboy_frames[NUM_BOOTBOY_FRAMES];

    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) bootup_frames[i] = NULL;
    for (int i = 0; i < NUM_BOOTBOY_FRAMES; i++) bootboy_frames[i] = NULL;

    char path[256];

    // Load bootup frames
    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) {
        snprintf(path, sizeof(path), "BOOT/BOOTUP/%d.jpg", i);
        SDL_Surface *surface = IMG_Load(path);
        if (!surface) continue;
        bootup_frames[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    // Load bootboy frames
    for (int i = 0; i < NUM_BOOTBOY_FRAMES; i++) {
        snprintf(path, sizeof(path), "BOOT/BootBoy/%d.jpg", i);
        SDL_Surface *surface = IMG_Load(path);
        if (!surface) continue;
        bootboy_frames[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    // Play bootup animation with a slower frame delay (80ms)
    play_animation(renderer, bootup_frames, NUM_BOOTUP_FRAMES, 80);

    // Play bootboy animation with a slower frame delay (120ms)
    play_animation(renderer, bootboy_frames, NUM_BOOTBOY_FRAMES, 120);

    // Free bootup textures
    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) {
        if (bootup_frames[i]) SDL_DestroyTexture(bootup_frames[i]);
    }

    // Free bootboy textures
    for (int i = 0; i < NUM_BOOTBOY_FRAMES; i++) {
        if (bootboy_frames[i]) SDL_DestroyTexture(bootboy_frames[i]);
    }
}

SDL_Texture *selectline_texture = NULL;

void load_selectline(SDL_Renderer *renderer) {
    SDL_Surface *surface = IMG_Load("STAT/SELECTLINE.jpg");
    if (surface) {
        selectline_texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
}

SDL_Texture *categoryline_texture = NULL;

void load_categoryline(SDL_Renderer *renderer) {
    SDL_Surface *surface = IMG_Load("STAT/CATEGORYLINE.jpg");
    if (surface) {
        categoryline_texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
}

void load_special_animation(SDL_Renderer *renderer, PipState *state) {
    const char *special_names[7] = {"Strength", "Perception", "Endurance", "Charisma", "Intelligence", "Agility", "Luck"};
    char path[256];

    for (int i = 0; i < 7; i++) {
        int frame_count = 0; // Counter for available frames
        int frame_number = 0; // Start checking frames from 0 upwards

        while (frame_count < 10) { // Load up to 10 frames per animation
            snprintf(path, sizeof(path), "STAT/%s/%d.jpg", special_names[i], frame_number);
            if (file_exists(path)) {
                SDL_Surface *surface = IMG_Load(path);
                if (!surface) {
                    state->special_animations[i][frame_count++] = NULL;
                } else {
                    state->special_animations[i][frame_count++] = SDL_CreateTextureFromSurface(renderer, surface);
                    SDL_FreeSurface(surface);
                }
            } else if (frame_number > 30) { // Stop searching if the file is missing after 50 attempts
                break;
            }
            frame_number++;
        }

        // Fill remaining slots with NULL
        for (int j = frame_count; j < 10; j++) {
            state->special_animations[i][j] = NULL;
        }
    }
}

void load_special_stats_from_csv(const char *file_path, PipState *state) {
    FILE *file = fopen(file_path, "r");

    char line[256];
    int i = 0;
    while (fgets(line, sizeof(line), file) && i < 7) {
        sscanf(line, "%*[^,],%.1f,%d", &state->special_stats[i]); // Skip the name, read the stat value
        i++;
    }
    fclose(file);
}

void render_tabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    const char *tab_names[] = {"STAT", "INV", "DATA", "MAP", "RADIO"};
    SDL_Color color = {0, 255, 0, 255}; // Green color for tab text
    int tab_spacing = 45; // Space between tabs

    // Load a larger font specifically for tabs
    TTF_Font *tab_font = TTF_OpenFont("monofonto.ttf", 28); // Larger font size for tabs

    // Calculate starting x-coordinate for centering
    // Calculate text widths for each tab
    int tab_text_widths[NUM_TABS];
    for (int i = 0; i < NUM_TABS; i++) {
        TTF_SizeText(tab_font, tab_names[i], &tab_text_widths[i], NULL);
    }

    // Calculate total width needed to align tabs by last letter
    int total_tab_width = 0;
    for (int i = 0; i < NUM_TABS; i++) {
        total_tab_width += tab_text_widths[i] + tab_spacing;
    }
    total_tab_width -= tab_spacing; // Remove last extra spacing

    int tab_x = (SCREEN_WIDTH - total_tab_width) / 2; // Center-align tabs
    int tab_y = 5; // Y position of the tabs

    // Render the CATEGORYLINE graphic (background line for all tabs)
    if (categoryline_texture) {
        SDL_Rect categoryline_rect = {
            tab_x - 90,                // Adjust to extend slightly to the left
            tab_y - -32,               // Adjust to appear slightly below the tabs
            total_tab_width + 175,     // Adjust to match the width of tabs
            12                         // Height with padding
        };
        SDL_SetTextureColorMod(categoryline_texture, 0, 255, 0); // Bright green for active tab
        SDL_RenderCopy(renderer, categoryline_texture, NULL, &categoryline_rect);
    }

    // Render the SELECTLINE graphic around the active tab
    if (selectline_texture) {
        int active_tab_x = tab_x;
        for (int i = 0; i < state->current_tab; i++) {
            active_tab_x += tab_text_widths[i] + tab_spacing;
        }

        SDL_Rect selectline_rect = {
            active_tab_x - 10, // Slight offset for alignment
            tab_y + 11,        // Position slightly above the tab text
            tab_text_widths[state->current_tab] + 18, // Adjust width to match the tab
            26                // Height to cover the tab area
        };
        SDL_SetTextureColorMod(selectline_texture, 0, 255, 0); // Bright green for active tab
        SDL_RenderCopy(renderer, selectline_texture, NULL, &selectline_rect);
    }

    // Render each tab text on top of CATEGORYLINE and SELECTLINE
    int current_x = tab_x;
    for (int i = 0; i < NUM_TABS; i++) {
        SDL_Surface *surface = TTF_RenderText_Solid(tab_font, tab_names[i], color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect text_rect = {
            current_x,
            tab_y,
            surface->w,
            surface->h
        };
        SDL_RenderCopy(renderer, texture, NULL, &text_rect);
        current_x += tab_text_widths[i] + tab_spacing;
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Close the larger font after rendering
    TTF_CloseFont(tab_font);
}

void render_attribute_description(SDL_Renderer *renderer, TTF_Font *font, int selector_position) {
    // Manually pre-split descriptions into lines
    const char *descriptions[][5] = {
        {"Strength is a measure of your raw physical power.",
         "It affects how much you can carry and",
         "determines the effectiveness of melee attacks."},

        {"Perception is your environmental awareness",
         "and 'sixth sense,' and affects weapon",
         "accuracy in V.A.T.S."},

        {"Endurance is a measure of your overall",
         "physical fitness. It affects your total health,",
         "and your resistance to damage and radiation."},

        {"Charisma is your ability to charm and convince.",
         "It affects your success in persuasion and",
         "prices when you barter."},

        {"Intelligence is a measure of your mental acuity.",
         "It affects the number of Experience Points",
         "earned."},

        {"Agility is a measure of your finesse and reflexes.",
         "It affects the number of action points in V.A.T.S.",
         "and your ability to sneak."},

        {"Luck is a measure of your general good fortune.",
         "It affects the recharge rate of critical hits."}
    };

    SDL_Color color = {0, 255, 0, 255};

    // Render each line of the description
    for (int i = 0; i < 5; i++) {
        if (descriptions[selector_position][i] == NULL) {
            break; // Stop rendering if there are no more lines
        }

        SDL_Surface *line_surface = TTF_RenderText_Solid(font, descriptions[selector_position][i], color);
        SDL_Texture *line_texture = SDL_CreateTextureFromSurface(renderer, line_surface);

        // Adjust the position of each line
        SDL_Rect line_rect = {250, 350 + i * 25, line_surface->w, line_surface->h}; // Y-coordinate increments by 25 for each line
        SDL_RenderCopy(renderer, line_texture, NULL, &line_rect);

        // Clean up resources for this line
        SDL_FreeSurface(line_surface);
        SDL_DestroyTexture(line_texture);
    }
}

void render_stat_tab(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
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
    // Create a larger font for HP text
    TTF_Font *hp_font = TTF_OpenFont("monofonto.ttf", 20); // Adjust size as needed
    char hp_text[20];
    snprintf(hp_text, sizeof(hp_text), "HP %d/%d", state->health, state->max_health);
    SDL_Surface *hp_surface = TTF_RenderText_Solid(hp_font, hp_text, color);
    SDL_Texture *hp_texture = SDL_CreateTextureFromSurface(renderer, hp_surface);
    SDL_Rect hp_rect = {115, 432, hp_surface->w, hp_surface->h}; // Left aligned
    SDL_RenderCopy(renderer, hp_texture, NULL, &hp_rect);
    SDL_FreeSurface(hp_surface);
    SDL_DestroyTexture(hp_texture);
    TTF_CloseFont(hp_font); // Clean up the HP font

    TTF_Font *ap_font = TTF_OpenFont("monofonto.ttf", 20); // Adjust size as needed
    char ap_text[20];
    snprintf(ap_text, sizeof(ap_text), "AP %d/%d", state->ap, state->max_ap);
    SDL_Surface *ap_surface = TTF_RenderText_Solid(ap_font, ap_text, color);
    SDL_Texture *ap_texture = SDL_CreateTextureFromSurface(renderer, ap_surface);
    SDL_Rect ap_rect = {SCREEN_WIDTH - ap_surface->w - 110, 432, ap_surface->w, ap_surface->h}; // Right aligned
    SDL_RenderCopy(renderer, ap_texture, NULL, &ap_rect);
    SDL_FreeSurface(ap_surface);
    SDL_DestroyTexture(ap_texture);
    TTF_CloseFont(ap_font); // Clean up the AP font


}

void render_status_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    SDL_Color color = {0, 255, 0, 255};

    // Render Vault Boy animation
    render_vaultboy(renderer);

     // Render Stimpak Background
    SDL_Texture *stimpak_background = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");
    SDL_Rect stimpak_rect = {110, 395, 100, 30}; // Adjust based on position and size
    SDL_SetTextureColorMod(stimpak_background, 100, 255, 100); // Green tint
    SDL_RenderCopy(renderer, stimpak_background, NULL, &stimpak_rect);
    SDL_DestroyTexture(stimpak_background);

    // Render RadAway Background
    SDL_Texture *radaway_background = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");
    SDL_Rect radaway_rect = {225, 395, 100, 30}; // Adjust position to the right of Stimpak
    SDL_SetTextureColorMod(radaway_background, 100, 255, 100); // Green tint
    SDL_RenderCopy(renderer, radaway_background, NULL, &radaway_rect);
    SDL_DestroyTexture(radaway_background);

    // Render Stimpak Text
    char stimpak_text[20];
    snprintf(stimpak_text, sizeof(stimpak_text), "Stimpak (%d)", state->stimpaks);
    SDL_Surface *stimpak_surface = TTF_RenderText_Solid(font, stimpak_text, color);
    SDL_Texture *stimpak_texture = SDL_CreateTextureFromSurface(renderer, stimpak_surface);
    SDL_Rect stimpak_text_rect = {115, 400, stimpak_surface->w, stimpak_surface->h}; // Center inside background
    SDL_RenderCopy(renderer, stimpak_texture, NULL, &stimpak_text_rect);
    SDL_FreeSurface(stimpak_surface);
    SDL_DestroyTexture(stimpak_texture);

    // Render RadAway Text
    char radaway_text[20];
    snprintf(radaway_text, sizeof(radaway_text), "RadAway (%d)", state->radaways);
    SDL_Surface *radaway_surface = TTF_RenderText_Solid(font, radaway_text, color);
    SDL_Texture *radaway_texture = SDL_CreateTextureFromSurface(renderer, radaway_surface);
    SDL_Rect radaway_text_rect = {230, 400, radaway_surface->w, radaway_surface->h}; // Center inside background
    SDL_RenderCopy(renderer, radaway_texture, NULL, &radaway_text_rect);
    SDL_FreeSurface(radaway_surface);
    SDL_DestroyTexture(radaway_texture);
}

void render_special_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    SDL_Color color = {0, 255, 0, 255};
    Uint32 current_time = SDL_GetTicks();
    float animation_progress = 1.0f; // Default to fully completed animation

    if (state->is_special_stat_animating) {
        animation_progress = (float)(current_time - state->special_stat_animation_start) / 300; // 300ms duration
        if (animation_progress >= 1.0f) {
            animation_progress = 1.0f;
            state->is_special_stat_animating = false; // Mark animation as complete
        }
    }

    // Use cubic easing for smoother transitions
    float vertical_offset = state->special_stat_animation_offset * (-1 + ease_out_cubic(animation_progress));

    // Render SPECIAL attributes list
    const char *attributes[] = {"Strength", "Perception", "Endurance", "Charisma", "Intelligence", "Agility", "Luck"};
    char attribute_text[50];
    int stat_x = 130; // X position for attribute names
    int value_x = 360; // X position for stat values
    int y_start = 100; // Starting Y position
    int y_spacing = 40; // Spacing between rows

    for (int i = 0; i < 7; i++) {
        // Render attribute name
        SDL_Surface *attr_surface = TTF_RenderText_Solid(font, attributes[i], color);
        SDL_Texture *attr_texture = SDL_CreateTextureFromSurface(renderer, attr_surface);
        SDL_Rect attr_rect = {stat_x, y_start + i * y_spacing, attr_surface->w, attr_surface->h};
        SDL_RenderCopy(renderer, attr_texture, NULL, &attr_rect);
        SDL_FreeSurface(attr_surface);
        SDL_DestroyTexture(attr_texture);

        // Render attribute value
        snprintf(attribute_text, sizeof(attribute_text), "%d", state->special_stats[i]);
        SDL_Surface *value_surface = TTF_RenderText_Solid(font, attribute_text, color);
        SDL_Texture *value_texture = SDL_CreateTextureFromSurface(renderer, value_surface);
        SDL_Rect value_rect = {value_x, y_start + i * y_spacing, value_surface->w, value_surface->h};
        SDL_RenderCopy(renderer, value_texture, NULL, &value_rect);
        SDL_FreeSurface(value_surface);
        SDL_DestroyTexture(value_texture);
    }

    // Render moving highlight box
    int highlight_y = y_start + state->selector_position * y_spacing + vertical_offset; // Calculate the highlight's Y position
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green color for the highlight
    SDL_Rect highlight_rect = {stat_x - 10, highlight_y - 5, value_x - stat_x + 50, y_spacing - 5};
    SDL_RenderDrawRect(renderer, &highlight_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Reset color

    // Render description (stays static)
    const char *descriptions[] = {
        "Strength is a measure of your raw physical power. It affects how much you can carry and determines the effectiveness of melee attacks.",
        "Perception is your environmental awareness and 'sixth sense,' and affects weapon accuracy in V.A.T.S.",
        "Endurance is a measure of your overall physical fitness. It affects your total health, and your resistance to damage and radiation.",
        "Charisma is your ability to charm and convince. It affects your success in persuasion and prices when you barter.",
        "Intelligence is a measure of your mental acuity. It affects the number of Experience Points earned.",
        "Agility is a measure of your finesse and reflexes. It affects the number of action points in V.A.T.S. and your ability to sneak.",
        "Luck is a measure of your general good fortune. It affects the recharge rate of critical hits."
    };

    SDL_Surface *desc_surface = TTF_RenderText_Blended_Wrapped(font, descriptions[state->selector_position], color, 300); // 300 = max width
    SDL_Texture *desc_texture = SDL_CreateTextureFromSurface(renderer, desc_surface);

    SDL_Rect desc_rect = {410, 300, desc_surface->w, desc_surface->h}; // Description stays fixed
    SDL_RenderCopy(renderer, desc_texture, NULL, &desc_rect);

    SDL_FreeSurface(desc_surface);
    SDL_DestroyTexture(desc_texture);
}

void render_perks_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    SDL_Color color = {0, 255, 0, 255};

    // Render title
    const char *perks_title = "PERKS: Placeholder Content";
    SDL_Surface *title_surface = TTF_RenderText_Solid(font, perks_title, color);
    SDL_Texture *title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
    SDL_Rect title_rect = {50, 100, title_surface->w, title_surface->h};
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

void render_map_tab(SDL_Renderer *renderer, TTF_Font *font) {
    map_render(renderer); // Calls your new map engine's render function
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

void render_current_tab(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    switch (state->current_tab) {
        case TAB_STAT:
            render_stat_tab(renderer, font, state);
            break;
        case TAB_INV:
            render_inv(renderer, font, state);
            break;
        case TAB_DATA:
            render_data_tab(renderer, font, state);
            break;
        case TAB_MAP:
            render_map_tab(renderer, font);
            break;
        case TAB_RADIO:
            render_radio_tab(renderer, font);
            break;
    }
}

int main(int argc, char *argv[]) {
    SDL_Window *window = NULL; // Declare window locally
    SDL_Renderer *renderer = NULL; // Declare renderer locally

    // Redirect stdout and stderr to debug.log
    freopen("debug.log", "w", stdout);
    freopen("debug.log", "a", stderr);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        return 1;
    }

    // Create SDL window
    window = SDL_CreateWindow(
        "Pip-Boy 3000",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        SDL_Quit();
        return 1;
    }

    // Create SDL renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load font
    TTF_Font *font = TTF_OpenFont("monofonto.ttf", 16);
    if (!font) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load resources and initialize pip state
    printf("Starting boot animation...\n");
    play_sound("Sounds/On.mp3");
    show_boot_animation(renderer);
    initialize_pip_state(&pip_state);
    load_vaultboy_frames(renderer);
    load_special_animation(renderer, &pip_state);
    load_selectline(renderer);
    load_categoryline(renderer);
    map_init(renderer);

    // Main pip loop
    bool running = true;
    SDL_Event event;
    Uint32 last_frame_time = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else {
                capture_input(&event); // Store the input in the queue
            }
        }

        handle_navigation(&pip_state); // Process input queue (One input per frame)

        // Frame update
        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_frame_time > 100) {
            vaultboy_frame_index = (vaultboy_frame_index + 1) % NUM_VAULTBOY_FRAMES;
            last_frame_time = current_time;
        }

        // Render
        SDL_RenderClear(renderer);
        render_health_background(renderer); // Render HP bar
        render_ap_bar(renderer, &pip_state);           // Render AP bar
        render_mid_background(renderer, &pip_state); // Render XP bar
        render_tabs(renderer, font, &pip_state); // Render the tabs with the selector line
        render_current_tab(renderer, font, &pip_state); // Render active tab content
        render_date_time(renderer, font, &pip_state);

        if (pip_state.current_tab == TAB_STAT && pip_state.current_subtab == SUBTAB_SPECIAL) {
            render_special_animation(renderer, &pip_state); // Render SPECIAL animations if applicable
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / FRAME_RATE);
    }

    // Cleanup resources
    if (pip_state.weapons) {
    free(pip_state.weapons);
    }

    if (pip_state.apparel) {
        free(pip_state.apparel);
    }

    if (pip_state.aid) {
        free(pip_state.aid);
    }

    if (pip_state.misc) {
        free(pip_state.misc);
    }

    if (pip_state.junk) {
        free(pip_state.junk);
    }

    if (pip_state.mods) {
        free(pip_state.mods);
    }

    if (pip_state.ammo) {
        free(pip_state.ammo);
    }

    if (pip_state.quests) {
        free(pip_state.quests);
    }

    if (pip_state.workshops) {
        free(pip_state.workshops);
    }

    if (pip_state.stats) {
        free(pip_state.stats);
    }

    map_shutdown();
    free_vaultboy_frames();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
