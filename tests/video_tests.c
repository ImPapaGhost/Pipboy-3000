#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "video.h"

typedef struct {
    const char *path;
    int width;
    int height;
} VideoExpectation;

static const VideoExpectation videos[] = {
    {"BOOT/bootup.mpg", 800, 480},
    {"BOOT/bootboy.mpg", 800, 480},
    {"STAT/vaultboy.mpg", 160, 256},
    {"STAT/vaultboy-combat.mpg", 160, 256},
    {"STAT/strength.mpg", 304, 176},
    {"STAT/perception.mpg", 304, 176},
    {"STAT/endurance.mpg", 304, 176},
    {"STAT/charisma.mpg", 304, 176},
    {"STAT/intelligence.mpg", 304, 176},
    {"STAT/agility.mpg", 304, 176},
    {"STAT/luck.mpg", 304, 176},
    {"RADIO/radio-waveform.mpg", 224, 224},
};

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        SDL_DestroyRenderer(renderer); \
        SDL_DestroyWindow(window); \
        SDL_Quit(); \
        return EXIT_FAILURE; \
    } \
} while (0)

int main(void) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    SDL_SetMainReady();
    CHECK(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);

    window = SDL_CreateWindow(
        "video-tests",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        800,
        480,
        SDL_WINDOW_HIDDEN
    );
    CHECK(window != NULL);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    CHECK(renderer != NULL);

    for (size_t index = 0; index < sizeof(videos) / sizeof(videos[0]); index++) {
        VideoPlayer *player = video_player_create(renderer, videos[index].path, true);
        CHECK(player != NULL);
        CHECK(video_player_width(player) == videos[index].width);
        CHECK(video_player_height(player) == videos[index].height);
        CHECK(fabs(video_player_framerate(player) - 25.0) < 0.001);
        CHECK(!video_player_has_ended(player));

        SDL_Rect destination = {0, 0, videos[index].width, videos[index].height};
        for (int step = 0; step < 50; step++) {
            video_player_update(player, 0.25);
            CHECK(video_player_render(player, renderer, &destination));
            CHECK(!video_player_has_ended(player));
        }
        CHECK(video_player_rewind(player));
        video_player_destroy(player);
    }

    VideoPlayer *single_run = video_player_create(renderer, "BOOT/bootboy.mpg", false);
    CHECK(single_run != NULL);
    for (int step = 0; step < 10; step++) {
        video_player_update(single_run, 0.25);
    }
    CHECK(video_player_has_ended(single_run));
    CHECK(video_player_rewind(single_run));
    CHECK(!video_player_has_ended(single_run));
    video_player_destroy(single_run);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
