#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pathfinding.h"
#include "memory_simple.h"

// Static bridge data
static bridge_t bridges[2] = {
    {{180, BRIDGE_1_Y}, BRIDGE_WIDTH, BRIDGE_HEIGHT},
    {{180, BRIDGE_2_Y}, BRIDGE_WIDTH, BRIDGE_HEIGHT}
};

// ============================================================================
// BRIDGE AND RIVER DETECTION
// ============================================================================

bool is_on_bridge(position_t pos) {
    // Simplified: check if X is near river (180) and Y is at bridge heights
    if (pos.x < 165 || pos.x > 195) return false;  // Not near river

    // Check if at either bridge Y position (with tolerance)
    return ((pos.y >= BRIDGE_1_Y - 12 && pos.y <= BRIDGE_1_Y + 12) ||
            (pos.y >= BRIDGE_2_Y - 12 && pos.y <= BRIDGE_2_Y + 12));
}

bool needs_to_cross_river(position_t current, position_t target) {
    // Check if movement would cross the river (X = 180)
    return (current.x < 180 && target.x > 180) ||
           (current.x > 180 && target.x < 180);
}

bridge_t* select_nearest_bridge(position_t current_pos, position_t target_pos) {
    // Simple selection: use bridge closer to current Y position
    // Bridge 0 is at Y=65, Bridge 1 is at Y=195
    (void)target_pos;  // Unused for now

    int mid_y = (BRIDGE_1_Y + BRIDGE_2_Y) / 2;  // ~130
    return (current_pos.y < mid_y) ? &bridges[0] : &bridges[1];
}

// ============================================================================
// TARGET FINDING
// ============================================================================

void* find_nearest_building_or_tower(position_t pos, player_t *opponent) {
    void *nearest = NULL;
    double min_dist_sq = 999999.0;  // Use squared distance to avoid sqrt

    // Tower sizes for center calculation
    const int PRINCESS_TOWER_SIZE = 50;
    const int KING_TOWER_SIZE = 60;

    // Check towers (array of 3)
    for (int i = 0; i < 3; i++) {
        tower_t *current_tower = &opponent->towers[i];
        if (current_tower->active) {
            // Use tower CENTER for distance calculation
            int tower_size = (i == 2) ? KING_TOWER_SIZE : PRINCESS_TOWER_SIZE;
            double tower_center_x = current_tower->position.x + (tower_size / 2);
            double tower_center_y = current_tower->position.y + (tower_size / 2);

            double dx = tower_center_x - pos.x;
            double dy = tower_center_y - pos.y;
            double dist_sq = dx * dx + dy * dy;

            if (dist_sq < min_dist_sq) {
                min_dist_sq = dist_sq;
                nearest = current_tower;
            }
        }
    }

    // Check buildings - use center approximation
    building_t *current_building = opponent->buildings;
    while (current_building != NULL) {
        double building_center_x = current_building->position.x + 15;
        double building_center_y = current_building->position.y + 15;
        double dx = building_center_x - pos.x;
        double dy = building_center_y - pos.y;
        double dist_sq = dx * dx + dy * dy;

        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            nearest = current_building;
        }
        current_building = current_building->next;
    }

    return nearest;
}

troop_t* find_nearest_troop(troop_t *troops, position_t pos, target_type_t target_type) {
    troop_t *nearest = NULL;
    double min_dist_sq = 999999.0;  // Use squared distance to avoid sqrt

    troop_t *current = troops;
    while (current != NULL) {
        // Check if this troop matches the target type
        bool is_valid_target = false;
        switch (target_type) {
            case AIR:
                is_valid_target = (current->movement == AIR_MOVEMENT);
                break;
            case GROUND:
                is_valid_target = (current->movement == GROUND_MOVEMENT ||
                                 current->movement == STATIONARY);
                break;
            case ALL:
                is_valid_target = true;
                break;
        }

        if (is_valid_target) {
            double dx = current->position.x - pos.x;
            double dy = current->position.y - pos.y;
            double dist_sq = dx * dx + dy * dy;

            if (dist_sq < min_dist_sq) {
                min_dist_sq = dist_sq;
                nearest = current;
            }
        }

        current = current->next;
    }

    return nearest;
}

