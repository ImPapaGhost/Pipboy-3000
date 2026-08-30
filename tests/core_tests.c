#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "events.h"
#include "input.h"
#include "inventory.h"
#include "state.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        cleanup_pip_state(&state); \
        SDL_Quit(); \
        return EXIT_FAILURE; \
    } \
} while (0)

static void press_key(SDL_Keycode key) {
    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = key;
    capture_input(&event);
}

int main(void) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL timer initialization failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    PipState state = {0};
    CHECK(initialize_pip_state(&state));

    CHECK(state.weapons_count == 5);
    CHECK(state.misc_count == 5);
    CHECK(state.junk_count == 5);
    CHECK(state.ammo_count == 8);
    CHECK(get_ammo_count("Fusion Cell", &state) == 100);

    // Regression: MISC/JUNK/MODS used to fall through and use the AMMO count.
    state.current_tab = TAB_INV;
    state.current_inv_subtab = SUBTAB_MISC;
    state.selector_position = state.misc_count - 1;
    press_key(SDLK_s);
    handle_navigation(&state);
    CHECK(state.selector_position == state.misc_count - 1);

    // W/S must not change DATA stats while another main tab is active.
    state.current_tab = TAB_STAT;
    state.current_subtab = SUBTAB_STATUS;
    state.current_stat_category = 1;
    press_key(SDLK_s);
    handle_navigation(&state);
    CHECK(state.current_stat_category == 1);

    // A single XP award can cross more than one level boundary.
    state.level = 1;
    state.current_xp = 0;
    state.xp_for_next_level = 100;
    add_experience(&state, 260);
    CHECK(state.level == 3);
    CHECK(state.current_xp == 10);
    CHECK(state.xp_for_next_level == 200);

    DamageBars bars = {0};
    update_damage(&bars, -5, 25, 50, 75, 100, 150);
    CHECK(bars.head == 0);
    CHECK(bars.right_leg == 100);

    cleanup_pip_state(&state);
    SDL_Quit();
    return EXIT_SUCCESS;
}
