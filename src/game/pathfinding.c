#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pathfinding.h"
#include "memory_simple.h"

// Static bridge data - Y uses walk position (top of bridge sprite)
static bridge_t bridges[2] = {
    {{BRIDGE_CENTER_X, BRIDGE_1_WALK_Y}, BRIDGE_WIDTH, BRIDGE_HEIGHT},
    {{BRIDGE_CENTER_X, BRIDGE_2_WALK_Y}, BRIDGE_WIDTH, BRIDGE_HEIGHT}
};

// ============================================================================
// BRIDGE AND RIVER DETECTION
// ============================================================================

bool is_on_bridge(position_t pos) {
    // Check if X is in river crossing zone
    if (pos.x < RIVER_X_MIN || pos.x > RIVER_X_MAX) return false;

    // Check if Y is at either bridge walk position (using defined tolerance)
    return ((pos.y >= BRIDGE_1_WALK_Y - BRIDGE_Y_TOLERANCE && pos.y <= BRIDGE_1_WALK_Y + BRIDGE_Y_TOLERANCE) ||
            (pos.y >= BRIDGE_2_WALK_Y - BRIDGE_Y_TOLERANCE && pos.y <= BRIDGE_2_WALK_Y + BRIDGE_Y_TOLERANCE));
}

// ============================================================================
// OBSTACLE DETECTION
// ============================================================================

bool is_in_river(position_t pos) {
    // Check if position is in the river zone but NOT on a bridge
    if (pos.x < RIVER_X_MIN || pos.x > RIVER_X_MAX) return false;

    // On a bridge? Not blocked
    if (is_on_bridge(pos)) return false;

    // In river zone but not on bridge = blocked
    return true;
}

bool is_blocked_by_tower(position_t pos, player_t *player, player_t *opponent) {
    const int PRINCESS_TOWER_SIZE = 50;
    const int KING_TOWER_SIZE = 60;

    // Check both player's and opponent's towers
    player_t *players[2] = {player, opponent};

    for (int p = 0; p < 2; p++) {
        if (players[p] == NULL || players[p]->towers == NULL) continue;

        for (int i = 0; i < 3; i++) {
            tower_t *tower = &players[p]->towers[i];
            if (!tower->active) continue;

            int tower_size = (i == 2) ? KING_TOWER_SIZE : PRINCESS_TOWER_SIZE;
            int tower_radius = (i == 2) ? KING_TOWER_RADIUS : PRINCESS_TOWER_RADIUS;

            // Tower center
            double center_x = tower->position.x + (tower_size / 2);
            double center_y = tower->position.y + (tower_size / 2);

            // Distance check (squared to avoid sqrt)
            double dx = pos.x - center_x;
            double dy = pos.y - center_y;
            double dist_sq = dx * dx + dy * dy;

            if (dist_sq < tower_radius * tower_radius) {
                return true;
            }
        }
    }

    return false;
}

// Check if position is inside any tower and return push-out position
// Returns the same position if not inside a tower, or a position pushed outside
position_t get_tower_pushout(position_t pos, player_t *player, player_t *opponent) {
    const int PRINCESS_TOWER_SIZE = 50;
    const int KING_TOWER_SIZE = 60;

    player_t *players[2] = {player, opponent};

    for (int p = 0; p < 2; p++) {
        if (players[p] == NULL || players[p]->towers == NULL) continue;

        for (int i = 0; i < 3; i++) {
            tower_t *tower = &players[p]->towers[i];
            if (!tower->active) continue;

            int tower_size = (i == 2) ? KING_TOWER_SIZE : PRINCESS_TOWER_SIZE;
            int tower_radius = (i == 2) ? KING_TOWER_RADIUS : PRINCESS_TOWER_RADIUS;

            double center_x = tower->position.x + (tower_size / 2);
            double center_y = tower->position.y + (tower_size / 2);

            double dx = pos.x - center_x;
            double dy = pos.y - center_y;
            double dist = sqrt(dx * dx + dy * dy);

            // If inside tower, calculate push direction
            if (dist < tower_radius) {
                position_t push;
                if (dist < 0.1) {
                    // Exactly at center - push in arbitrary direction (toward closest edge)
                    push.x = pos.x + tower_radius + 5;
                    push.y = pos.y;
                } else {
                    // Push radially outward past the tower edge
                    double push_dist = tower_radius + 5 - dist;
                    push.x = pos.x + (dx / dist) * push_dist;
                    push.y = pos.y + (dy / dist) * push_dist;
                }
                // Clamp to screen bounds (320x240 rotated: x is 0-319, y is 0-239)
                if (push.x < 0) push.x = 0;
                if (push.x > 319) push.x = 319;
                if (push.y < 0) push.y = 0;
                if (push.y > 239) push.y = 239;
                return push;
            }
        }
    }

    // Not inside any tower
    return pos;
}

