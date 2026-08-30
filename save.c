#include "save.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <direct.h>
#define create_directory(path) _mkdir(path)
#else
#include <sys/stat.h>
#define create_directory(path) mkdir(path, 0755)
#endif

#include "inventory.h"
#include "core.h"
#include "third_party/cjson/cJSON.h"

#define PIP_SAVE_VERSION 1
#define MAX_SAVE_BYTES (4L * 1024L * 1024L)

static char *read_text_file(const char *path, size_t *length_out);

static bool save_file_is_valid(const char *path) {
    size_t length = 0;
    char *text = read_text_file(path, &length);
    if (!text) return false;
    cJSON *root = cJSON_ParseWithLength(text, length);
    free(text);
    if (!root) return false;

    const cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
    const cJSON *player = cJSON_GetObjectItemCaseSensitive(root, "player");
    const cJSON *inventory = cJSON_GetObjectItemCaseSensitive(root, "inventory");
    const bool valid = cJSON_IsNumber(version) && version->valueint == PIP_SAVE_VERSION &&
                       cJSON_IsObject(player) && cJSON_IsArray(inventory);
    cJSON_Delete(root);
    return valid;
}

static bool ensure_parent_directory(const char *path) {
    char directory[1024];
    if (!path || strlen(path) >= sizeof(directory)) {
        return false;
    }
    snprintf(directory, sizeof(directory), "%s", path);

    char *separator = strrchr(directory, '/');
    char *backslash = strrchr(directory, '\\');
    if (!separator || (backslash && backslash > separator)) {
        separator = backslash;
    }
    if (!separator) {
        return true;
    }

    *separator = '\0';
    if (directory[0] == '\0') {
        return true;
    }

    if (create_directory(directory) == 0 || errno == EEXIST) {
        return true;
    }
    fprintf(stderr, "Could not create save directory %s\n", directory);
    return false;
}

static void add_inventory_list(cJSON *array, const invItem *items, int count) {
    for (int index = 0; items && index < count; index++) {
        cJSON *entry = cJSON_CreateObject();
        if (!entry) continue;
        cJSON_AddStringToObject(entry, "id", items[index].id);
        cJSON_AddNumberToObject(entry, "quantity", items[index].quantity);
        cJSON_AddBoolToObject(entry, "equipped", items[index].equipped);
        cJSON_AddBoolToObject(entry, "favorite", items[index].favorite);
        cJSON_AddItemToArray(array, entry);
    }
}

static cJSON *state_to_json(const PipState *state) {
    cJSON *root = cJSON_CreateObject();
    cJSON *player = cJSON_CreateObject();
    cJSON *inventory = cJSON_CreateArray();
    if (!root || !player || !inventory) {
        cJSON_Delete(root);
        cJSON_Delete(player);
        cJSON_Delete(inventory);
        return NULL;
    }

    cJSON_AddNumberToObject(root, "version", PIP_SAVE_VERSION);
    cJSON_AddItemToObject(root, "player", player);
    cJSON_AddNumberToObject(player, "health", state->health);
    cJSON_AddNumberToObject(player, "max_health", state->max_health);
    cJSON_AddNumberToObject(player, "ap", state->ap);
    cJSON_AddNumberToObject(player, "max_ap", state->max_ap);
    cJSON_AddNumberToObject(player, "radiation", state->radiation);
    cJSON_AddNumberToObject(player, "max_radiation", state->max_radiation);
    cJSON_AddNumberToObject(player, "level", state->level);
    cJSON_AddNumberToObject(player, "current_xp", state->current_xp);
    cJSON_AddNumberToObject(player, "xp_for_next_level", state->xp_for_next_level);

    cJSON_AddItemToObject(root, "inventory", inventory);
    add_inventory_list(inventory, state->weapons, state->weapons_count);
    add_inventory_list(inventory, state->apparel, state->apparel_count);
    add_inventory_list(inventory, state->aid, state->aid_count);
    add_inventory_list(inventory, state->misc, state->misc_count);
    add_inventory_list(inventory, state->junk, state->junk_count);
    add_inventory_list(inventory, state->mods, state->mods_count);
    add_inventory_list(inventory, state->ammo, state->ammo_count);
    return root;
}

static bool write_text_file(const char *path, const char *text) {
    FILE *file = fopen(path, "wb");
    if (!file) {
        return false;
    }
    const size_t length = strlen(text);
    bool written = fwrite(text, 1, length, file) == length;
    if (fflush(file) != 0) written = false;
    if (fclose(file) != 0) written = false;
    if (!written) {
        remove(path);
    }
    return written;
}

bool save_pip_state(const PipState *state, const char *path) {
    if (!state || !path || !ensure_parent_directory(path)) {
        return false;
    }

    cJSON *root = state_to_json(state);
    if (!root) {
        return false;
    }
    char *json = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json) {
        return false;
    }

    char temporary_path[1024];
    char backup_path[1024];
    char corrupt_path[1024];
    if (snprintf(temporary_path, sizeof(temporary_path), "%s.tmp", path) >= (int)sizeof(temporary_path) ||
        snprintf(backup_path, sizeof(backup_path), "%s.backup", path) >= (int)sizeof(backup_path) ||
        snprintf(corrupt_path, sizeof(corrupt_path), "%s.corrupt", path) >= (int)sizeof(corrupt_path)) {
        cJSON_free(json);
        return false;
    }

    const bool temporary_written = write_text_file(temporary_path, json);
    cJSON_free(json);
    if (!temporary_written) {
        fprintf(stderr, "Could not write temporary save file %s\n", temporary_path);
        return false;
    }

    FILE *current = fopen(path, "rb");
    const bool current_exists = current != NULL;
    if (current) fclose(current);

    bool had_previous = false;
    bool moved_corrupt = false;
    if (current_exists && save_file_is_valid(path)) {
        remove(backup_path);
        had_previous = rename(path, backup_path) == 0;
        if (!had_previous) {
            remove(temporary_path);
            return false;
        }
    } else if (current_exists) {
        remove(corrupt_path);
        moved_corrupt = rename(path, corrupt_path) == 0;
        if (!moved_corrupt) {
            remove(temporary_path);
            return false;
        }
    }

    if (rename(temporary_path, path) != 0) {
        fprintf(stderr, "Could not install save file %s\n", path);
        if (had_previous) rename(backup_path, path);
        if (moved_corrupt) rename(corrupt_path, path);
        remove(temporary_path);
        return false;
    }
    return true;
}

