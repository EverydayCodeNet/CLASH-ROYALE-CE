#include "projectiles.h"
#include "structs.h"
#include "memory_simple.h"
#include <math.h>
#include <stdlib.h>

#define COLLISION_THRESHOLD 8.0
#define TOWER_CENTER_OFFSET_X 25
#define TOWER_CENTER_OFFSET_Y 25
#define KING_TOWER_CENTER_OFFSET_X 30
#define KING_TOWER_CENTER_OFFSET_Y 30

position_t get_tower_center(tower_t *tower) {
    position_t center;
    // Adjust for tower sprite size - towers should fire from center
    center.x = tower->position.x + TOWER_CENTER_OFFSET_X;
    center.y = tower->position.y + TOWER_CENTER_OFFSET_Y;
    return center;
}

position_t get_troop_center(troop_t *troop) {
    position_t center;
    // Get center of troop sprite for accurate targeting
    if (troop->sprite != NULL) {
        center.x = troop->position.x + (troop->sprite->width / 2);
        center.y = troop->position.y + (troop->sprite->height / 2);
    } else {
        // Fallback if sprite is null
        center.x = troop->position.x + 8;
        center.y = troop->position.y + 8;
    }
    return center;
}

troop_t* find_closest_target_in_range(tower_t *tower, troop_t *troops, double max_range) {
    if (troops == NULL || tower == NULL) return NULL;
    
    position_t tower_center = get_tower_center(tower);
    troop_t *closest_target = NULL;
    double closest_distance = max_range + 1;
    
    troop_t *current = troops;
    while (current != NULL) {
        // Only target living troops
        if (current->health > 0) {
            position_t troop_center = get_troop_center(current);
            
            // Calculate distance from tower center to troop center
            double dx = troop_center.x - tower_center.x;
            double dy = troop_center.y - tower_center.y;
            double distance = sqrt(dx * dx + dy * dy);
            
            // Use a larger range for initial target finding (towers have range issues)
            double effective_range = max_range * 10; // Temporarily increase range for testing
            
            // Check if within range and closer than current closest
            if (distance <= effective_range && distance < closest_distance) {
                closest_target = current;
                closest_distance = distance;
            }
        }
        current = current->next;
    }
    
    return closest_target;
}

projectile_t* create_tower_projectile(tower_t *tower, troop_t *target) {
    if (tower == NULL || target == NULL) return NULL;
    
    // Use the memory system from the game
    projectile_t *projectile = CR_MALLOC(sizeof(projectile_t));
    if (projectile == NULL) return NULL;
    
    // Start projectile from tower center
    position_t tower_center = get_tower_center(tower);
    position_t target_center = get_troop_center(target);
    
    projectile->sprite = tower->projectile_sprite;
    projectile->position = tower_center;
    projectile->target_type = TROOP;
    projectile->damage = tower->damage;
    projectile->speed = tower->projectile_speed;
    projectile->target = target;
    projectile->next = NULL;
    projectile->prev = NULL;
    
    // Calculate initial angle toward target
    double dx = target_center.x - tower_center.x;
    double dy = target_center.y - tower_center.y;
    projectile->angle = (unsigned int)(atan2(dy, dx) * 180.0 / M_PI);
    
    return projectile;
}

void move_projectile_toward_target(projectile_t *projectile) {
    if (projectile == NULL || projectile->target == NULL) return;
    
    // Check if target is still alive
    if (projectile->target_type == TROOP) {
        troop_t *troop = (troop_t*)projectile->target;
        if (troop->health <= 0) {
            projectile->target = NULL;
            return;
        }
    }
    
    // Update projectile angle toward moving target (pathfinding)
    position_t target_center = get_troop_center((troop_t*)projectile->target);
    
    double dx = target_center.x - projectile->position.x;
    double dy = target_center.y - projectile->position.y;
    
    // Only update angle if target has moved significantly
    double distance_to_target = sqrt(dx * dx + dy * dy);
    if (distance_to_target > COLLISION_THRESHOLD) {
        projectile->angle = (unsigned int)(atan2(dy, dx) * 180.0 / M_PI);
        
        // Move projectile toward target
        double rad_angle = projectile->angle * M_PI / 180.0;
        projectile->position.x += cos(rad_angle) * projectile->speed;
        projectile->position.y += sin(rad_angle) * projectile->speed;
    }
}

bool projectile_hit_target(projectile_t *projectile, void *target, card_type_t target_type) {
    if (projectile == NULL || target == NULL) return false;
    
    position_t target_center;
    
    switch (target_type) {
        case TROOP:
            target_center = get_troop_center((troop_t*)target);
            break;
        case TOWER:
            target_center = get_tower_center((tower_t*)target);
            break;
        case BUILDING:
            // Buildings use position directly for now
            target_center = ((building_t*)target)->position;
            break;
        default:
            return false;
    }
    
    // Calculate distance between projectile and target center
    double dx = projectile->position.x - target_center.x;
    double dy = projectile->position.y - target_center.y;
    double distance = sqrt(dx * dx + dy * dy);
    
    // Use a larger collision threshold to make hits more reliable
    return distance <= (COLLISION_THRESHOLD * 2);
}

void update_projectile_target(projectile_t *projectile) {
    if (projectile->target == NULL) return;
    
    // Check if target is still alive before updating
    if (projectile->target_type == TROOP) {
        troop_t *troop = (troop_t*)projectile->target;
        if (troop->health <= 0) {
            projectile->target = NULL;
            return;
        }
    }
    
    // Update projectile movement toward target
    move_projectile_toward_target(projectile);
}