bool is_blocked_by_building(position_t pos, player_t *player, player_t *opponent) {
    // Check both player's and opponent's buildings
    player_t *players[2] = {player, opponent};

    // Margin to keep troops away from building edges
    const int MARGIN = 5;

    for (int p = 0; p < 2; p++) {
        if (players[p] == NULL) continue;

        building_t *building = players[p]->buildings;
        while (building != NULL) {
            // Use actual sprite dimensions for collision
            int sprite_w = building->sprite ? building->sprite->width : 30;
            int sprite_h = building->sprite ? building->sprite->height : 30;

            // Simple bounding box check with margin
            int bx = building->position.x - MARGIN;
            int by = building->position.y - MARGIN;
            int bw = sprite_w + (MARGIN * 2);
            int bh = sprite_h + (MARGIN * 2);

            if (pos.x >= bx && pos.x < bx + bw &&
                pos.y >= by && pos.y < by + bh) {
                return true;
            }

            building = building->next;
        }
    }

    return false;
}

bool is_position_blocked(position_t pos, player_t *player, player_t *opponent) {
    // Check all obstacle types
    if (is_in_river(pos)) return true;
    if (is_blocked_by_tower(pos, player, opponent)) return true;
    if (is_blocked_by_building(pos, player, opponent)) return true;

    return false;
}

// Check for obstacles excluding river (for use when navigating TO bridge)
bool is_blocked_by_structure(position_t pos, player_t *player, player_t *opponent) {
    if (is_blocked_by_tower(pos, player, opponent)) return true;
    if (is_blocked_by_building(pos, player, opponent)) return true;
    return false;
}

// Check if a position or the path to it is blocked
static bool is_move_blocked(position_t current, position_t desired, player_t *player, player_t *opponent,
                            bool (*is_blocked)(position_t, player_t*, player_t*)) {
    // Check destination
    if (is_blocked(desired, player, opponent)) return true;

    // Check midpoint to catch stepping over obstacles
    position_t midpoint;
    midpoint.x = (current.x + desired.x) / 2;
    midpoint.y = (current.y + desired.y) / 2;
    if (is_blocked(midpoint, player, opponent)) return true;

    return false;
}

// Internal steering helper - uses a provided blocking function
static position_t steer_with_check(position_t current, position_t desired, player_t *player, player_t *opponent,
                                    bool (*is_blocked)(position_t, player_t*, player_t*)) {
    // If path to desired position is clear, go there
    if (!is_move_blocked(current, desired, player, opponent, is_blocked)) {
        return desired;
    }

    // Calculate movement vector
    double dx = desired.x - current.x;
    double dy = desired.y - current.y;
    double step = sqrt(dx * dx + dy * dy);

    if (step < 0.1) return current;  // No movement

    // Normalize the movement vector
    double nx = dx / step;
    double ny = dy / step;

    // Try perpendicular directions first (better for going around obstacles)
    position_t alternatives[4];

    // Perpendicular left
    alternatives[0].x = current.x + step * (-ny);
    alternatives[0].y = current.y + step * nx;

    // Perpendicular right
    alternatives[1].x = current.x + step * ny;
    alternatives[1].y = current.y + step * (-nx);

    // 45 degree turns
    alternatives[2].x = current.x + step * (nx * 0.707 - ny * 0.707);
    alternatives[2].y = current.y + step * (nx * 0.707 + ny * 0.707);

    alternatives[3].x = current.x + step * (nx * 0.707 + ny * 0.707);
    alternatives[3].y = current.y + step * (-nx * 0.707 + ny * 0.707);

    // Find the first unblocked alternative
    for (int i = 0; i < 4; i++) {
        if (!is_move_blocked(current, alternatives[i], player, opponent, is_blocked)) {
            return alternatives[i];
        }
    }

    // All alternatives blocked - stay in place
    return current;
}

