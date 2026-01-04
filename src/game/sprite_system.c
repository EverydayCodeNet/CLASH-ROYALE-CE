#include "sprite_system.h"
#include <graphx.h>

void draw_troop_frame(const troop_sprite_def_t *def, bool facing_down,
                      uint8_t frame_index, int x, int y) {
    if (def == NULL) return;

    // Select the appropriate sheet based on facing direction
    gfx_sprite_t *sheet = facing_down ? def->sheet_down : def->sheet_up;
    if (sheet == NULL) return;

    // Calculate source x position in the sheet
    int src_x = frame_index * def->frame_width;

    // Set clip region for this frame
    gfx_SetClipRegion(x, y, x + def->frame_width, y + def->frame_height);

    // Draw from sheet using clipping (sheet is offset left so correct frame shows)
    gfx_TransparentSprite(sheet, x - src_x, y);

    // Reset clip region to full screen
    gfx_SetClipRegion(0, 0, 320, 240);
}

void draw_troop_movement(const troop_sprite_def_t *def, bool facing_down,
                         uint8_t seq_index, int x, int y) {
    if (def == NULL) return;

    // Get frame from movement sequence
    uint8_t frame = def->movement_start + def->movement_sequence[seq_index % def->movement_sequence_len];
    draw_troop_frame(def, facing_down, frame, x, y);
}

void draw_troop_attack(const troop_sprite_def_t *def, bool facing_down,
                       uint8_t attack_frame, int x, int y) {
    if (def == NULL) return;

    uint8_t frame = def->attack_start + (attack_frame % def->attack_count);
    draw_troop_frame(def, facing_down, frame, x, y);
}
