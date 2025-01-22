#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h> // Required for checking file existence
#include <unistd.h>
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define FRAME_RATE 60
#define NUM_VAULTBOY_FRAMES 8 // Total frames for VaultBoy animation
#define SUBTAB_SPACING 75

// Utility functions
SDL_Texture *vaultboy_frames[NUM_VAULTBOY_FRAMES];
int vaultboy_frame_index = 0;
Uint32 last_vaultboy_update = 0; // Time tracker for animation updates
int file_exists(const char *path);

int file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// Easing subtab animation
float ease_out_cubic(float t) {
    return 1 - pow(1 - t, 3);
}

typedef struct {
    SDL_Texture *texture;
    SDL_Rect rect;
} Sprite;

typedef enum {
    TAB_STAT,
    TAB_INV,
    TAB_DATA,
    TAB_MAP,
    TAB_RADIO,
    NUM_TABS
} PipboyTab;

typedef enum {
    SUBTAB_STATUS,
    SUBTAB_SPECIAL,
    SUBTAB_PERKS,
    NUM_SUBTABS
} StatSubTab;

typedef struct {
    PipboyTab current_tab;
    StatSubTab current_subtab; // STAT subtabs
    int selector_position;
    int special_stats[7];
    int level;
    int health;
    int max_health;
    int ap;
    int max_ap;
    int experience;
    char perks[10][50];
    SDL_Texture *special_animations[7][10]; // 10 frames per SPECIAL animation
    Uint32 animation_start_time; // Track the start of subtab animation
    int subtab_animation_offset; // Offset for animation during transition
    bool is_animating;           // Whether an animation is in progress
} GameState;

GameState game_state;
void render_stat_subtabs(SDL_Renderer *renderer, TTF_Font *font, GameState *state);
void render_status_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state);
void render_special_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state);
void render_perks_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state);
void play_sound(const char *file) {
    Mix_Chunk *sound = Mix_LoadWAV(file);
    Mix_PlayChannel(-1, sound, 0);
    while (Mix_Playing(-1)) {
        SDL_Delay(10);
    }
    Mix_FreeChunk(sound);
}

void play_animation(SDL_Renderer *renderer, SDL_Texture *frames[], int frame_count) {
    for (int i = 0; i < frame_count; i++) {
        if (frames[i]) {
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, frames[i], NULL, NULL);
            SDL_RenderPresent(renderer);
            SDL_Delay(1000 / FRAME_RATE);
        }
    }
}

