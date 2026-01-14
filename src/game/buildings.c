#include "buildings.h"
#include "structs.h"

void update_elixir_collector(building_t *collector, player_t *owner) {
    if (collector->attack_ticks % 30 == 0) { // Every ~1 second
        owner->elixir += collector->elixir_generated;
    }
}
