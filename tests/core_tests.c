#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "events.h"
#include "input.h"
#include "inventory.h"
#include "save.h"
#include "state.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        cleanup_pip_state(&state); \
        cleanup_pip_state(&restored); \
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
    PipState restored = {0};
    CHECK(initialize_pip_state(&state));

    CHECK(state.weapons_count == 5);
    CHECK(state.misc_count == 5);
    CHECK(state.junk_count == 5);
    CHECK(state.ammo_count == 7);
    CHECK(get_ammo_count("ammo_fusion_cell", &state) == 100);
    CHECK(find_inventory_item(&state, "aid_stimpak") != NULL);

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

    invItem *stimpak = find_inventory_item(&state, "aid_stimpak");
    CHECK(stimpak != NULL);
    CHECK(stimpak->quantity == 5);

    const PipCommand damage = {PIP_COMMAND_TAKE_DAMAGE, NULL, 40};
    PipCommandOutcome command_result = pipboy_execute_command(&state, &damage);
    CHECK(command_result.result == PIP_COMMAND_OK);
    CHECK(state.health == 75);

    const PipCommand use_stimpak = {PIP_COMMAND_USE_ITEM, "aid_stimpak", 0};
    command_result = pipboy_execute_command(&state, &use_stimpak);
    CHECK(command_result.result == PIP_COMMAND_OK);
    CHECK(state.health == 105);
    CHECK(stimpak->quantity == 4);

    command_result = pipboy_execute_command(&state, &use_stimpak);
    CHECK(command_result.result == PIP_COMMAND_OK);
    CHECK(state.health == state.max_health);
    CHECK(stimpak->quantity == 3);
    command_result = pipboy_execute_command(&state, &use_stimpak);
    CHECK(command_result.result == PIP_COMMAND_NO_EFFECT);
    CHECK(stimpak->quantity == 3);

    const PipCommand radiation = {PIP_COMMAND_ADD_RADIATION, NULL, 100};
    CHECK(pipboy_execute_command(&state, &radiation).result == PIP_COMMAND_OK);
    CHECK(state.radiation == 100);
    const PipCommand use_radaway = {PIP_COMMAND_USE_ITEM, "aid_radaway", 0};
    CHECK(pipboy_execute_command(&state, &use_radaway).result == PIP_COMMAND_OK);
    CHECK(state.radiation == 70);
    CHECK(get_inventory_quantity(&state, "aid_radaway") == 2);

    const PipCommand equip_pistol = {PIP_COMMAND_EQUIP_ITEM, "weapon_10mm_pistol", 0};
    const PipCommand equip_rifle = {PIP_COMMAND_EQUIP_ITEM, "weapon_hunting_rifle", 0};
    CHECK(pipboy_execute_command(&state, &equip_pistol).result == PIP_COMMAND_OK);
    CHECK(find_inventory_item(&state, "weapon_10mm_pistol")->equipped);
    CHECK(pipboy_execute_command(&state, &equip_rifle).result == PIP_COMMAND_OK);
    CHECK(!find_inventory_item(&state, "weapon_10mm_pistol")->equipped);
    CHECK(find_inventory_item(&state, "weapon_hunting_rifle")->equipped);

    const PipCommand favorite = {PIP_COMMAND_TOGGLE_FAVORITE, "weapon_hunting_rifle", 0};
    CHECK(pipboy_execute_command(&state, &favorite).result == PIP_COMMAND_OK);
    CHECK(find_inventory_item(&state, "weapon_hunting_rifle")->favorite);

    const char *test_save = "build/test-player-save.json";
    remove(test_save);
    remove("build/test-player-save.json.backup");
    remove("build/test-player-save.json.corrupt");
    remove("build/test-player-save.json.tmp");
    CHECK(save_pip_state(&state, test_save));
    CHECK(initialize_pip_state(&restored));
    CHECK(load_pip_state(&restored, test_save) == PIP_SAVE_LOADED);
    CHECK(restored.health == state.health);
    CHECK(restored.radiation == state.radiation);
    CHECK(get_inventory_quantity(&restored, "aid_stimpak") == 3);
    CHECK(find_inventory_item(&restored, "weapon_hunting_rifle")->equipped);
    CHECK(find_inventory_item(&restored, "weapon_hunting_rifle")->favorite);

    const int backed_up_health = state.health;
    state.health = 42;
    CHECK(save_pip_state(&state, test_save));
    FILE *corrupt_save = fopen(test_save, "wb");
    CHECK(corrupt_save != NULL);
    CHECK(fputs("{corrupt", corrupt_save) >= 0);
    CHECK(fclose(corrupt_save) == 0);
    cleanup_pip_state(&restored);
    CHECK(initialize_pip_state(&restored));
    CHECK(load_pip_state(&restored, test_save) == PIP_SAVE_RECOVERED_BACKUP);
    CHECK(restored.health == backed_up_health);
    CHECK(save_pip_state(&restored, test_save));
    FILE *preserved_corrupt = fopen("build/test-player-save.json.corrupt", "rb");
    CHECK(preserved_corrupt != NULL);
    CHECK(fclose(preserved_corrupt) == 0);

    restored.persistence_enabled = false;
    restored.health = restored.max_health;
    press_key(SDLK_h);
    handle_navigation(&restored);
    CHECK(restored.health == restored.max_health - 40);

    restored.current_tab = TAB_INV;
    restored.current_inv_subtab = SUBTAB_AID;
    restored.selector_position = 0;
    const int event_stimpaks = get_inventory_quantity(&restored, "aid_stimpak");
    press_key(SDLK_RETURN);
    handle_navigation(&restored);
    CHECK(restored.health == restored.max_health - 10);
    CHECK(get_inventory_quantity(&restored, "aid_stimpak") == event_stimpaks - 1);

    remove(test_save);
    remove("build/test-player-save.json.backup");
    remove("build/test-player-save.json.corrupt");
    remove("build/test-player-save.json.tmp");

    cleanup_pip_state(&state);
    cleanup_pip_state(&restored);
    SDL_Quit();
    return EXIT_SUCCESS;
}
