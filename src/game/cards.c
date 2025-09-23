#include "cards.h"
#include "structs.h"

bool can_afford_card(player_t *player, card_t *card) {
    return player->elixir >= card->elixir;
}

card_t* get_card_by_index(player_t *player, int index) {
    if (index < 0 || index >= 4) return NULL;
    // MEMORY OPTIMIZATION: Use index-based lookup instead of direct array access
    int card_index = player->deck->card_indices[index];
    return &player->deck->available_cards[card_index];
}

rarity_t get_card_rarity_from_chest(chest_rarity_t chest_rarity) {
    switch (chest_rarity) {
        case SILVER:
            return (rand() % 100 < 95) ? COMMON : RARE;
        case GOLD:
            return (rand() % 100 < 80) ? COMMON : ((rand() % 100 < 95) ? RARE : EPIC);
        case MAGICAL:
            return (rand() % 100 < 60) ? RARE : ((rand() % 100 < 90) ? EPIC : LEGENDARY);
        default:
            return COMMON;
    }
}
