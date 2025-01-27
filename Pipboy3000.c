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
#define SUBTAB_SPACING 80

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
    int current_xp;         // Current XP the player has
    int xp_for_next_level;  // XP required for the next level
    int stimpaks;
    int radaways;
    char perks[10][50];
    SDL_Texture *special_animations[7][10]; // 10 frames per SPECIAL animation
    Uint32 subtab_animation_start_time; // Track the start of subtab animation
    int subtab_animation_offset; // Offset for animation during transition
    bool is_animating;           // Whether an animation is in progress
    Uint32 special_stat_animation_start; // Start time for SPECIAL stat animation
    int special_stat_animation_offset;   // Vertical offset for animating stat transitions
    bool is_special_stat_animating;      // Whether a SPECIAL stat animation is active
} PipState;

PipState pip_state;
void render_stat_subtabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_status_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_special_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void render_perks_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state);
void play_sound(const char *file) {
    Mix_Chunk *sound = Mix_LoadWAV(file);
    Mix_PlayChannel(-1, sound, 0);
    while (Mix_Playing(-1)) {
        SDL_Delay(10);
    }
    Mix_FreeChunk(sound);
}

typedef struct {
    int head;
    int left_arm;
    int right_arm;
    int torso;
    int left_leg;
    int right_leg;
} DamageBars;

// Initialize damage bars with full health
DamageBars damage_bars = {100, 100, 100, 100, 100, 100};

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

