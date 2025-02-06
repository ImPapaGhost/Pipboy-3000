#include "inventory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pipboy.h"

// Function to load inventory items from a file
int load_inv(const char *file_path, invItem **inv_list, int *inv_count, int *inv_capacity) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        printf("Failed to open file: %s\n", file_path);
        return 0;
    }

    char line[256];
    int count = 0;

    // Skip the header line
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file)) {
        if (count >= *inv_capacity) {
            int new_capacity = *inv_capacity * 2;
            invItem *temp = realloc(*inv_list, new_capacity * sizeof(invItem));
            if (!temp) {
                printf("Memory allocation failed during resizing.\n");
                fclose(file);
                return 0;
            }
            *inv_list = temp;
            *inv_capacity = new_capacity;
        }

        // Ensure we're storing each entry separately
        invItem *item = &(*inv_list)[count];  // Assign a pointer to the correct index
        // Check if loading weapons or aid based on the file name
        if (strstr(file_path, "weapons.csv")) {
            sscanf(line, "%49[^,],%d,%f,%d,%d,%d,%d,%d,%d",
                item->name, &item->quantity, &item->weight, &item->damage,
                &item->ammo, &item->fire_rate, &item->range, &item->accuracy, &item->value);
        } else if (strstr(file_path, "apparel.csv")) {
            sscanf(line, "%49[^,],%d,%f,%d",
                item->name, &item->quantity, &item->weight, &item->value);
        } else if (strstr(file_path, "aid.csv")) {
            sscanf(line, "%49[^,],%d,%f,%d",
                item->name, &item->quantity, &item->weight, &item->value);
        } else if (strstr(file_path, "misc.csv")) {
            sscanf(line, "%49[^,],%d,%f,%d",
                item->name, &item->quantity, &item->weight, &item->value);
        } else if (strstr(file_path, "junk.csv")) {
            sscanf(line, "%49[^,],%d,%f,%d",
                item->name, &item->quantity, &item->weight, &item->value);
        } else if (strstr(file_path, "mods.csv")) {
            sscanf(line, "%49[^,],%d,%f,%d",
                item->name, &item->quantity, &item->weight, &item->value);
        } else if (strstr(file_path, "ammo.csv")) {
            sscanf(line, "%49[^,],%d,%f,%d",
                item->name, &item->quantity, &item->weight, &item->value);
        } else {
            // Default parsing in case of unknown category
            sscanf(line, "%49[^,],%d,%f,%d",
                item->name, &item->quantity, &item->weight, &item->value);
        }
        /*sscanf(line, "%49[^,],%d,%f,%d,%d,%d,%d,%d,%d",
            item->name,
            &item->quantity,
            &item->weight,
            &item->damage,
            &item->ammo,
            &item->fire_rate,
            &item->range,
            &item->accuracy,
            &item->value); */

        count++;
    }

    fclose(file);
    *inv_count = count;
    return count;
}


// Reset inventory navigation when changing subtabs
void reset_inventory_navigation(PipState *state) {
    invItem *current_list = NULL;
    int current_count = 0;


    // Determine the current list and count based on the active subtab
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

    // Clamp selector_position and inv_scroll_index
    if (current_list) {
        if (state->selector_position >= current_count) {
            state->selector_position = (current_count > 0) ? current_count - 1 : 0;
        }
        if (state->inv_scroll_index > state->selector_position) {
            state->inv_scroll_index = state->selector_position;
        }
    } else {
        state->selector_position = 0;
        state->inv_scroll_index = 0;
    }
}

void handle_inventory_scroll(PipState *state, int direction);
