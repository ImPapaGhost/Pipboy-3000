#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL_ttf.h>
#include "pipboy.h"
#include "state.h"
#include "ui.h"
#include "inventory.h"
#define BOX_WIDTH 200
#define BOX_HEIGHT 20
#define X_VALUE 500
#define Y_VALUE 390
#define Y_POSITION(index) (Y_VALUE - (25 * (index)))

// Easing subtab animation
float ease_out_cubic(float t) {
    const float inverse = 1.0f - t;
    return 1.0f - (inverse * inverse * inverse);
}

void render_inv(SDL_Renderer *renderer, const AppResources *resources, PipState *state) {
    TTF_Font *font = resources->body_font;
    SDL_Color color = {0, 255, 0, 255};         // Normal bright green
    SDL_Color dimmed_color = {0, 80, 0, 255};  // Dimmed green for highlighted text
    int x = 100, y = 120;
    int spacing = 30;

    render_inv_subtabs(renderer, resources, state);

    InventoryView view = get_inventory_view(state);
    invItem *current_list = view.items;
    int current_count = view.count;

    if (current_count == 0) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, "No items in this category.", color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        return;
    }

    if (state->selector_position < 0 || state->selector_position >= current_count) {
        state->selector_position = 0;
    }

    int i = state->selector_position;
    for (int item_index = state->inv_scroll_index;
         item_index < state->inv_scroll_index + 10 && item_index < current_count;
         item_index++) {

        char name_with_quantity[100];
        const char *marker = current_list[item_index].equipped
            ? (current_list[item_index].favorite ? "[E*] " : "[E] ")
            : (current_list[item_index].favorite ? "[*] " : "");
        snprintf(name_with_quantity, sizeof(name_with_quantity), "%s%s (%d)",
                 marker, current_list[item_index].name, current_list[item_index].quantity);

        // Determine text color (dimmed if selected)
        SDL_Color text_color = (item_index == state->selector_position) ? dimmed_color : color;

        // Render the name (including quantity in parentheses)
        SDL_Surface *name_surface = TTF_RenderText_Solid(font, name_with_quantity, text_color);
        SDL_Texture *name_texture = SDL_CreateTextureFromSurface(renderer, name_surface);
        SDL_Rect name_rect = {x, y, name_surface->w, name_surface->h};

        // Highlight the selected item dynamically based on text width
        if (item_index == state->selector_position) {
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
        if (item_index == state->selector_position) {

        // Render a dim highlight box for the value stat
        SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
        SDL_Rect value_box_rect = {X_VALUE, Y_POSITION(1), BOX_WIDTH, BOX_HEIGHT};
        SDL_RenderFillRect(renderer, &value_box_rect);

        // Reset render color back to black (prevents global color issues)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Render "Value" label on the left
        SDL_Surface *value_label_surface = TTF_RenderText_Solid(font, "Value", color);
        SDL_Texture *value_label_texture = SDL_CreateTextureFromSurface(renderer, value_label_surface);
        int value_label_y_center = Y_POSITION(1) + (BOX_HEIGHT - value_label_surface->h) / 2;
        SDL_Rect value_label_rect = {X_VALUE + 10, value_label_y_center, value_label_surface->w, value_label_surface->h};
        SDL_RenderCopy(renderer, value_label_texture, NULL, &value_label_rect);
        SDL_FreeSurface(value_label_surface);
        SDL_DestroyTexture(value_label_texture);

        // Render weight value on the right
        char value_text[20];
        snprintf(value_text, sizeof(value_text), "%d", current_list[item_index].value);
        SDL_Surface *value_value_surface = TTF_RenderText_Solid(font, value_text, color);
        SDL_Texture *value_value_texture = SDL_CreateTextureFromSurface(renderer, value_value_surface);
        int value_value_y_center = Y_POSITION(1) + (BOX_HEIGHT - value_value_surface->h) / 2;
        SDL_Rect value_value_rect = {X_VALUE + BOX_WIDTH - value_value_surface->w - 10, value_value_y_center, value_value_surface->w, value_value_surface->h};
        SDL_RenderCopy(renderer, value_value_texture, NULL, &value_value_rect);
        SDL_FreeSurface(value_value_surface);
        SDL_DestroyTexture(value_value_texture);

        // Render a dim highlight box for the weight stat
        SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
        SDL_Rect weight_box_rect = {X_VALUE, Y_POSITION(2), BOX_WIDTH, BOX_HEIGHT};
        SDL_RenderFillRect(renderer, &weight_box_rect);

        // Reset render color back to black (prevents global color issues)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Render "Weight" label on the left
        SDL_Surface *weight_label_surface = TTF_RenderText_Solid(font, "Weight", color);
        SDL_Texture *weight_label_texture = SDL_CreateTextureFromSurface(renderer, weight_label_surface);
        int weight_label_y_center = Y_POSITION(2) + (BOX_HEIGHT - weight_label_surface->h) / 2;
        SDL_Rect weight_label_rect = {X_VALUE + 10, weight_label_y_center, weight_label_surface->w, weight_label_surface->h};
        SDL_RenderCopy(renderer, weight_label_texture, NULL, &weight_label_rect);
        SDL_FreeSurface(weight_label_surface);
        SDL_DestroyTexture(weight_label_texture);

        // Render weight value on the right
        char weight_text[20];
        snprintf(weight_text, sizeof(weight_text), "%.1f", (double)current_list[item_index].weight);
        SDL_Surface *weight_value_surface = TTF_RenderText_Solid(font, weight_text, color);
        SDL_Texture *weight_value_texture = SDL_CreateTextureFromSurface(renderer, weight_value_surface);
        int weight_value_y_center = Y_POSITION(2) + (BOX_HEIGHT - weight_value_surface->h) / 2;
        SDL_Rect weight_value_rect = {X_VALUE + BOX_WIDTH - weight_value_surface->w - 10, weight_value_y_center, weight_value_surface->w, weight_value_surface->h};
        SDL_RenderCopy(renderer, weight_value_texture, NULL, &weight_value_rect);
        SDL_FreeSurface(weight_value_surface);
        SDL_DestroyTexture(weight_value_texture);
    }

    y += spacing;

    }

    // Render damage box and damage numerical box separate
    if (state->current_inv_subtab == SUBTAB_WEAPONS) {
        int damage_label_width = 120;

        // Render the damage highlight box
        SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255);
        //SDL_Rect damage_label_rect = {X_VALUE, Y_POSITION(7), damage_label_width, BOX_HEIGHT};
        int damage_y_position = (strcmp(current_list[i].ammo_type, "None") == 0) ? Y_POSITION(4) : Y_POSITION(7);
        SDL_Rect damage_label_rect = {X_VALUE, damage_y_position, damage_label_width, BOX_HEIGHT};
        SDL_RenderFillRect(renderer, &damage_label_rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Render damage text over box
        SDL_Surface *damage_label_surface = TTF_RenderText_Solid(font, "Damage", color);
        SDL_Texture *damage_label_texture = SDL_CreateTextureFromSurface(renderer, damage_label_surface);
        int damage_label_y_center = damage_y_position + (BOX_HEIGHT - damage_label_surface->h) / 2;
        SDL_Rect damage_label_rect_text = {X_VALUE + 10, damage_label_y_center, damage_label_surface->w, damage_label_surface->h};
        SDL_RenderCopy(renderer, damage_label_texture, NULL, &damage_label_rect_text);
        SDL_FreeSurface(damage_label_surface);
        SDL_DestroyTexture(damage_label_texture);

        int damage_X_VALUE = X_VALUE + damage_label_width + 5;
        int damage_BOX_WIDTH = 75;

        // Render the highlight box for the damage value
        SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255);
        SDL_Rect damage_value_rect = {damage_X_VALUE, damage_y_position, damage_BOX_WIDTH, BOX_HEIGHT};
        SDL_RenderFillRect(renderer, &damage_value_rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Define position for the target icon (left-aligned next to damage value)
        int target_icon_width = 20;  // Adjust size if needed
        int target_icon_height = 20; // Adjust size if needed
        int target_icon_y_center = damage_y_position + (BOX_HEIGHT - target_icon_height) / 2;
        SDL_Rect target_rect = {damage_X_VALUE + 5, target_icon_y_center, target_icon_width, target_icon_height};
        if (resources->target_icon) {
            SDL_SetTextureColorMod(resources->target_icon, 0, 255, 0);
            SDL_RenderCopy(renderer, resources->target_icon, NULL, &target_rect);
        }

        // Render damage text over box (right-aligned)
        char damage_text[20];
        snprintf(damage_text, sizeof(damage_text), "%d", current_list[i].damage);
        SDL_Surface *damage_value_surface = TTF_RenderText_Solid(font, damage_text, color);
        SDL_Texture *damage_value_texture = SDL_CreateTextureFromSurface(renderer, damage_value_surface);
        int damage_value_y_center = damage_y_position + (BOX_HEIGHT - damage_value_surface->h) / 2;
        SDL_Rect damage_value_rect_text = {damage_X_VALUE + damage_BOX_WIDTH - damage_value_surface->w - 10, damage_value_y_center, damage_value_surface->w, damage_value_surface->h};
        SDL_RenderCopy(renderer, damage_value_texture, NULL, &damage_value_rect_text);
        SDL_FreeSurface(damage_value_surface);
        SDL_DestroyTexture(damage_value_texture);

            if (strcmp(current_list[i].ammo_type, "None") != 0) {  //Skip for melee weapons

            // Render highlight box for the ammo stat
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
            SDL_Rect ammo_box_rect = {X_VALUE, Y_POSITION(6), BOX_WIDTH, BOX_HEIGHT};
            SDL_RenderFillRect(renderer, &ammo_box_rect);
            // Define position for the Ammo icon (aligned with the "Ammo" label) over box
            int ammo_icon_width = 15;  // Adjust size if needed
            int ammo_icon_height = 15; // Adjust size if needed
            int ammo_icon_y_center = Y_POSITION(6) + (BOX_HEIGHT - ammo_icon_height) / 2;
            SDL_Rect ammo_icon_rect = {X_VALUE + 2, ammo_icon_y_center, ammo_icon_width, ammo_icon_height};
            if (resources->ammo_icon) {
                SDL_SetTextureColorMod(resources->ammo_icon, 0, 255, 0);
                SDL_RenderCopy(renderer, resources->ammo_icon, NULL, &ammo_icon_rect);
            }

            // Reset render color back to black (prevents global color issues)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            // Render "Ammo" label on the left over box
            SDL_Surface *ammo_label_surface = TTF_RenderText_Solid(font, current_list[i].ammo_type, color);
            SDL_Texture *ammo_label_texture = SDL_CreateTextureFromSurface(renderer, ammo_label_surface);
            int ammo_label_y_center = Y_POSITION(6) + (BOX_HEIGHT - ammo_label_surface->h) / 2;
            SDL_Rect ammo_label_rect = {X_VALUE + ammo_icon_width + 7, ammo_label_y_center, ammo_label_surface->w, ammo_label_surface->h};
            SDL_RenderCopy(renderer, ammo_label_texture, NULL, &ammo_label_rect);
            SDL_FreeSurface(ammo_label_surface);
            SDL_DestroyTexture(ammo_label_texture);

            // Render ammo value on the right
            char ammo_text[20];
            // snprintf(ammo_text, sizeof(ammo_text), "%d", current_list[i].ammo);
            int ammo_count = get_ammo_count(current_list[i].ammo_id, state);
            snprintf(ammo_text, sizeof(ammo_text), "%d", ammo_count);
            SDL_Surface *ammo_value_surface = TTF_RenderText_Solid(font, ammo_text, color);
            SDL_Texture *ammo_value_texture = SDL_CreateTextureFromSurface(renderer, ammo_value_surface);
            int ammo_value_y_center = Y_POSITION(6) + (BOX_HEIGHT - ammo_value_surface->h) / 2;
            SDL_Rect ammo_value_rect = {X_VALUE + BOX_WIDTH - ammo_value_surface->w - 10, ammo_value_y_center, ammo_value_surface->w, ammo_value_surface->h};
            SDL_RenderCopy(renderer, ammo_value_texture, NULL, &ammo_value_rect);
            SDL_FreeSurface(ammo_value_surface);
            SDL_DestroyTexture(ammo_value_texture);

            // Render the fire_rate
            // Render a dim highlight box for the fire_rate stat
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
            SDL_Rect fire_rate_box_rect = {X_VALUE, Y_POSITION(5), BOX_WIDTH, BOX_HEIGHT};
            SDL_RenderFillRect(renderer, &fire_rate_box_rect);

            // Reset render color back to black (prevents global color issues)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            // Render "fire rate" label on the left
            SDL_Surface *fire_rate_label_surface = TTF_RenderText_Solid(font, "Fire Rate", color);
            SDL_Texture *fire_rate_label_texture = SDL_CreateTextureFromSurface(renderer, fire_rate_label_surface);
            int fire_rate_label_y_center = Y_POSITION(5) + (BOX_HEIGHT - fire_rate_label_surface->h) / 2;
            SDL_Rect fire_rate_label_rect = {X_VALUE + 10, fire_rate_label_y_center, fire_rate_label_surface->w, fire_rate_label_surface->h};
            SDL_RenderCopy(renderer, fire_rate_label_texture, NULL, &fire_rate_label_rect);
            SDL_FreeSurface(fire_rate_label_surface);
            SDL_DestroyTexture(fire_rate_label_texture);

            // Render fire rate value on the right
            char fire_rate_text[20];
            snprintf(fire_rate_text, sizeof(fire_rate_text), "%d", current_list[i].fire_rate);
            SDL_Surface *fire_rate_value_surface = TTF_RenderText_Solid(font, fire_rate_text, color);
            SDL_Texture *fire_rate_value_texture = SDL_CreateTextureFromSurface(renderer, fire_rate_value_surface);
            int fire_rate_value_y_center = Y_POSITION(5) + (BOX_HEIGHT - fire_rate_value_surface->h) / 2;
            SDL_Rect fire_rate_value_rect = {X_VALUE + BOX_WIDTH - fire_rate_value_surface->w - 10, fire_rate_value_y_center, fire_rate_value_surface->w, fire_rate_value_surface->h};
            SDL_RenderCopy(renderer, fire_rate_value_texture, NULL, &fire_rate_value_rect);
            SDL_FreeSurface(fire_rate_value_surface);
            SDL_DestroyTexture(fire_rate_value_texture);

            // Render the range
            // Render a dim highlight box for the range stat
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
            SDL_Rect range_box_rect = {X_VALUE, Y_POSITION(3), BOX_WIDTH, BOX_HEIGHT};
            SDL_RenderFillRect(renderer, &range_box_rect);

            // Reset render color back to black (prevents global color issues)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            // Render "accuracy" label on the left
            SDL_Surface *range_label_surface = TTF_RenderText_Solid(font, "Range", color);
            SDL_Texture *range_label_texture = SDL_CreateTextureFromSurface(renderer, range_label_surface);
            int range_label_y_center = Y_POSITION(3) + (BOX_HEIGHT - range_label_surface->h) / 2;
            SDL_Rect range_label_rect = {X_VALUE + 10, range_label_y_center, range_label_surface->w, range_label_surface->h};
            SDL_RenderCopy(renderer, range_label_texture, NULL, &range_label_rect);
            SDL_FreeSurface(range_label_surface);
            SDL_DestroyTexture(range_label_texture);

            // Render range value on the right
            char range_text[20];
            snprintf(range_text, sizeof(range_text), "%d", current_list[i].range);
            SDL_Surface *range_value_surface = TTF_RenderText_Solid(font, range_text, color);
            SDL_Texture *range_value_texture = SDL_CreateTextureFromSurface(renderer, range_value_surface);
            int range_value_y_center = Y_POSITION(3) + (BOX_HEIGHT - range_value_surface->h) / 2;
            SDL_Rect range_value_rect = {X_VALUE + BOX_WIDTH - range_value_surface->w - 10, range_value_y_center, range_value_surface->w, range_value_surface->h};
            SDL_RenderCopy(renderer, range_value_texture, NULL, &range_value_rect);
            SDL_FreeSurface(range_value_surface);
            SDL_DestroyTexture(range_value_texture);

            // Render the accuracy
            // Render a dim highlight box for the accuracy stat
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dimmed green
            SDL_Rect accuracy_box_rect = {X_VALUE, Y_POSITION(4), BOX_WIDTH, BOX_HEIGHT};
            SDL_RenderFillRect(renderer, &accuracy_box_rect);

            // Reset render color back to black (prevents global color issues)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            // Render "accuracy" label on the left
            SDL_Surface *accuracy_label_surface = TTF_RenderText_Solid(font, "Accuracy", color);
            SDL_Texture *accuracy_label_texture = SDL_CreateTextureFromSurface(renderer, accuracy_label_surface);
            int accuracy_label_y_center = Y_POSITION(4) + (BOX_HEIGHT - accuracy_label_surface->h) / 2;
            SDL_Rect accuracy_label_rect = {X_VALUE + 10, accuracy_label_y_center, accuracy_label_surface->w, accuracy_label_surface->h};
            SDL_RenderCopy(renderer, accuracy_label_texture, NULL, &accuracy_label_rect);
            SDL_FreeSurface(accuracy_label_surface);
            SDL_DestroyTexture(accuracy_label_texture);

            // Render accuracy value on the right
            char accuracy_text[20];
            snprintf(accuracy_text, sizeof(accuracy_text), "%d", current_list[i].accuracy);
            SDL_Surface *accuracy_value_surface = TTF_RenderText_Solid(font, accuracy_text, color);
            SDL_Texture *accuracy_value_texture = SDL_CreateTextureFromSurface(renderer, accuracy_value_surface);
            int accuracy_value_y_center = Y_POSITION(4) + (BOX_HEIGHT - accuracy_value_surface->h) / 2;
            SDL_Rect accuracy_value_rect = {X_VALUE + BOX_WIDTH - accuracy_value_surface->w - 10, accuracy_value_y_center, accuracy_value_surface->w, accuracy_value_surface->h};
            SDL_RenderCopy(renderer, accuracy_value_texture, NULL, &accuracy_value_rect);
            SDL_FreeSurface(accuracy_value_surface);
            SDL_DestroyTexture(accuracy_value_texture);
        }

        if (strcmp(current_list[i].ammo_type, "None") == 0) {
            // Render the speed box highlight
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255);
            SDL_Rect speed_box_rect = {X_VALUE, Y_POSITION(3), BOX_WIDTH, BOX_HEIGHT};
            SDL_RenderFillRect(renderer, &speed_box_rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

            SDL_Surface *speed_label_surface = TTF_RenderText_Solid(font, "Speed", color);
            SDL_Texture *speed_label_texture = SDL_CreateTextureFromSurface(renderer, speed_label_surface);
            int speed_label_y_center = Y_POSITION(3) + (BOX_HEIGHT - speed_label_surface->h) / 2;
            SDL_Rect speed_label_rect = {X_VALUE + 10, speed_label_y_center, speed_label_surface->w, speed_label_surface->h};
            SDL_RenderCopy(renderer, speed_label_texture, NULL, &speed_label_rect);
            SDL_FreeSurface(speed_label_surface);
            SDL_DestroyTexture(speed_label_texture);

            SDL_Surface *speed_value_surface = TTF_RenderText_Solid(font, current_list[i].speed, color);
            SDL_Texture *speed_value_texture = SDL_CreateTextureFromSurface(renderer, speed_value_surface);
            int speed_value_y_center = Y_POSITION(3) + (BOX_HEIGHT - speed_value_surface->h) / 2;
            SDL_Rect speed_value_rect = {X_VALUE + BOX_WIDTH - speed_value_surface->w - 10, speed_value_y_center, speed_value_surface->w, speed_value_surface->h};
            SDL_RenderCopy(renderer, speed_value_texture, NULL, &speed_value_rect);
            SDL_FreeSurface(speed_value_surface);
            SDL_DestroyTexture(speed_value_texture);
        }
    }

    if (state->current_inv_subtab == SUBTAB_JUNK) {
        int component_x = 500;  // Position of highlight box
        int component_y = 250;

        // Split the component string into separate lines
        char component_copy[sizeof(current_list[i].component)];
        snprintf(component_copy, sizeof(component_copy), "%s", current_list[i].component);
        char *token = strtok(component_copy, ",");  // Split by comma
        int component_y_offset = 0; // Offset to move each line down

        while (token) {
            while (*token == ' ') token++;  // Remove leading spaces

            // Render the cleaned component
            SDL_Surface *component_value_surface = TTF_RenderText_Solid(font, token, color);
            SDL_Texture *component_value_texture = SDL_CreateTextureFromSurface(renderer, component_value_surface);

            SDL_Rect component_value_rect = {component_x + 10, component_y + 5 + component_y_offset,
                                                component_value_surface->w, component_value_surface->h};

            SDL_RenderCopy(renderer, component_value_texture, NULL, &component_value_rect);
            SDL_FreeSurface(component_value_surface);
            SDL_DestroyTexture(component_value_texture);

            component_y_offset += 20;

            token = strtok(NULL, ",");  // Next component
        }
    }

    SDL_Surface *hint_surface = TTF_RenderText_Solid(
        font,
        "ENTER: USE/EQUIP    F: FAVORITE",
        dimmed_color
    );
    if (hint_surface) {
        SDL_Texture *hint_texture = SDL_CreateTextureFromSurface(renderer, hint_surface);
        if (hint_texture) {
            SDL_Rect hint_rect = {100, 405, hint_surface->w, hint_surface->h};
            SDL_RenderCopy(renderer, hint_texture, NULL, &hint_rect);
            SDL_DestroyTexture(hint_texture);
        }
        SDL_FreeSurface(hint_surface);
    }
}
