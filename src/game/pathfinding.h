#ifndef PATHFINDING_H
#define PATHFINDING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h"
#include "utils.h"

// Bridge and river constants
#define BRIDGE_1_Y 65
#define BRIDGE_2_Y 195
#define BRIDGE_WIDTH 30
#define BRIDGE_HEIGHT 25

typedef struct {
    position_t center;
    int width;
    int height;
} bridge_t;

// Bridge detection
bool is_on_bridge(position_t pos);
bool needs_to_cross_river(position_t current, position_t target);
bridge_t* select_nearest_bridge(position_t current_pos, position_t target_pos);

// Target finding functions
void* find_target(troop_t *troop, player_t *my_player, player_t *opponent);
void* find_nearest_building_or_tower(position_t pos, player_t *opponent);
troop_t* find_nearest_troop(troop_t *troops, position_t pos, target_type_t target_type);

// Waypoint management
waypoint_t* create_waypoint(position_t pos);
void add_waypoint(troop_t *troop, position_t pos);
void clear_waypoints(troop_t *troop);
bool has_reached_waypoint(troop_t *troop, waypoint_t *waypoint);
void advance_waypoint(troop_t *troop);

// Movement functions
void move_troop_with_pathfinding(troop_t *troop, player_t *my_player, player_t *opponent);

#ifdef __cplusplus
}
#endif

#endif
