#include "video.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#define PL_MPEG_IMPLEMENTATION
#include "third_party/pl_mpeg/pl_mpeg.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

struct VideoPlayer {
    plm_t *decoder;
    SDL_Texture *texture;
    double frame_duration;
    double accumulator;
    int width;
    int height;
    bool loop;
    bool ended;
    bool has_frame;
};

static bool upload_next_frame(VideoPlayer *player) {
    plm_frame_t *frame = plm_decode_video(player->decoder);

    if (!frame && player->loop) {
        plm_rewind(player->decoder);
        frame = plm_decode_video(player->decoder);
    }

    if (!frame) {
        player->ended = true;
        return false;
    }

    if (SDL_UpdateYUVTexture(
            player->texture,
            NULL,
            frame->y.data,
            (int)frame->y.width,
            frame->cb.data,
            (int)frame->cb.width,
            frame->cr.data,
            (int)frame->cr.width
        ) != 0) {
        fprintf(stderr, "Failed to upload video frame: %s\n", SDL_GetError());
        player->ended = true;
        return false;
    }

    player->has_frame = true;
    player->ended = false;
    return true;
}

VideoPlayer *video_player_create(SDL_Renderer *renderer, const char *path, bool loop) {
    if (!renderer || !path) {
        return NULL;
    }

    VideoPlayer *player = calloc(1, sizeof(*player));
    if (!player) {
        return NULL;
    }

    player->decoder = plm_create_with_filename(path);
    if (!player->decoder) {
        fprintf(stderr, "Could not open video %s\n", path);
        video_player_destroy(player);
        return NULL;
    }

    plm_set_audio_enabled(player->decoder, false);
    if (!plm_probe(player->decoder, 5 * 1024 * 1024) ||
        plm_get_num_video_streams(player->decoder) < 1) {
        fprintf(stderr, "No MPEG-1 video stream found in %s\n", path);
        video_player_destroy(player);
        return NULL;
    }

    player->width = plm_get_width(player->decoder);
    player->height = plm_get_height(player->decoder);
    const double framerate = plm_get_framerate(player->decoder);
    if (player->width <= 0 || player->height <= 0 || framerate <= 0.0) {
        fprintf(stderr, "Invalid video metadata in %s\n", path);
        video_player_destroy(player);
        return NULL;
    }

    player->texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING,
        player->width,
        player->height
    );
    if (!player->texture) {
        fprintf(stderr, "Could not create video texture for %s: %s\n", path, SDL_GetError());
        video_player_destroy(player);
        return NULL;
    }

    player->frame_duration = 1.0 / framerate;
    player->loop = loop;
    if (!upload_next_frame(player)) {
        fprintf(stderr, "Could not decode the first frame of %s\n", path);
        video_player_destroy(player);
        return NULL;
    }

    return player;
}

void video_player_destroy(VideoPlayer *player) {
    if (!player) {
        return;
    }

    if (player->texture) SDL_DestroyTexture(player->texture);
    if (player->decoder) plm_destroy(player->decoder);
    free(player);
}

void video_player_update(VideoPlayer *player, double elapsed_seconds) {
    if (!player || player->ended || elapsed_seconds <= 0.0) {
        return;
    }

    // Avoid decoding a large backlog after a breakpoint or a suspended window.
    if (elapsed_seconds > 0.25) {
        elapsed_seconds = 0.25;
    }

    player->accumulator += elapsed_seconds;
    while (player->accumulator >= player->frame_duration) {
        player->accumulator -= player->frame_duration;
        if (!upload_next_frame(player)) {
            player->accumulator = 0.0;
            break;
        }
    }
}

bool video_player_render(VideoPlayer *player, SDL_Renderer *renderer, const SDL_Rect *destination) {
    if (!player || !renderer || !player->has_frame) {
        return false;
    }
    return SDL_RenderCopy(renderer, player->texture, NULL, destination) == 0;
}

bool video_player_rewind(VideoPlayer *player) {
    if (!player) {
        return false;
    }

    plm_rewind(player->decoder);
    player->accumulator = 0.0;
    player->ended = false;
    player->has_frame = false;
    return upload_next_frame(player);
}

bool video_player_has_ended(const VideoPlayer *player) {
    return !player || player->ended;
}

void video_player_set_color_mod(VideoPlayer *player, Uint8 red, Uint8 green, Uint8 blue) {
    if (player && player->texture) {
        SDL_SetTextureColorMod(player->texture, red, green, blue);
    }
}

int video_player_width(const VideoPlayer *player) {
    return player ? player->width : 0;
}

int video_player_height(const VideoPlayer *player) {
    return player ? player->height : 0;
}

double video_player_framerate(const VideoPlayer *player) {
    return (player && player->frame_duration > 0.0) ? 1.0 / player->frame_duration : 0.0;
}

static VideoPlaybackResult wait_for_frame(Uint64 started_at, double frame_duration) {
    const Uint64 frequency = SDL_GetPerformanceFrequency();

    while ((double)(SDL_GetPerformanceCounter() - started_at) / (double)frequency < frame_duration) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return VIDEO_PLAYBACK_QUIT;
            }
            if (event.type == SDL_KEYDOWN) {
                return VIDEO_PLAYBACK_SKIPPED;
            }
        }
        SDL_Delay(1);
    }

    return VIDEO_PLAYBACK_FINISHED;
}

VideoPlaybackResult video_play_blocking(SDL_Renderer *renderer, const char *path) {
    VideoPlayer *player = video_player_create(renderer, path, false);
    if (!player) {
        return VIDEO_PLAYBACK_ERROR;
    }

    VideoPlaybackResult result = VIDEO_PLAYBACK_FINISHED;
    while (!video_player_has_ended(player)) {
        const Uint64 frame_started_at = SDL_GetPerformanceCounter();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        video_player_render(player, renderer, NULL);
        SDL_RenderPresent(renderer);

        result = wait_for_frame(frame_started_at, player->frame_duration);
        if (result != VIDEO_PLAYBACK_FINISHED) {
            break;
        }

        if (!upload_next_frame(player)) {
            result = VIDEO_PLAYBACK_FINISHED;
            break;
        }
    }

    video_player_destroy(player);
    return result;
}
