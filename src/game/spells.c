#include "spells.h"
#include "structs.h"
#include <math.h>

#define TILE_SIZE 10

void apply_spell_damage_to_enemy(spell_t *spell, player_t *enemy) {
    // Determine if this is an instant spell or DoT spell
    // Instant: duration <= 1 (like Zap) - apply all damage once
    // DoT: duration > 1 (like Poison) - apply damage every frame

    bool is_instant = (spell->duration <= 1);

    // For instant spells, only apply damage on first tick
    if (is_instant && spell->time_elapsed > 0) return;

    // Calculate damage for this frame
    int frame_damage;
    if (is_instant) {
        // Instant spell: all damage at once (minimum 400)
        frame_damage = spell->damage;
        if (frame_damage < 400) frame_damage = 400;
    } else {
        // DoT spell (Poison): apply damage at tick intervals
        // Total damage is spread across number of applications
        // Applications = duration / ticks (e.g., 480 / 60 = 8 applications)
        unsigned int total = spell->damage;
        unsigned int num_applications = 1;
        if (spell->ticks > 0 && spell->duration > 0) {
            num_applications = spell->duration / spell->ticks;
        }
        if (num_applications < 1) num_applications = 1;
        frame_damage = total / num_applications;  // e.g., 600 / 8 = 75 per application
    }

    double spell_radius = spell->radius * TILE_SIZE;

    // Damage troops in radius - use troop center for distance check
    troop_t *current_troop = enemy->troops;
    while (current_troop != NULL) {
        // Get troop center position (account for sprite size)
        double troop_cx = current_troop->position.x;
        double troop_cy = current_troop->position.y;
        if (current_troop->sprite_def != NULL) {
            // Adjust for hitbox offset
            troop_cy -= current_troop->sprite_def->hitbox_y_offset;
        }

        double dx = troop_cx - spell->position.x;
        double dy = troop_cy - spell->position.y;
        double distance = sqrt(dx*dx + dy*dy);

        // Add troop hitbox radius to spell radius for generous hit detection
        int troop_radius = 10;  // Default
        if (current_troop->sprite_def != NULL) {
            troop_radius = current_troop->sprite_def->hitbox_radius;
        }

        if (distance <= spell_radius + troop_radius) {
            current_troop->health -= frame_damage;
        }
        current_troop = current_troop->next;
    }

    // Damage buildings in radius
    building_t *current_building = enemy->buildings;
    while (current_building != NULL) {
        double dx = current_building->position.x - spell->position.x;
        double dy = current_building->position.y - spell->position.y;
        double distance = sqrt(dx*dx + dy*dy);

        if (distance <= spell_radius) {
            current_building->health -= frame_damage;
        }
        current_building = current_building->next;
    }

    // Damage towers in radius
    for (int i = 0; i < 3; i++) {
        tower_t *tower = &enemy->towers[i];
        if (!tower->active || tower->health <= 0) continue;

        // Get tower center
        int tower_size = (i == 2) ? 60 : 50;  // King=60, Princess=50
        double tower_cx = tower->position.x + tower_size / 2;
        double tower_cy = tower->position.y + tower_size / 2;

        double dx = tower_cx - spell->position.x;
        double dy = tower_cy - spell->position.y;
        double distance = sqrt(dx*dx + dy*dy);

        // Tower is hit if spell center is within radius + half tower size
        if (distance <= spell_radius + tower_size / 2) {
            tower->health -= frame_damage;
        }
    }
}
