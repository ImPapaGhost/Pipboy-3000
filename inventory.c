#include "inventory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pipboy.h"

int get_ammo_count(const char *ammo_type, PipState *state) {
    int total = 0;

    if (!ammo_type || !state) {
        return 0;
    }

    for (int i = 0; i < state->ammo_count; i++) {
        if (strcmp(state->ammo[i].name, ammo_type) == 0) {
            total += state->ammo[i].quantity;
        }
    }

    return total;
}

InventoryView get_inventory_view(PipState *state) {
    InventoryView view = {NULL, 0};

    if (!state) {
        return view;
    }

    switch (state->current_inv_subtab) {
        case SUBTAB_WEAPONS:
            view.items = state->weapons;
            view.count = state->weapons_count;
            break;
        case SUBTAB_APPAREL:
            view.items = state->apparel;
            view.count = state->apparel_count;
            break;
        case SUBTAB_AID:
            view.items = state->aid;
            view.count = state->aid_count;
            break;
        case SUBTAB_MISC:
            view.items = state->misc;
            view.count = state->misc_count;
            break;
        case SUBTAB_JUNK:
            view.items = state->junk;
            view.count = state->junk_count;
            break;
        case SUBTAB_MODS:
            view.items = state->mods;
            view.count = state->mods_count;
            break;
        case SUBTAB_AMMO:
            view.items = state->ammo;
            view.count = state->ammo_count;
            break;
        case NUM_INV_SUBTABS:
            break;
    }

    return view;
}

static int reserve_items(invItem **items, int *capacity, int required) {
    if (*capacity >= required && *items) {
        return 1;
    }

    int new_capacity = (*capacity > 0) ? *capacity : 8;
    while (new_capacity < required) {
        new_capacity *= 2;
    }

    invItem *resized = realloc(*items, (size_t)new_capacity * sizeof(*resized));
    if (!resized) {
        return 0;
    }

    *items = resized;
    *capacity = new_capacity;
    return 1;
}

// Function to load inventory items from a file
int load_inv(const char *file_path, invItem **inv_list, int *inv_count, int *inv_capacity) {
    if (!file_path || !inv_list || !inv_count || !inv_capacity) {
        return -1;
    }

    FILE *file = fopen(file_path, "r");
    if (!file) {
        fprintf(stderr, "Failed to open inventory file: %s\n", file_path);
        return -1;
    }

    char line[256];
    int count = 0;

    // Skip the header line
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        *inv_count = 0;
        return 0;
    }

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') {
            continue;
        }

        if (!reserve_items(inv_list, inv_capacity, count + 1)) {
            fprintf(stderr, "Memory allocation failed while loading %s.\n", file_path);
            fclose(file);
            *inv_count = count;
            return -1;
        }

        invItem item = {0};
        int parsed_fields = 0;
        int required_fields = 4;

        if (strstr(file_path, "weapons.csv")) {
            required_fields = 11;
            parsed_fields = sscanf(line, "%49[^,],%d,%f,%d,%d,%49[^,],%19[^,],%d,%d,%d,%d",
                item.name, &item.quantity, &item.weight, &item.damage,
                &item.ammo, item.ammo_type, item.speed, &item.fire_rate,
                &item.range, &item.accuracy, &item.value);
        } else if (strstr(file_path, "apparel.csv")) {
            parsed_fields = sscanf(line, "%49[^,],%d,%f,%d",
                item.name, &item.quantity, &item.weight, &item.value);
        } else if (strstr(file_path, "aid.csv")) {
            parsed_fields = sscanf(line, "%49[^,],%d,%f,%d",
                item.name, &item.quantity, &item.weight, &item.value);
        } else if (strstr(file_path, "misc.csv")) {
            parsed_fields = sscanf(line, "%49[^,],%d,%f,%d",
                item.name, &item.quantity, &item.weight, &item.value);
        } else if (strstr(file_path, "junk.csv")) {
            required_fields = 5;
            parsed_fields = sscanf(line, "%49[^,],%d,%f,%d,%49[^\r\n]",
                item.name, &item.quantity, &item.weight, &item.value, item.component);

            size_t length = strlen(item.component);
            if (length > 1 && item.component[0] == '"' && item.component[length - 1] == '"') {
                memmove(item.component, item.component + 1, length - 2);
                item.component[length - 2] = '\0';
            }
        } else if (strstr(file_path, "mods.csv")) {
            parsed_fields = sscanf(line, "%49[^,],%d,%f,%d",
                item.name, &item.quantity, &item.weight, &item.value);
        } else if (strstr(file_path, "ammo.csv")) {
            parsed_fields = sscanf(line, "%49[^,],%d,%f,%d",
                item.name, &item.quantity, &item.weight, &item.value);
        } else {
            parsed_fields = sscanf(line, "%49[^,],%d,%f,%d",
                item.name, &item.quantity, &item.weight, &item.value);
        }

        if (parsed_fields < required_fields) {
            fprintf(stderr, "Skipping malformed row in %s: %s", file_path, line);
            continue;
        }

        (*inv_list)[count++] = item;
    }

    fclose(file);
    *inv_count = count;
    return count;
}


// Reset inventory navigation when changing subtabs
void reset_inventory_navigation(PipState *state) {
    InventoryView view = get_inventory_view(state);

    // Clamp selector_position and inv_scroll_index
    if (view.items) {
        if (state->selector_position >= view.count) {
            state->selector_position = (view.count > 0) ? view.count - 1 : 0;
        }
        if (state->inv_scroll_index > state->selector_position) {
            state->inv_scroll_index = state->selector_position;
        }
    } else {
        state->selector_position = 0;
        state->inv_scroll_index = 0;
    }
}
