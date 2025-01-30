#include "inventory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to load inventory items from a file
int load_inv(const char *file_path, invItem **inv_list, int *inv_count, int *inv_capacity) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        printf("Failed to open file: %s\n", file_path);
        return 0;
    }

    char line[256];
    int count = 0;

    // Ensure initial memory allocation
    if (*inv_capacity == 0) {
        *inv_capacity = 10;
        *inv_list = malloc(*inv_capacity * sizeof(invItem));
    }

    while (fgets(line, sizeof(line), file)) {
        if (count >= *inv_capacity) {
            *inv_capacity *= 2;
            *inv_list = realloc(*inv_list, *inv_capacity * sizeof(invItem));
        }

        sscanf(line, "%[^,],%d,%f", (*inv_list)[count].name, &(*inv_list)[count].quantity, &(*inv_list)[count].weight);
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

