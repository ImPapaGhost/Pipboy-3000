#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "pipboy.h"
#include "state.h"
#include "ui.h"



int file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// Easing subtab animation
float ease_out_cubic(float t) {
    return 1 - pow(1 - t, 3);
}

void render_inv(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    SDL_Color color = {0, 255, 0, 255};         // Normal bright green
    SDL_Color dimmed_color = {0, 80, 0, 255};  // Dimmed green for highlighted text
    int x = 100, y = 120;
    int spacing = 30;

    render_inv_subtabs(renderer, font, state);

    invItem *current_list = NULL;
    int current_count = 0;

    // Determine which list to display
    switch (state->current_inv_subtab) {
        case SUBTAB_WEAPONS:
            current_list = state->weapons;
            current_count = state->weapons_count;
            break;
        case SUBTAB_APPAREL:
            current_list = state->apparel;
            current_count = state->apparel_count;
            break;
        case SUBTAB_AID:
            current_list = state->aid;
            current_count = state->aid_count;
            break;
    }

    if (current_count == 0) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, "No items in this category.", color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        return;
    }

    for (int i = state->inv_scroll_index; i < current_count && i < state->inv_scroll_index + 10; i++) {
        // Format the item name with quantity
        char name_with_quantity[100];
        snprintf(name_with_quantity, sizeof(name_with_quantity), "%s (%d)", current_list[i].name, current_list[i].quantity);

        // Determine text color (dimmed if selected)
        SDL_Color text_color = (i == state->selector_position) ? dimmed_color : color;

        // Render the name (including quantity in parentheses)
        SDL_Surface *name_surface = TTF_RenderText_Solid(font, name_with_quantity, text_color);
        SDL_Texture *name_texture = SDL_CreateTextureFromSurface(renderer, name_surface);
        SDL_Rect name_rect = {x, y, name_surface->w, name_surface->h};

        // Highlight the selected item dynamically based on text width
        if (i == state->selector_position) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Bright green highlight
            SDL_Rect highlight_rect = {x - 10, y - 5, name_surface->w + 20, name_surface->h + 10};
            SDL_RenderFillRect(renderer, &highlight_rect);  // Filled highlight box
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        }

        // Render text on top of the highlight (selected gets dimmed green)
        SDL_RenderCopy(renderer, name_texture, NULL, &name_rect);

        SDL_FreeSurface(name_surface);
        SDL_DestroyTexture(name_texture);

        // Render the weight in a specific location for the selected item
        if (i == state->selector_position) {
            int weight_x = 600;  // Adjust to lower right corner
            int weight_y = 300;  // Adjust to lower right corner

            char weight_text[20];
            snprintf(weight_text, sizeof(weight_text), "%.2f lbs", current_list[i].weight);

            SDL_Surface *weight_surface = TTF_RenderText_Solid(font, weight_text, color);  // Keep original color
            SDL_Texture *weight_texture = SDL_CreateTextureFromSurface(renderer, weight_surface);
            SDL_Rect weight_rect = {weight_x, weight_y, weight_surface->w, weight_surface->h};

            SDL_RenderCopy(renderer, weight_texture, NULL, &weight_rect);
            SDL_FreeSurface(weight_surface);
            SDL_DestroyTexture(weight_texture);
        }

        y += spacing;
    }
}