void show_boot_animation(SDL_Renderer *renderer) {
    const int NUM_BOOTUP_FRAMES = 119;
    const int NUM_BOOTBOY_FRAMES = 14;

    SDL_Texture *bootup_frames[NUM_BOOTUP_FRAMES];
    SDL_Texture *bootboy_frames[NUM_BOOTBOY_FRAMES];

    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) bootup_frames[i] = NULL;
    for (int i = 0; i < NUM_BOOTBOY_FRAMES; i++) bootboy_frames[i] = NULL;

    char path[256];

    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) {
        snprintf(path, sizeof(path), "BOOT/BOOTUP/%d.jpg", i);
        SDL_Surface *surface = IMG_Load(path);
        if (!surface) continue;
        bootup_frames[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    for (int i = 0; i < NUM_BOOTBOY_FRAMES; i++) {
        snprintf(path, sizeof(path), "BOOT/BootBoy/%d.jpg", i);
        SDL_Surface *surface = IMG_Load(path);
        if (!surface) continue;
        bootboy_frames[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    play_animation(renderer, bootup_frames, NUM_BOOTUP_FRAMES);
    play_animation(renderer, bootboy_frames, NUM_BOOTBOY_FRAMES);

    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) {
        if (bootup_frames[i]) SDL_DestroyTexture(bootup_frames[i]);
    }
    for (int i = 0; i < NUM_BOOTBOY_FRAMES; i++) {
        if (bootboy_frames[i]) SDL_DestroyTexture(bootboy_frames[i]);
    }
}
SDL_Texture *selectline_texture = NULL;

void load_selectline(SDL_Renderer *renderer) {
    SDL_Surface *surface = IMG_Load("STAT/SELECTLINE.jpg");
    if (surface) {
        selectline_texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
}

SDL_Texture *categoryline_texture = NULL;

void load_categoryline(SDL_Renderer *renderer) {
    SDL_Surface *surface = IMG_Load("STAT/CATEGORYLINE.jpg");
    if (surface) {
        categoryline_texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
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
        SDL_Rect dest_rect = {325, 125, 150, 250};
        SDL_SetTextureColorMod(vaultboy_frames[vaultboy_frame_index], 0, 255, 0); // SDL texture used to turn vault boy greeen
        SDL_RenderCopy(renderer, vaultboy_frames[vaultboy_frame_index], NULL, &dest_rect);
    }
}

void initialize_game_state(GameState *state) {
    state->current_tab = TAB_STAT;
    state->current_subtab = SUBTAB_STATUS;
    state->selector_position = 0;

    // Set default SPECIAL stats
    for (int i = 0; i < 7; i++) {
        state->special_stats[i] = 5; // Default value for SPECIAL stats
    }

    state->health = 115;       // Full health
    state->max_health = 115;   // Full max health
    state->ap = 90;            // Full action points
    state->max_ap = 90;        // Full max action points
    state->level = 1;          // Starting level
    state->experience = 0;     // Starting experience

    // Initialize perks to empty
    for (int i = 0; i < 10; i++) {
        memset(state->perks[i], 0, sizeof(state->perks[i]));
    }

    // Initialize SPECIAL animations to NULL
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 10; j++) {
            state->special_animations[i][j] = NULL;
        }
    }
}

void load_special_animations(SDL_Renderer *renderer, GameState *state) {
    const char *special_names[7] = {"Strength", "Perception", "Endurance", "Charisma", "Intelligence", "Agility", "Luck"};
    char path[256];

    for (int i = 0; i < 7; i++) {
        int frame_count = 0; // Counter for available frames
        int frame_number = 0; // Start checking frames from 0 upwards

        while (frame_count < 10) { // Load up to 10 frames per animation
            snprintf(path, sizeof(path), "STAT/%s/%d.jpg", special_names[i], frame_number);
            if (file_exists(path)) {
                SDL_Surface *surface = IMG_Load(path);
                if (!surface) {
                    state->special_animations[i][frame_count++] = NULL;
                } else {
                    state->special_animations[i][frame_count++] = SDL_CreateTextureFromSurface(renderer, surface);
                    SDL_FreeSurface(surface);
                }
            } else if (frame_number > 30) { // Stop searching if the file is missing after 50 attempts
                break;
            }
            frame_number++;
        }


        // Fill remaining slots with NULL
        for (int j = frame_count; j < 10; j++) {
            state->special_animations[i][j] = NULL;
        }
    }
}

void render_special_animation(SDL_Renderer *renderer, GameState *state) {
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
        SDL_Rect dest_rect = {325, 100, 225, 225}; // Position and size of animation
        SDL_RenderCopy(renderer, current_frame, NULL, &dest_rect);
    }
}

void load_special_stats_from_csv(const char *file_path, GameState *state) {
    FILE *file = fopen(file_path, "r");


    char line[256];
    int i = 0;
    while (fgets(line, sizeof(line), file) && i < 7) {
        sscanf(line, "%*[^,],%d", &state->special_stats[i]); // Skip the name, read the stat value
        i++;
    }
    fclose(file);
}

void render_health_background(SDL_Renderer *renderer) {
    // Load the decorative PNG texture
    SDL_Texture *background = IMG_LoadTexture(renderer, "STAT/BOXHP1.jpg");

    // Define the position and size of the decorative container
    SDL_Rect background_rect = {50, 425, 195, 30}; // Adjust based on x, y, width, height
    SDL_SetTextureColorMod(background, 0, 255, 0);
    // Render the texture
    SDL_RenderCopy(renderer, background, NULL, &background_rect);

    // Clean up the texture after rendering
    SDL_DestroyTexture(background);
}

void render_ap_bar(SDL_Renderer *renderer) {
    // Load the decorative PNG texture
    SDL_Texture *bar = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");


    // Define the position and size of the decorative container
    int bar_width = 195;  // Adjust based on your design
    int bar_x = SCREEN_WIDTH - bar_width - 50; // Align with AP text
    int bar_y = 425; // Same y-coordinate as the AP text
    SDL_Rect bar_rect = {bar_x, bar_y, bar_width, 30};
    SDL_SetTextureColorMod(bar, 0, 255, 0);
    // Render the texture
    SDL_RenderCopy(renderer, bar, NULL, &bar_rect);

    // Clean up the texture after rendering
    SDL_DestroyTexture(bar);
}

