#ifndef STRUCTS_H
#define STRUCTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <graphx.h>
#include "utils.h"

typedef enum {AIR, GROUND, ALL} target_type_t;
// Change to attack_target_type
// typedef enum {TOWER, TROOP, BUILDING} projectile_target_type_t;
typedef enum {AIR_MOVEMENT, GROUND_MOVEMENT, STATIONARY} movement_type_t;
typedef enum {MELEE, RANGED} attack_type_t;
typedef enum {COMMON, RARE, EPIC, LEGENDARY} rarity_t;
typedef enum {TROOP, SPELL, BUILDING, TOWER} card_type_t; 
typedef enum {SILVER, GOLD, MAGICAL} chest_rarity_t;
typedef enum {LOCKED, UNLOCKING, OPEN, EMPTY, OPENING} chest_status_t;

// The name could be changed to time_t
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
} timer_t;

typedef struct {
    bool empty;
} chest_slot_t;


typedef struct {
    gfx_sprite_t *sprite;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    double radius;
} cursor_t;

typedef struct {
    gfx_sprite_t *sprite;
    position_t position;

    // Maybe 
    card_type_t target_type;
    unsigned int angle;
    unsigned int damage;
    int speed;

    void *target;
    void *next;
    void *prev;
} projectile_t;

typedef struct {
    gfx_sprite_t *sprite;
    // gfx_sprite_t *building_sprite;
    gfx_sprite_t **forward_movement;
    gfx_sprite_t **backward_movement;
    gfx_sprite_t **attack_cycle;
    gfx_sprite_t **attack_cycle_rev;

    // Projectile template (only sprite and speed needed)
    gfx_sprite_t *projectile_sprite;
    int projectile_speed;

    unsigned int movement_steps;
    unsigned int attack_steps;
    unsigned int movement_ticks;
    unsigned int movement_frame;
    unsigned int attack_frame;
    unsigned int attack_ticks;
    unsigned int attack_speed;
    unsigned int movement_update_rate;
    unsigned int damage;

    double radius;

    cursor_t cursor;
    int elixir;
    unsigned int elixir_generated;
    unsigned int level;
    unsigned int duration;
    bool selected;
    int color;
    int health;

    attack_type_t attack_type;
    target_type_t target;
    movement_type_t movement;
    rarity_t rarity;
    card_type_t type;
    
    bool TARGET_TROOPS;

    // Anchor point for sprite positioning (feet/center of mass)
    int anchor_x;
    int anchor_y;

    // enum for card type (troop, spell, building)?
    // store troop movement, etc. in here or not?
    // Maybe have the card type here determine the troop spawned in
} card_t;

typedef struct {
    unsigned int count;
    card_t *card;
} card_pack_t;

typedef struct {
    gfx_sprite_t *sprite;
    chest_rarity_t rarity;
    chest_status_t status;
    unsigned int gold;
    unsigned int duration;
    unsigned int time_elapsed;
    unsigned int total_cards;

    card_pack_t pack;
    // timer_t timer;

    unsigned int unlock_step;
    // Total cards + 1 (gold)
    unsigned int total_steps;
    // Create struct for partitioning the cards by rarity
    // Make a structure that has cards and amount
} chest_t;

typedef struct {
    // MEMORY OPTIMIZATION: Use indices instead of copying entire card structures  
    int *card_indices;     // Array of indices pointing to available_cards
    int selected_card;     // Index into card_indices array (0-3 for hand)
    card_t *available_cards; // Pointer to the master card array
} deck_t;

typedef struct {
    unsigned int x;
    unsigned int y;
} point_t;

typedef struct waypoint {
    position_t position;
    struct waypoint *next;
} waypoint_t;

typedef struct {
    bool visible;
    bool placement_allowed;
    bool movement_allowed;
    point_t *points;
    int num_points;

    // have points array with the last point always connecting to the first point in the draw routines
    // how do I want to structure the bounds?
    // four corners of the polygon

    // in the draw function, it will handle the drawing of the rectangles around these bounds
     
    // visible indicator cue bool?
    // personal towers should be included in the bounds

    // 6 is max vertices?
    // type of boundary? Placement, movement (not exclusive)  
    // Add these bounds also when buildings are placed - to both players
} bounds_t;

