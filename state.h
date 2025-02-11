#ifndef STATE_H
#define STATE_H

#include "pipboy.h"

// Function Prototypes
void initialize_pip_state(PipState *state);
void add_experience(PipState *state, int xp);
void update_damage(DamageBars *bars, int head, int left_arm, int right_arm, int torso, int left_leg, int right_leg);

#endif