#include "sprite_anchors.h"

// Anchor points for each card type
// Index corresponds to available_cards[] array position
// Values are (x, y) offset from sprite top-left to the "feet" position
//
// TODO: In the future, we may need different anchors for different animation
// frames (e.g., attack frames with extended sword vs idle frames).
// Could expand to SPRITE_ANCHORS[card_index][frame_index] if needed.

const anchor_t SPRITE_ANCHORS[MAX_CARD_ANCHORS] = {
    [0]  = {0, 10},  // miner - placeholder, measure actual sprite
    [1]  = {4, 6},  // musketeer - placeholder
    [2]  = {0, 15},  // balloon - placeholder (air unit)
    [3]  = {0, 20},  // giant - placeholder
    [4]  = {0, 0},   // zap spell - no anchor needed
    [5]  = {0, 0},   // poison spell - no anchor needed
    [6]  = {0, 0},   // elixir_collector building - position is top-left
    [7]  = {6, 12},  // knight - placeholder
    [8]  = {15, 30},  // placeholder
    [9]  = {15, 30},  // placeholder
    [10] = {15, 30},  // placeholder
    [11] = {15, 30},  // placeholder
    [12] = {15, 30},  // placeholder
    [13] = {15, 30},  // placeholder
    [14] = {15, 30},  // placeholder
    [15] = {15, 30},  // placeholder
};
