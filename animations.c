#include <stdio.h>
#include "animations.h"
#include "pipboy.h"
#include "ui.h"
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

void load_vaultboy_frames(SDL_Renderer *renderer) {
    char path[256];
    for (int i = 0; i < NUM_VAULTBOY_FRAMES; i++) {
        snprintf(path, sizeof(path), "STAT/VaultBoy/%02d.png", i);
        SDL_Surface *surface = IMG_Load(path);
        if (!surface) {
            vaultboy_frames[i] = NULL;
            continue;
        }
        vaultboy_frames[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
}

void free_vaultboy_frames() {
    for (int i = 0; i < NUM_VAULTBOY_FRAMES; i++) {
        if (vaultboy_frames[i]) {
            SDL_DestroyTexture(vaultboy_frames[i]);
            vaultboy_frames[i] = NULL;
        }
    }
}

void render_vaultboy(SDL_Renderer *renderer) {
    if (vaultboy_frames[vaultboy_frame_index]) {
        SDL_Rect dest_rect = {325, 130, 110, 200};

        // Render the Vault Boy with green tint
        SDL_SetTextureColorMod(vaultboy_frames[vaultboy_frame_index], 0, 255, 0); // Green tint
        SDL_RenderCopy(renderer, vaultboy_frames[vaultboy_frame_index], NULL, &dest_rect);

        // Reset the texture color mod to default
        SDL_SetTextureColorMod(vaultboy_frames[vaultboy_frame_index], 255, 255, 255);

        // Render damage bars
        render_damage_bar(renderer, 370, 120, 25, 5, damage_bars.head);       // Head
        render_damage_bar(renderer, 470, 190, 25, 5, damage_bars.left_arm);   // Left Arm
        render_damage_bar(renderer, 260, 190, 25, 5, damage_bars.right_arm);  // Right Arm
        render_damage_bar(renderer, 370, 330, 25, 5, damage_bars.torso);      // Torso
        render_damage_bar(renderer, 470, 280, 25, 5, damage_bars.left_leg);   // Left Leg
        render_damage_bar(renderer, 260, 280, 25, 5, damage_bars.right_leg);  // Right Leg
    }
}

void render_special_animation(SDL_Renderer *renderer, PipState *state) {
    static Uint32 last_frame_time = 0;
    static int frame_index = 0;
    Uint32 current_time = SDL_GetTicks();

    int current_stat = state->selector_position;

    // Determine the actual number of frames for the current stat
    int frame_count = 0;
    while (frame_count < 10 && state->special_animations[current_stat][frame_count] != NULL) {
        frame_count++;
    }

    if (frame_count == 0) return; // No frames available, skip rendering

    // Update frame every 100ms
    if (current_time - last_frame_time > 175) {
        frame_index = (frame_index + 1) % frame_count; // Cycle through available frames
        last_frame_time = current_time;
    }

    SDL_Texture *current_frame = state->special_animations[current_stat][frame_index];
    if (current_frame) {
        SDL_Rect dest_rect = {415, 65, 275, 225}; // Position and size of animation
        SDL_RenderCopy(renderer, current_frame, NULL, &dest_rect);
    }
}