void render_level_xp_background(SDL_Renderer *renderer) {
    SDL_Texture *background = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");


    // Define the position and size of the decorative background
    int bg_width = 300; // Adjust width to fit the Level/XP text
    int bg_height = 30; // Adjust height as needed
    int bg_x = SCREEN_WIDTH / 2 - bg_width / 2; // Center-align like the Level/XP text
    int bg_y = 425; // Adjust y-coordinate for placement
    SDL_Rect background_rect = {bg_x, bg_y, bg_width, bg_height};
    SDL_SetTextureColorMod(background, 0, 255, 0);
    SDL_RenderCopy(renderer, background, NULL, &background_rect); // Render the texture
    SDL_DestroyTexture(background); // Clean up the texture after rendering
}

void render_tabs(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    const char *tab_names[] = {"STAT", "INV", "DATA", "MAP", "RADIO"};
    SDL_Color color = {0, 255, 0, 255}; // Green color for tab text
    int tab_spacing = 45; // Space between tabs

    // Load a larger font specifically for tabs
    TTF_Font *tab_font = TTF_OpenFont("monofonto.ttf", 28); // Larger font size for tabs


    // Calculate starting x-coordinate for centering
    int total_tab_width = 0;
    int tab_text_widths[NUM_TABS]; // Array to store text widths

    for (int i = 0; i < NUM_TABS; i++) {
        // Calculate the width of each tab's text
        int w;
        TTF_SizeText(tab_font, tab_names[i], &w, NULL);
        tab_text_widths[i] = w;
        total_tab_width += w + tab_spacing;
    }
    total_tab_width -= tab_spacing; // Remove extra spacing after the last tab
    int tab_x = (SCREEN_WIDTH - total_tab_width) / 2; // Center-align tabs
    int tab_y = 25; // Starting y-coordinate

    // Render the CATEGORYLINE graphic (background line for all tabs)
    if (categoryline_texture) {
        SDL_Rect categoryline_rect = {
            tab_x - 150,                // Adjust to extend slightly to the left
            tab_y - -32,               // Adjust to appear slightly above the tabs
            total_tab_width + 300,     // Adjust to match the width of tabs
            12                         // Height with padding
        };
        SDL_SetTextureColorMod(categoryline_texture, 0, 255, 0); // Bright green for active tab
        SDL_RenderCopy(renderer, categoryline_texture, NULL, &categoryline_rect);
    }

    // Render the SELECTLINE graphic around the active tab
    if (selectline_texture) {
        int active_tab_x = tab_x;
        for (int i = 0; i < state->current_tab; i++) {
            active_tab_x += tab_text_widths[i] + tab_spacing;
        }

        SDL_Rect selectline_rect = {
            active_tab_x - 10, // Slight offset for alignment
            tab_y + 11,        // Position slightly above the tab text
            tab_text_widths[state->current_tab] + 18, // Adjust width to match the tab
            26                // Height to cover the tab area
        };
        SDL_SetTextureColorMod(selectline_texture, 0, 255, 0); // Bright green for active tab
        SDL_RenderCopy(renderer, selectline_texture, NULL, &selectline_rect);
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

    // Close the larger font after rendering
    TTF_CloseFont(tab_font);
}

/*void render_static_overlays(SDL_Renderer *renderer) {
    SDL_Texture *category_line = IMG_LoadTexture(renderer, "STAT/PIPBAR1.jpg");
     if (!category_line) {
        // Log an error if the texture fails to load
        // fprintf(stderr, "Failed to load PIPBAR1.jpg: %s\n", SDL_GetError());
        return;
    }
    SDL_Rect line_rect = {0, 0, SCREEN_WIDTH, 10};  // Adjust dimensions and position
    SDL_RenderCopy(renderer, category_line, NULL, &line_rect);
    SDL_DestroyTexture(category_line);
}*/

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

void render_stat_tab(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    SDL_Color color = {0, 255, 0, 255};

    // Render the sub-tabs
    render_stat_subtabs(renderer, font, state);

    // Render content based on the active sub-tab
    switch (state->current_subtab) {
        case SUBTAB_STATUS:
            render_status_content(renderer, font, state);
            break;
        case SUBTAB_SPECIAL:
            render_special_content(renderer, font, state);
            break;
        case SUBTAB_PERKS:
            render_perks_content(renderer, font, state);
            break;
    }

    // Render general stats at the bottom
    char hp_text[20];
    snprintf(hp_text, sizeof(hp_text), "HP: %d/%d", state->health, state->max_health);
    SDL_Surface *hp_surface = TTF_RenderText_Solid(font, hp_text, color);
    SDL_Texture *hp_texture = SDL_CreateTextureFromSurface(renderer, hp_surface);
    SDL_Rect hp_rect = {55, 430, hp_surface->w, hp_surface->h}; // Left aligned
    SDL_RenderCopy(renderer, hp_texture, NULL, &hp_rect);
    SDL_FreeSurface(hp_surface);
    SDL_DestroyTexture(hp_texture);

    char level_text[20];
    snprintf(level_text, sizeof(level_text), "Level %d XP: %d", state->level, state->experience);
    SDL_Surface *level_surface = TTF_RenderText_Solid(font, level_text, color);
    SDL_Texture *level_texture = SDL_CreateTextureFromSurface(renderer, level_surface);
    SDL_Rect level_rect = {SCREEN_WIDTH / 2 - level_surface->w / 2, 430, level_surface->w, level_surface->h};
    SDL_RenderCopy(renderer, level_texture, NULL, &level_rect);
    SDL_FreeSurface(level_surface);
    SDL_DestroyTexture(level_texture);

    char ap_text[20];
    snprintf(ap_text, sizeof(ap_text), "AP: %d/%d", state->ap, state->max_ap);
    SDL_Surface *ap_surface = TTF_RenderText_Solid(font, ap_text, color);
    SDL_Texture *ap_texture = SDL_CreateTextureFromSurface(renderer, ap_surface);
    SDL_Rect ap_rect = {SCREEN_WIDTH - ap_surface->w - 65, 430, ap_surface->w, ap_surface->h}; // Right aligned
    SDL_RenderCopy(renderer, ap_texture, NULL, &ap_rect);
    SDL_FreeSurface(ap_surface);
    SDL_DestroyTexture(ap_texture);
}

void render_status_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    SDL_Color color = {0, 255, 0, 255};

    // Render title
    const char *status_title = "Status";
    SDL_Surface *title_surface = TTF_RenderText_Solid(font, status_title, color);
    SDL_Texture *title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
    SDL_Rect title_rect = {50, 150, title_surface->w, title_surface->h};
    SDL_RenderCopy(renderer, title_texture, NULL, &title_rect);
    SDL_FreeSurface(title_surface);
    SDL_DestroyTexture(title_texture);

    // Render Vault Boy animation
    render_vaultboy(renderer);
}

