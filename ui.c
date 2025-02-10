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
    int center_x = (SCREEN_WIDTH / 2) - 100;
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



void render_level_xp_background(SDL_Renderer *renderer, PipState *state) {
    // Load the decorative texture for the background
    SDL_Texture *background = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");

    // Define the position and size of the decorative box
    int bg_width = 300;  // Width of the decorative box
    int bg_height = 30;  // Height of the decorative box
    int bg_x = (SCREEN_WIDTH / 2) - (bg_width / 2); // Center horizontally
    int bg_y = 430;       // Position above AP box but below HP text
    SDL_Rect background_rect = {bg_x, bg_y, bg_width, bg_height};

    // Tint the decorative box green
    SDL_SetTextureColorMod(background, 0, 255, 0);

    // Render the decorative box
    SDL_RenderCopy(renderer, background, NULL, &background_rect);
    SDL_DestroyTexture(background); // Free the texture after rendering

    // Define XP bar dimensions (inside the decorative box)
    int bar_width = bg_width - 100; // Add padding inside the box
    int bar_height = 10;           // Height of the XP bar
    int bar_x = bg_x + 90;         // Padding inside the box
    int bar_y = bg_y + 10;         // Center vertically inside the box

    // Calculate XP progress
    float xp_progress = (float)(state->current_xp) / state->xp_for_next_level;
    if (xp_progress > 1.0f) xp_progress = 1.0f;

    // Draw XP bar background (dark green)
    SDL_SetRenderDrawColor(renderer, 0, 50, 0, 255);
    SDL_Rect bar_background_rect = {bar_x, bar_y, bar_width, bar_height};
    SDL_RenderFillRect(renderer, &bar_background_rect);

    // Draw XP bar fill (bright green)
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect bar_fill_rect = {bar_x, bar_y, (int)(bar_width * xp_progress), bar_height};
    SDL_RenderFillRect(renderer, &bar_fill_rect);

    // Draw XP bar border
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &bar_background_rect);

    // Render LEVEL and XP text around the bar
    TTF_Font *font = TTF_OpenFont("monofonto.ttf", 20);
    SDL_Color color = {0, 255, 0, 255}; // Bright green text

    // Render LEVEL text
    char level_text[20];
    snprintf(level_text, sizeof(level_text), "LEVEL %d", state->level);
    SDL_Surface *level_surface = TTF_RenderText_Solid(font, level_text, color);
    SDL_Texture *level_texture = SDL_CreateTextureFromSurface(renderer, level_surface);
    SDL_Rect level_rect = {bg_x + 5, bg_y + 3, level_surface->w, level_surface->h}; // Positioned clearly above the XP bar
    SDL_RenderCopy(renderer, level_texture, NULL, &level_rect);
    SDL_FreeSurface(level_surface);
    SDL_DestroyTexture(level_texture);

    TTF_CloseFont(font);

    // Reset render color to default
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
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