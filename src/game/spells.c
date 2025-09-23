#include "spells.h"
#include "structs.h"
#include <math.h>

#define TILE_SIZE 10

void apply_spell_damage(spell_t *spell, game_t *game) {
    if (spell->time_elapsed % spell->ticks != 0) return;
    
    // Get enemy units (opposite of who cast spell)
    player_t *enemy = game->player; // Assume AI cast this - adjust logic as needed
    
    // Damage troops in radius
    troop_t *current_troop = enemy->troops;
    while (current_troop != NULL) {
        double dx = current_troop->position.x - spell->position.x;
        double dy = current_troop->position.y - spell->position.y;
        double distance = sqrt(dx*dx + dy*dy);
        
        if (distance <= spell->radius * TILE_SIZE) {
            current_troop->health -= spell->damage / spell->duration;
        }
        current_troop = current_troop->next;
    }
    
    // Damage buildings in radius
    building_t *current_building = enemy->buildings;
    while (current_building != NULL) {
        double dx = current_building->position.x - spell->position.x;
        double dy = current_building->position.y - spell->position.y;
        double distance = sqrt(dx*dx + dy*dy);
        
        if (distance <= spell->radius * TILE_SIZE) {
            current_building->health -= spell->damage / spell->duration;
        }
        current_building = current_building->next;
    }
}
