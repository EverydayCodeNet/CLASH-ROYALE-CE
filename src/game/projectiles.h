#ifndef PROJECTILES_H
#define PROJECTILES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <graphx.h>
#include "utils.h"
#include "structs.h"

void update_projectile_target(projectile_t *projectile);
projectile_t* create_tower_projectile(tower_t *tower, troop_t *target);
position_t get_tower_center(tower_t *tower);
position_t get_troop_center(troop_t *troop);
bool projectile_hit_target(projectile_t *projectile, void *target, card_type_t target_type);
void move_projectile_toward_target(projectile_t *projectile);
troop_t* find_closest_target_in_range(tower_t *tower, troop_t *troops, double max_range);

#ifdef __cplusplus
}
#endif

#endif