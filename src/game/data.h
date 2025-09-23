#ifndef DATA_H
#define DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h"

typedef struct {
    unsigned int games_played;
    unsigned int games_won;
    unsigned int trophies;
    unsigned int gold;

    // Global cards array
    card_t *available_cards;
    // Player's unlocked card array
    card_t *unlocked_cards;
    // Player's 8 selected cards
    card_t *deck;

    unsigned int time_played;
    chest_t *chests;

    // How should I store the chest slots?
    // Do they need to be in persistent data?
        // - Not necessarily. init_chest_slots 
    // But what values do chest slots have that chests don't?
    // occupied or not occupied

    // need a player instance that stores deck and local instance that has elixir, deck orders, game stats 
    // Player *player;

    // // player name
    // // gems, coins
    // Chest *chests[4];

    // // extend card class to include level and num cards collected
    // Card **deck;

    // need to get deck from here into player instance

    // // arena, decks, card levels, chests, experience
} data_t;

// Global data variable
extern data_t data;

void create_save(void);
void load_data(void);
void save_data(void);
void cleanup_data(data_t *data);

#ifdef __cplusplus
}
#endif

#endif