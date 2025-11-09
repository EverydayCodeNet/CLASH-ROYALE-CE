#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <graphx.h>
#include <tice.h>
#include <sys/rtc.h>
#include <sys/timers.h>
#include <fileioc.h>
#include <fontlibc.h>
#include <keypadc.h>
#include <limits.h>

#include "game.h"
#include "map.h"
#include "structs.h"
#include "memory_simple.h"
#include "cards.h"
#include "projectiles.h"
#include "../ui/menu.h"
#include "/gfx/gfx.h"
#include "player.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// M_PI is already defined in math.h
#define GAME_DURATION 90
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define TILE_SIZE 10
#define NUM_TOWERS 3
#define BOUNDARY_WIDTH 4
#define RIVER_X (TILE_SIZE * 18)

int num_projectiles = 0;

bool is_out_of_bounds(position_t pos) {
    return pos.x < 0 || pos.x >= SCREEN_WIDTH || pos.y < 0 || pos.y >= SCREEN_HEIGHT;
}

void draw_placement_bounds(player_t *player, card_t *selected_card, tower_t *enemy_towers) {
    if (selected_card == NULL) return;

    // Only show bounds for troops and buildings, not spells
    if (selected_card->type == SPELL) return;

    gfx_SetColor(1); // Red color

    // Draw the RESTRICTED zone (where player CANNOT place)
    // For player: restricted zone is x from 170 to 320
    // This is OPPOSITE of player->bounds which shows where they CAN place

    int restricted_min_x = 170;
    int restricted_max_x = SCREEN_HEIGHT; // 320

    // Check for destroyed towers to extend the restricted zone
    bool top_tower_down = (enemy_towers[0].health <= 0);
    bool bottom_tower_down = (enemy_towers[1].health <= 0);

    if (top_tower_down && bottom_tower_down) {
        // Both towers down - restricted zone shrinks to 220-320
        restricted_min_x = 220;
    } else if (top_tower_down || bottom_tower_down) {
        // One tower down - restricted zone shrinks to 210-320 (split)
        restricted_min_x = 210;
    }

    // Draw left boundary line (the river edge)
    gfx_FillRectangle(restricted_min_x, 0, BOUNDARY_WIDTH, SCREEN_WIDTH);

    // Draw right boundary line (back edge)
    gfx_FillRectangle(restricted_max_x - BOUNDARY_WIDTH, 0, BOUNDARY_WIDTH, SCREEN_WIDTH);

    // Draw top boundary line
    gfx_FillRectangle(restricted_min_x, 0, restricted_max_x - restricted_min_x, BOUNDARY_WIDTH);

    // Draw bottom boundary line
    gfx_FillRectangle(restricted_min_x, SCREEN_WIDTH - BOUNDARY_WIDTH, restricted_max_x - restricted_min_x, BOUNDARY_WIDTH);

    // Draw middle split line when only one tower is down
    if (top_tower_down ^ bottom_tower_down) {
        int line_y = SCREEN_WIDTH / 2 - BOUNDARY_WIDTH / 2; // Middle of the screen (y=120)
        gfx_FillRectangle(restricted_min_x, line_y, restricted_max_x - restricted_min_x, BOUNDARY_WIDTH);
    }
}

// Constrain cursor to valid placement area
void constrain_cursor_to_bounds(player_t *player, card_t *selected_card) {
    if (selected_card == NULL) return;

    cursor_t *cursor = &player->cursor;

    if (selected_card->type == SPELL) {
        // Spells can go anywhere - just constrain to screen
        cursor->x = MAX(0, MIN(cursor->x, SCREEN_WIDTH - 1));
        cursor->y = MAX(0, MIN(cursor->y, SCREEN_HEIGHT - 1));
        return;
    }

    bounds_t *bounds = player->bounds;

    // Constrain to current placement bounds
    cursor->x = MAX(bounds->points[0].x, MIN(cursor->x, bounds->points[1].x));
    cursor->y = MAX(bounds->points[0].y, MIN(cursor->y, bounds->points[3].y));
}


bool is_position_valid(player_t *player, cursor_t cursor, card_t *card) {
    if (card->type == SPELL) {
        // Spells can be placed anywhere on the map
        return (cursor.x >= 0 && cursor.x < SCREEN_WIDTH &&
                cursor.y >= 0 && cursor.y < SCREEN_HEIGHT);
    }

    bounds_t *bounds = player->bounds;
    int x = cursor.x;
    int y = cursor.y;

    // Point-in-polygon test for rectangular bounds
    return (x >= bounds->points[0].x && x <= bounds->points[1].x &&
            y >= bounds->points[0].y && y <= bounds->points[3].y);
}


void update_placement_bounds(game_t *game) {
    player_t *player = game->player;
    player_t *opponent = game->opponent;
    tower_t *enemy_towers = opponent->towers;

    // Check which enemy towers are destroyed
    bool top_tower_down = (enemy_towers[0].health <= 0);
    bool bottom_tower_down = (enemy_towers[1].health <= 0);
    bool king_tower_down = (enemy_towers[2].health <= 0);

    // Update player bounds based on destroyed enemy towers
    // Player can extend rightward into enemy territory (from initial x=170)
    if (top_tower_down || bottom_tower_down) {
        int x_offset = TILE_SIZE * 4;  // 40 pixels

        if (top_tower_down) {
            // Can place deeper in top half - extend top-right corner
            player->bounds->points[1].x = 170 + x_offset; // Top-right extends (210)
        }

        if (bottom_tower_down) {
            // Can place deeper in bottom half - extend bottom-right corner
            player->bounds->points[2].x = 170 + x_offset; // Bottom-right extends (210)
        }

        if (top_tower_down && bottom_tower_down) {
            // Both princess towers down - can place up to x=220
            player->bounds->points[1].x = 220;
            player->bounds->points[2].x = 220;
        }
    }

    // Mirror logic for opponent bounds (extends leftward into player territory from initial x=200)
    tower_t *player_towers = player->towers;
    bool player_top_down = (player_towers[0].health <= 0);
    bool player_bottom_down = (player_towers[1].health <= 0);

    if (player_top_down || player_bottom_down) {
        int x_offset = TILE_SIZE * 4;  // 40 pixels

        if (player_top_down) {
            opponent->bounds->points[0].x = 200 - x_offset; // Top-left extends (160)
        }

        if (player_bottom_down) {
            opponent->bounds->points[3].x = 200 - x_offset; // Bottom-left extends (160)
        }

        if (player_top_down && player_bottom_down) {
            opponent->bounds->points[0].x = 150;
            opponent->bounds->points[3].x = 150;
        }
    }
}


