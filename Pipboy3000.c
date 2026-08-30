#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(_WIN32)
#include <direct.h>
#define change_directory _chdir
#else
#include <unistd.h>
#define change_directory chdir
#endif
#include "pipboy.h"
#include "inventory.h"
#include "render.h"
#include "animations.h"
#include "input.h"
#include "ui.h"
#include "events.h"
#include "core.h"
#include "MAP/map.h"
#include "save.h"


// Initialize damage bars with full health
DamageBars damage_bars = {100, 100, 100, 100, 100, 100};

static void render_status_content(
    SDL_Renderer *renderer,
    AppResources *resources,
    PipState *state,
    double elapsed_seconds
);
static void render_special_content(SDL_Renderer *renderer, const AppResources *resources, PipState *state);
static void render_perks_content(SDL_Renderer *renderer, const AppResources *resources, PipState *state);

static bool runtime_assets_available(void) {
    FILE *font = fopen("monofonto.ttf", "rb");
    FILE *boot_video = fopen("BOOT/bootup.mpg", "rb");
    const bool available = font && boot_video;
    if (font) fclose(font);
    if (boot_video) fclose(boot_video);
    return available;
}

static void trim_to_parent_directory(char *path) {
    size_t length = strlen(path);
    while (length > 0 && (path[length - 1] == '/' || path[length - 1] == '\\')) {
        path[--length] = '\0';
    }
    while (length > 0 && path[length - 1] != '/' && path[length - 1] != '\\') {
        length--;
    }
    path[length] = '\0';
}

static bool select_runtime_asset_directory(void) {
    if (runtime_assets_available()) {
        return true;
    }

    char *base_path = SDL_GetBasePath();
    if (!base_path) {
        return false;
    }

    bool found = change_directory(base_path) == 0 && runtime_assets_available();
    if (!found) {
        trim_to_parent_directory(base_path);
        found = base_path[0] != '\0' &&
                change_directory(base_path) == 0 &&
                runtime_assets_available();
    }

    SDL_free(base_path);
    return found;
}

static bool has_argument(int argc, char *argv[], const char *argument) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], argument) == 0) {
            return true;
        }
    }
    return false;
}

static VideoPlaybackResult show_boot_animation(SDL_Renderer *renderer) {
    VideoPlaybackResult result = video_play_blocking(renderer, "BOOT/bootup.mpg");
    if (result != VIDEO_PLAYBACK_FINISHED) {
        return result;
    }

    return video_play_blocking(renderer, "BOOT/bootboy.mpg");
}

