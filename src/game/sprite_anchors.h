#ifndef SPRITE_ANCHORS_H
#define SPRITE_ANCHORS_H

#ifdef __cplusplus
extern "C" {
#endif

// Anchor point structure - defines where the "feet" or center of mass
// is relative to sprite top-left corner
typedef struct {
    int x;
    int y;
} anchor_t;

// Maximum number of cards (adjust if you add more)
#define MAX_CARD_ANCHORS 16

// Anchor data indexed by card array position
extern const anchor_t SPRITE_ANCHORS[MAX_CARD_ANCHORS];

#ifdef __cplusplus
}
#endif

#endif
