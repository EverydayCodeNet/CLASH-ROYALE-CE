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
        // DoT spell (Poison): HIGH DPS - deal significant damage every tick
        // Real Clash Royale poison deals ~75 damage per second over 8 seconds
        // At ~60 ticks per second, that's about 1-2 damage per tick
        // But we want MUCH more damage to be impactful, so do 50+ per tick
        unsigned int total = spell->damage;
        if (total < 600) total = 600;  // Minimum 600 total damage for poison

        // High DPS: deal total/duration damage per TICK (not per second)
        // This means poison will do its full damage much faster
        unsigned int dur = spell->duration;
        if (dur < 1) dur = 1;
        frame_damage = total / (dur * 10);  // Much higher damage per tick
        if (frame_damage < 30) frame_damage = 30;  // Minimum 30 damage per tick for visible impact
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
