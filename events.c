#include "events.h"
#include "input.h"
#include "inventory.h"
#include "MAP/map.h"
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
                // Reset DATA subtabs to Quests when switching to DATA
                if (state->current_tab == TAB_DATA) {
                    state->current_data_subtab = SUBTAB_QUESTS;
                }
                break;
            case SDLK_e:
                state->current_tab = (state->current_tab + 1) % NUM_TABS;
                state->selector_position = 0; // Reset selection position
                state->inv_scroll_index = 0;  // Reset scroll index
                if (state->current_tab == TAB_DATA) {
                    state->current_data_subtab = SUBTAB_QUESTS;
                }
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
                } else if (state->current_tab == TAB_DATA && !state->is_data_animating) {
                    state->data_subtab_animation_offset = SUBTAB_SPACING;
                    state->is_data_animating = true;
                    state->data_subtab_animation_start_time = SDL_GetTicks();
                    state->current_data_subtab = (state->current_data_subtab - 1 + NUM_DATA_SUBTABS) % NUM_DATA_SUBTABS;
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
                } else if (state->current_tab == TAB_DATA && !state->is_data_animating) {
                    state->data_subtab_animation_offset = -SUBTAB_SPACING;
                    state->is_data_animating = true;
                    state->data_subtab_animation_start_time = SDL_GetTicks();
                    state->current_data_subtab = (state->current_data_subtab + 1) % NUM_DATA_SUBTABS;
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
                } else if (state->current_tab == TAB_DATA && state->current_data_subtab == SUBTAB_QUESTS) {
                    if (state->current_quest > 0) {
                        state->current_quest--;
                    }
                } else if (state->current_tab == TAB_DATA && state->current_data_subtab == SUBTAB_WORKSHOPS) {
                    if (state->current_workshop > 0) {
                        state->current_workshop--;
                    }
                } else if (state->current_tab == TAB_DATA && state->current_data_subtab == SUBTAB_STATS &&
                           state->current_stat_category > 0) {
                    state->current_stat_category--;
                }
                break;

            case SDLK_s:
                if (state->current_tab == TAB_STAT && state->current_subtab == SUBTAB_SPECIAL && !state->is_special_stat_animating) {
                    state->special_stat_animation_offset = 30; // Move downwards
                    state->is_special_stat_animating = true;
                    state->special_stat_animation_start = SDL_GetTicks();
                    state->selector_position = (state->selector_position + 1) % 7; // Wrap around SPECIAL stats
                } else if (state->current_tab == TAB_INV) {
                    InventoryView view = get_inventory_view(state);

                    // Scroll down within the current inventory subtab
                    if (view.items && state->selector_position < view.count - 1) {
                        state->selector_position++;
                        if (state->selector_position >= state->inv_scroll_index + 10) {
                            state->inv_scroll_index++;
                        }
                    }
                } else if (state->current_tab == TAB_DATA && state->current_data_subtab == SUBTAB_QUESTS) {
                    if (state->current_quest < state->quest_count - 1) {
                        state->current_quest++;
                    }
                } else if (state->current_tab == TAB_DATA && state->current_data_subtab == SUBTAB_WORKSHOPS) {
                    if (state->current_workshop < state->workshop_count - 1) {
                        state->current_workshop++;
                    }
                } else if (state->current_tab == TAB_DATA && state->current_data_subtab == SUBTAB_STATS &&
                           state->current_stat_category < NUM_STAT_CATEGORIES - 1) {
                    state->current_stat_category++;
                }
                break;

            case SDLK_UP:
            case SDLK_DOWN:
            case SDLK_LEFT:
            case SDLK_RIGHT:
                if (state->current_tab == TAB_MAP) {
                    map_handle_key(key);
                }
                break;

            // Simulate gaining XP (testing)
            case SDLK_x:
                add_experience(state, 10);
                break;

            default:
                break;
        }
    }
}