static char *read_text_file(const char *path, size_t *length_out) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    const long file_size = ftell(file);
    if (file_size < 0 || file_size > MAX_SAVE_BYTES || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char *text = malloc((size_t)file_size + 1);
    if (!text) {
        fclose(file);
        return NULL;
    }
    const size_t length = fread(text, 1, (size_t)file_size, file);
    fclose(file);
    if (length != (size_t)file_size) {
        free(text);
        return NULL;
    }
    text[length] = '\0';
    if (length_out) *length_out = length;
    return text;
}

static int json_int(const cJSON *object, const char *name, int fallback, int minimum, int maximum) {
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, name);
    if (!cJSON_IsNumber(value)) return fallback;
    int number = value->valueint;
    if (number < minimum) number = minimum;
    if (number > maximum) number = maximum;
    return number;
}

static void normalize_equipped_items(invItem *items, int count) {
    bool found_equipped = false;
    for (int index = 0; items && index < count; index++) {
        if (items[index].equipped && !found_equipped && items[index].quantity > 0) {
            found_equipped = true;
        } else if (items[index].equipped) {
            items[index].equipped = false;
        }
    }
}

static bool apply_json(PipState *state, const cJSON *root) {
    const cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
    const cJSON *player = cJSON_GetObjectItemCaseSensitive(root, "player");
    const cJSON *inventory = cJSON_GetObjectItemCaseSensitive(root, "inventory");
    if (!cJSON_IsNumber(version) || version->valueint != PIP_SAVE_VERSION ||
        !cJSON_IsObject(player) || !cJSON_IsArray(inventory)) {
        return false;
    }

    state->max_health = json_int(player, "max_health", state->max_health, 1, 100000);
    state->health = json_int(player, "health", state->health, 0, state->max_health);
    state->max_ap = json_int(player, "max_ap", state->max_ap, 1, 100000);
    state->ap = json_int(player, "ap", state->ap, 0, state->max_ap);
    state->max_radiation = json_int(player, "max_radiation", state->max_radiation, 1, 100000);
    state->radiation = json_int(player, "radiation", state->radiation, 0, state->max_radiation);
    const int effective_max_health = pipboy_effective_max_health(state);
    if (state->health > effective_max_health) {
        state->health = effective_max_health;
    }
    state->level = json_int(player, "level", state->level, 1, 100000);
    state->current_xp = json_int(player, "current_xp", state->current_xp, 0, 100000000);
    state->xp_for_next_level = json_int(
        player,
        "xp_for_next_level",
        state->xp_for_next_level,
        1,
        100000000
    );

    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, inventory) {
        const cJSON *id = cJSON_GetObjectItemCaseSensitive(entry, "id");
        if (!cJSON_IsString(id) || !id->valuestring) continue;
        invItem *item = find_inventory_item(state, id->valuestring);
        if (!item) continue;
        item->quantity = json_int(entry, "quantity", item->quantity, 0, 1000000);
        const cJSON *equipped = cJSON_GetObjectItemCaseSensitive(entry, "equipped");
        const cJSON *favorite = cJSON_GetObjectItemCaseSensitive(entry, "favorite");
        item->equipped = cJSON_IsTrue(equipped);
        item->favorite = cJSON_IsTrue(favorite);
        if (item->quantity == 0) {
            item->equipped = false;
            item->favorite = false;
        }
    }
    normalize_equipped_items(state->weapons, state->weapons_count);
    normalize_equipped_items(state->apparel, state->apparel_count);
    return true;
}

static bool load_from_path(PipState *state, const char *path) {
    size_t length = 0;
    char *text = read_text_file(path, &length);
    if (!text) return false;
    cJSON *root = cJSON_ParseWithLength(text, length);
    free(text);
    if (!root) return false;
    const bool applied = apply_json(state, root);
    cJSON_Delete(root);
    return applied;
}

PipSaveLoadResult load_pip_state(PipState *state, const char *path) {
    if (!state || !path) {
        return PIP_SAVE_ERROR;
    }
    if (load_from_path(state, path)) {
        return PIP_SAVE_LOADED;
    }

    FILE *primary = fopen(path, "rb");
    const bool primary_exists = primary != NULL;
    if (primary) fclose(primary);

    char backup_path[1024];
    if (snprintf(backup_path, sizeof(backup_path), "%s.backup", path) >= (int)sizeof(backup_path)) {
        return PIP_SAVE_ERROR;
    }
    if (load_from_path(state, backup_path)) {
        return PIP_SAVE_RECOVERED_BACKUP;
    }
    return primary_exists ? PIP_SAVE_ERROR : PIP_SAVE_NOT_FOUND;
}
