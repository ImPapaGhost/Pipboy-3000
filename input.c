#include "input.h"
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define INPUT_QUEUE_SIZE 32 // Limits the number of stored inputs

typedef struct {
    SDL_Keycode key; // Store the key that was pressed
} InputEvent;

// Input event queue
static InputEvent input_queue[INPUT_QUEUE_SIZE];
static int queue_front = 0, queue_back = 0, queue_count = 0;

// **Queue Utility Functions**
bool input_queue_is_empty() {
    return queue_count == 0;
}

bool input_queue_is_full() {
    return queue_count >= INPUT_QUEUE_SIZE;
}

void input_enqueue(SDL_Keycode key) {
    if (!input_queue_is_full()) {
        input_queue[queue_back].key = key;
        queue_back = (queue_back + 1) % INPUT_QUEUE_SIZE;
        queue_count++;
    }
}

SDL_Keycode input_dequeue() {
    if (!input_queue_is_empty()) {
        SDL_Keycode key = input_queue[queue_front].key;
        queue_front = (queue_front + 1) % INPUT_QUEUE_SIZE;
        queue_count--;
        return key;
    }
    return SDLK_UNKNOWN; // Return an invalid key if queue is empty
}

// **Process Input Events (Called Every Frame)**
void capture_input(SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        input_enqueue(event->key.keysym.sym);
    }
}
