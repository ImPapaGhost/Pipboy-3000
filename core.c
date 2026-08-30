#include "core.h"

#include <stdio.h>
#include <string.h>

#include "inventory.h"

int pipboy_effective_max_health(const PipState *state) {
    if (!state || state->max_health <= 0 || state->max_radiation <= 0) {
        return 0;
    }

    int radiation = state->radiation;
    if (radiation < 0) radiation = 0;
    if (radiation > state->max_radiation) radiation = state->max_radiation;
    const long long available = (long long)state->max_health *
                                (long long)(state->max_radiation - radiation);
    return (int)((available + (state->max_radiation / 2)) / state->max_radiation);
}

int pipboy_radiation_blocked_health(const PipState *state) {
    if (!state) return 0;
    const int blocked = state->max_health - pipboy_effective_max_health(state);
    return blocked > 0 ? blocked : 0;
}

static PipCommandOutcome outcome(PipCommandResult result, bool changed, const char *message) {
    PipCommandOutcome value = {result, changed, {0}};
    if (message) {
        snprintf(value.message, sizeof(value.message), "%s", message);
    }
    return value;
}

static bool item_is_in_list(const invItem *item, const invItem *items, int count) {
    for (int index = 0; items && index < count; index++) {
        if (item == &items[index]) {
            return true;
        }
    }
    return false;
}

static void clear_equipped(invItem *items, int count) {
    for (int index = 0; items && index < count; index++) {
        items[index].equipped = false;
    }
}

static PipCommandOutcome use_item(PipState *state, const char *id) {
    invItem *item = find_inventory_item(state, id);
    if (!item || !item_is_in_list(item, state->aid, state->aid_count)) {
        return outcome(PIP_COMMAND_NOT_FOUND, false, "ITEM CANNOT BE USED");
    }
    if (item->quantity <= 0) {
        return outcome(PIP_COMMAND_NO_QUANTITY, false, "NONE REMAINING");
    }

    int healed = 0;
    int rads_removed = 0;
    const int effective_max_health = pipboy_effective_max_health(state);
    if (item->heal_amount > 0 && state->health < effective_max_health) {
        healed = item->heal_amount;
        if (healed > effective_max_health - state->health) {
            healed = effective_max_health - state->health;
        }
        state->health += healed;
    }
    if (item->radiation_delta < 0 && state->radiation > 0) {
        rads_removed = -item->radiation_delta;
        if (rads_removed > state->radiation) {
            rads_removed = state->radiation;
        }
        state->radiation -= rads_removed;
    }

    if (healed == 0 && rads_removed == 0) {
        return outcome(PIP_COMMAND_NO_EFFECT, false, "NO EFFECT NEEDED");
    }

    item->quantity--;
    PipCommandOutcome result = outcome(PIP_COMMAND_OK, true, NULL);
    if (healed > 0 && rads_removed > 0) {
        snprintf(result.message, sizeof(result.message), "%s USED  +%d HP  -%d RADS", item->name, healed, rads_removed);
    } else if (healed > 0) {
        snprintf(result.message, sizeof(result.message), "%s USED  +%d HP", item->name, healed);
    } else {
        snprintf(result.message, sizeof(result.message), "%s USED  -%d RADS", item->name, rads_removed);
    }
    return result;
}

static PipCommandOutcome equip_item(PipState *state, const char *id) {
    invItem *item = find_inventory_item(state, id);
    if (!item || item->quantity <= 0) {
        return outcome(PIP_COMMAND_NOT_FOUND, false, "ITEM CANNOT BE EQUIPPED");
    }

    if (item_is_in_list(item, state->weapons, state->weapons_count)) {
        clear_equipped(state->weapons, state->weapons_count);
    } else if (item_is_in_list(item, state->apparel, state->apparel_count)) {
        clear_equipped(state->apparel, state->apparel_count);
    } else {
        return outcome(PIP_COMMAND_INVALID, false, "ITEM CANNOT BE EQUIPPED");
    }

    item->equipped = true;
    PipCommandOutcome result = outcome(PIP_COMMAND_OK, true, NULL);
    snprintf(result.message, sizeof(result.message), "%s EQUIPPED", item->name);
    return result;
}