void init_bounds(game_t *game) {
    // Player bounds (left side of map)
    game->player->bounds = CR_MALLOC(sizeof(bounds_t));
    game->player->bounds->visible = false;
    game->player->bounds->placement_allowed = true;
    game->player->bounds->movement_allowed = true;
    game->player->bounds->num_points = 4;
    game->player->bounds->points = CR_MALLOC(sizeof(point_t) * 4);
    
    // Initial player bounds: x from 0 to 170, y from 0 to 240 (WHERE PLAYER CAN PLACE - left side)
    game->player->bounds->points[0] = (point_t){0, 0};                      // Top-left
    game->player->bounds->points[1] = (point_t){170, 0};                    // Top-right
    game->player->bounds->points[2] = (point_t){170, SCREEN_WIDTH};         // Bottom-right
    game->player->bounds->points[3] = (point_t){0, SCREEN_WIDTH};           // Bottom-left

    // Opponent bounds (WHERE OPPONENT CAN PLACE - right side)
    game->opponent->bounds = CR_MALLOC(sizeof(bounds_t));
    game->opponent->bounds->visible = false;
    game->opponent->bounds->placement_allowed = true;
    game->opponent->bounds->movement_allowed = false;  // Bounds are for placement only, not movement
    game->opponent->bounds->num_points = 4;
    game->opponent->bounds->points = CR_MALLOC(sizeof(point_t) * 4);

    // Initial opponent bounds: x from 200 to 320, y from 0 to 240 (WHERE OPPONENT CAN PLACE - right side)
    game->opponent->bounds->points[0] = (point_t){200, 0};                  // Top-left
    game->opponent->bounds->points[1] = (point_t){SCREEN_HEIGHT, 0};        // Top-right
    game->opponent->bounds->points[2] = (point_t){SCREEN_HEIGHT, SCREEN_WIDTH}; // Bottom-right
    game->opponent->bounds->points[3] = (point_t){200, SCREEN_WIDTH};       // Bottom-left
}


// Move time routines to utils
void init_timer(game_t *game) {
    boot_GetTime(&game->timer.seconds, &game->timer.minutes, &game->timer.hours);
    game->time_remaining = GAME_DURATION;
}

void set_timer(timer_t *timer) {
    boot_GetTime(&timer->seconds, &timer->minutes, &timer->hours);
}

unsigned int get_elapsed_time(timer_t *timer) {
    uint8_t current_seconds, current_minutes, current_hours;
    boot_GetTime(&current_seconds, &current_minutes, &current_hours);

    unsigned int elapsed_time = 
        (current_hours - timer->hours) * 3600 +
        (current_minutes - timer->minutes) * 60 +
        (current_seconds - timer->seconds);

    return elapsed_time;
}

// Move everything to a generic timer library
void update_timer(game_t *game) {
    uint8_t current_seconds, current_minutes, current_hours;
    boot_GetTime(&current_seconds, &current_minutes, &current_hours);
    
    uint24_t elapsed_seconds = (current_hours - game->timer.hours) * 3600 +
                               (current_minutes - game->timer.minutes) * 60 +
                               (current_seconds - game->timer.seconds);
    
    game->time_remaining = (GAME_DURATION > elapsed_seconds) ? (GAME_DURATION - elapsed_seconds) : 0;
}

uint24_t get_remaining_time(game_t *game) {
    uint8_t current_seconds, current_minutes, current_hours;
    boot_GetTime(&current_seconds, &current_minutes, &current_hours);
    
    uint24_t elapsed_seconds = (current_hours - game->timer.hours) * 3600 +
                               (current_minutes - game->timer.minutes) * 60 +
                               (current_seconds - game->timer.seconds);
    
    return (GAME_DURATION > elapsed_seconds) ? (GAME_DURATION - elapsed_seconds) : 0;
}

game_t *init_game(data_t *data) {
    bool opponent = false;
    game_t *game = CR_MALLOC(sizeof(game_t));

    // AI can choose from its own deck that levels up as you level up
    card_t *available_cards = data->available_cards;
    card_t *deck = data->deck;
    game->player = init_player(opponent, deck);
    opponent = true;
    game->opponent = init_player(opponent, deck);
    init_towers(game);
    init_bounds(game);
    init_timer(game);

    return game;
}

void display_stats(void) {

}

bool check_collision(projectile_t *projectile, void *target, card_type_t target_type) {
    position_t target_pos;
    int target_width, target_height;

    switch (target_type) {
        case TROOP:
            target_pos = ((troop_t*)target)->position;
            target_width = ((troop_t*)target)->sprite->width;
            target_height = ((troop_t*)target)->sprite->height;
            break;
        case TOWER:
            target_pos = ((tower_t*)target)->position;
            target_width = ((tower_t*)target)->sprite->width;
            target_height = ((tower_t*)target)->sprite->height;
            break;
        case BUILDING:
            target_pos = ((building_t*)target)->position;
            target_width = ((building_t*)target)->sprite->width;
            target_height = ((building_t*)target)->sprite->height;
            break;
        default:
            return false;
    }

    // Check if the projectile's position is within the target's bounding box
    return (projectile->position.x >= target_pos.x &&
            projectile->position.x < target_pos.x + target_width &&
            projectile->position.y >= target_pos.y &&
            projectile->position.y < target_pos.y + target_height);
}

void apply_damage(void *target, card_type_t target_type, int damage) {
     switch (target_type) {
        case TROOP:
        {
            troop_t *troop = (troop_t *) target;
            troop->health -= damage;
        }
            
            break;
        case TOWER:
        {
            tower_t *tower = (tower_t *) target;
            tower->health -= damage; 
        }
            
            break;
        case BUILDING:
        {
            building_t *building = (building_t *) target;
            building->health -= damage;
        }
             
            break;
        default:
            return;
    }
}

