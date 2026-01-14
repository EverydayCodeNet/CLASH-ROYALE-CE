#ifndef PATHFINDING_H
#define PATHFINDING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h"
#include "utils.h"

// Bridge and river constants
// Bridge Y positions match the drawing in map.c (no additional offset)
#define BRIDGE_1_Y 65
#define BRIDGE_2_Y 195
#define BRIDGE_WIDTH 30
#define BRIDGE_HEIGHT 25

// Bridge walk positions - where troops actually walk (top of bridge sprite)
#define BRIDGE_1_WALK_Y (BRIDGE_1_Y - (BRIDGE_HEIGHT / 2))
#define BRIDGE_2_WALK_Y (BRIDGE_2_Y - (BRIDGE_HEIGHT / 2))

// River bounds (actual sprite drawing: riverbank at TILE_SIZE * 18 - 5 = 175)
#define TILE_SIZE_PF 10
#define RIVER_X_MIN 175
#define RIVER_X_MAX 185
#define RIVER_CENTER_X 180

// Bridge center X for pathfinding: center of river crossing zone
// (Visual bridge sprite is wider, but troops target river midpoint)
#define BRIDGE_CENTER_X RIVER_CENTER_X

// Bridge Y tolerance (half height for detection zone)
#define BRIDGE_Y_TOLERANCE (BRIDGE_HEIGHT / 2)

// Tower/building collision radii (half of actual sprite size)
#define PRINCESS_TOWER_RADIUS 25   // 50x50 tower / 2
#define KING_TOWER_RADIUS 30       // 60x60 tower / 2
#define BUILDING_RADIUS 15         // ~30x30 building / 2

typedef struct {
    position_t center;
    int width;
    int height;
} bridge_t;

// Obstacle checking (for pathfinding around river, towers, buildings)
bool is_in_river(position_t pos);
bool is_blocked_by_tower(position_t pos, player_t *player, player_t *opponent);
bool is_blocked_by_building(position_t pos, player_t *player, player_t *opponent);
bool is_position_blocked(position_t pos, player_t *player, player_t *opponent);

// Steering around obstacles
position_t steer_around_obstacle(position_t current, position_t desired, player_t *player, player_t *opponent);
position_t steer_around_structures(position_t current, position_t desired, player_t *player, player_t *opponent);

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
