#include "animations.h"
#include "pipboy.h"
#include "ui.h"
#include <SDL2/SDL.h>

void render_vaultboy(SDL_Renderer *renderer, AppResources *resources, double elapsed_seconds) {
    if (resources && resources->vaultboy_video) {
        SDL_Rect destination = {325, 130, 110, 200};
        video_player_update(resources->vaultboy_video, elapsed_seconds);
        video_player_render(resources->vaultboy_video, renderer, &destination);
    }

    render_damage_bar(renderer, 370, 120, 25, 5, damage_bars.head);
    render_damage_bar(renderer, 470, 190, 25, 5, damage_bars.left_arm);
    render_damage_bar(renderer, 260, 190, 25, 5, damage_bars.right_arm);
    render_damage_bar(renderer, 370, 330, 25, 5, damage_bars.torso);
    render_damage_bar(renderer, 470, 280, 25, 5, damage_bars.left_leg);
    render_damage_bar(renderer, 260, 280, 25, 5, damage_bars.right_leg);
}

void render_special_animation(
    SDL_Renderer *renderer,
    AppResources *resources,
    PipState *state,
    double elapsed_seconds
) {
    if (!resources || !state || state->selector_position < 0 || state->selector_position >= 7) {
        return;
    }

    VideoPlayer *video = resources->special_videos[state->selector_position];
    if (!video) {
        return;
    }

    SDL_Rect destination = {415, 65, 275, 225};
    video_player_update(video, elapsed_seconds);
    video_player_render(video, renderer, &destination);
}