void draw_timer(game_t *game) {
    uint8_t seconds, minutes, hours;
    uint8_t elapsed, total_time;
    uint8_t remaining = get_remaining_time(game);
    uint8_t hand_position;

    double timer_adjusted = (double) remaining / GAME_DURATION;

    gfx_SetColor(136);
    gfx_FillRectangle(130,10,100,20);

    gfx_SetColor(158);
    if (remaining < (GAME_DURATION / 2)) gfx_SetColor(150);
    
    gfx_FillRectangle(130,11,100 * timer_adjusted,18);

    gfx_SetColor(0);
    gfx_Rectangle(130,10,100,20);

    gfx_TransparentSprite(clock,224,8);
    int8_t x_offset, y_offset, hand_height, hand_width;
    hand_position = remaining % 4;
    if (hand_position % 2 == 0) {
        hand_width = 2;
        hand_height = 5;
        x_offset = 0;
        y_offset = -3;
        if (hand_position == 0) y_offset = 0;
    } else {
        hand_width = 5;
        hand_height = 2;
        x_offset = -4;
        y_offset = 0;
        if (hand_position == 1) x_offset = 0;
    }

    // Remove magic numbers
    gfx_FillRectangle(235 + x_offset,19 + y_offset,hand_width, hand_height);

    // Alternatively, use text for the time
}

void draw_cursor(player_t *player) {
    int selected = player->deck->selected_card;
    
    if (selected != -1) {
        card_t *card = get_card_by_index(player, selected);
        cursor_t cursor = card->cursor;

        // NEW: Draw placement bounds
        // Assuming we need access to opponent towers - pass from game context
        // This will be called from run_game with game->opponent->towers
        
        gfx_PrintStringXY("Cursor X: ", 10, 10);
        gfx_PrintInt(player->cursor.x, 1);
        gfx_PrintStringXY("Cursor Y: ", 10, 20);
        gfx_PrintInt(player->cursor.y, 1);
        
        if (cursor.sprite != NULL) {
            gfx_TransparentSprite(card->cursor.sprite, player->cursor.x, player->cursor.y);
        } else {    
            gfx_SetColor(card->color);
            gfx_Circle(player->cursor.x, player->cursor.y, card->radius * TILE_SIZE);
        }            
    }
}

void draw_cursor_with_bounds(player_t *player, tower_t *enemy_towers) {
    int selected = player->deck->selected_card;
    
    if (selected != -1) {
        card_t *card = get_card_by_index(player, selected);
        cursor_t cursor = card->cursor;

        // Draw placement bounds for selected card
        draw_placement_bounds(player, card, enemy_towers);

        gfx_PrintStringXY("Cursor X: ", 10, 10);
        gfx_PrintInt(player->cursor.x, 1);
        gfx_PrintStringXY("Cursor Y: ", 10, 20);
        gfx_PrintInt(player->cursor.y, 1);
        
        if (cursor.sprite != NULL) {
            gfx_TransparentSprite(card->cursor.sprite, player->cursor.x, player->cursor.y);
        } else {    
            gfx_SetColor(card->color);
            gfx_Circle(player->cursor.x, player->cursor.y, card->radius * TILE_SIZE);
        }            
    }
}

void draw_elixir_counter(player_t *player, card_t *selected_card) {
    const int MAX_ELIXIR = 10;
    // there is a double black line on the top. just extend the elixir rectangle
    gfx_SetColor(55);
    gfx_FillRectangle(0,60,10,180);
    
    // CHANGE ORDERING to avoid double call of SetColor(0)
    gfx_SetColor(202);
    // get rid of these magic numbers
    gfx_FillRectangle(0,60,10, (int) (player->elixir * 18));

    // draw lines between elixir
    gfx_SetColor(0);
    gfx_Rectangle(0,60,10,180);
    for (int i = 0; i < 10; i++) {
        gfx_HorizLine(0,60 + i * 18,10);
    }

    // draw white rectangle around the amount of elixir the selected card costs
    if (selected_card != NULL) {
        gfx_SetColor(255);
        gfx_Rectangle(0,60,10,18 * selected_card->elixir);
    }
    
    
}

void draw_card_carousel(player_t *player) {
    deck_t *deck = player->deck;
    // card_t *selected_card = deck->cards[selected_card];
    card_t *selected_card = NULL;

    if (deck->selected_card != -1) {
        selected_card = get_card_by_index(player, deck->selected_card);
    }
 
    const int CARD_WIDTH = 30;
    const int CARD_HEIGHT = 40;

    gfx_SetColor(56);
    gfx_FillRectangle(10,60,70,180);

    int y = 0;
    const int NUM_AVAILABLE = 4;
    for (int i = 0; i < NUM_AVAILABLE; i++) {
        card_t *card_data = get_card_by_index(player, i);
        gfx_sprite_t *card = card_data->sprite;
        
        y = 75 + i * CARD_HEIGHT;
        gfx_SetColor(255);
        // darker - when card slot is empty
        gfx_FillRectangle(20,75 + i * CARD_HEIGHT, CARD_HEIGHT, CARD_WIDTH);
        // Figure out why card isn't being initialized
        gfx_TransparentSprite(card,20,75 + i * 40);

        // change outline color based on the card selected
        gfx_SetColor(0);
        if (i == deck->selected_card) gfx_SetColor(171);
        
        // remove 10
        gfx_Rectangle(20,75 + i * CARD_HEIGHT,CARD_HEIGHT,CARD_WIDTH);   
    }

    draw_elixir_counter(player, selected_card);

    gfx_SetColor(0);
    gfx_Rectangle(0,60,70,180);
}

void draw_player_modal(player_t *player, tower_t *enemy_towers) {
    draw_card_carousel(player);
    draw_cursor_with_bounds(player, enemy_towers);
}

void update_elixir(game_t *game) {
    const double MAX_ELIXIR = 10.0;

    player_t *player = game->player;
    player_t *opponent = game->opponent;

    // Double elixir in last 60 seconds
    bool double_elixir = (game->time_remaining <= 60);
    double elixir_increment = double_elixir ? 0.1 : 0.05;

    // Update player elixir every tick
    if (player->elixir < MAX_ELIXIR) {
        player->elixir += elixir_increment;
        if (player->elixir > MAX_ELIXIR) {
            player->elixir = MAX_ELIXIR;
        }
    }

    // Update opponent elixir every tick
    if (opponent->elixir < MAX_ELIXIR) {
        opponent->elixir += elixir_increment;
        if (opponent->elixir > MAX_ELIXIR) {
            opponent->elixir = MAX_ELIXIR;
        }
    }
}

