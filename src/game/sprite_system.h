#ifndef SPRITE_SYSTEM_H
#define SPRITE_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <graphx.h>
#include <stdbool.h>
#include "structs.h"

// Draw a specific frame from a sprite sheet using clipping
// def: sprite definition containing sheet and frame info
// facing_down: true if troop is facing toward bottom of screen (front visible)
// frame_index: which frame to draw (0-indexed within the current direction)
// x, y: screen position to draw at
void draw_troop_frame(const troop_sprite_def_t *def, bool facing_down,
                      uint8_t frame_index, int x, int y);

// Draw movement animation frame
// seq_index: index into the movement_sequence array
void draw_troop_movement(const troop_sprite_def_t *def, bool facing_down,
                         uint8_t seq_index, int x, int y);

// Draw attack animation frame
// attack_frame: which attack frame to draw (0 to attack_count-1)
void draw_troop_attack(const troop_sprite_def_t *def, bool facing_down,
                       uint8_t attack_frame, int x, int y);

#ifdef __cplusplus
}
#endif

#endif
