#ifndef INVENTORY_H
#define INVENTORY_H

#include "pipboy.h"

// Function Prototypes
int load_inv(const char *file_path, invItem **inv_list, int *inv_count, int *inv_capacity);
void reset_inventory_navigation(PipState *state);

#endif