// MEMORY OPTIMIZATION: Shift indices instead of copying entire card structures  
void shift_deck_indices(int *card_indices, int selected_card) {
    // Assumes indices are correct
    int temp = card_indices[selected_card];
    card_indices[selected_card] = card_indices[4];
    for (int i = 4; i < 7; i++) {
        card_indices[i] = card_indices[i + 1];
    }
    card_indices[7] = temp;
}

// OLD: Shift deck (COMMENTED OUT - keeping for reference)
/*
void shift_deck(card_t *deck, int selected_card) {
    // Assumes indices are correct
    card_t temp = deck[selected_card];
    deck[selected_card] = deck[4];
    for (int i = 4; i < 7; i++) {
        deck[i] = deck[i + 1];
    }
    deck[7] = temp;
}
*/

void place_card(player_t *player, card_t *card, cursor_t cursor, void **list) {
    card_type_t card_type = card->type;

    position_t position;
    position.x = cursor.x;
    position.y = cursor.y;

    if (card_type == TROOP) {
        troop_t *troop = CR_MALLOC(sizeof(troop_t));
        if (troop == NULL) {
            return;
        }
        // Transfer all relevant properties from card to troop
        troop->sprite = player->opponent ? card->backward_movement[0] : card->forward_movement[0];
        troop->forward_movement = card->forward_movement;
        troop->backward_movement = card->backward_movement;
        troop->attack_cycle = card->attack_cycle;
        troop->attack_cycle_rev = card->attack_cycle_rev;
        troop->movement_steps = card->movement_steps;
        troop->health = card->health;
        troop->movement_ticks = 0;
        troop->movement_update_rate = card->movement_update_rate;
        troop->movement_frame = 0;
        troop->attack_type = card->attack_type;
        troop->attack_ticks = 0; 
        troop->attack_frame = 0;
        troop->attack_steps = card->attack_steps;
        troop->attack_speed = card->attack_speed;
        troop->position = position;
        troop->range = card->radius;
        troop->damage = card->damage;
        troop->projectile = card->projectile;

        // Transfer other relevant properties
        troop->target = card->target;
        troop->movement = card->movement;
        troop->TARGET_TROOPS = card->TARGET_TROOPS;

        // You might want to set these based on the card or game logic
        troop->angle = 0; // or set based on player's side
        troop->step_size = 3; // Increased from 1 to 3 for faster movement
        troop->attack_speed = 100; // or set based on card properties

        // Add to the front of the list (troop's next points to current head)
        troop->next = *list;
        troop->prev = NULL;
        
        if (*list != NULL) {
            ((troop_t *)*list)->prev = troop;
        }

        // Update the list head to the new troop
        *list = (void *) troop;
    } else if (card_type == SPELL) {
        spell_t *spell = CR_MALLOC(sizeof(spell_t));
        if (spell == NULL) {
            return;
        }

        spell->position = position;
        spell->radius = card->radius;
        spell->color = card->color;
        spell->duration = card->duration;
        spell->ticks = card->attack_ticks;
        spell->damage = card->damage;

        // Add to the front of the list
        spell->next = *list;
        spell->prev = NULL;
        
        if (*list != NULL) {
            ((spell_t *)*list)->prev = spell;
        }

        // Update the list head to the new spell
        *list = spell;

    } else if (card_type == BUILDING) {
        building_t *building = CR_MALLOC(sizeof(building_t));
        if (building == NULL) {
            return;
        }

        building->sprite = card->cursor.sprite;
        building->position = position;
        building->health = card->health;

        // Add to the front of the list
        building->next = *list;
        building->prev = NULL;
        
        if (*list != NULL) {
            ((building_t *)*list)->prev = building;
        }

        // Update the list head to the new building
        *list = building;
    }

    player->elixir -= card->elixir;
}

void handle_keys(player_t *player) {
    card_t *card;
    int init_selected = player->deck->selected_card;
    int selected = init_selected;
    int cursor_speed = 5;

    // Card selection (unchanged)
    if (kb_Data[6] & kb_Div) {
        player->deck->selected_card = 0;
    } else if (kb_Data[6] & kb_Mul) {
        player->deck->selected_card = 1;
    } else if (kb_Data[6] & kb_Sub) {
        player->deck->selected_card = 2;
    } else if (kb_Data[6] & kb_Add) {
        player->deck->selected_card = 3;
    }

    // Clear selection (unchanged)
    if (kb_Data[1] & kb_Del) {
        player->deck->selected_card = -1;
        player->cursor.x = SCREEN_WIDTH / 2;
        player->cursor.y = 100;
    }

    // UPDATED: Cursor movement with bounds checking
    if (kb_Data[7] & kb_Up) {
        player->cursor.y -= cursor_speed;
    } else if (kb_Data[7] & kb_Down) {
        player->cursor.y += cursor_speed;
    }

    if (kb_Data[7] & kb_Right) {
        player->cursor.x += cursor_speed;
    } else if (kb_Data[7] & kb_Left) {
        player->cursor.x -= cursor_speed;
    }

    // NEW: Constrain cursor to valid bounds
    if (selected >= 0 && selected < 4) {
        constrain_cursor_to_bounds(player, get_card_by_index(player, selected));
    }

    // UPDATED: Card placement with validation
    if (kb_Data[6] & kb_Enter && (selected > -1 && selected < 4)) {
        card = get_card_by_index(player, selected);

        if (player->elixir < card->elixir) return;
        
        // NEW: Check if placement is valid
        if (!is_position_valid(player, player->cursor, card)) return;
        
        if (card->type == TROOP) {
            place_card(player, card, player->cursor, (void **)&player->troops);
        } else if (card->type == BUILDING) {
            place_card(player, card, player->cursor, (void **)&player->buildings);
        } else if (card->type == SPELL) {
            place_card(player, card, player->cursor, (void **)&player->spells);
        }
            
        shift_deck_indices(player->deck->card_indices, selected);
        player->deck->selected_card = -1;
        
        player->cursor.x = SCREEN_WIDTH / 2;
        player->cursor.y = 100;
    }
}

void get_troop_priority(troop_t *troops, troop_t *nearest_troop, troop_t *strongest_troop) {
    int MAX_DISTANCE = INT_MAX;
    int shortest_distance = MAX_DISTANCE;

    // Traverse troops linked list
}

