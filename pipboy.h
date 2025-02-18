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
    int ammo;        // Amount of ammo (for weapons)
    char ammo_type[50]; // Ammo type used (for weapons)
    char speed[20];     // Speed of melee weapon
    int fire_rate;      // Fire rate (for weapons)
    int range;          // Range (for weapons)
    int accuracy;       // Accuracy (for weapons)
    int armor;       // Armor rating (for armor)
    int condition;   // Condition percentage (0-100)
    char icon_path[100]; // Path to item icon
    char component[50];
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

// DATA Subtabs
typedef enum {
    SUBTAB_QUESTS,
    SUBTAB_WORKSHOPS,
    SUBTAB_STATS,
    NUM_DATA_SUBTABS
} DataSubTab;

typedef struct {
    char name[50];           // Quest title
    char description[256];   // Quest description
    bool is_active;          // Whether the quest is active
} Quest;

typedef struct {
    char name[50];        // Workshop name
    int population;       // Number of settlers
    int food;             // Food production
    int water;            // Water production
    int power;            // Power available
    int defense;          // Defense rating
    int beds;             // Number of beds
    int happiness;        // Happiness level (0-100)
} Workshop;

typedef enum {
    STAT_CATEGORY_GENERAL,
    STAT_CATEGORY_QUEST,
    STAT_CATEGORY_COMBAT,
    STAT_CATEGORY_CRAFTING,
    STAT_CATEGORY_CRIME,
    NUM_STAT_CATEGORIES
} StatCategory;

typedef struct {
    char name[50];
    char description[256];
    int value;
    StatCategory category;
} PlayerStat;

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
    // AmmoEntry ammo_type[50]; // Lookup table for ammo (max 50 types)
    int inv_scroll_index;                   // Scroll index for navigation
    InvSubTab current_inv_subtab;           // Current inv subtab
    int inv_subtab_animation_offset;        // Animation offset for subtabs
    bool is_inv_animating;                  // Animation flag
    Uint32 inv_subtab_animation_start_time; // Animation start time
    DataSubTab current_data_subtab;         // Current DATA subtab
    bool is_data_animating;                 // Animation flag
    int data_subtab_animation_offset;
    Uint32 data_subtab_animation_start_time;
    int current_quest;           // Selected quest index
    Quest *quests;               // Pointer to dynamically allocated quests
    int quest_count;             // Number of active quests
    int quest_capacity;          // Current capacity of the quest list
    Workshop *workshops;  // Dynamic array of workshops
    int workshop_count;   // Number of workshops
    int current_workshop; // Currently selected workshop
    int workshop_capacity; // Dynamic allocation capacity
    int current_stat_category; // Index of the selected category (0 = General, 1 = Quest, etc.)
    PlayerStat *stats;
    int stats_count;
    int stats_capacity;
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
void load_ammo_map(const char *filename, PipState *state);


#endif
