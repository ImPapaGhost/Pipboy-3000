#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <math.h>

void render_stat_subtabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    const char *subtab_names[] = {"STATUS", "SPECIAL", "PERKS"};
    SDL_Color color_active = {0, 255, 0, 255};   // Bright green for active subtab
    SDL_Color color_inactive = {0, 100, 0, 255}; // Dim for inactive subtabs

    TTF_Font *subtab_font = TTF_OpenFont("monofonto.ttf", 22); // Font size for subtabs

    int base_x = 205; // Base x-coordinate for the centered subtab
    int base_y = 45;  // Y-coordinate for subtabs

    // Calculate animation progress
    Uint32 current_time = SDL_GetTicks();
    float progress = 1.0f; // Default to fully completed animation
    if (state->is_animating) {
        progress = (float)(current_time - state->subtab_animation_start_time) / 300; // 300ms animation duration
        if (progress >= 1.0f) {
            progress = 1.0f;
            state->is_animating = false; // End animation
        }
    }

    float offset = (-1.0f + progress) * state->subtab_animation_offset; // Control direction of animation by linear interpolation

    // Render each subtab
    for (int i = 0; i < NUM_SUBTABS; i++) {
        int x_position = base_x + (i - state->current_subtab) * SUBTAB_SPACING + offset;

        SDL_Color current_color = (i == state->current_subtab) ? color_active : color_inactive;

        SDL_Surface *surface = TTF_RenderText_Solid(subtab_font, subtab_names[i], current_color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect rect = {x_position - surface->w / 2, base_y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    TTF_CloseFont(subtab_font);
}

void render_inv_subtabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    const char *subtab_names[] = {"WEAPONS", "APPAREL", "AID", "MISC", "JUNK", "MODS", "AMMO"};
    SDL_Color color_active = {0, 255, 0, 255};   // Bright green for active subtab
    SDL_Color color_inactive = {0, 100, 0, 255}; // Dim for inactive subtabs

    TTF_Font *subtab_font = TTF_OpenFont("monofonto.ttf", 22); // Font size for subtabs

    int base_y = 45;  // Y-coordinate for subtabs

    // Calculate widths of each subtab
    int subtab_widths[NUM_INV_SUBTABS];
    int total_width = 0;

    for (int i = 0; i < NUM_INV_SUBTABS; i++) {
        TTF_SizeText(subtab_font, subtab_names[i], &subtab_widths[i], NULL);
        total_width += subtab_widths[i] + 25; // Adding padding
    }
    total_width -= 25; // Remove extra padding at the end

    // Calculate offset so that the **selected** subtab is centered
    int center_x = 300;
    int selected_offset = 0;
    for (int i = 0; i < state->current_inv_subtab; i++) {
        selected_offset += subtab_widths[i] + 25;
    }

    int base_x = center_x - (selected_offset + (subtab_widths[state->current_inv_subtab] / 2));

    // Calculate animation progress
    Uint32 current_time = SDL_GetTicks();
    float progress = 1.0f; // Default to fully completed animation
    if (state->is_inv_animating) {
        progress = (float)(current_time - state->inv_subtab_animation_start_time) / 300; // 300ms animation duration
        if (progress >= 1.0f) {
            progress = 1.0f;
            state->is_inv_animating = false; // End animation
        }
    }

    float offset = (-1.0f + progress) * state->inv_subtab_animation_offset; // Animation shift

    // Render each subtab
    int current_x = base_x;
    for (int i = 0; i < NUM_INV_SUBTABS; i++) {
        int x_position = current_x + offset;

        SDL_Color current_color = (i == state->current_inv_subtab) ? color_active : color_inactive;

        SDL_Surface *surface = TTF_RenderText_Solid(subtab_font, subtab_names[i], current_color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect rect = {x_position, base_y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);

        current_x += subtab_widths[i] + 25; // Move to the next subtab
    }

    TTF_CloseFont(subtab_font);
}

void render_data_subtabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    const char *subtab_names[] = {"QUESTS", "WORKSHOPS", "STATS"};
    SDL_Color color_active = {0, 255, 0, 255};   // Bright green for active subtab
    SDL_Color color_inactive = {0, 100, 0, 255}; // Dim for inactive subtabs
    TTF_Font *subtab_font = TTF_OpenFont("monofonto.ttf", 22); // Font size for subtabs
    // Calculate widths of each subtab
    int subtab_widths[NUM_DATA_SUBTABS];
    int total_width = 0;
    for (int i = 0; i < NUM_DATA_SUBTABS; i++) {
        TTF_SizeText(subtab_font, subtab_names[i], &subtab_widths[i], NULL);
        total_width += subtab_widths[i] + 25; // Adding padding
    }
    total_width -= 25; // Remove extra padding at the end
    // Calculate offset so that the **selected** subtab is centered
    int center_x = 395;
    int selected_offset = 0;
    for (int i = 0; i < state->current_data_subtab; i++) {
        selected_offset += subtab_widths[i] + 25;
    }

    int base_x = center_x - (selected_offset + (subtab_widths[state->current_data_subtab] / 2));

    int base_y = 45;

    // Calculate animation progress
    Uint32 current_time = SDL_GetTicks();
    float progress = 1.0f; // Default to fully completed animation
    if (state->is_data_animating) {
        progress = (float)(current_time - state->data_subtab_animation_start_time) / 300; // 300ms animation duration
        if (progress >= 1.0f) {
            progress = 1.0f;
            state->is_data_animating = false; // End animation
        }
    }

    float offset = (-1.0f + progress) * state->data_subtab_animation_offset;

    // Render each subtab
    int current_x = base_x;
    for (int i = 0; i < NUM_DATA_SUBTABS; i++) {
        int x_position = current_x + offset;

        SDL_Color current_color = (i == state->current_data_subtab) ? color_active : color_inactive;

        SDL_Surface *surface = TTF_RenderText_Solid(subtab_font, subtab_names[i], current_color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect rect = {x_position, base_y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);

        current_x += subtab_widths[i] + 25; // Move to the next subtab
    }

    TTF_CloseFont(subtab_font);
}

void render_quests(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    SDL_Color color_active = {0, 255, 0, 255}; // Bright green for selected quest
    SDL_Color color_inactive = {0, 100, 0, 255}; // Dim for unselected

    int x = 100, y = 120, spacing = 30;

    for (int i = 0; i < state->quest_count; i++) {
        SDL_Color current_color = (state->current_quest == i) ? color_active : color_inactive;

        SDL_Surface *surface = TTF_RenderText_Solid(font, state->quests[i].name, current_color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {x, y + (i * spacing), surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Render Quest Description on the Right
    SDL_Color desc_color = {0, 255, 0, 255};
    SDL_Surface *desc_surface = TTF_RenderText_Blended_Wrapped(font, state->quests[state->current_quest].description, desc_color, 300);
    SDL_Texture *desc_texture = SDL_CreateTextureFromSurface(renderer, desc_surface);
    SDL_Rect desc_rect = {400, 120, desc_surface->w, desc_surface->h};
    SDL_RenderCopy(renderer, desc_texture, NULL, &desc_rect);

    SDL_FreeSurface(desc_surface);
    SDL_DestroyTexture(desc_texture);
}

void render_workshops(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    SDL_Color color_active = {0, 255, 0, 255}; // Bright green for selected workshop
    SDL_Color color_inactive = {0, 100, 0, 255}; // Dim for unselected

    int x = 100, y = 120, spacing = 30;

    // Render Workshop List
    for (int i = 0; i < state->workshop_count; i++) {
        SDL_Color current_color = (state->current_workshop == i) ? color_active : color_inactive;

        SDL_Surface *surface = TTF_RenderText_Solid(font, state->workshops[i].name, current_color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {x, y + (i * spacing), surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Render Workshop Details on the Right
    Workshop *selected = &state->workshops[state->current_workshop];

    char details[256];
    snprintf(details, sizeof(details), 
             "Population: %d\nFood: %d\nWater: %d\nPower: %d\nDefense: %d\nBeds: %d\nHappiness: %d%%",
             selected->population, selected->food, selected->water, 
             selected->power, selected->defense, selected->beds, 
             selected->happiness);

    SDL_Color desc_color = {0, 255, 0, 255};
    SDL_Surface *desc_surface = TTF_RenderText_Blended_Wrapped(font, details, desc_color, 300);
    SDL_Texture *desc_texture = SDL_CreateTextureFromSurface(renderer, desc_surface);
    SDL_Rect desc_rect = {400, 120, desc_surface->w, desc_surface->h};
    SDL_RenderCopy(renderer, desc_texture, NULL, &desc_rect);

    SDL_FreeSurface(desc_surface);
    SDL_DestroyTexture(desc_texture);
}


void render_stats(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    SDL_Color color_active = {0, 255, 0, 255}; // Bright green for selected stat
    SDL_Color color_inactive = {0, 100, 0, 255}; // Dim for unselected

    int x = 100, y = 120, spacing = 30;

    // Render Stats List
    for (int i = 0; i < state->stats_count; i++) {
        SDL_Color current_color = (state->current_stat == i) ? color_active : color_inactive;

        char stat_display[100];
        snprintf(stat_display, sizeof(stat_display), "%s: %d", state->stats[i].name, state->stats[i].value);

        SDL_Surface *surface = TTF_RenderText_Solid(font, stat_display, current_color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {x, y + (i * spacing), surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Render Stat Description on the Right
    PlayerStat *selected = &state->stats[state->current_stat];

    SDL_Color desc_color = {0, 255, 0, 255};
    SDL_Surface *desc_surface = TTF_RenderText_Blended_Wrapped(font, selected->description, desc_color, 300);
    SDL_Texture *desc_texture = SDL_CreateTextureFromSurface(renderer, desc_surface);
    SDL_Rect desc_rect = {400, 120, desc_surface->w, desc_surface->h};
    SDL_RenderCopy(renderer, desc_texture, NULL, &desc_rect);

    SDL_FreeSurface(desc_surface);
    SDL_DestroyTexture(desc_texture);
}

void render_data_tab(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    render_data_subtabs(renderer, font, state);

    switch (state->current_data_subtab) {
        case SUBTAB_QUESTS:
            render_quests(renderer, font, &pip_state);
            break;
        case SUBTAB_WORKSHOPS:
            render_workshops(renderer, font, state);
            break;
        case SUBTAB_STATS:
            render_stats(renderer, font, state);
            break;
    }
}

void render_mid_background(SDL_Renderer *renderer, PipState *state) {
    SDL_Texture *background = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");
    if (!background) return;

    // Default XP bar settings (STAT tab)
    int bg_width = 300;
    int bg_x = (SCREEN_WIDTH / 2) - (bg_width / 2);

    // Adjust width and position in DATA tab
    if (state->current_tab == TAB_DATA) {
        bg_width = 100;  // Adjusted width
        bg_x = (SCREEN_WIDTH / 2) - (bg_width / 2) - 100;  // Move slightly left
    }

    SDL_Rect background_rect = {bg_x, 430, bg_width, 30};

    SDL_SetTextureColorMod(background, 0, 255, 0);
    SDL_RenderCopy(renderer, background, NULL, &background_rect);
    SDL_DestroyTexture(background);
}

void render_damage_bar(SDL_Renderer *renderer, int x, int y, int width, int height, int health) {
    // Draw the background (gray bar)
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Dark gray
    SDL_Rect background_rect = {x, y, width, height};
    SDL_RenderFillRect(renderer, &background_rect);

    // Draw the health portion (green to red based on health percentage)
    SDL_Color bar_color = {255 - (2.55 * health), 2.55 * health, 0, 255}; // Gradient
    SDL_SetRenderDrawColor(renderer, bar_color.r, bar_color.g, bar_color.b, bar_color.a);
    SDL_Rect health_rect = {x, y, (width * health) / 100, height};
    SDL_RenderFillRect(renderer, &health_rect);

    // Draw the border (bright green)
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &background_rect);

    // Reset render color to black for safety
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}