void handle_ai(game_t *game) {
    // Give the AI all of the game state information (except player's elixir)
    // Reach: Understand the current hand of the opponent (cycles), what's currently in hand
    // Use currently placed troops, tower health, etc. to weight judgement
    player_t *player = game->player;
    player_t *opponent = game->opponent;

    tower_t *towers = player->towers;
    tower_t *opponent_towers = opponent->towers;

    // Find nearest and or strongest troops - confine to one traversal of troops list
    // Determine strength of troop by DPS / (health / max health)

    // Find x number of troops from each flag
    troop_t *nearest_troop;
    troop_t *strongest_troop;
    get_troop_priority(player->troops, nearest_troop, strongest_troop);
    
}

void handle_players(game_t *game) {
    player_t *player = game->player;
    player_t *opponent = game->opponent;

    // For the human player, handle key presses
    handle_keys(player);
    
    // Handle computer logic
    handle_ai(game);
}

void update_projectiles(game_t *game) {
    player_t *players[] = {game->player, game->opponent};
    bool is_opponent[] = {false, true};

    for (int i = 0; i < 2; i++) {
        projectile_t *current = players[i]->projectiles;
        projectile_t *prev = NULL;

        while (current != NULL) {
            bool remove_projectile = false;

            // Update projectile with improved pathfinding
            update_projectile_target(current);

            // Check for collision with target using improved detection
            if (current->target != NULL) {
                if (projectile_hit_target(current, current->target, current->target_type)) {
                    apply_damage(current->target, current->target_type, current->damage);
                    remove_projectile = true;
                }
            }

            // Check if out of bounds
            if (is_out_of_bounds(current->position)) {
                remove_projectile = true;
            }

            if (remove_projectile) {
                // Remove projectile from linked list
                projectile_t *to_remove = current;
                current = current->next;

                if (prev == NULL) {
                    players[i]->projectiles = current;
                } else {
                    prev->next = current;
                }

                if (current != NULL) {
                    current->prev = prev;
                }

                CR_FREE(to_remove);
            } else {
                prev = current;
                current = current->next;
            }
        }
    }
}


void create_projectile(player_t *player, projectile_t *template) {
    projectile_t *new_projectile = CR_MALLOC(sizeof(projectile_t));
    if (new_projectile == NULL) {
        // Handle memory allocation failure
        return;
    }

    // Copy the template data to the new projectile
    *new_projectile = *template;

    // Add to the head of the list
    new_projectile->next = player->projectiles;
    new_projectile->prev = NULL;
    if (player->projectiles != NULL) {
        player->projectiles->prev = new_projectile;
    }
    player->projectiles = new_projectile;
}

void draw_troops(game_t *game) {
    troop_t *current_troop;

    // Draw player troops
    current_troop = game->player->troops;
    
    // array of troops should be sorted by z index, put flying troops above (AKA end of the array)
    while (current_troop != NULL) {
        // Sprite should store the current movement/attack animation from the sequence
        if (current_troop->sprite != NULL) {
            gfx_TransparentSprite(current_troop->sprite, current_troop->position.x, current_troop->position.y);
        }

        // gfx_TransparentSprite(current_troop->sprite, current_troop->position.x, current_troop->position.y);
        current_troop = current_troop->next;
    }

    // Draw opponent troops
    current_troop = game->opponent->troops;

    while (current_troop != NULL) {
        gfx_TransparentSprite(current_troop->sprite, current_troop->position.x, current_troop->position.y);
        current_troop = current_troop->next;
    }
}

void draw_spells(game_t *game) {
    spell_t *spells = game->player->spells;
    spell_t *current_spell = spells;

    while (current_spell != NULL) {
        gfx_SetColor(current_spell->color);
        gfx_Circle(current_spell->position.x, current_spell->position.y, current_spell->radius * TILE_SIZE);
        current_spell = (spell_t *) current_spell->next;
    }

    spells = game->opponent->spells;
    current_spell = spells;

    while (current_spell != NULL) {
        gfx_SetColor(current_spell->color);
        gfx_Circle(current_spell->position.x, current_spell->position.y, current_spell->radius * TILE_SIZE);
        current_spell = (spell_t *) current_spell->next;
    }
}

void draw_buildings(game_t *game) {
    player_t *players[] = {game->player, game->opponent};
    bool is_opponent[] = {false, true};

    for (int i = 0; i < 2; i++) {
        building_t *current_building = players[i]->buildings;

        while (current_building != NULL) {
            gfx_TransparentSprite(current_building->sprite, 
                                  current_building->position.x, 
                                  current_building->position.y);

            // Draw health bar if building is damaged
            if (current_building->health < current_building->MAX_HEALTH) {
                int sprite_width = current_building->sprite->width;
                int sprite_height = current_building->sprite->height;
                int bar_width = 5;
                int x = current_building->position.x + sprite_width;
                int y = current_building->position.y;

                // Draw background of health bar
                gfx_SetColor(136);
                gfx_FillRectangle(x, y, bar_width, sprite_height);

                // Calculate and draw current health
                int health_height = (sprite_height * current_building->health) / current_building->MAX_HEALTH;
                gfx_SetColor(is_opponent[i] ? 150 : 151);
                gfx_FillRectangle(x, y + (sprite_height - health_height), bar_width, health_height);

                // Draw border of health bar
                gfx_SetColor(0);
                gfx_Rectangle(x, y, bar_width, sprite_height);
            }

            current_building = current_building->next;
        }
    }
}

void draw_projectiles(game_t *game) {
    player_t *players[] = {game->player, game->opponent};
    
    for (int i = 0; i < 2; i++) {
        projectile_t *current_projectile = players[i]->projectiles;

        while (current_projectile != NULL) {
            // Convert angle from degrees to the 256-position format
            uint8_t angle_256 = (uint8_t)((current_projectile->angle * 256) / 360);

            // Use 100% scaling (64 represents 100%)
            uint8_t scale = 64;

            // Get the size of the rotated sprite
            uint8_t sprite_size = current_projectile->sprite->width;

            // Calculate centered position
            int16_t centered_x = current_projectile->position.x - sprite_size / 2;
            int16_t centered_y = current_projectile->position.y - sprite_size / 2;


            // Can't do rotated scaling well yet because of the return value
            gfx_TransparentSprite(bullet, current_projectile->position.x, current_projectile->position.y);

            current_projectile = current_projectile->next;
        }
    }
}