void render_special_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    SDL_Color color = {0, 255, 0, 255};

    // Render title
    const char *special_title = "SPECIAL";
    SDL_Surface *title_surface = TTF_RenderText_Solid(font, special_title, color);
    SDL_Texture *title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
    SDL_Rect title_rect = {50, 100, title_surface->w, title_surface->h};
    SDL_RenderCopy(renderer, title_texture, NULL, &title_rect);
    SDL_FreeSurface(title_surface);
    SDL_DestroyTexture(title_texture);

    // Render SPECIAL attributes list
    const char *attributes[] = {"Strength", "Perception", "Endurance", "Charisma", "Intelligence", "Agility", "Luck"};
    char attribute_text[50];
    for (int i = 0; i < 7; i++) {
        snprintf(attribute_text, sizeof(attribute_text), "%s: %02d", attributes[i], state->special_stats[i]);
        SDL_Surface *attr_surface = TTF_RenderText_Solid(font, attribute_text, color);
        SDL_Texture *attr_texture = SDL_CreateTextureFromSurface(renderer, attr_surface);
        SDL_Rect attr_rect = {50, 120 + i * 40, attr_surface->w, attr_surface->h};
        SDL_RenderCopy(renderer, attr_texture, NULL, &attr_rect);
        SDL_FreeSurface(attr_surface);
        SDL_DestroyTexture(attr_texture);

        // Highlight the selected attribute
        if (i == state->selector_position) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_Rect highlight_rect = {45, 120 + i * 40 - 5, 150, 30}; // Adjust dimensions for the highlight
            SDL_RenderDrawRect(renderer, &highlight_rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        }
    }

    // Render attribute description
    render_attribute_description(renderer, font, state->selector_position);
}

