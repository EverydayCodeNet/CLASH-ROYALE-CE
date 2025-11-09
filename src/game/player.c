#include "structs.h"
#include "towers.h"
#include "game.h"
#include "memory_simple.h"

// Function declaration for shuffle function from menu.c
void shuffle_card_indices(int *indices, int size);

player_t *init_player(bool opponent, card_t *deck) {
    player_t *player = CR_MALLOC(sizeof(player_t));
    const int SCREEN_HEIGHT = 240;
    // Initialize cursor in the middle of player's placement area
    const int x = opponent ? 260 : 85;  // opponent: 260 (right side 200-320), player: 85 (left side 0-170)
    const int y = 120;  // middle of screen height
    player->troops = NULL;
    player->spells = NULL;
    player->buildings = NULL;
    player->deck = CR_MALLOC(sizeof(deck_t));
    // MEMORY OPTIMIZATION: Allocate indices array instead of copying cards
    player->deck->card_indices = CR_MALLOC(sizeof(int) * 8);  // 8 card deck
    player->deck->available_cards = deck;  // Point to master card array
    player->deck->selected_card = -1;
    
    // Initialize deck with all 8 cards (indices 0-7)
    for (int i = 0; i < 8; i++) {
        player->deck->card_indices[i] = i;
    }
    
    // Shuffle the deck indices for variety
    shuffle_card_indices(player->deck->card_indices, 8);
    player->elixir = 5;
    player->cursor.x = x;
    player->cursor.y = y;
    player->towers_destroyed = 0;
    player->opponent = opponent;
    // init_towers(opponent);
    // init_bounds();
    // init_deck(game);

    return player;
}