void show_boot_animation(SDL_Renderer *renderer) {
    const int NUM_BOOTUP_FRAMES = 119;  // Number of bootup frames
    const int NUM_BOOTBOY_FRAMES = 14; // Number of bootboy frames

    SDL_Texture *bootup_frames[NUM_BOOTUP_FRAMES];
    SDL_Texture *bootboy_frames[NUM_BOOTBOY_FRAMES];

    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) bootup_frames[i] = NULL;
    for (int i = 0; i < NUM_BOOTBOY_FRAMES; i++) bootboy_frames[i] = NULL;

    char path[256];

    // Load bootup frames
    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) {
        snprintf(path, sizeof(path), "BOOT/BOOTUP/%d.jpg", i);
        SDL_Surface *surface = IMG_Load(path);
        if (!surface) continue;
        bootup_frames[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    // Load bootboy frames
    for (int i = 0; i < NUM_BOOTBOY_FRAMES; i++) {
        snprintf(path, sizeof(path), "BOOT/BootBoy/%d.jpg", i);
        SDL_Surface *surface = IMG_Load(path);
        if (!surface) continue;
        bootboy_frames[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    // Play bootup animation with a slower frame delay (e.g., 150ms)
    play_animation(renderer, bootup_frames, NUM_BOOTUP_FRAMES, 90);

    // Play bootboy animation with a slower frame delay (e.g., 200ms)
    play_animation(renderer, bootboy_frames, NUM_BOOTBOY_FRAMES, 140);

    // Free bootup textures
    for (int i = 0; i < NUM_BOOTUP_FRAMES; i++) {
        if (bootup_frames[i]) SDL_DestroyTexture(bootup_frames[i]);
    }

    // Free bootboy textures
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

void render_damage_bar(SDL_Renderer *renderer, int x, int y, int width, int height, int health) {
    // Draw the background (gray bar)
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Dark gray
    SDL_Rect background_rect = {x, y, width, height};
    SDL_RenderFillRect(renderer, &background_rect);

    // Draw the health portion (green to red based on health percentage)
    SDL_Color bar_color = {255 - (2.55 * health), 2.55 * health, 0, 255}; // Gradient
    SDL_SetRenderDrawColor(renderer, bar_color.r, bar_color.g, bar_color.b, bar_color.a);
    SDL_Rect health_rect = {x, y, (width * health) / 100, height};
    SDL_RenderFillRect(renderer, &health_rect);

    // Draw the border (bright green)
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &background_rect);

    // Reset render color to black for safety
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
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

void initialize_pip_state(PipState *state) {
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
    state->stimpaks = 0; // Starting value
    state->radaways = 0; // Starting value
    state->current_xp = 50;         // Start with 50 XP
    state->xp_for_next_level = 100; // XP needed for level 2

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

void add_experience(PipState *state, int xp) {
    state->current_xp += xp;
    if (state->current_xp >= state->xp_for_next_level) {
        state->current_xp -= state->xp_for_next_level; // Rollover XP
        state->level += 1;                            // Level up
        state->xp_for_next_level += 50;               // Increase XP threshold
        printf("Level up! Current level: %d\n", state->level);
    }
}

void update_damage(DamageBars *bars, int head, int left_arm, int right_arm, int torso, int left_leg, int right_leg) {
    bars->head = head;
    bars->left_arm = left_arm;
    bars->right_arm = right_arm;
    bars->torso = torso;
    bars->left_leg = left_leg;
    bars->right_leg = right_leg;
}

void load_special_animations(SDL_Renderer *renderer, PipState *state) {
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

void load_special_stats_from_csv(const char *file_path, PipState *state) {
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
    SDL_Rect background_rect = {110, 430, 135, 30}; // Adjust based on x, y, width, height
    SDL_SetTextureColorMod(background, 0, 255, 0);
    // Render the texture
    SDL_RenderCopy(renderer, background, NULL, &background_rect);

    // Clean up the texture after rendering
    SDL_DestroyTexture(background);
}

void render_ap_bar(SDL_Renderer *renderer) {
    // Load the decorative PNG texture
    SDL_Texture *bar = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");

    // Define the position and size of the ap container
    int bar_width = 145;  // Adjust based on design
    int bar_x = SCREEN_WIDTH - bar_width - 100; // Align with AP text
    int bar_y = 430; // Same y-coordinate as the AP text
    SDL_Rect bar_rect = {bar_x, bar_y, bar_width, 30};
    SDL_SetTextureColorMod(bar, 0, 255, 0);
    // Render the texture
    SDL_RenderCopy(renderer, bar, NULL, &bar_rect);

    // Clean up the texture after rendering
    SDL_DestroyTexture(bar);
}

void render_level_xp_background(SDL_Renderer *renderer, PipState *state) {
    // Load the decorative texture for the background
    SDL_Texture *background = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");

    // Define the position and size of the decorative box
    int bg_width = 300;  // Width of the decorative box
    int bg_height = 30;  // Height of the decorative box
    int bg_x = (SCREEN_WIDTH / 2) - (bg_width / 2); // Center horizontally
    int bg_y = 430;       // Position above AP box but below HP text
    SDL_Rect background_rect = {bg_x, bg_y, bg_width, bg_height};

    // Tint the decorative box green
    SDL_SetTextureColorMod(background, 0, 255, 0);

    // Render the decorative box
    SDL_RenderCopy(renderer, background, NULL, &background_rect);
    SDL_DestroyTexture(background); // Free the texture after rendering

    // Define XP bar dimensions (inside the decorative box)
    int bar_width = bg_width - 100; // Add padding inside the box
    int bar_height = 10;           // Height of the XP bar
    int bar_x = bg_x + 90;         // Padding inside the box
    int bar_y = bg_y + 10;         // Center vertically inside the box

    // Calculate XP progress
    float xp_progress = (float)(state->current_xp) / state->xp_for_next_level;
    if (xp_progress > 1.0f) xp_progress = 1.0f;

    // Draw XP bar background (dark green)
    SDL_SetRenderDrawColor(renderer, 0, 50, 0, 255);
    SDL_Rect bar_background_rect = {bar_x, bar_y, bar_width, bar_height};
    SDL_RenderFillRect(renderer, &bar_background_rect);

    // Draw XP bar fill (bright green)
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect bar_fill_rect = {bar_x, bar_y, (int)(bar_width * xp_progress), bar_height};
    SDL_RenderFillRect(renderer, &bar_fill_rect);

    // Draw XP bar border
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &bar_background_rect);

    // Render LEVEL and XP text around the bar
    TTF_Font *font = TTF_OpenFont("monofonto.ttf", 20);
    SDL_Color color = {0, 255, 0, 255}; // Bright green text

    // Render LEVEL text
    char level_text[20];
    snprintf(level_text, sizeof(level_text), "LEVEL %d", state->level);
    SDL_Surface *level_surface = TTF_RenderText_Solid(font, level_text, color);
    SDL_Texture *level_texture = SDL_CreateTextureFromSurface(renderer, level_surface);
    SDL_Rect level_rect = {bg_x + 5, bg_y + 3, level_surface->w, level_surface->h}; // Positioned clearly above the XP bar
    SDL_RenderCopy(renderer, level_texture, NULL, &level_rect);
    SDL_FreeSurface(level_surface);
    SDL_DestroyTexture(level_texture);

    TTF_CloseFont(font);

    // Reset render color to default
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}



void render_tabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
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
            tab_x - 90,                // Adjust to extend slightly to the left
            tab_y - -32,               // Adjust to appear slightly below the tabs
            total_tab_width + 175,     // Adjust to match the width of tabs
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

void render_stat_tab(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
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
    // Create a larger font for HP text
    TTF_Font *hp_font = TTF_OpenFont("monofonto.ttf", 20); // Adjust size as needed
    char hp_text[20];
    snprintf(hp_text, sizeof(hp_text), "HP %d/%d", state->health, state->max_health);
    SDL_Surface *hp_surface = TTF_RenderText_Solid(hp_font, hp_text, color);
    SDL_Texture *hp_texture = SDL_CreateTextureFromSurface(renderer, hp_surface);
    SDL_Rect hp_rect = {115, 432, hp_surface->w, hp_surface->h}; // Left aligned
    SDL_RenderCopy(renderer, hp_texture, NULL, &hp_rect);
    SDL_FreeSurface(hp_surface);
    SDL_DestroyTexture(hp_texture);
    TTF_CloseFont(hp_font); // Clean up the HP font

    TTF_Font *ap_font = TTF_OpenFont("monofonto.ttf", 20); // Adjust size as needed
    char ap_text[20];
    snprintf(ap_text, sizeof(ap_text), "AP %d/%d", state->ap, state->max_ap);
    SDL_Surface *ap_surface = TTF_RenderText_Solid(ap_font, ap_text, color);
    SDL_Texture *ap_texture = SDL_CreateTextureFromSurface(renderer, ap_surface);
    SDL_Rect ap_rect = {SCREEN_WIDTH - ap_surface->w - 110, 432, ap_surface->w, ap_surface->h}; // Right aligned
    SDL_RenderCopy(renderer, ap_texture, NULL, &ap_rect);
    SDL_FreeSurface(ap_surface);
    SDL_DestroyTexture(ap_texture);
    TTF_CloseFont(ap_font); // Clean up the AP font
}

void render_status_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    SDL_Color color = {0, 255, 0, 255};

    // Render Vault Boy animation
    render_vaultboy(renderer);

     // Render Stimpak Background
    SDL_Texture *stimpak_background = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");
    SDL_Rect stimpak_rect = {110, 395, 100, 30}; // Adjust based on position and size
    SDL_SetTextureColorMod(stimpak_background, 100, 255, 100); // Green tint
    SDL_RenderCopy(renderer, stimpak_background, NULL, &stimpak_rect);
    SDL_DestroyTexture(stimpak_background);

    // Render RadAway Background
    SDL_Texture *radaway_background = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");
    SDL_Rect radaway_rect = {225, 395, 100, 30}; // Adjust position to the right of Stimpak
    SDL_SetTextureColorMod(radaway_background, 100, 255, 100); // Green tint
    SDL_RenderCopy(renderer, radaway_background, NULL, &radaway_rect);
    SDL_DestroyTexture(radaway_background);

    // Render Stimpak Text
    char stimpak_text[20];
    snprintf(stimpak_text, sizeof(stimpak_text), "Stimpak (%d)", state->stimpaks);
    SDL_Surface *stimpak_surface = TTF_RenderText_Solid(font, stimpak_text, color);
    SDL_Texture *stimpak_texture = SDL_CreateTextureFromSurface(renderer, stimpak_surface);
    SDL_Rect stimpak_text_rect = {115, 400, stimpak_surface->w, stimpak_surface->h}; // Center inside background
    SDL_RenderCopy(renderer, stimpak_texture, NULL, &stimpak_text_rect);
    SDL_FreeSurface(stimpak_surface);
    SDL_DestroyTexture(stimpak_texture);

    // Render RadAway Text
    char radaway_text[20];
    snprintf(radaway_text, sizeof(radaway_text), "RadAway (%d)", state->radaways);
    SDL_Surface *radaway_surface = TTF_RenderText_Solid(font, radaway_text, color);
    SDL_Texture *radaway_texture = SDL_CreateTextureFromSurface(renderer, radaway_surface);
    SDL_Rect radaway_text_rect = {230, 400, radaway_surface->w, radaway_surface->h}; // Center inside background
    SDL_RenderCopy(renderer, radaway_texture, NULL, &radaway_text_rect);
    SDL_FreeSurface(radaway_surface);
    SDL_DestroyTexture(radaway_texture);
}

void render_special_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
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
    float vertical_offset = state->special_stat_animation_offset * (-1 + ease_out_cubic(animation_progress));

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
    int highlight_y = y_start + state->selector_position * y_spacing + vertical_offset; // Calculate the highlight's Y position
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


void render_perks_content(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
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

void render_stat_subtabs(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    const char *subtab_names[] = {"STATUS", "SPECIAL", "PERKS"};
    SDL_Color color_active = {0, 255, 0, 255};   // Bright green for active subtab
    SDL_Color color_inactive = {0, 100, 0, 255}; // Dim for inactive subtabs

    TTF_Font *subtab_font = TTF_OpenFont("monofonto.ttf", 22); // Font size for subtabs

    int base_x = 205; // Base x-coordinate for the centered subtab
    int base_y = 65;  // Y-coordinate for subtabs

    // Calculate animation progress
    Uint32 current_time = SDL_GetTicks();
    float progress = 1.0f; // Default to fully completed animation
    if (state->is_animating) {
        progress = (float)(current_time - state->subtab_animation_start_time) / 300; // 300ms animation duration
        if (progress >= 1.0f) {
            progress = 1.0f;
            state->is_animating = false; // End animation
        }
    }

    float offset = (-1.0f + progress) * state->subtab_animation_offset; // Control direction of animation by linear interpolation

    // Render each subtab
    for (int i = 0; i < NUM_SUBTABS; i++) {
        int x_position = base_x + (i - state->current_subtab) * SUBTAB_SPACING + offset;

        SDL_Color current_color = (i == state->current_subtab) ? color_active : color_inactive;

        SDL_Surface *surface = TTF_RenderText_Solid(subtab_font, subtab_names[i], current_color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect rect = {x_position - surface->w / 2, base_y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

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

void render_current_tab(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
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

void handle_navigation(SDL_Event *event, PipState *state) {
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
                    state->subtab_animation_offset = SUBTAB_SPACING; // Move all subtabs right
                    state->is_animating = true;
                    state->subtab_animation_start_time = SDL_GetTicks();
                    state->current_subtab = (state->current_subtab - 1 + NUM_SUBTABS) % NUM_SUBTABS;
                }
                break;

            case SDLK_d: // Navigate right
                if (state->current_tab == TAB_STAT && !state->is_animating) {
                    state->subtab_animation_offset = -SUBTAB_SPACING; // Move all subtabs left
                    state->is_animating = true;
                    state->subtab_animation_start_time = SDL_GetTicks();
                    state->current_subtab = (state->current_subtab + 1) % NUM_SUBTABS;
                }
                break;

            // SPECIAL Attributes Navigation (W for up, S for down)
            case SDLK_w:
                if (state->current_tab == TAB_STAT && state->current_subtab == SUBTAB_SPECIAL && !state->is_special_stat_animating) {
                    state->special_stat_animation_offset = -30; // Move upwards
                    state->is_special_stat_animating = true;
                    state->special_stat_animation_start = SDL_GetTicks();
                    state->selector_position = (state->selector_position - 1 + 7) % 7; // Wrap around SPECIAL stats
                }
                break;

            case SDLK_s:
                if (state->current_tab == TAB_STAT && state->current_subtab == SUBTAB_SPECIAL && !state->is_special_stat_animating) {
                    state->special_stat_animation_offset = 30; // Move downwards
                    state->is_special_stat_animating = true;
                    state->special_stat_animation_start = SDL_GetTicks();
                    state->selector_position = (state->selector_position + 1) % 7; // Wrap around SPECIAL stats
                }
                break;
                // Simulate gaining 10 XP when pressing 'x' (for testing)
            case SDLK_x:
                add_experience(state, 10);
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

    // Load resources and initialize pip state
    printf("Starting boot animation...\n");
    play_sound("Sounds/On.mp3");
    show_boot_animation(renderer);
    initialize_pip_state(&pip_state);
    load_vaultboy_frames(renderer);
    load_special_animations(renderer, &pip_state);
    load_selectline(renderer);
    load_categoryline(renderer);

    // Main pip loop
    bool running = true;
    SDL_Event event;
    Uint32 last_frame_time = SDL_GetTicks();

    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else {
                handle_navigation(&event, &pip_state);
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
        render_level_xp_background(renderer, &pip_state); // Render XP bar
        render_tabs(renderer, font, &pip_state); // Render the tabs with the selector line
        render_current_tab(renderer, font, &pip_state); // Render active tab content

        if (pip_state.current_tab == TAB_STAT && pip_state.current_subtab == SUBTAB_SPECIAL) {
            render_special_animation(renderer, &pip_state); // Render SPECIAL animations if applicable
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
