#ifndef INVENTORY_H
#define INVENTORY_H

#include "pipboy.h"

typedef struct {
    invItem *items;
    int count;
} InventoryView;

// Function Prototypes
int get_ammo_count(const char *ammo_type, PipState *state);
InventoryView get_inventory_view(PipState *state);
void reset_inventory_navigation(PipState *state);


#endif
