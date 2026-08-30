#ifndef CORE_H
#define CORE_H

#include <stdbool.h>

#include "pipboy.h"

typedef enum {
    PIP_COMMAND_USE_ITEM,
    PIP_COMMAND_EQUIP_ITEM,
    PIP_COMMAND_TOGGLE_FAVORITE,
    PIP_COMMAND_DROP_ITEM,
    PIP_COMMAND_TAKE_DAMAGE,
    PIP_COMMAND_ADD_RADIATION,
    PIP_COMMAND_RESET_TEST_VITALS
} PipCommandType;

typedef enum {
    PIP_COMMAND_OK,
    PIP_COMMAND_INVALID,
    PIP_COMMAND_NOT_FOUND,
    PIP_COMMAND_NO_QUANTITY,
    PIP_COMMAND_NO_EFFECT
} PipCommandResult;

typedef struct {
    PipCommandType type;
    const char *target_id;
    int value;
} PipCommand;

typedef struct {
    PipCommandResult result;
    bool state_changed;
    char message[128];
} PipCommandOutcome;

PipCommandOutcome pipboy_execute_command(PipState *state, const PipCommand *command);
int pipboy_effective_max_health(const PipState *state);
int pipboy_radiation_blocked_health(const PipState *state);

#endif
