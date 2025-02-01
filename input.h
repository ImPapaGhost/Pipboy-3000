#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <SDL2/SDL.h>

// Input handling functions
void capture_input(SDL_Event *event);
bool input_queue_is_empty();
SDL_Keycode input_dequeue();

#endif