position_t steer_around_obstacle(position_t current, position_t desired, player_t *player, player_t *opponent) {
    return steer_with_check(current, desired, player, opponent, is_position_blocked);
}

// Steer around structures only (towers/buildings) - ignores river
// Use this when navigating TO the bridge
position_t steer_around_structures(position_t current, position_t desired, player_t *player, player_t *opponent) {
    return steer_with_check(current, desired, player, opponent, is_blocked_by_structure);
}

bool needs_to_cross_river(position_t current, position_t target) {
    // Check if currently IN the river zone - always need bridge logic
    bool in_river_zone = (current.x >= RIVER_X_MIN && current.x <= RIVER_X_MAX);
    if (in_river_zone) return true;

    // Check if path would cross the river (from outside one side to outside other side)
    bool crossing_left_to_right = (current.x < RIVER_X_MIN && target.x > RIVER_X_MAX);
    bool crossing_right_to_left = (current.x > RIVER_X_MAX && target.x < RIVER_X_MIN);

    return crossing_left_to_right || crossing_right_to_left;
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
    // AIR TROOPS: Simple direct flight to nearest tower - no obstacle avoidance needed
    if (troop->movement == AIR_MOVEMENT) {
        // Find nearest enemy tower directly
        tower_t *nearest_tower = NULL;
        double min_dist_sq = 999999.0;

        const int PRINCESS_TOWER_SIZE = 50;
        const int KING_TOWER_SIZE = 60;

        for (int i = 0; i < 3; i++) {
            tower_t *t = &opponent->towers[i];
            if (t->active && t->health > 0) {
                int tower_size = (i == 2) ? KING_TOWER_SIZE : PRINCESS_TOWER_SIZE;
                double tower_center_x = t->position.x + (tower_size / 2);
                double tower_center_y = t->position.y + (tower_size / 2);

                double dx = tower_center_x - troop->position.x;
                double dy = tower_center_y - troop->position.y;
                double dist_sq = dx * dx + dy * dy;

                if (dist_sq < min_dist_sq) {
                    min_dist_sq = dist_sq;
                    nearest_tower = t;
                }
            }
        }

        if (nearest_tower != NULL) {
            // Calculate tower center
            int tower_idx = nearest_tower - opponent->towers;
            int tower_size = (tower_idx == 2) ? KING_TOWER_SIZE : PRINCESS_TOWER_SIZE;
            double target_x = nearest_tower->position.x + (tower_size / 2);
            double target_y = nearest_tower->position.y + (tower_size / 2);

            // Fly directly toward tower center
            double dx = target_x - troop->position.x;
            double dy = target_y - troop->position.y;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist > 0.1) {
                troop->angle = atan2(dy, dx);
                troop->position.x += troop->step_size * cos(troop->angle);
                troop->position.y += troop->step_size * sin(troop->angle);
            }
        }
        return;  // AIR troops done, skip ground pathfinding
    }

    // GROUND TROOPS: Need obstacle avoidance
    // First, check if troop is stuck inside a tower and push them out (check ALL towers)
    position_t pushed = get_tower_pushout(troop->position, my_player, opponent);
    if (pushed.x != troop->position.x || pushed.y != troop->position.y) {
        troop->position = pushed;
        return;  // Don't do normal movement this tick - just escape the tower
    }

    // Find target (troop or building/tower depending on TARGET_TROOPS flag)
    void *target = find_target(troop, my_player, opponent);

    if (target == NULL) return;

    // Determine target position - use the FACING EDGE of target
    // Player troops (on left) walk right -> target LEFT edge of opponent towers
    // Opponent troops (on right) walk left -> target RIGHT edge of player towers
    position_t target_pos;
    target_pos.x = troop->position.x;  // Initialize to current position as fallback
    target_pos.y = troop->position.y;

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
            int tower_center_x = tower->position.x + (tower_size / 2);
            int tower_center_y = tower->position.y + (tower_size / 2);

            // AIR troops (balloon) fly to tower CENTER to drop bombs
            if (troop->movement == AIR_MOVEMENT) {
                target_pos.x = tower_center_x;
                target_pos.y = tower_center_y;
            }
            // GROUND troops target the FACING edge of the tower
            else if (troop_is_on_left) {
                // Player troop walking right -> target LEFT edge of opponent tower
                target_pos.x = tower->position.x;
                // Y: offset so sprite CENTER aligns with tower center, not feet
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

    // Fallback: If target is a tower but wasn't found via pointer comparison,
    // try to identify it by checking if it looks like a tower (has position field at tower offset)
    if (!is_tower) {
        // Check if target might be a tower by trying to match position
        tower_t *potential_tower = (tower_t*)target;
        for (int i = 0; i < 3; i++) {
            tower_t *t = &opponent->towers[i];
            if (t->active &&
                potential_tower->position.x == t->position.x &&
                potential_tower->position.y == t->position.y) {
                is_tower = true;
                int tower_size = (i == 2) ? KING_TOWER_SIZE : PRINCESS_TOWER_SIZE;
                int tower_center_x = t->position.x + (tower_size / 2);
                int tower_center_y = t->position.y + (tower_size / 2);

                if (troop->movement == AIR_MOVEMENT) {
                    target_pos.x = tower_center_x;
                    target_pos.y = tower_center_y;
                } else if (troop_is_on_left) {
                    target_pos.x = t->position.x;
                    target_pos.y = tower_center_y;
                } else {
                    target_pos.x = t->position.x + tower_size;
                    target_pos.y = tower_center_y;
                }
                break;
            }
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

    // GROUND TROOPS: Must use bridges to cross river and avoid obstacles
    if (troop->movement == GROUND_MOVEMENT) {
        position_t anchor_pos = troop->position;
        position_t desired_pos;

        // Check if we need to cross the river
        if (needs_to_cross_river(anchor_pos, target_pos)) {
            if (!is_on_bridge(anchor_pos)) {
                // Not on bridge, navigate to one
                bridge_t *bridge = select_nearest_bridge(anchor_pos, target_pos);

                double dx = bridge->center.x - troop->position.x;
                double dy = bridge->center.y - troop->position.y;
                troop->angle = atan2(dy, dx);

                // Calculate desired position
                desired_pos.x = troop->position.x + troop->step_size * cos(troop->angle);
                desired_pos.y = troop->position.y + troop->step_size * sin(troop->angle);

                // Steer around ENEMY structures only (troops walk past own towers freely)
                position_t steered = steer_around_structures(anchor_pos, desired_pos, NULL, opponent);

                // Update angle based on actual movement direction
                double actual_dx = steered.x - anchor_pos.x;
                double actual_dy = steered.y - anchor_pos.y;
                if (actual_dx != 0 || actual_dy != 0) {
                    troop->angle = atan2(actual_dy, actual_dx);
                }

                troop->position = steered;
                return;
            }
            // On bridge - continue to cross
        }

        // Normal ground movement toward target (also applies after crossing bridge)
        double dx = target_pos.x - anchor_pos.x;
        double dy = target_pos.y - anchor_pos.y;
        troop->angle = atan2(dy, dx);

        // Calculate desired position
        desired_pos.x = anchor_pos.x + troop->step_size * cos(troop->angle);
        desired_pos.y = anchor_pos.y + troop->step_size * sin(troop->angle);

        // Steer around ENEMY obstacles only (troops walk past own towers freely)
        position_t steered = steer_around_obstacle(anchor_pos, desired_pos, NULL, opponent);

        // Update angle based on actual movement direction
        double actual_dx = steered.x - anchor_pos.x;
        double actual_dy = steered.y - anchor_pos.y;
        if (actual_dx != 0 || actual_dy != 0) {
            troop->angle = atan2(actual_dy, actual_dx);
        }

        troop->position = steered;
        return;
    }

    // STATIONARY or fallback: Normal movement without obstacle avoidance
    double dx = target_pos.x - troop->position.x;
    double dy = target_pos.y - troop->position.y;
    troop->angle = atan2(dy, dx);

    troop->position.x += troop->step_size * cos(troop->angle);
    troop->position.y += troop->step_size * sin(troop->angle);
}
