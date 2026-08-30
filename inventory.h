#ifndef INVENTORY_H
#define INVENTORY_H

#include "pipboy.h"

typedef struct {
    invItem *items;
    int count;
} InventoryView;

// Function Prototypes
int get_ammo_count(const char *ammo_id, PipState *state);
invItem *find_inventory_item(PipState *state, const char *id);
int get_inventory_quantity(PipState *state, const char *id);
InventoryView get_inventory_view(PipState *state);
void reset_inventory_navigation(PipState *state);


#endif
