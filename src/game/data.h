#ifndef DATA_H
#define DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h"

// Persistent chest data (for save file)
typedef struct {
    chest_rarity_t rarity;
    chest_status_t status;
    unsigned int gold;
    unsigned int total_cards;
    unsigned int time_elapsed;
    unsigned int duration;
} save_chest_t;

// Persistent save data (written to file)
typedef struct {
    unsigned int games_played;
    unsigned int games_won;
    unsigned int trophies;
    unsigned int gold;
    unsigned int time_played;

    // Deck order - indices into available_cards (0-7)
    int deck_order[8];

    // Chest slots (4 chests)
    save_chest_t chests[4];
} save_data_t;

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

    // Deck order for persistence
    int deck_order[8];

    // Last game result (for battle_results screen)
    bool has_pending_result;
    bool last_victory;
    unsigned int last_player_crowns;
    unsigned int last_opponent_crowns;
    int last_trophy_change;
    chest_rarity_t last_chest_awarded;
    bool last_chest_given;

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
gfx_sprite_t *get_chest_sprite(chest_rarity_t rarity);

#ifdef __cplusplus
}
#endif

#endif