static PipCommandOutcome toggle_favorite(PipState *state, const char *id) {
    invItem *item = find_inventory_item(state, id);
    if (!item || item->quantity <= 0) {
        return outcome(PIP_COMMAND_NOT_FOUND, false, "ITEM NOT FOUND");
    }

    item->favorite = !item->favorite;
    PipCommandOutcome result = outcome(PIP_COMMAND_OK, true, NULL);
    snprintf(
        result.message,
        sizeof(result.message),
        "%s %s FAVORITES",
        item->name,
        item->favorite ? "ADDED TO" : "REMOVED FROM"
    );
    return result;
}

static PipCommandOutcome drop_item(PipState *state, const char *id, int quantity) {
    invItem *item = find_inventory_item(state, id);
    if (!item) {
        return outcome(PIP_COMMAND_NOT_FOUND, false, "ITEM NOT FOUND");
    }
    if (item->quantity <= 0) {
        return outcome(PIP_COMMAND_NO_QUANTITY, false, "NONE REMAINING");
    }

    int amount = quantity > 0 ? quantity : 1;
    if (amount > item->quantity) {
        amount = item->quantity;
    }
    item->quantity -= amount;
    if (item->quantity == 0) {
        item->equipped = false;
        item->favorite = false;
    }

    PipCommandOutcome result = outcome(PIP_COMMAND_OK, true, NULL);
    snprintf(result.message, sizeof(result.message), "%s DROPPED  -%d", item->name, amount);
    return result;
}

static PipCommandOutcome take_damage(PipState *state, int amount) {
    if (amount <= 0 || state->health <= 0) {
        return outcome(PIP_COMMAND_NO_EFFECT, false, "NO DAMAGE APPLIED");
    }
    if (amount > state->health) {
        amount = state->health;
    }
    state->health -= amount;

    PipCommandOutcome result = outcome(PIP_COMMAND_OK, true, NULL);
    snprintf(result.message, sizeof(result.message), "DAMAGE TAKEN  -%d HP", amount);
    return result;
}

static PipCommandOutcome add_radiation(PipState *state, int amount) {
    if (amount <= 0 || state->radiation >= state->max_radiation) {
        return outcome(PIP_COMMAND_NO_EFFECT, false, "NO RADIATION APPLIED");
    }
    if (amount > state->max_radiation - state->radiation) {
        amount = state->max_radiation - state->radiation;
    }
    state->radiation += amount;
    const int effective_max_health = pipboy_effective_max_health(state);
    if (state->health > effective_max_health) {
        state->health = effective_max_health;
    }

    PipCommandOutcome result = outcome(PIP_COMMAND_OK, true, NULL);
    snprintf(
        result.message,
        sizeof(result.message),
        "RADIATION +%d RADS  MAX HP %d",
        amount,
        effective_max_health
    );
    return result;
}

PipCommandOutcome pipboy_execute_command(PipState *state, const PipCommand *command) {
    if (!state || !command) {
        return outcome(PIP_COMMAND_INVALID, false, "INVALID COMMAND");
    }

    switch (command->type) {
        case PIP_COMMAND_USE_ITEM:
            return use_item(state, command->target_id);
        case PIP_COMMAND_EQUIP_ITEM:
            return equip_item(state, command->target_id);
        case PIP_COMMAND_TOGGLE_FAVORITE:
            return toggle_favorite(state, command->target_id);
        case PIP_COMMAND_DROP_ITEM:
            return drop_item(state, command->target_id, command->value);
        case PIP_COMMAND_TAKE_DAMAGE:
            return take_damage(state, command->value);
        case PIP_COMMAND_ADD_RADIATION:
            return add_radiation(state, command->value);
    }

    return outcome(PIP_COMMAND_INVALID, false, "INVALID COMMAND");
}