void* find_target(troop_t *troop, player_t *my_player, player_t *opponent) {
    (void)my_player;  // Unused for now

    if (!troop->TARGET_TROOPS) {
        // TARGET_TROOPS = false means building-targeting only
        // Only consider towers and buildings (ignore troops)
        return find_nearest_building_or_tower(troop->position, opponent);
    } else {
        // TARGET_TROOPS = true means can target troops
        // Search for nearest troop within reasonable range
        troop_t *nearest_troop = find_nearest_troop(opponent->troops, troop->position, troop->target);

        // If found a troop within attack range, target it
        if (nearest_troop != NULL) {
            double dx = nearest_troop->position.x - troop->position.x;
            double dy = nearest_troop->position.y - troop->position.y;
            double dist_sq = dx * dx + dy * dy;

            // Only target troops that are relatively close (within sight range)
            // Otherwise, continue toward buildings/towers
            if (dist_sq < 10000) {  // 100^2 = 10000
                return nearest_troop;
            }
        }

        // Fallback to targeting buildings/towers
        return find_nearest_building_or_tower(troop->position, opponent);
    }
}


// ============================================================================
// WAYPOINT MANAGEMENT
// ============================================================================

waypoint_t* create_waypoint(position_t pos) {
    waypoint_t *waypoint = CR_MALLOC(sizeof(waypoint_t));
    if (waypoint) {
        waypoint->position = pos;
        waypoint->next = NULL;
    }
    return waypoint;
}

void add_waypoint(troop_t *troop, position_t pos) {
    waypoint_t *new_waypoint = create_waypoint(pos);
    if (!new_waypoint) return;

    if (troop->path == NULL) {
        troop->path = new_waypoint;
        troop->current_waypoint = new_waypoint;
    } else {
        // Add to end of path
        waypoint_t *current = troop->path;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_waypoint;
    }
}

void clear_waypoints(troop_t *troop) {
    waypoint_t *current = troop->path;
    while (current != NULL) {
        waypoint_t *next = current->next;
        CR_FREE(current);
        current = next;
    }
    troop->path = NULL;
    troop->current_waypoint = NULL;
}

bool has_reached_waypoint(troop_t *troop, waypoint_t *waypoint) {
    if (!waypoint) return true;

    double dx = waypoint->position.x - troop->position.x;
    double dy = waypoint->position.y - troop->position.y;
    double dist_sq = dx * dx + dy * dy;

    // Reached if within 5 pixels
    return dist_sq < 25.0;
}

void advance_waypoint(troop_t *troop) {
    if (troop->current_waypoint != NULL) {
        troop->current_waypoint = troop->current_waypoint->next;
    }
}

// ============================================================================
// MAIN PATHFINDING MOVEMENT
// ============================================================================