void* find_nearest_tower(tower_t *towers, position_t position) {
    tower_t *nearest_tower = NULL;
    double min_distance = INFINITY;

    for (int i = 0; i < NUM_TOWERS; i++) {
        if (towers[i].active) {
            double dx = towers[i].position.x - position.x;
            double dy = towers[i].position.y - position.y;
            double distance = sqrt(dx*dx + dy*dy);

            if (distance < min_distance) {
                min_distance = distance;
                nearest_tower = &towers[i];
            }
        }
    }

    return (void *) nearest_tower;
}

troop_t* find_nearest_troop(troop_t *troops, position_t source_position, target_type_t target_type) {
    troop_t *nearest_troop = NULL;
    double min_distance = INT_MAX;

    troop_t *current_troop = troops;
    while (current_troop != NULL) {
        // Check if the current troop matches the target type
        bool is_valid_target = false;
        switch (target_type) {
            case AIR:
                is_valid_target = (current_troop->movement == AIR_MOVEMENT);
                break;
            case GROUND:
                is_valid_target = (current_troop->movement == GROUND_MOVEMENT || current_troop->movement == STATIONARY);
                break;
            case ALL:
                is_valid_target = true;
                break;
        }

        if (is_valid_target) {
            // Calculate distance
            double dx = current_troop->position.x - source_position.x;
            double dy = current_troop->position.y - source_position.y;
            double distance = sqrt(dx*dx + dy*dy);

            // Update nearest troop if this one is closer
            if (distance < min_distance) {
                min_distance = distance;
                nearest_troop = current_troop;
            }
        }

        current_troop = current_troop->next;
    }

    return nearest_troop;
}

bool in_range(position_t position, double range, position_t target_position) {
    double dx = target_position.x - position.x;
    double dy = target_position.y - position.y;
    double distance = sqrt(dx * dx + dy * dy);

    return distance <= range * TILE_SIZE;
}

void update_troops(game_t *game) {
    player_t *players[] = {game->player, game->opponent};
    bool is_opponent[] = {false, true};

    for (int i = 0; i < 2; i++) {
        troop_t *current_troop = players[i]->troops;
        troop_t *prev_troop = NULL;
        player_t *opponent = is_opponent[i] ? game->player : game->opponent;

        while (current_troop != NULL) {
            if (current_troop->health <= 0) {
                // Remove troop
                troop_t *to_remove = current_troop;
                current_troop = current_troop->next;
                
                if (prev_troop == NULL) {
                    players[i]->troops = current_troop;
                } else {
                    prev_troop->next = current_troop;
                }
                
                if (current_troop != NULL) {
                    current_troop->prev = prev_troop;
                }

                CR_FREE(to_remove);
                continue;
            }
            
            // Check for targets in range
            void *target = NULL;
            card_type_t target_type;
            bool in_attack_range = false;

            // Check for nearby opponent troop only if the current troop targets troops
            if (current_troop->TARGET_TROOPS) {
                troop_t *nearest_opponent_troop = find_nearest_troop(opponent->troops, current_troop->position, current_troop->target);
                if (nearest_opponent_troop != NULL && 
                    in_range(current_troop->position, current_troop->range, nearest_opponent_troop->position)) {
                    target = nearest_opponent_troop;
                    target_type = TROOP;
                    in_attack_range = true;
                }
            }

            // If no nearby troop or troop doesn't target troops, check for tower in range
            if (!in_attack_range) {
                if (current_troop->nearest_tower == NULL) {
                    current_troop->nearest_tower = find_nearest_tower(opponent->towers, current_troop->position);
                }
                tower_t *nearest_tower = (tower_t *)current_troop->nearest_tower;
                if (nearest_tower != NULL && 
                    in_range(current_troop->position, current_troop->range, nearest_tower->position)) {
                    target = nearest_tower;
                    target_type = TOWER;
                    in_attack_range = true;
                }
            }

            if (in_attack_range) {
                // Attack logic
                current_troop->attack_ticks++;
                if (current_troop->attack_ticks % current_troop->attack_speed == 0) {
                    if (current_troop->attack_type == MELEE) {
                        // Perform melee attack
                        apply_damage(target, target_type, current_troop->damage);
                    } else if (current_troop->attack_type == RANGED) {
                       projectile_t *new_projectile = CR_MALLOC(sizeof(projectile_t));
           
                        *new_projectile = (projectile_t) {
                            .sprite = current_troop->projectile.sprite,
                            .position = current_troop->position,
                            .target_type = target_type,
                            .damage = current_troop->damage,
                            .speed = current_troop->projectile.speed,
                            .target = target,
                            .next = NULL,
                            .prev = NULL
                        };
                    
                        // Calculate angle
                        position_t target_pos = (target_type == TROOP) ? 
                            ((troop_t*)target)->position : ((tower_t*)target)->position;
                        double dx = target_pos.x - current_troop->position.x;
                        double dy = target_pos.y - current_troop->position.y;
                        new_projectile->angle = (unsigned int)(atan2(dy, dx) * 180 / M_PI);

                        // Add the projectile to the player's projectile list
                        if (players[i]->projectiles == NULL) {
                            players[i]->projectiles = new_projectile;
                        } else {
                            new_projectile->next = players[i]->projectiles;
                            players[i]->projectiles->prev = new_projectile;
                            players[i]->projectiles = new_projectile;
                        }
                    }
                }
                // Update attack sprite
                current_troop->attack_frame = (current_troop->attack_ticks / current_troop->attack_speed) % current_troop->attack_steps;
                current_troop->sprite = is_opponent[i] ? 
                    current_troop->attack_cycle_rev[current_troop->attack_frame] : 
                    current_troop->attack_cycle[current_troop->attack_frame];

                // Reset movement ticks when switching to attack mode
                current_troop->movement_ticks = 0;
            } else {
                // Movement logic
                current_troop->movement_ticks++;
                if (current_troop->movement_ticks >= current_troop->movement_update_rate) {
                    current_troop->movement_ticks = 0;

                    // Ensure we have a target tower
                    if (current_troop->nearest_tower == NULL) {
                        current_troop->nearest_tower = find_nearest_tower(opponent->towers, current_troop->position);
                    }

                    tower_t *nearest_tower = (tower_t *)current_troop->nearest_tower;
                    if (nearest_tower != NULL) {
                        // Move towards the tower
                        double dx = nearest_tower->position.x - current_troop->position.x;
                        double dy = nearest_tower->position.y - current_troop->position.y;
                        current_troop->angle = atan2(dy, dx);

                        current_troop->position.x += current_troop->step_size * cos(current_troop->angle);
                        current_troop->position.y += current_troop->step_size * sin(current_troop->angle);

                        // Update movement sprite
                        current_troop->movement_frame = (current_troop->movement_frame + 1) % current_troop->movement_steps;
                        current_troop->sprite = is_opponent[i] ? 
                            current_troop->backward_movement[current_troop->movement_frame] : 
                            current_troop->forward_movement[current_troop->movement_frame];
                    }
                }

                // Reset attack ticks when switching to movement mode
                current_troop->attack_ticks = 0;
            }

            prev_troop = current_troop;
            current_troop = current_troop->next;
        }
    }
}


