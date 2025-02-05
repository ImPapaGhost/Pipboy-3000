#include "events.h"
#include "inventory.h"
#include "pipboy.h"
#include <SDL2/SDL.h>


#include "events.h"
#include "input.h" // Include the new input queue system
#include "inventory.h"
#include "pipboy.h"
#include <SDL2/SDL.h>

void handle_navigation(PipState *state) {
    while (!input_queue_is_empty()) { // Process inputs from the queue
        SDL_Keycode key = input_dequeue();

        switch (key) {
            // Main Tabs Navigation (Q for left, E for right)
            case SDLK_q:
                state->current_tab = (state->current_tab - 1 + NUM_TABS) % NUM_TABS;
                state->selector_position = 0; // Reset selection position
                state->inv_scroll_index = 0;  // Reset scroll index
                break;
            case SDLK_e:
                state->current_tab = (state->current_tab + 1) % NUM_TABS;
                state->selector_position = 0; // Reset selection position
                state->inv_scroll_index = 0;  // Reset scroll index
                break;

            // Sub-tabs Navigation
            case SDLK_a: // Navigate left in sub-tabs
                if (state->current_tab == TAB_STAT && !state->is_animating) {
                    // Handle STAT sub-tabs
                    state->subtab_animation_offset = SUBTAB_SPACING;
                    state->is_animating = true;
                    state->subtab_animation_start_time = SDL_GetTicks();
                    state->current_subtab = (state->current_subtab - 1 + NUM_SUBTABS) % NUM_SUBTABS;
                } else if (state->current_tab == TAB_INV && !state->is_inv_animating) {
                    // Handle INV sub-tabs
                    state->inv_subtab_animation_offset = SUBTAB_SPACING;
                    state->is_inv_animating = true;
                    state->inv_subtab_animation_start_time = SDL_GetTicks();
                    state->current_inv_subtab = (state->current_inv_subtab - 1 + NUM_INV_SUBTABS) % NUM_INV_SUBTABS;

                    // Reset inventory navigation when changing subtabs
                    reset_inventory_navigation(state);
                }
                break;

            case SDLK_d: // Navigate right in sub-tabs
                if (state->current_tab == TAB_STAT && !state->is_animating) {
                    // Handle STAT sub-tabs
                    state->subtab_animation_offset = -SUBTAB_SPACING;
                    state->is_animating = true;
                    state->subtab_animation_start_time = SDL_GetTicks();
                    state->current_subtab = (state->current_subtab + 1) % NUM_SUBTABS;
                } else if (state->current_tab == TAB_INV && !state->is_inv_animating) {
                    // Handle INV sub-tabs
                    state->inv_subtab_animation_offset = -SUBTAB_SPACING;
                    state->is_inv_animating = true;
                    state->inv_subtab_animation_start_time = SDL_GetTicks();
                    state->current_inv_subtab = (state->current_inv_subtab + 1) % NUM_INV_SUBTABS;

                    // Reset inventory navigation when changing subtabs
                    reset_inventory_navigation(state);
                }
                break;

            // SPECIAL Attributes Navigation (W and S for up/down)
            case SDLK_w:
                if (state->current_tab == TAB_STAT && state->current_subtab == SUBTAB_SPECIAL && !state->is_special_stat_animating) {
                    state->special_stat_animation_offset = -30; // Move upwards
                    state->is_special_stat_animating = true;
                    state->special_stat_animation_start = SDL_GetTicks();
                    state->selector_position = (state->selector_position - 1 + 7) % 7; // Wrap around SPECIAL stats
                } else if (state->current_tab == TAB_INV) {
                    // Inventory scrolling up
                    if (state->selector_position > 0) {
                        state->selector_position--;
                        if (state->selector_position < state->inv_scroll_index) {
                            state->inv_scroll_index--;
                        }
                    }
                }
                break;

            case SDLK_s:
                if (state->current_tab == TAB_STAT && state->current_subtab == SUBTAB_SPECIAL && !state->is_special_stat_animating) {
                    state->special_stat_animation_offset = 30; // Move downwards
                    state->is_special_stat_animating = true;
                    state->special_stat_animation_start = SDL_GetTicks();
                    state->selector_position = (state->selector_position + 1) % 7; // Wrap around SPECIAL stats
                } else if (state->current_tab == TAB_INV) {
                    // Inventory scrolling down
                    invItem *current_list = NULL;
                    int current_count = 0;

                    // Determine the active inventory subtab list
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
                        case SUBTAB_JUNK:
                            current_list = state->junk;
                            current_count = state->junk_count;
                        case SUBTAB_MODS:
                            current_list = state->mods;
                            current_count = state->mods_count;
                        case SUBTAB_AMMO:
                            current_list = state->ammo;
                            current_count = state->ammo_count;
                    }

                    // Scroll down within the current inventory subtab
                    if (current_list && state->selector_position < current_count - 1) {
                        state->selector_position++;
                        if (state->selector_position >= state->inv_scroll_index + 10) {
                            state->inv_scroll_index++;
                        }
                    }
                }
                break;

            // Simulate gaining XP (testing)
            case SDLK_x:
                add_experience(state, 10);
                break;
        }
    }
}
