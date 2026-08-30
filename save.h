#ifndef SAVE_H
#define SAVE_H

#include <stdbool.h>

#include "pipboy.h"

#define PIP_SAVE_PATH "saves/player.json"

typedef enum {
    PIP_SAVE_LOADED,
    PIP_SAVE_RECOVERED_BACKUP,
    PIP_SAVE_NOT_FOUND,
    PIP_SAVE_ERROR
} PipSaveLoadResult;

bool save_pip_state(const PipState *state, const char *path);
PipSaveLoadResult load_pip_state(PipState *state, const char *path);

#endif