void move_troop_with_pathfinding(troop_t *troop, player_t *my_player, player_t *opponent) {
    // Find target (troop or building/tower depending on TARGET_TROOPS flag)
    void *target = find_target(troop, my_player, opponent);

    if (target == NULL) return;

    // Determine target position - use the FACING EDGE of target
    // Player troops (on left) walk right -> target LEFT edge of opponent towers
    // Opponent troops (on right) walk left -> target RIGHT edge of player towers
    position_t target_pos;

    // Tower dimensions
    const int PRINCESS_TOWER_SIZE = 50;
    const int KING_TOWER_SIZE = 60;

    // Determine if this troop belongs to player or opponent
    // Player troops are on left side (x < 180), opponent on right (x > 180)
    bool troop_is_on_left = (my_player->opponent == false);

    // Try to determine what type of target this is
    // Check if it's in the opponent's tower array
    bool is_tower = false;
    for (int i = 0; i < 3; i++) {
        if (target == &opponent->towers[i]) {
            is_tower = true;
            tower_t *tower = (tower_t*)target;
            int tower_size = (i == 2) ? KING_TOWER_SIZE : PRINCESS_TOWER_SIZE;

            // Target the FACING edge of the tower
            if (troop_is_on_left) {
                // Player troop walking right -> target LEFT edge of opponent tower
                target_pos.x = tower->position.x;
                // Y: offset so sprite CENTER aligns with tower center, not feet
                // For up-facing sprites, body extends above feet position
                // offset = anchor_y - frame_height/2 (opposite of down-facing)
                int tower_center_y = tower->position.y + (tower_size / 2);
                if (troop->sprite_def != NULL) {
                    int offset = troop->sprite_def->anchor_y - (troop->sprite_def->frame_height / 2);
                    target_pos.y = tower_center_y + offset;
                } else {
                    target_pos.y = tower_center_y;
                }
            } else {
                // Opponent troop walking left -> target RIGHT edge of player tower
                target_pos.x = tower->position.x + tower_size;
                // Y: offset so sprite CENTER aligns with tower center, not feet
                // For down-facing sprites, body extends below feet position
                // offset = frame_height/2 - anchor_y (negative = target higher Y)
                int tower_center_y = tower->position.y + (tower_size / 2);
                if (troop->sprite_def != NULL) {
                    int offset = (troop->sprite_def->frame_height / 2) - troop->sprite_def->anchor_y;
                    target_pos.y = tower_center_y + offset;
                } else {
                    target_pos.y = tower_center_y;
                }
            }
            break;
        }
    }

    if (!is_tower) {
        // Check if it's a building
        building_t *check_building = opponent->buildings;
        bool is_building = false;
        while (check_building != NULL) {
            if (target == check_building) {
                is_building = true;
                // Target building facing edge (approximate 30px width)
                int building_center_y = check_building->position.y + 15;
                if (troop_is_on_left) {
                    target_pos.x = check_building->position.x;
                    // Up-facing sprite: body extends above feet
                    if (troop->sprite_def != NULL) {
                        int offset = troop->sprite_def->anchor_y - (troop->sprite_def->frame_height / 2);
                        target_pos.y = building_center_y + offset;
                    } else {
                        target_pos.y = building_center_y;
                    }
                } else {
                    target_pos.x = check_building->position.x + 30;
                    // Down-facing sprite: body extends below feet
                    if (troop->sprite_def != NULL) {
                        int offset = (troop->sprite_def->frame_height / 2) - troop->sprite_def->anchor_y;
                        target_pos.y = building_center_y + offset;
                    } else {
                        target_pos.y = building_center_y;
                    }
                }
                break;
            }
            check_building = check_building->next;
        }

        // Otherwise assume it's a troop - position is already anchor point
        if (!is_building) {
            target_pos = ((troop_t*)target)->position;
        }
    }

    // AIR TROOPS: Ignore river, fly straight
    if (troop->movement == AIR_MOVEMENT) {
        double dx = target_pos.x - troop->position.x;
        double dy = target_pos.y - troop->position.y;
        troop->angle = atan2(dy, dx);

        troop->position.x += troop->step_size * cos(troop->angle);
        troop->position.y += troop->step_size * sin(troop->angle);
        return;
    }

    // GROUND TROOPS: Must use bridges to cross river
    if (troop->movement == GROUND_MOVEMENT) {
        // troop->position IS the anchor point (feet position)
        // No need to add anchor offset again
        position_t anchor_pos = troop->position;

        // Check if we need to cross the river (using anchor position)
        if (needs_to_cross_river(anchor_pos, target_pos)) {
            if (!is_on_bridge(anchor_pos)) {
                // Not on bridge, need to navigate to one
                bridge_t *bridge = select_nearest_bridge(anchor_pos, target_pos);

                // Move toward the bridge center (no Y offset - is_on_bridge checks feet position)
                double dx = bridge->center.x - troop->position.x;
                double dy = bridge->center.y - troop->position.y;
                troop->angle = atan2(dy, dx);

                troop->position.x += troop->step_size * cos(troop->angle);
                troop->position.y += troop->step_size * sin(troop->angle);
                return;
            }
            // If on bridge, continue normal movement (will cross river)
        }
    }

    // Normal movement toward target
    double dx = target_pos.x - troop->position.x;
    double dy = target_pos.y - troop->position.y;
    troop->angle = atan2(dy, dx);

    troop->position.x += troop->step_size * cos(troop->angle);
    troop->position.y += troop->step_size * sin(troop->angle);
}