void update_buildings(game_t *game) {
    player_t *players[] = {game->player, game->opponent};
    bool is_opponent[] = {false, true};

    const int DECAY = 5;

    for (int i = 0; i < 2; i++) {
        building_t *current_building = players[i]->buildings;
        building_t *prev_building = NULL;

        while (current_building != NULL) {
            current_building->health -= DECAY;
            current_building->attack_ticks++;

            if (current_building->health <= 0) {
                // Remove the building from the list
                if (prev_building == NULL) {
                    players[i]->buildings = current_building->next;
                } else {
                    prev_building->next = current_building->next;
                }
                building_t *temp = current_building;
                current_building = current_building->next;
                CR_FREE(temp);
                continue;
            }

            if (current_building->attack_ticks % current_building->attack_speed == 0) {
                // Ensure that this is initialized
                players[i]->elixir += current_building->elixir_generated;

                // Leave the option for attacking
            } 

            prev_building = current_building;
            current_building = current_building->next;
        }
    }
}

void update_spells(game_t *game) {
    player_t *players[] = {game->player, game->opponent};
    bool is_opponent[] = {false, true};

    for (int i = 0; i < 2; i++) {
        spell_t *current_spell = players[i]->spells;
        spell_t *prev_spell = NULL;

        while (current_spell != NULL) {
            // Replace this with the game time
            current_spell->time_elapsed++;

            // Check if the spell duration has expired
            if (current_spell->time_elapsed >= current_spell->duration) {
                // Remove the expired spell
                if (prev_spell == NULL) {
                    players[i]->spells = current_spell->next;
                } else {
                    prev_spell->next = current_spell->next;
                }
                spell_t *temp = current_spell;
                current_spell = current_spell->next;
                CR_FREE(temp);
                continue;
            }

            // Apply spell effects (damage, slow, etc.)
            // Change this such that time_elapsed is based on the game time and not ticks
            if (current_spell->time_elapsed % current_spell->ticks == 0) {
                // get troops, towers, buildings in the radius
                // and apply damage to them
                // in the future, have a struct member for function that handles specialty events

                // Apply spell damage or effect
                // You might want to implement a function to find targets within the spell's radius
                // and apply the effect to them
            }

            prev_spell = current_spell;
            current_spell = current_spell->next;
        }
    }
}

void update_towers(game_t *game) {
    player_t *player = game->player;
    player_t *opponent = game->opponent;
    tower_t *player_towers = player->towers;
    tower_t *opponent_towers = opponent->towers;
    player_t *players[] = {player, opponent};
    bool is_opponent[] = {false, true};
    static bool bounds_need_update = true;

    if (bounds_need_update) {
        update_placement_bounds(game);
        bounds_need_update = false;
    }

    for (int i = 0; i < 3; i++) {
        if (player_towers[i].health <= 0 && player_towers[i].active) {
            player_towers[i].active = false;
            bounds_need_update = true;
        }
        if (opponent_towers[i].health <= 0 && opponent_towers[i].active) {
            opponent_towers[i].active = false;
            bounds_need_update = true;
        }
    }

    for (int i = 0; i < 2; i++) {
        tower_t *current_towers = (i == 0) ? player_towers : opponent_towers;
        troop_t *enemy_troops = is_opponent[i] ? player->troops : opponent->troops;

        for (int j = 0; j < NUM_TOWERS; j++) {
            tower_t *current_tower = &current_towers[j];

            if (current_tower->health <= 0) {
                current_tower->active = false;
                continue;
            }

            if (current_tower->active) {
                // Use improved target finding that searches within range
                troop_t *target = find_closest_target_in_range(current_tower, enemy_troops, current_tower->range);
                
                // Fallback to original system if new system doesn't find target
                if (target == NULL) {
                    target = find_nearest_troop(enemy_troops, current_tower->position, ALL);
                }
                
                current_tower->nearest_troop = target;

                if (target != NULL && in_range(current_tower->position, current_tower->range, target->position)) {
                    current_tower->attack_ticks++;

                    if (current_tower->attack_ticks % current_tower->attack_speed == 0) {
                        // Create projectile using improved system
                        projectile_t *new_projectile = create_tower_projectile(current_tower, target);
                        
                        if (new_projectile != NULL) {
                            // Add projectile to player's projectile list
                            new_projectile->next = players[i]->projectiles;
                            new_projectile->prev = NULL;
                            if (players[i]->projectiles != NULL) {
                                players[i]->projectiles->prev = new_projectile;
                            }
                            players[i]->projectiles = new_projectile;
                        }

                        // Reset attack ticks
                        current_tower->attack_ticks = 0;
                    }
                } else {
                    // No target in range, reset nearest troop
                    current_tower->nearest_troop = NULL;
                }
            }
        }
    }
}

bool check_win(game_t *game) {
    player_t *player = game->player;
    player_t *opponent = game->opponent;

    // Flatten this conditional
    if (get_remaining_time(game) > 0) {
        if (player->towers_destroyed == NUM_TOWERS || opponent->towers_destroyed == NUM_TOWERS) {
            return true;
        }
    } else {
        return true;
    }

    // return true if 3 towers are destroyeds
    return false;
}

void free_troop_list(troop_t *head) {
    troop_t *current = head;
    while (current != NULL) {
        troop_t *next = current->next;

        // DON'T free sprite arrays - they're shared references from card templates
        // Only freed once via free_card_sprites() in free_data()

        CR_FREE(current);
        current = next;
    }
}