void render_perks_content(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
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

void render_stat_subtabs(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    static int last_logged_subtab = -1; // Keep track of the last logged subtab
    const char *subtab_names[] = {"STATUS", "SPECIAL", "PERKS"};
    SDL_Color color_active = {0, 255, 0, 255};   // Bright green for active subtab
    SDL_Color color_inactive = {0, 100, 0, 255}; // Dim for inactive subtabs

    // Load a larger font specifically for subtabs
    TTF_Font *subtab_font = TTF_OpenFont("monofonto.ttf", 18); // Adjust font size as needed


    // Base position for the subtabs group
    int base_y = 65;          // Y-position of the subtabs
    int subtab_spacing = SUBTAB_SPACING; // Spacing between subtabs

    // Calculate animation progress
    Uint32 current_time = SDL_GetTicks();
    float progress = 1.0f; // Default to fully completed animation
    if (state->is_animating) {
        progress = (float)(current_time - state->animation_start_time) / 300; // 300ms duration
        if (progress >= 1.0f) {
            progress = 1.0f;
            state->is_animating = false; // End animation
        }
    }
    float offset = (1.0f - progress) * state->subtab_animation_offset;
    // Base x-coordinate for the centered subtab
    int base_x = 205; // Starting position for centered subtabs
    // Log positions only if current_subtab changes
    if (state->current_subtab != last_logged_subtab) {
        // fprintf(stdout, "Subtab positions (current_subtab = %d):\n", state->current_subtab);
        for (int j = 0; j < NUM_SUBTABS; j++) {
            int logged_x_position = base_x + (j - state->current_subtab) * subtab_spacing + offset;
            // fprintf(stdout, "  %s: x_position = %d\n", subtab_names[j], logged_x_position);
        }
        last_logged_subtab = state->current_subtab;
    }
    // Add debug statement here
    // // fprintf(stdout, "Animation progress: %.2f, Offset: %.2f\n", progress, offset);


    // Render each subtab
    for (int i = 0; i < NUM_SUBTABS; i++) {
        // Calculate the position of each subtab relative to the center
        int x_position = base_x + (i - state->current_subtab) * subtab_spacing + offset;
        /* fprintf(stdout, "Subtab '%s': base_x = %d, i = %d, current_subtab = %d, offset = %.2f, x_position = %d\n",
        subtab_names[i], base_x, i, state->current_subtab, offset, x_position); */
        // Determine text color (active or inactive)
        SDL_Color current_color = (i == state->current_subtab) ? color_active : color_inactive;

        // Render text
        SDL_Surface *surface = TTF_RenderText_Solid(subtab_font, subtab_names[i], current_color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect rect = {x_position - surface->w / 2, base_y, surface->w, surface->h}; // Center text horizontally
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Close the larger font after rendering
    TTF_CloseFont(subtab_font);
}




void render_inv_tab(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Color color = {0, 255, 0, 255};
    const char *placeholder = "Inventory Tab Placeholder";
    SDL_Surface *surface = TTF_RenderText_Solid(font, placeholder, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {75, 140, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_data_tab(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Color color = {0, 255, 0, 255};
    const char *placeholder = "Data Tab Placeholder";
    SDL_Surface *surface = TTF_RenderText_Solid(font, placeholder, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {75, 140, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_map_tab(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Color color = {0, 255, 0, 255};
    const char *placeholder = "Map Tab Placeholder";
    SDL_Surface *surface = TTF_RenderText_Solid(font, placeholder, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {75, 140, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_radio_tab(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Color color = {0, 255, 0, 255};
    const char *placeholder = "Radio Tab Placeholder";
    SDL_Surface *surface = TTF_RenderText_Solid(font, placeholder, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {75, 140, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_current_tab(SDL_Renderer *renderer, TTF_Font *font, GameState *state) {
    switch (state->current_tab) {
        case TAB_STAT:
            render_stat_tab(renderer, font, state);
            break;
        case TAB_INV:
            render_inv_tab(renderer, font);
            break;
        case TAB_DATA:
            render_data_tab(renderer, font);
            break;
        case TAB_MAP:
            render_map_tab(renderer, font);
            break;
        case TAB_RADIO:
            render_radio_tab(renderer, font);
            break;
    }
}

void handle_navigation(SDL_Event *event, GameState *state) {
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            // Main Tabs Navigation (Q for left, E for right)
            case SDLK_q:
                state->current_tab = (state->current_tab - 1 + NUM_TABS) % NUM_TABS;
                break;
            case SDLK_e:
                state->current_tab = (state->current_tab + 1) % NUM_TABS;
                break;

            // Sub-tabs Navigation within STAT (A for left, D for right)
            case SDLK_a: // Navigate left
                if (state->current_tab == TAB_STAT && !state->is_animating) {
                    state->subtab_animation_offset = SUBTAB_SPACING; // Shift all tabs to the right
                    state->is_animating = true;
                    state->animation_start_time = SDL_GetTicks();
                    state->current_subtab = (state->current_subtab - 1 + NUM_SUBTABS) % NUM_SUBTABS;
                    // fprintf(stdout, "Navigated left: new current_subtab = %d, animation_offset = %d\n", state->current_subtab, state->subtab_animation_offset);
                }
                break;

            case SDLK_d: // Navigate right
                if (state->current_tab == TAB_STAT && !state->is_animating) {
                    state->subtab_animation_offset = -SUBTAB_SPACING; // Shift all tabs to the left
                    state->is_animating = true;
                    state->animation_start_time = SDL_GetTicks();
                    state->current_subtab = (state->current_subtab + 1) % NUM_SUBTABS;
                    // fprintf(stdout, "Navigated right: new current_subtab = %d, animation_offset = %d\n", state->current_subtab, state->subtab_animation_offset);
                }
                break;
            // SPECIAL Attributes Navigation (W for up, S for down)
            case SDLK_w:
                if (state->current_tab == TAB_STAT && state->current_subtab == SUBTAB_SPECIAL) {
                    state->selector_position = (state->selector_position - 1 + 7) % 7; // 7 attributes
                }
                break;
            case SDLK_s:
                if (state->current_tab == TAB_STAT && state->current_subtab == SUBTAB_SPECIAL) {
                    state->selector_position = (state->selector_position + 1) % 7;
                }
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    SDL_Window *window = NULL; // Declare window locally
    SDL_Renderer *renderer = NULL; // Declare renderer locally

    // Redirect stdout and stderr to debug.log
    freopen("debug.log", "w", stdout);
    freopen("debug.log", "a", stderr);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        return 1;
    }

    // Create SDL window
    window = SDL_CreateWindow(
        "Pip-Boy 3000",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        SDL_Quit();
        return 1;
    }

    // Create SDL renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load font
    TTF_Font *font = TTF_OpenFont("monofonto.ttf", 16);
    if (!font) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load resources and initialize game state
    printf("Starting boot animation...\n");
    play_sound("Sounds/On.mp3");
    show_boot_animation(renderer);
    initialize_game_state(&game_state);
    load_vaultboy_frames(renderer);
    load_special_animations(renderer, &game_state);
    load_selectline(renderer);
    load_categoryline(renderer);

    // Main game loop
    bool running = true;
    SDL_Event event;
    Uint32 last_frame_time = SDL_GetTicks();

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else {
                handle_navigation(&event, &game_state);
            }
        }

        // Frame update
        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_frame_time > 100) {
            vaultboy_frame_index = (vaultboy_frame_index + 1) % NUM_VAULTBOY_FRAMES;
            last_frame_time = current_time;
        }

        // Render
        SDL_RenderClear(renderer);
        render_health_background(renderer); // Render HP bar
        render_ap_bar(renderer);           // Render AP bar
        render_level_xp_background(renderer); // Render Level/XP bar
        render_tabs(renderer, font, &game_state); // Render the tabs with the selector line
        render_current_tab(renderer, font, &game_state); // Render active tab content

        if (game_state.current_tab == TAB_STAT && game_state.current_subtab == SUBTAB_SPECIAL) {
            render_special_animation(renderer, &game_state); // Render SPECIAL animations if applicable
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / FRAME_RATE);
    }

    // Cleanup resources
    free_vaultboy_frames();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
