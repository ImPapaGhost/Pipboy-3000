#ifndef VIDEO_H
#define VIDEO_H

#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct VideoPlayer VideoPlayer;

typedef enum {
    VIDEO_PLAYBACK_FINISHED,
    VIDEO_PLAYBACK_SKIPPED,
    VIDEO_PLAYBACK_QUIT,
    VIDEO_PLAYBACK_ERROR
} VideoPlaybackResult;

VideoPlayer *video_player_create(SDL_Renderer *renderer, const char *path, bool loop);
void video_player_destroy(VideoPlayer *player);

void video_player_update(VideoPlayer *player, double elapsed_seconds);
bool video_player_render(VideoPlayer *player, SDL_Renderer *renderer, const SDL_Rect *destination);
bool video_player_rewind(VideoPlayer *player);
bool video_player_has_ended(const VideoPlayer *player);

void video_player_set_color_mod(VideoPlayer *player, Uint8 red, Uint8 green, Uint8 blue);

int video_player_width(const VideoPlayer *player);
int video_player_height(const VideoPlayer *player);
double video_player_framerate(const VideoPlayer *player);

VideoPlaybackResult video_play_blocking(SDL_Renderer *renderer, const char *path);

#endif