void free_spell_list(spell_t *head) {
    spell_t *current = head;
    while (current != NULL) {
        spell_t *next = current->next;
        CR_FREE(current);
        current = next;
    }
}

void free_building_list(building_t *head) {
    building_t *current = head;
    while (current != NULL) {
        building_t *next = current->next;
        CR_FREE(current);
        current = next;
    }
}

void free_projectile_list(projectile_t *head) {
    projectile_t *current = head;
    while (current != NULL) {
        projectile_t *next = current->next;
        CR_FREE(current);
        current = next;
    }
}

void free_linked_list(void *head, void (*free_node)(void*)) {
        void *current = head;
        while (current != NULL) {
            void *next = *(void**)((char*)current + sizeof(void*));  // Assumes 'next' is the first member after any data
            free_node(current);
            current = next;
        }
    }

    // Helper functions to free specific structures
    void free_troop(void *troop) {
        troop_t *t = (troop_t*)troop;
        // Free any dynamically allocated members of troop if necessary
        CR_FREE(t);
    }

    void free_spell(void *spell) {
        spell_t *s = (spell_t*)spell;
        // Free any dynamically allocated members of spell if necessary
        CR_FREE(s);
    }

    void free_building(void *building) {
        building_t *b = (building_t*)building;
        // Free any dynamically allocated members of building if necessary
        CR_FREE(b);
    }

    void free_projectile(void *projectile) {
        projectile_t *p = (projectile_t*)projectile;
        // Free any dynamically allocated members of projectile if necessary
        CR_FREE(p);
    }

    void free_player(player_t *player) {
    if (player == NULL) return;

    // Free deck structure (but not the cards themselves - they're shared references)
    if (player->deck != NULL) {
        // CRITICAL: Free card_indices array before freeing deck
        if (player->deck->card_indices != NULL) {
            CR_FREE(player->deck->card_indices);
        }
        CR_FREE(player->deck);
    }

    // Free towers array
    if (player->towers != NULL) {
        CR_FREE(player->towers);
    }

    // Free linked lists with proper cleanup
    free_troop_list(player->troops);
    free_spell_list(player->spells);
    free_building_list(player->buildings);
    free_projectile_list(player->projectiles);

    // Free bounds if allocated
    if (player->bounds != NULL) {
        if (player->bounds->points != NULL) {
            CR_FREE(player->bounds->points);
        }
        CR_FREE(player->bounds);
    }

    CR_FREE(player);
}

void free_card_sprites(card_t *cards, int num_cards) {
    for (int i = 0; i < num_cards; i++) {
        card_t *card = &cards[i];
        
        if (card->type == TROOP) {
            if (card->forward_movement) CR_FREE(card->forward_movement);
            if (card->backward_movement) CR_FREE(card->backward_movement);
            if (card->attack_cycle) CR_FREE(card->attack_cycle);
            if (card->attack_cycle_rev) CR_FREE(card->attack_cycle_rev);
        }
    }
}

void free_game(game_t *game) {
    if (game == NULL) return;

    // Free players
    free_player(game->player);
    free_player(game->opponent);

    CR_FREE(game);
}

void free_data(data_t *data) {
    if (data == NULL) return;

    // Free sprite arrays in available cards
    if (data->available_cards != NULL) {
        free_card_sprites(data->available_cards, 8); // TOTAL_CARDS
        CR_FREE(data->available_cards);
    }

    // Free unlocked cards array
    if (data->unlocked_cards != NULL) {
        CR_FREE(data->unlocked_cards);
    }

    // Note: deck points to same cards as available_cards, so don't double-free

    // Free chests array
    if (data->chests != NULL) {
        CR_FREE(data->chests);
    }
}

void run_game(game_t *game) {
    srand(rtc_Time());

    const int SIZE_OF_DECK = 8;
    const int NUM_CARDS = 8;
    bool exit = false;
    bool debug = false;

    player_t *player = game->player;
    player_t *opponent = game->opponent;

    // Toggle draw loading screen in settings
    // drawLoadingScreen();
    // while (!(kb_Data[6] & kb_Enter));
    gfx_SetDrawBuffer();

    // SIMPLIFIED: Just draw basic background
    gfx_FillScreen(80);

    bool view_opponent = false;
    // initTimer();
    while (exit == false) {
        kb_Scan();

        handle_keys(player);

        update_timer(game);

        // COMMENTED OUT: Complex game logic - rebuild incrementally
        update_elixir(game);
        update_towers(game);
        // update_troops(game);
        // update_buildings(game);
        // update_spells(game);
        // update_projectiles(game);

        // SIMPLIFIED: Just draw background and deck
        gfx_FillScreen(80);

        // COMMENTED OUT: Complex rendering - rebuild incrementally
        // draw_tiles();
        draw_map(game);
        draw_troops(game);
        // draw_buildings(game);
        // draw_spells(game);
        // draw_projectiles(game);

        // COMMENTED OUT: Debug toggle
        // if (kb_Data[4] & kb_Stat) debug = !debug;
        // if (debug == true) {
        //     if (kb_Data[1] & kb_Mode) view_opponent = !view_opponent;
        // }

        // Draw player's deck/modal with cursor and bounds
        if (view_opponent == false) {
            draw_player_modal(player, opponent->towers);
        }
        // COMMENTED OUT: Opponent view
        // else {
        //     draw_player_modal(opponent, player->towers);
        // }

        // ADDED BACK: Timer rendering
        draw_timer(game);

        gfx_BlitBuffer();

        // Exit on Clear key or timer run-out
        if (kb_Data[6] & kb_Clear) {
            exit = true;
        }

        // Check if timer has run out - go to game over screen
        if (get_remaining_time(game) == 0) {
            exit = true;
        }

        // COMMENTED OUT: Win condition check
        // if (kb_Data[6] & kb_Clear || check_win(game) == true) exit = true;
    } while (!(exit));

    // pass in game attribute to get the amount of trophies
    // handle_end_game(game);

    // create a free all function - need to go inside each of the arrays and free the elements
    // free_all(player)
    // free_game(game);
}

void start_game(void *args) {

    screen_t *screen = (screen_t *) args;
    // disable screen
    screen->active = false;
    game_t *game = init_game(screen->data);
    run_game(game);
    // Draw end screen
    // Restore home screen when the game is done
    // Update game data
    // free_game(game)
    screen->active = true;
}