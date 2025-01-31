#include "state.h"
#include "pipboy.h"  //  PipState and DamageBars
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initialize_pip_state(PipState *state) {
    // Initialize general state values
    state->current_tab = TAB_STAT;
    state->current_subtab = SUBTAB_STATUS;
    state->selector_position = 0;

    // Set default SPECIAL stats
    for (int i = 0; i < 7; i++) {
        state->special_stats[i] = 5; // Default SPECIAL stats
    }

    state->health = 115;       // Full health
    state->max_health = 115;   // Max health
    state->ap = 90;            // Action points
    state->max_ap = 90;        // Max action points
    state->level = 1;          // Starting level
    state->experience = 0;     // Starting experience
    state->stimpaks = 0;       // Starting number of Stimpaks
    state->radaways = 0;       // Starting number of RadAways
    state->current_xp = 50;    // Start with 50 XP
    state->xp_for_next_level = 100; // XP needed for level 2

    // Allocate initial space for Weapons, Apparel, and Aid
    state->weapons_capacity = 10;
    state->weapons = malloc(state->weapons_capacity * sizeof(invItem));
    state->weapons_count = 0;

    state->apparel_capacity = 10;
    state->apparel = malloc(state->apparel_capacity * sizeof(invItem));
    state->apparel_count = 0;

    state->aid_capacity = 10;
    state->aid = malloc(state->aid_capacity * sizeof(invItem));
    state->aid_count = 0;

    // Load inv items into respective lists
    load_inv("weapons.csv", &state->weapons, &state->weapons_count, &state->weapons_capacity);
    load_inv("apparel.csv", &state->apparel, &state->apparel_count, &state->apparel_capacity);
    load_inv("aid.csv", &state->aid, &state->aid_count, &state->aid_capacity);

    // Initialize perks to empty
    for (int i = 0; i < 10; i++) {
        memset(state->perks[i], 0, sizeof(state->perks[i]));
    }

    // Initialize SPECIAL animations to NULL
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 10; j++) {
            state->special_animations[i][j] = NULL;
        }
    }
}

void add_experience(PipState *state, int xp) {
    state->current_xp += xp;
    if (state->current_xp >= state->xp_for_next_level) {
        state->current_xp -= state->xp_for_next_level; // Rollover XP
        state->level += 1;                            // Level up
        state->xp_for_next_level += 50;               // Increase XP threshold
        printf("Level up! Current level: %d\n", state->level);
    }
}

void update_damage(DamageBars *bars, int head, int left_arm, int right_arm, int torso, int left_leg, int right_leg) {
    bars->head = head;
    bars->left_arm = left_arm;
    bars->right_arm = right_arm;
    bars->torso = torso;
    bars->left_leg = left_leg;
    bars->right_leg = right_leg;
}

// Define `pip_state` here to allocate memory for it
PipState pip_state;