#include "animation.h"
#include "pipboy.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

void play_animation(SDL_Renderer *renderer, SDL_Texture *frames[], int frame_count, int frame_delay) {
    for (int i = 0; i < frame_count; i++) {
        if (frames[i]) {
            SDL_RenderClear(renderer);                     // Clear the renderer
            SDL_RenderCopy(renderer, frames[i], NULL, NULL); // Render the current frame
            SDL_RenderPresent(renderer);                  // Present the frame
            SDL_Delay(frame_delay);                       // Add delay for frame timing
        }
    }
}