typedef struct{
    gfx_sprite_t *sprite;
    gfx_sprite_t **forward_movement;
    gfx_sprite_t **backward_movement;
    gfx_sprite_t **attack_cycle;
    gfx_sprite_t **attack_cycle_rev;
    // gfx_sprite_t **projectile_sprites;

    position_t position;

    // Anchor point - where the "feet" are relative to sprite top-left
    int anchor_x;
    int anchor_y;

    // Projectile template (only sprite and speed needed)
    gfx_sprite_t *projectile_sprite;
    int projectile_speed;

    // void *nearest_troop;
    // nearest building
    // or just have nearest_enemies (generic linked list capable of any pointer)
    void *nearest_tower;
    void *nearest_target;

    // Waypoint system for pathfinding
    waypoint_t *path;
    waypoint_t *current_waypoint;

    double movement_speed;
    double step_size;
    double range;

    unsigned int attack_speed;
    unsigned int damage;

    unsigned int MAX_HEALTH;
    int health;

    int angle;

    unsigned int movement_ticks;
    unsigned int movement_update_rate;
    unsigned int movement_frame;
    unsigned int movement_frame_count;
    unsigned int movement_steps;

    unsigned int attack_ticks;
    unsigned int attack_frame;
    unsigned int attack_frame_count;
    unsigned int attack_steps;

    attack_type_t attack_type;
    target_type_t target;
    movement_type_t movement;
    bool TARGET_TROOPS;

    // Head of the linked list
    bool head;
    bool opponent_sprite_variant;
    void *next;
    void *prev;
} troop_t;

typedef struct {
    position_t position;
    double radius;
    unsigned int color;
    int damage;
    unsigned int attack_speed;
    // Ticks to distribute the damage over
    unsigned int ticks;
    unsigned int duration;
    unsigned int time_elapsed;
    timer_t time_placed;
    void *next;
    void *prev;
} spell_t;

typedef struct {
    gfx_sprite_t *sprite;;
    // Can there be a nearest building?
    troop_t *nearest_troop;

    // Projectile template (only sprite and speed needed)
    gfx_sprite_t *projectile_sprite;
    int projectile_speed;

    position_t position;
    unsigned int MAX_HEALTH;
    int health;
    double range;
    unsigned int damage;
    unsigned int attack_ticks;
    unsigned int attack_speed;
    bool active;
    bool opponent;
} tower_t;

typedef struct {
    gfx_sprite_t *sprite;
    // gfx_sprite_t *projectile_sprite;
    projectile_t projectile;
    troop_t *nearest_troop;
    position_t position;
    unsigned int elixir_generated;
    // Health is a function of 
    int health;
    unsigned int MAX_HEALTH;
    int decay;
    unsigned int damage;
    unsigned int attack_ticks;
    unsigned int attack_speed;
    unsigned int duration;
    // timer_t time_placed;
    unsigned int time_elapsed;

    target_type_t target;
    void *next;
    void *prev;
} building_t;

typedef struct {
    // Deck is just a card array
    deck_t *deck;
    tower_t *towers;
    troop_t *troops;
    // Projectiles should belong to the troop and towers structs
    spell_t *spells;
    building_t *buildings;
    projectile_t *projectiles;

    unsigned int crowns;
    // put this in a position struct?
    cursor_t cursor;

    // bounds change dynamically
    bounds_t *bounds;

    unsigned int num_troops;
    unsigned int num_buildings;
    unsigned int num_spells;
    unsigned int towers_destroyed; 

    double elixir;
    bool opponent;
    bool show_cursor;
    
} player_t;

typedef struct {
    unsigned int time_remaining;

    // Alternative structures (towers, troops, spells, etc.) are stored under player struct
    player_t *player;
    player_t *opponent;

    // Init tower given the current app time, set in menu
    timer_t timer;

    bool debug;
    // need a player instance that stores deck and local instance that has elixir, deck orders, game stats
    // Player *player;

    // // player name
    // // gems, coins
    // Chest *chests[4];

    // // arena, decks, card levels, chests, experience
} game_t;

typedef struct {
    bool victory;
    unsigned int player_crowns;
    unsigned int opponent_crowns;
    int trophy_change;  // Can be negative for loss
    chest_rarity_t chest_awarded;
    bool chest_given;
} game_result_t;

#ifdef __cplusplus
}
#endif

#endif