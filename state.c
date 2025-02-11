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

    state->misc_capacity = 10;
    state->misc = malloc(state->misc_capacity * sizeof(invItem));
    state->misc_count = 0;

    state->junk_capacity = 10;
    state->junk = malloc(state->junk_capacity * sizeof(invItem));
    state->junk_count = 0;

    state->mods_capacity = 10;
    state->mods = malloc(state->mods_capacity * sizeof(invItem));
    state->mods_count = 0;

    state->ammo_capacity = 10;
    state->ammo = malloc(state->ammo_capacity * sizeof(invItem));
    state->ammo_count = 0;

    // Load inv items into respective lists
    load_inv("weapons.csv", &state->weapons, &state->weapons_count, &state->weapons_capacity);
    load_inv("apparel.csv", &state->apparel, &state->apparel_count, &state->apparel_capacity);
    load_inv("aid.csv", &state->aid, &state->aid_count, &state->aid_capacity);
    load_inv("misc.csv", &state->misc, &state->misc_count, &state->misc_capacity);
    load_inv("junk.csv", &state->junk, &state->junk_count, &state->junk_capacity);
    load_inv("mods.csv", &state->mods, &state->mods_count, &state->mods_capacity);
    load_inv("ammo.csv", &state->ammo, &state->ammo_count, &state->ammo_capacity);

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

    state->current_data_subtab = SUBTAB_QUESTS;
    state->current_quest = 0;

    // Allocate initial space for quests
    state->quest_capacity = 5; // Start with room for 5 quests
    state->quest_count = 0;
    state->quests = malloc(state->quest_capacity * sizeof(Quest));

    // Add initial quests
    if (state->quests) {
        strcpy(state->quests[state->quest_count].name, "Welcome to the Wasteland");
        strcpy(state->quests[state->quest_count].description, "Survive your first day in the wasteland.");
        state->quests[state->quest_count].is_active = true;
        state->quest_count++;

        strcpy(state->quests[state->quest_count].name, "Find Shelter");
        strcpy(state->quests[state->quest_count].description, "Locate a safe place to stay for the night.");
        state->quests[state->quest_count].is_active = true;
        state->quest_count++;

        strcpy(state->quests[state->quest_count].name, "Gather Supplies");
        strcpy(state->quests[state->quest_count].description, "Find food, water, and medical supplies.");
        state->quests[state->quest_count].is_active = false;
        state->quest_count++;
    }

    state->current_workshop = 0; // Start at the first workshop

    // Allocate initial space for workshops
    state->workshop_capacity = 5; // Start with room for 5 workshops
    state->workshop_count = 0;
    state->workshops = malloc(state->workshop_capacity * sizeof(Workshop));

    // Add initial workshops
    if (state->workshops) {
        strcpy(state->workshops[state->workshop_count].name, "Sanctuary Hills");
        state->workshops[state->workshop_count].population = 10;
        state->workshops[state->workshop_count].food = 20;
        state->workshops[state->workshop_count].water = 15;
        state->workshops[state->workshop_count].power = 5;
        state->workshops[state->workshop_count].defense = 25;
        state->workshops[state->workshop_count].beds = 12;
        state->workshops[state->workshop_count].happiness = 80;
        state->workshop_count++;

        strcpy(state->workshops[state->workshop_count].name, "Red Rocket Truck Stop");
        state->workshops[state->workshop_count].population = 5;
        state->workshops[state->workshop_count].food = 10;
        state->workshops[state->workshop_count].water = 8;
        state->workshops[state->workshop_count].power = 3;
        state->workshops[state->workshop_count].defense = 15;
        state->workshops[state->workshop_count].beds = 6;
        state->workshops[state->workshop_count].happiness = 60;
        state->workshop_count++;
    }

    state->current_data_subtab = SUBTAB_STATS;
    state->current_stat = 0; // Start at the first stat

    // Allocate initial space for stats
    state->stats_capacity = 10; // Room for 10 stats initially
    state->stats_count = 0;
    state->stats = malloc(state->stats_capacity * sizeof(PlayerStat));

    // Add initial stats
    if (state->stats) {
        strcpy(state->stats[state->stats_count].name, "Level");
        strcpy(state->stats[state->stats_count].description, "Your current experience level.");
        state->stats[state->stats_count].value = state->level;
        state->stats_count++;

        strcpy(state->stats[state->stats_count].name, "Experience Points");
        strcpy(state->stats[state->stats_count].description, "Total XP earned.");
        state->stats[state->stats_count].value = state->current_xp;
        state->stats_count++;

        strcpy(state->stats[state->stats_count].name, "Locations Discovered");
        strcpy(state->stats[state->stats_count].description, "The number of locations you have visited.");
        state->stats[state->stats_count].value = 5; // Example value
        state->stats_count++;

        strcpy(state->stats[state->stats_count].name, "Quests Completed");
        strcpy(state->stats[state->stats_count].description, "The number of quests you have completed.");
        state->stats[state->stats_count].value = 2; // Example value
        state->stats_count++;

        strcpy(state->stats[state->stats_count].name, "Enemies Killed");
        strcpy(state->stats[state->stats_count].description, "The number of enemies you have defeated.");
        state->stats[state->stats_count].value = 12; // Example value
        state->stats_count++;
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

// Define `pip_state` to allocate memory for it
PipState pip_state;