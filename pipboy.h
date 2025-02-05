#ifndef PIPBOY_H
#define PIPBOY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

// Constants
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define FRAME_RATE 60
#define NUM_VAULTBOY_FRAMES 8 // Total frames for VaultBoy animation
#define SUBTAB_SPACING 80

// Enum for Tabs
typedef enum {
    TAB_STAT,
    TAB_INV,
    TAB_DATA,
    TAB_MAP,
    TAB_RADIO,
    NUM_TABS
} PipboyTab;

// Subtabs
typedef enum {
    SUBTAB_STATUS,
    SUBTAB_SPECIAL,
    SUBTAB_PERKS,
    NUM_SUBTABS
} StatSubTab;

// Inv Subtab
typedef enum {
    SUBTAB_WEAPONS,
    SUBTAB_APPAREL,
    SUBTAB_AID,
    SUBTAB_MISC,
    SUBTAB_JUNK,
    SUBTAB_MODS,
    SUBTAB_AMMO,
    NUM_INV_SUBTABS
} InvSubTab;

// Inventory Item Structure
typedef struct {
    char name[50];   // Item name
    int quantity;    // Item quantity
    float weight;    // Item weight
    int value;       // Item's in-game value
    int damage;      // Damage dealt (for weapons)
    int ammo;
    int fire_rate;
    int range;
    int accuracy;
    int armor;       // Armor rating (for armor)
    int condition;   // Condition percentage (0-100)
    char icon_path[100]; // Path to item icon
} invItem;

typedef struct {
    int head;
    int left_arm;
    int right_arm;
    int torso;
    int left_leg;
    int right_leg;
} DamageBars;

extern DamageBars damage_bars;

// Pip-Boy State
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
    // invItem inv[100];   // Max inv items changed to line below. Changed to pointer for dynamic array of items
    invItem *weapons;
    int weapons_count;
    int weapons_capacity;
    invItem *apparel;
    int apparel_count;
    int apparel_capacity;
    invItem *aid;
    int aid_count;
    int aid_capacity;
    int misc_count;
    int misc_capacity;
    invItem *misc;
    int junk_count;
    int junk_capacity;
    invItem *junk;
    int mods_count;
    int mods_capacity;
    invItem *mods;
    int ammo_count;
    int ammo_capacity;
    invItem *ammo;
    int inv_scroll_index; // Scroll index for navigation
    InvSubTab current_inv_subtab;       // Current inv subtab
    int inv_subtab_animation_offset;   // Animation offset for subtabs
    bool is_inv_animating;             // Animation flag
    Uint32 inv_subtab_animation_start_time; // Animation start time
} PipState;

extern PipState pip_state; // Declare the game state

// Function Prototypes
int file_exists(const char *path);
float ease_out_cubic(float t);
void initialize_pip_state(PipState *state);
void add_experience(PipState *state, int xp);
void update_damage(DamageBars *bars, int head, int left_arm, int right_arm, int torso, int left_leg, int right_leg);
int load_inv(const char *file_path, invItem **inv_list, int *inv_count, int *inv_capacity);
void render_inv(SDL_Renderer *renderer, TTF_Font *font, PipState *state);


#endif
