#ifndef CARDS_H
#define CARDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h"

// Function declarations
bool can_afford_card(player_t *player, card_t *card);
card_t* get_card_by_index(player_t *player, int index);
rarity_t get_card_rarity_from_chest(chest_rarity_t chest_rarity);

#ifdef __cplusplus
}
#endif

#endif