static void render_tabs(SDL_Renderer *renderer, const AppResources *resources, PipState *state) {
    const char *tab_names[] = {"STAT", "INV", "DATA", "MAP", "RADIO"};
    SDL_Color color = {0, 255, 0, 255}; // Green color for tab text
    int tab_spacing = 45; // Space between tabs

    TTF_Font *tab_font = resources->tab_font;

    // Calculate starting x-coordinate for centering
    // Calculate text widths for each tab
    int tab_text_widths[NUM_TABS];
    for (int i = 0; i < NUM_TABS; i++) {
        TTF_SizeText(tab_font, tab_names[i], &tab_text_widths[i], NULL);
    }

    // Calculate total width needed to align tabs by last letter
    int total_tab_width = 0;
    for (int i = 0; i < NUM_TABS; i++) {
        total_tab_width += tab_text_widths[i] + tab_spacing;
    }
    total_tab_width -= tab_spacing; // Remove last extra spacing

    int tab_x = (SCREEN_WIDTH - total_tab_width) / 2; // Center-align tabs
    int tab_y = 5; // Y position of the tabs

    // Render the CATEGORYLINE graphic (background line for all tabs)
    if (resources->category_line) {
        SDL_Rect categoryline_rect = {
            tab_x - 90,                // Adjust to extend slightly to the left
            tab_y - -32,               // Adjust to appear slightly below the tabs
            total_tab_width + 175,     // Adjust to match the width of tabs
            12                         // Height with padding
        };
        SDL_SetTextureColorMod(resources->category_line, 0, 255, 0);
        SDL_RenderCopy(renderer, resources->category_line, NULL, &categoryline_rect);
    }

    // Render the SELECTLINE graphic around the active tab
    if (resources->select_line) {
        int active_tab_x = tab_x;
        for (int i = 0; i < (int)state->current_tab; i++) {
            active_tab_x += tab_text_widths[i] + tab_spacing;
        }

        SDL_Rect selectline_rect = {
            active_tab_x - 10, // Slight offset for alignment
            tab_y + 11,        // Position slightly above the tab text
            tab_text_widths[state->current_tab] + 18, // Adjust width to match the tab
            26                // Height to cover the tab area
        };
        SDL_SetTextureColorMod(resources->select_line, 0, 255, 0);
        SDL_RenderCopy(renderer, resources->select_line, NULL, &selectline_rect);
    }

    // Render each tab text on top of CATEGORYLINE and SELECTLINE
    int current_x = tab_x;
    for (int i = 0; i < NUM_TABS; i++) {
        SDL_Surface *surface = TTF_RenderText_Solid(tab_font, tab_names[i], color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect text_rect = {
            current_x,
            tab_y,
            surface->w,
            surface->h
        };
        SDL_RenderCopy(renderer, texture, NULL, &text_rect);
        current_x += tab_text_widths[i] + tab_spacing;
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void render_attribute_description(SDL_Renderer *renderer, TTF_Font *font, int selector_position) {
    // Manually pre-split descriptions into lines
    const char *descriptions[][5] = {
        {"Strength is a measure of your raw physical power.",
         "It affects how much you can carry and",
         "determines the effectiveness of melee attacks."},

        {"Perception is your environmental awareness",
         "and 'sixth sense,' and affects weapon",
         "accuracy in V.A.T.S."},

        {"Endurance is a measure of your overall",
         "physical fitness. It affects your total health,",
         "and your resistance to damage and radiation."},

        {"Charisma is your ability to charm and convince.",
         "It affects your success in persuasion and",
         "prices when you barter."},

        {"Intelligence is a measure of your mental acuity.",
         "It affects the number of Experience Points",
         "earned."},

        {"Agility is a measure of your finesse and reflexes.",
         "It affects the number of action points in V.A.T.S.",
         "and your ability to sneak."},

        {"Luck is a measure of your general good fortune.",
         "It affects the recharge rate of critical hits."}
    };

    SDL_Color color = {0, 255, 0, 255};

    // Render each line of the description
    for (int i = 0; i < 5; i++) {
        if (descriptions[selector_position][i] == NULL) {
            break; // Stop rendering if there are no more lines
        }

        SDL_Surface *line_surface = TTF_RenderText_Solid(font, descriptions[selector_position][i], color);
        SDL_Texture *line_texture = SDL_CreateTextureFromSurface(renderer, line_surface);

        // Adjust the position of each line
        SDL_Rect line_rect = {250, 350 + i * 25, line_surface->w, line_surface->h}; // Y-coordinate increments by 25 for each line
        SDL_RenderCopy(renderer, line_texture, NULL, &line_rect);

        // Clean up resources for this line
        SDL_FreeSurface(line_surface);
        SDL_DestroyTexture(line_texture);
    }
}

static void render_stat_tab(
    SDL_Renderer *renderer,
    AppResources *resources,
    PipState *state,
    double elapsed_seconds
) {
    SDL_Color color = {0, 255, 0, 255};

    // Render the sub-tabs
    render_stat_subtabs(renderer, resources, state);

    // Render content based on the active sub-tab
    switch (state->current_subtab) {
        case SUBTAB_STATUS:
            render_status_content(renderer, resources, state, elapsed_seconds);
            break;
        case SUBTAB_SPECIAL:
            render_special_content(renderer, resources, state);
            break;
        case SUBTAB_PERKS:
            render_perks_content(renderer, resources, state);
            break;
        case NUM_SUBTABS:
            break;
    }

    // Render general stats at the bottom
    TTF_Font *detail_font = resources->detail_font;
    TTF_Font *footer_font = resources->body_font;
    const int effective_max_health = pipboy_effective_max_health(state);
    char hp_text[20];
    snprintf(hp_text, sizeof(hp_text), "HP %d/%d", state->health, effective_max_health);
    SDL_Surface *hp_surface = TTF_RenderText_Solid(footer_font, hp_text, color);
    SDL_Texture *hp_texture = SDL_CreateTextureFromSurface(renderer, hp_surface);
    SDL_Rect hp_rect = {115, 433, hp_surface->w, hp_surface->h}; // Left aligned
    SDL_RenderCopy(renderer, hp_texture, NULL, &hp_rect);
    SDL_FreeSurface(hp_surface);
    SDL_DestroyTexture(hp_texture);

    if (state->radiation > 0) {
        const SDL_Color radiation_color = {255, 96, 32, 255};
        char radiation_text[20];
        snprintf(radiation_text, sizeof(radiation_text), "RAD %d", state->radiation);
        SDL_Surface *radiation_surface = TTF_RenderText_Solid(
            footer_font,
            radiation_text,
            radiation_color
        );
        SDL_Texture *radiation_texture = SDL_CreateTextureFromSurface(renderer, radiation_surface);
        SDL_Rect radiation_rect = {
            285 - radiation_surface->w,
            433,
            radiation_surface->w,
            radiation_surface->h
        };
        SDL_RenderCopy(renderer, radiation_texture, NULL, &radiation_rect);
        SDL_FreeSurface(radiation_surface);
        SDL_DestroyTexture(radiation_texture);
    }

    char ap_text[20];
    snprintf(ap_text, sizeof(ap_text), "AP %d/%d", state->ap, state->max_ap);
    SDL_Surface *ap_surface = TTF_RenderText_Solid(detail_font, ap_text, color);
    SDL_Texture *ap_texture = SDL_CreateTextureFromSurface(renderer, ap_surface);
    SDL_Rect ap_rect = {SCREEN_WIDTH - ap_surface->w - 110, 432, ap_surface->w, ap_surface->h}; // Right aligned
    SDL_RenderCopy(renderer, ap_texture, NULL, &ap_rect);
    SDL_FreeSurface(ap_surface);
    SDL_DestroyTexture(ap_texture);

    char level_text[48];
    snprintf(
        level_text,
        sizeof(level_text),
        "LEVEL %d   XP %d/%d",
        state->level,
        state->current_xp,
        state->xp_for_next_level
    );
    SDL_Surface *level_surface = TTF_RenderText_Solid(footer_font, level_text, color);
    SDL_Texture *level_texture = SDL_CreateTextureFromSurface(renderer, level_surface);
    SDL_Rect level_rect = {
        295 + (255 - level_surface->w) / 2,
        433,
        level_surface->w,
        level_surface->h
    };
    SDL_RenderCopy(renderer, level_texture, NULL, &level_rect);
    SDL_FreeSurface(level_surface);
    SDL_DestroyTexture(level_texture);
}

static void render_status_content(
    SDL_Renderer *renderer,
    AppResources *resources,
    PipState *state,
    double elapsed_seconds
) {
    TTF_Font *font = resources->body_font;
    SDL_Color color = {0, 255, 0, 255};

    // Render Vault Boy animation
    render_vaultboy(renderer, resources, elapsed_seconds);

     // Render Stimpak Background
    SDL_Rect stimpak_rect = {110, 395, 100, 30}; // Adjust based on position and size
    if (resources->box_background) {
        SDL_SetTextureColorMod(resources->box_background, 100, 255, 100);
        SDL_RenderCopy(renderer, resources->box_background, NULL, &stimpak_rect);
    }

    // Render RadAway Background
    SDL_Rect radaway_rect = {225, 395, 100, 30}; // Adjust position to the right of Stimpak
    if (resources->box_background) {
        SDL_RenderCopy(renderer, resources->box_background, NULL, &radaway_rect);
    }

    // Render Stimpak Text
    const int stimpak_count = get_inventory_quantity(state, "aid_stimpak");
    const int radaway_count = get_inventory_quantity(state, "aid_radaway");
    char stimpak_text[20];
    snprintf(stimpak_text, sizeof(stimpak_text), "Stimpak (%d)", stimpak_count);
    SDL_Surface *stimpak_surface = TTF_RenderText_Solid(font, stimpak_text, color);
    SDL_Texture *stimpak_texture = SDL_CreateTextureFromSurface(renderer, stimpak_surface);
    SDL_Rect stimpak_text_rect = {115, 400, stimpak_surface->w, stimpak_surface->h}; // Center inside background
    SDL_RenderCopy(renderer, stimpak_texture, NULL, &stimpak_text_rect);
    SDL_FreeSurface(stimpak_surface);
    SDL_DestroyTexture(stimpak_texture);

    // Render RadAway Text
    char radaway_text[20];
    snprintf(radaway_text, sizeof(radaway_text), "RadAway (%d)", radaway_count);
    SDL_Surface *radaway_surface = TTF_RenderText_Solid(font, radaway_text, color);
    SDL_Texture *radaway_texture = SDL_CreateTextureFromSurface(renderer, radaway_surface);
    SDL_Rect radaway_text_rect = {230, 400, radaway_surface->w, radaway_surface->h}; // Center inside background
    SDL_RenderCopy(renderer, radaway_texture, NULL, &radaway_text_rect);
    SDL_FreeSurface(radaway_surface);
    SDL_DestroyTexture(radaway_texture);
}

static void render_special_content(SDL_Renderer *renderer, const AppResources *resources, PipState *state) {
    TTF_Font *font = resources->body_font;
    SDL_Color color = {0, 255, 0, 255};
    Uint32 current_time = SDL_GetTicks();
    float animation_progress = 1.0f; // Default to fully completed animation

    if (state->is_special_stat_animating) {
        animation_progress = (float)(current_time - state->special_stat_animation_start) / 300; // 300ms duration
        if (animation_progress >= 1.0f) {
            animation_progress = 1.0f;
            state->is_special_stat_animating = false; // Mark animation as complete
        }
    }

    // Use cubic easing for smoother transitions
    float vertical_offset = (float)state->special_stat_animation_offset *
                            (-1.0f + ease_out_cubic(animation_progress));

    // Render SPECIAL attributes list
    const char *attributes[] = {"Strength", "Perception", "Endurance", "Charisma", "Intelligence", "Agility", "Luck"};
    char attribute_text[50];
    int stat_x = 130; // X position for attribute names
    int value_x = 360; // X position for stat values
    int y_start = 100; // Starting Y position
    int y_spacing = 40; // Spacing between rows

    for (int i = 0; i < 7; i++) {
        // Render attribute name
        SDL_Surface *attr_surface = TTF_RenderText_Solid(font, attributes[i], color);
        SDL_Texture *attr_texture = SDL_CreateTextureFromSurface(renderer, attr_surface);
        SDL_Rect attr_rect = {stat_x, y_start + i * y_spacing, attr_surface->w, attr_surface->h};
        SDL_RenderCopy(renderer, attr_texture, NULL, &attr_rect);
        SDL_FreeSurface(attr_surface);
        SDL_DestroyTexture(attr_texture);

        // Render attribute value
        snprintf(attribute_text, sizeof(attribute_text), "%d", state->special_stats[i]);
        SDL_Surface *value_surface = TTF_RenderText_Solid(font, attribute_text, color);
        SDL_Texture *value_texture = SDL_CreateTextureFromSurface(renderer, value_surface);
        SDL_Rect value_rect = {value_x, y_start + i * y_spacing, value_surface->w, value_surface->h};
        SDL_RenderCopy(renderer, value_texture, NULL, &value_rect);
        SDL_FreeSurface(value_surface);
        SDL_DestroyTexture(value_texture);
    }

    // Render moving highlight box
    int highlight_y = y_start + state->selector_position * y_spacing + (int)vertical_offset;
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green color for the highlight
    SDL_Rect highlight_rect = {stat_x - 10, highlight_y - 5, value_x - stat_x + 50, y_spacing - 5};
    SDL_RenderDrawRect(renderer, &highlight_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Reset color

    // Render description (stays static)
    const char *descriptions[] = {
        "Strength is a measure of your raw physical power. It affects how much you can carry and determines the effectiveness of melee attacks.",
        "Perception is your environmental awareness and 'sixth sense,' and affects weapon accuracy in V.A.T.S.",
        "Endurance is a measure of your overall physical fitness. It affects your total health, and your resistance to damage and radiation.",
        "Charisma is your ability to charm and convince. It affects your success in persuasion and prices when you barter.",
        "Intelligence is a measure of your mental acuity. It affects the number of Experience Points earned.",
        "Agility is a measure of your finesse and reflexes. It affects the number of action points in V.A.T.S. and your ability to sneak.",
        "Luck is a measure of your general good fortune. It affects the recharge rate of critical hits."
    };

    SDL_Surface *desc_surface = TTF_RenderText_Blended_Wrapped(font, descriptions[state->selector_position], color, 300); // 300 = max width
    SDL_Texture *desc_texture = SDL_CreateTextureFromSurface(renderer, desc_surface);

    SDL_Rect desc_rect = {410, 300, desc_surface->w, desc_surface->h}; // Description stays fixed
    SDL_RenderCopy(renderer, desc_texture, NULL, &desc_rect);

    SDL_FreeSurface(desc_surface);
    SDL_DestroyTexture(desc_texture);
}

static void render_perks_content(SDL_Renderer *renderer, const AppResources *resources, PipState *state) {
    TTF_Font *font = resources->body_font;
    SDL_Color color = {0, 255, 0, 255};

    // Render title
    const char *perks_title = "PERKS: Placeholder Content";
    SDL_Surface *title_surface = TTF_RenderText_Solid(font, perks_title, color);
    SDL_Texture *title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
    SDL_Rect title_rect = {50, 100, title_surface->w, title_surface->h};
    SDL_RenderCopy(renderer, title_texture, NULL, &title_rect);
    SDL_FreeSurface(title_surface);
    SDL_DestroyTexture(title_texture);

    // Render perks list (if any perks exist, currently empty as placeholder)
    for (int i = 0; i < 10; i++) {
        if (strlen(state->perks[i]) > 0) {
            SDL_Surface *perk_surface = TTF_RenderText_Solid(font, state->perks[i], color);
            SDL_Texture *perk_texture = SDL_CreateTextureFromSurface(renderer, perk_surface);
            SDL_Rect perk_rect = {50, 120 + i * 40, perk_surface->w, perk_surface->h};
            SDL_RenderCopy(renderer, perk_texture, NULL, &perk_rect);
            SDL_FreeSurface(perk_surface);
            SDL_DestroyTexture(perk_texture);
        }
    }
}

static void render_map_tab(SDL_Renderer *renderer) {
    map_render(renderer); // Calls your new map engine's render function
}


static void render_radio_tab(
    SDL_Renderer *renderer,
    AppResources *resources,
    double elapsed_seconds
) {
    TTF_Font *font = resources->body_font;
    SDL_Color color = {0, 255, 0, 255};
    SDL_Rect video_rect = {290, 105, 220, 220};
    video_player_update(resources->radio_video, elapsed_seconds);
    video_player_render(resources->radio_video, renderer, &video_rect);

    const char *station_name = "UNKNOWN SIGNAL";
    SDL_Surface *surface = TTF_RenderText_Solid(font, station_name, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {(SCREEN_WIDTH - surface->w) / 2, 345, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

static void render_current_tab(
    SDL_Renderer *renderer,
    AppResources *resources,
    PipState *state,
    double elapsed_seconds
) {
    switch (state->current_tab) {
        case TAB_STAT:
            render_stat_tab(renderer, resources, state, elapsed_seconds);
            break;
        case TAB_INV:
            render_inv(renderer, resources, state);
            break;
        case TAB_DATA:
            render_data_tab(renderer, resources, state);
            break;
        case TAB_MAP:
            render_map_tab(renderer);
            break;
        case TAB_RADIO:
            render_radio_tab(renderer, resources, elapsed_seconds);
            break;
        case NUM_TABS:
            break;
    }
}

int main(int argc, char *argv[]) {
    int exit_code = EXIT_FAILURE;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    Mix_Chunk *boot_sound = NULL;
    AppResources resources = {0};
    bool sdl_initialized = false;
    bool image_initialized = false;
    bool ttf_initialized = false;
    bool mixer_initialized = false;
    bool mixer_open = false;
    bool resources_initialized = false;
    bool state_initialized = false;
    bool renderer_vsync = false;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        goto cleanup;
    }
    sdl_initialized = true;

    if (!select_runtime_asset_directory()) {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Pip-Boy 3000",
            "Could not locate the Pip-Boy runtime assets.",
            NULL
        );
        goto cleanup;
    }

    // Keep the runtime log next to the source assets, regardless of launch location.
    freopen("debug.log", "w", stdout);
    freopen("debug.log", "a", stderr);

    const int requested_image_formats = IMG_INIT_JPG | IMG_INIT_PNG;
    const int initialized_image_formats = IMG_Init(requested_image_formats);
    image_initialized = true;
    if ((initialized_image_formats & requested_image_formats) != requested_image_formats) {
        fprintf(stderr, "SDL_image initialization failed: %s\n", IMG_GetError());
        goto cleanup;
    }

    window = SDL_CreateWindow(
        "Pip-Boy 3000",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        goto cleanup;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "VSync renderer unavailable, falling back: %s\n", SDL_GetError());
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        goto cleanup;
    }

    SDL_RendererInfo renderer_info;
    if (SDL_GetRendererInfo(renderer, &renderer_info) == 0) {
        renderer_vsync = (renderer_info.flags & SDL_RENDERER_PRESENTVSYNC) != 0;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    if (TTF_Init() == -1) {
        fprintf(stderr, "SDL_ttf initialization failed: %s\n", TTF_GetError());
        goto cleanup;
    }
    ttf_initialized = true;

    // Audio is optional: the UI still runs on machines without an audio device.
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) {
        Mix_Init(MIX_INIT_MP3);
        mixer_initialized = true;
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0) {
            mixer_open = true;
        } else {
            fprintf(stderr, "Audio disabled: %s\n", Mix_GetError());
        }
    } else {
        fprintf(stderr, "Audio subsystem unavailable: %s\n", SDL_GetError());
    }

    if (!has_argument(argc, argv, "--skip-boot")) {
        printf("Starting boot animation...\n");
        if (mixer_open) {
            boot_sound = Mix_LoadWAV("Sounds/On.mp3");
            if (boot_sound) {
                Mix_PlayChannel(-1, boot_sound, 0);
            } else {
                fprintf(stderr, "Boot sound unavailable: %s\n", Mix_GetError());
            }
        }

        const VideoPlaybackResult boot_result = show_boot_animation(renderer);
        if (boot_result == VIDEO_PLAYBACK_QUIT) {
            exit_code = EXIT_SUCCESS;
            goto cleanup;
        }
        if (boot_result == VIDEO_PLAYBACK_ERROR) {
            fprintf(stderr, "Boot video playback failed; continuing without it.\n");
        }

        if (boot_sound) {
            Mix_HaltChannel(-1);
            Mix_FreeChunk(boot_sound);
            boot_sound = NULL;
        }
    }

    if (!resources_init(&resources, renderer)) {
        goto cleanup;
    }
    resources_initialized = true;

    if (!initialize_pip_state(&pip_state)) {
        fprintf(stderr, "Failed to initialize Pip-Boy state.\n");
        goto cleanup;
    }
    state_initialized = true;

    const PipSaveLoadResult save_result = load_pip_state(&pip_state, PIP_SAVE_PATH);
    if (save_result == PIP_SAVE_LOADED) {
        snprintf(pip_state.notification, sizeof(pip_state.notification), "SAVE LOADED");
        pip_state.notification_start_time = SDL_GetTicks();
    } else if (save_result == PIP_SAVE_RECOVERED_BACKUP) {
        snprintf(pip_state.notification, sizeof(pip_state.notification), "BACKUP SAVE RECOVERED");
        pip_state.notification_start_time = SDL_GetTicks();
        save_pip_state(&pip_state, PIP_SAVE_PATH);
    } else if (save_result == PIP_SAVE_ERROR) {
        pip_state.persistence_enabled = false;
        fprintf(stderr, "Save data was present but could not be loaded.\n");
        snprintf(pip_state.notification, sizeof(pip_state.notification), "SAVE LOAD FAILED - USING DEFAULTS");
        pip_state.notification_start_time = SDL_GetTicks();
        pip_state.notification_is_error = true;
    }

    map_init(renderer);

    bool running = true;
    SDL_Event event;
    const Uint64 performance_frequency = SDL_GetPerformanceFrequency();
    Uint64 last_update_counter = SDL_GetPerformanceCounter();
    const Uint32 target_frame_time = 1000 / FRAME_RATE;

    while (running) {
        const Uint32 frame_started_at = SDL_GetTicks();
        const Uint64 update_counter = SDL_GetPerformanceCounter();
        const double elapsed_seconds =
            (double)(update_counter - last_update_counter) / (double)performance_frequency;
        last_update_counter = update_counter;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else {
                capture_input(&event); // Store the input in the queue
            }
        }

        if (!running) break;

        handle_navigation(&pip_state);
        if (pip_state.current_tab == TAB_MAP) {
            map_update();
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        render_health_background(renderer, &resources, &pip_state);
        render_ap_bar(renderer, &resources, &pip_state);
        render_mid_background(renderer, &resources, &pip_state);
        render_tabs(renderer, &resources, &pip_state);
        render_current_tab(renderer, &resources, &pip_state, elapsed_seconds);
        render_date_time(renderer, &resources, &pip_state);

        if (pip_state.current_tab == TAB_STAT && pip_state.current_subtab == SUBTAB_SPECIAL) {
            render_special_animation(renderer, &resources, &pip_state, elapsed_seconds);
        }
        render_notification(renderer, &resources, &pip_state);

        SDL_RenderPresent(renderer);

        // Present already blocks when VSync is active. Delay only on fallback renderers.
        if (!renderer_vsync) {
            const Uint32 elapsed = SDL_GetTicks() - frame_started_at;
            if (elapsed < target_frame_time) {
                SDL_Delay(target_frame_time - elapsed);
            }
        }
    }

    exit_code = EXIT_SUCCESS;

cleanup:
    if (boot_sound) {
        Mix_HaltChannel(-1);
        Mix_FreeChunk(boot_sound);
    }
    map_shutdown();
    if (state_initialized && pip_state.persistence_enabled) {
        save_pip_state(&pip_state, PIP_SAVE_PATH);
    }
    if (state_initialized) cleanup_pip_state(&pip_state);
    if (resources_initialized) resources_destroy(&resources);
    if (mixer_open) Mix_CloseAudio();
    if (mixer_initialized) Mix_Quit();
    if (ttf_initialized) TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (image_initialized) IMG_Quit();
    if (sdl_initialized) SDL_Quit();

    return exit_code;
}
