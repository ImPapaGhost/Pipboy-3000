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
        case SUBTAB_MISC:
            current_list = state->misc;
            current_count = state->misc_count;
            break;
        case SUBTAB_JUNK:
            current_list = state->junk;
            current_count = state->junk_count;
            break;
        case SUBTAB_MODS:
            current_list = state->mods;
            current_count = state->mods_count;
            break;
        case SUBTAB_AMMO:
            current_list = state->ammo;
            current_count = state->ammo_count;
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
    int i = state->selector_position;
        for (int i = state->inv_scroll_index; i < state->inv_scroll_index + 10 && i < current_count; i++) {

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

        // Render the value in a specific location for the selected item
        if (i == state->selector_position) {
            int weight_x = 500;  // Position of highlight box
            int weight_y = 360;
            int weight_width = 200;  // Adjusted width for "Weight" text + value
            int box_height = 30;

            // Render a dim highlight box for the weight stat
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
            SDL_Rect weight_box_rect = {weight_x, weight_y, weight_width, box_height};
            SDL_RenderFillRect(renderer, &weight_box_rect);

            // Reset render color back to black (prevents global color issues)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            // Render "Weight" label on the left
            SDL_Surface *weight_label_surface = TTF_RenderText_Solid(font, "Weight", color);
            SDL_Texture *weight_label_texture = SDL_CreateTextureFromSurface(renderer, weight_label_surface);
            SDL_Rect weight_label_rect = {weight_x + 10, weight_y + 5, weight_label_surface->w, weight_label_surface->h};
            SDL_RenderCopy(renderer, weight_label_texture, NULL, &weight_label_rect);
            SDL_FreeSurface(weight_label_surface);
            SDL_DestroyTexture(weight_label_texture);

            // Render weight value on the right
            char weight_text[20];
            snprintf(weight_text, sizeof(weight_text), "%.1f", current_list[i].weight);
            SDL_Surface *weight_value_surface = TTF_RenderText_Solid(font, weight_text, color);
            SDL_Texture *weight_value_texture = SDL_CreateTextureFromSurface(renderer, weight_value_surface);
            SDL_Rect weight_value_rect = {weight_x + weight_width - weight_value_surface->w - 10, weight_y + 5, weight_value_surface->w, weight_value_surface->h};
            SDL_RenderCopy(renderer, weight_value_texture, NULL, &weight_value_rect);
            SDL_FreeSurface(weight_value_surface);
            SDL_DestroyTexture(weight_value_texture);

        // Render the value in a specific location for the selected item
            int value_x = 500;  // Position of highlight box
            int value_y = 390;
            int value_width = 200;  // Adjusted width for "Value" text + value

            // Render a dim highlight box for the value stat
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
            SDL_Rect value_box_rect = {value_x, value_y, value_width, box_height};
            SDL_RenderFillRect(renderer, &value_box_rect);

            // Reset render color back to black (prevents global color issues)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            // Render "Value" label on the left
            SDL_Surface *value_label_surface = TTF_RenderText_Solid(font, "Value", color);
            SDL_Texture *value_label_texture = SDL_CreateTextureFromSurface(renderer, value_label_surface);
            SDL_Rect value_label_rect = {value_x + 10, value_y + 5, value_label_surface->w, value_label_surface->h};
            SDL_RenderCopy(renderer, value_label_texture, NULL, &value_label_rect);
            SDL_FreeSurface(value_label_surface);
            SDL_DestroyTexture(value_label_texture);

            // Render weight value on the right
            char value_text[20];
            snprintf(value_text, sizeof(value_text), "%d", current_list[i].value);
            SDL_Surface *value_value_surface = TTF_RenderText_Solid(font, value_text, color);
            SDL_Texture *value_value_texture = SDL_CreateTextureFromSurface(renderer, value_value_surface);
            SDL_Rect value_value_rect = {value_x + value_width - value_value_surface->w - 10, value_y + 5, value_value_surface->w, value_value_surface->h};
            SDL_RenderCopy(renderer, value_value_texture, NULL, &value_value_rect);
            SDL_FreeSurface(value_value_surface);
            SDL_DestroyTexture(value_value_texture);
        }

        y += spacing;
    }
    // Render damage box and damage numerical box separate
        if (state->current_inv_subtab == SUBTAB_WEAPONS) {
            int damage_label_x = 500;
            int damage_label_y = 210;
            int damage_label_width = 120;
            int box_height = 30;

            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255);
            SDL_Rect damage_label_rect = {damage_label_x, damage_label_y, damage_label_width, box_height};
            SDL_RenderFillRect(renderer, &damage_label_rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            SDL_Surface *damage_label_surface = TTF_RenderText_Solid(font, "Damage", color);
            SDL_Texture *damage_label_texture = SDL_CreateTextureFromSurface(renderer, damage_label_surface);
            SDL_Rect damage_label_rect_text = {damage_label_x + 10, damage_label_y + 5, damage_label_surface->w, damage_label_surface->h};
            SDL_RenderCopy(renderer, damage_label_texture, NULL, &damage_label_rect_text);
            SDL_FreeSurface(damage_label_surface);
            SDL_DestroyTexture(damage_label_texture);

            int damage_value_x = damage_label_x + damage_label_width + 5;
            int damage_value_y = damage_label_y;
            int damage_value_width = 75;

            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255);
            SDL_Rect damage_value_rect = {damage_value_x, damage_value_y, damage_value_width, box_height};
            SDL_RenderFillRect(renderer, &damage_value_rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            // Load the target icon
        SDL_Surface *target_surface = IMG_Load("INV/TARGET1.jpg");
        SDL_Texture *target_texture = NULL;

        if (!target_surface) {
            printf("Failed to load target icon: %s\n", IMG_GetError());
        } else {
            target_texture = SDL_CreateTextureFromSurface(renderer, target_surface);
            SDL_FreeSurface(target_surface); // Free surface after creating texture
        }

        // Apply green tint if texture loaded successfully
        if (target_texture) {
            SDL_SetTextureColorMod(target_texture, 0, 255, 0); // Apply green tint (R = 0, G = 255, B = 0)
        }

        // Define position for the target icon (left-aligned next to damage value)
        int target_icon_width = 20;  // Adjust size if needed
        int target_icon_height = 20; // Adjust size if needed
        SDL_Rect target_rect = {damage_value_x + 5, damage_value_y + 5, target_icon_width, target_icon_height};

        // Render the target icon if it was loaded successfully
        if (target_texture) {
            SDL_RenderCopy(renderer, target_texture, NULL, &target_rect);
            SDL_DestroyTexture(target_texture); // Clean up texture
        }

        // Render damage text (right-aligned)
        char damage_text[20];
        snprintf(damage_text, sizeof(damage_text), "%d", current_list[i].damage);
        SDL_Surface *damage_value_surface = TTF_RenderText_Solid(font, damage_text, color);
        SDL_Texture *damage_value_texture = SDL_CreateTextureFromSurface(renderer, damage_value_surface);
        SDL_Rect damage_value_rect_text = {damage_value_x + damage_value_width - damage_value_surface->w - 10, damage_value_y + 5, damage_value_surface->w, damage_value_surface->h};
        SDL_RenderCopy(renderer, damage_value_texture, NULL, &damage_value_rect_text);
        SDL_FreeSurface(damage_value_surface);
        SDL_DestroyTexture(damage_value_texture);

            // Render the ammo
            int ammo_value_x = 500;  // Position of highlight box
            int ammo_value_y = 240;
            int ammo_value_width = 200;  // Adjusted width for "Ammo" text + value
            // Render a dim highlight box for the weight stat
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
            SDL_Rect ammo_box_rect = {ammo_value_x, ammo_value_y, ammo_value_width, box_height};
            SDL_RenderFillRect(renderer, &ammo_box_rect);
            // Load the Ammo icon
            SDL_Surface *ammo_surface = IMG_Load("INV/ammo.png");
            SDL_Texture *ammo_texture = NULL;

            if (!ammo_surface) {
                printf("Failed to load Ammo icon: %s\n", IMG_GetError());
            } else {
                ammo_texture = SDL_CreateTextureFromSurface(renderer, ammo_surface);
                SDL_FreeSurface(ammo_surface); // Free surface after creating texture
            }

            // Apply green tint to match Pip-Boy aesthetic
            if (ammo_texture) {
                SDL_SetTextureColorMod(ammo_texture, 0, 255, 0); // Apply green tint (R = 0, G = 255, B = 0)
            }

            // Define position for the Ammo icon (aligned with the "Ammo" label)
            int ammo_icon_width = 15;  // Adjust size if needed
            int ammo_icon_height = 15; // Adjust size if needed
            SDL_Rect ammo_icon_rect = {ammo_value_x + 2, ammo_value_y + (box_height - ammo_icon_height) / 2, ammo_icon_width, ammo_icon_height};

            // Render the Ammo icon if it was loaded successfully
            if (ammo_texture) {
                SDL_RenderCopy(renderer, ammo_texture, NULL, &ammo_icon_rect);
                SDL_DestroyTexture(ammo_texture); // Clean up texture
            }

            // Reset render color back to black (prevents global color issues)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            // Render "Ammo" label on the left
            SDL_Surface *ammo_label_surface = TTF_RenderText_Solid(font, "Ammo", color);
            SDL_Texture *ammo_label_texture = SDL_CreateTextureFromSurface(renderer, ammo_label_surface);
            SDL_Rect ammo_label_rect = {ammo_value_x + ammo_icon_width + 7, ammo_value_y + 5, ammo_label_surface->w, ammo_label_surface->h};
            SDL_RenderCopy(renderer, ammo_label_texture, NULL, &ammo_label_rect);
            SDL_FreeSurface(ammo_label_surface);
            SDL_DestroyTexture(ammo_label_texture);

            // Render ammo value on the right
            char ammo_text[20];
            snprintf(ammo_text, sizeof(ammo_text), "%d", current_list[i].ammo);
            SDL_Surface *ammo_value_surface = TTF_RenderText_Solid(font, ammo_text, color);
            SDL_Texture *ammo_value_texture = SDL_CreateTextureFromSurface(renderer, ammo_value_surface);
            SDL_Rect ammo_value_rect = {ammo_value_x + ammo_value_width - ammo_value_surface->w - 10, ammo_value_y + 5, ammo_value_surface->w, ammo_value_surface->h};
            SDL_RenderCopy(renderer, ammo_value_texture, NULL, &ammo_value_rect);
            SDL_FreeSurface(ammo_value_surface);
            SDL_DestroyTexture(ammo_value_texture);

            // Render the fire_rate
            int fire_rate_value_x = 500;  // Position of highlight box
            int fire_rate_value_y = 270;
            int fire_rate_value_width = 200;  // Adjusted width for "Fire rate" text + value

            // Render a dim highlight box for the fire_rate stat
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
            SDL_Rect fire_rate_box_rect = {fire_rate_value_x, fire_rate_value_y, fire_rate_value_width, box_height};
            SDL_RenderFillRect(renderer, &fire_rate_box_rect);

            // Reset render color back to black (prevents global color issues)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            // Render "fire rate" label on the left
            SDL_Surface *fire_rate_label_surface = TTF_RenderText_Solid(font, "Fire Rate", color);
            SDL_Texture *fire_rate_label_texture = SDL_CreateTextureFromSurface(renderer, fire_rate_label_surface);
            SDL_Rect fire_rate_label_rect = {fire_rate_value_x + 10, fire_rate_value_y + 5, fire_rate_label_surface->w, fire_rate_label_surface->h};
            SDL_RenderCopy(renderer, fire_rate_label_texture, NULL, &fire_rate_label_rect);
            SDL_FreeSurface(fire_rate_label_surface);
            SDL_DestroyTexture(fire_rate_label_texture);

            // Render ammo value on the right
            char fire_rate_text[20];
            snprintf(fire_rate_text, sizeof(fire_rate_text), "%d", current_list[i].fire_rate);
            SDL_Surface *fire_rate_value_surface = TTF_RenderText_Solid(font, fire_rate_text, color);
            SDL_Texture *fire_rate_value_texture = SDL_CreateTextureFromSurface(renderer, fire_rate_value_surface);
            SDL_Rect fire_rate_value_rect = {fire_rate_value_x + fire_rate_value_width - fire_rate_value_surface->w - 10, fire_rate_value_y + 5, fire_rate_value_surface->w, fire_rate_value_surface->h};
            SDL_RenderCopy(renderer, fire_rate_value_texture, NULL, &fire_rate_value_rect);
            SDL_FreeSurface(fire_rate_value_surface);
            SDL_DestroyTexture(fire_rate_value_texture);


            // Render the accuracy
            int range_value_x = 500;  // Position of highlight box
            int range_value_y = 330;
            int range_value_width = 200;  // Adjusted width for "Ammo" text + value

            // Render a dim highlight box for the fire_rate stat
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
            SDL_Rect range_box_rect = {range_value_x, range_value_y, range_value_width, box_height};
            SDL_RenderFillRect(renderer, &range_box_rect);

            // Reset render color back to black (prevents global color issues)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            // Render "accuracy" label on the left
            SDL_Surface *range_label_surface = TTF_RenderText_Solid(font, "Range", color);
            SDL_Texture *range_label_texture = SDL_CreateTextureFromSurface(renderer, range_label_surface);
            SDL_Rect range_label_rect = {range_value_x + 10, range_value_y + 5, range_label_surface->w, range_label_surface->h};
            SDL_RenderCopy(renderer, range_label_texture, NULL, &range_label_rect);
            SDL_FreeSurface(range_label_surface);
            SDL_DestroyTexture(range_label_texture);

            // Render accuracy value on the right
            char range_text[20];
            snprintf(range_text, sizeof(range_text), "%d", current_list[i].range);
            SDL_Surface *range_value_surface = TTF_RenderText_Solid(font, range_text, color);
            SDL_Texture *range_value_texture = SDL_CreateTextureFromSurface(renderer, range_value_surface);
            SDL_Rect range_value_rect = {range_value_x + range_value_width - range_value_surface->w - 10, range_value_y + 5, range_value_surface->w, range_value_surface->h};
            SDL_RenderCopy(renderer, range_value_texture, NULL, &range_value_rect);
            SDL_FreeSurface(range_value_surface);
            SDL_DestroyTexture(range_value_texture);

            // Render the accuracy
            int accuracy_value_x = 500;  // Position of highlight box
            int accuracy_value_y = 300;
            int accuracy_value_width = 200;  // Adjusted width for "Accuracy" text + value

            // Render a dim highlight box for the fire_rate stat
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
            SDL_Rect accuracy_box_rect = {accuracy_value_x, accuracy_value_y, accuracy_value_width, box_height};
            SDL_RenderFillRect(renderer, &accuracy_box_rect);

            // Reset render color back to black (prevents global color issues)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            // Render "accuracy" label on the left
            SDL_Surface *accuracy_label_surface = TTF_RenderText_Solid(font, "Accuracy", color);
            SDL_Texture *accuracy_label_texture = SDL_CreateTextureFromSurface(renderer, accuracy_label_surface);
            SDL_Rect accuracy_label_rect = {accuracy_value_x + 10, accuracy_value_y + 5, accuracy_label_surface->w, accuracy_label_surface->h};
            SDL_RenderCopy(renderer, accuracy_label_texture, NULL, &accuracy_label_rect);
            SDL_FreeSurface(accuracy_label_surface);
            SDL_DestroyTexture(accuracy_label_texture);

            // Render accuracy value on the right
            char accuracy_text[20];
            snprintf(accuracy_text, sizeof(accuracy_text), "%d", current_list[i].accuracy);
            SDL_Surface *accuracy_value_surface = TTF_RenderText_Solid(font, accuracy_text, color);
            SDL_Texture *accuracy_value_texture = SDL_CreateTextureFromSurface(renderer, accuracy_value_surface);
            SDL_Rect accuracy_value_rect = {accuracy_value_x + accuracy_value_width - accuracy_value_surface->w - 10, accuracy_value_y + 5, accuracy_value_surface->w, accuracy_value_surface->h};
            SDL_RenderCopy(renderer, accuracy_value_texture, NULL, &accuracy_value_rect);
            SDL_FreeSurface(accuracy_value_surface);
            SDL_DestroyTexture(accuracy_value_texture);
        }
        if (state->current_inv_subtab == SUBTAB_JUNK) {
            int component_x = 500;  // Position of highlight box
            int component_y = 250;
            int component_width = 200;  // Adjusted width for "Component" text + value
            int box_height = 30;

            // Render component value on the right
            char component_text[20];

            // Split the component string into separate lines
            char *component_copy = strdup(current_list[i].component);  // Make a copy to modify
            char *token = strtok(component_copy, ",");  // Split by comma
            int component_y_offset = 0; // Offset to move each line down

            while (token) {
                SDL_Surface *component_value_surface = TTF_RenderText_Solid(font, token, color);
                SDL_Texture *component_value_texture = SDL_CreateTextureFromSurface(renderer, component_value_surface);

                SDL_Rect component_value_rect = {
                    component_x + 10, component_y + 5 + component_y_offset,
                    component_value_surface->w, component_value_surface->h
                };

                SDL_RenderCopy(renderer, component_value_texture, NULL, &component_value_rect);
                SDL_FreeSurface(component_value_surface);
                SDL_DestroyTexture(component_value_texture);

                // Move down for the next line
                component_y_offset += 20;  // Adjust spacing between lines

                // Get next component in the list
                token = strtok(NULL, ",");
            }

        }
}