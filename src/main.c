#include <tice.h>
#include <graphx.h>
#include <stdlib.h>

#include "ui/menu.h"
#include "gfx/gfx.h"
#include "game/memory_simple.h"
#include "game/game.h"
#include "game/data.h"

// Function declarations that are normally in menu.h
void init_deck(data_t *data);
void free_game(game_t *game);
void load_data(void);
void free_data(data_t *data);

// Using simple memory system - no complex debugging or pools

int main(void) {
    // Simple memory system - no initialization needed
    
    srand(rtc_Time());
    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_SetTransparentColor(170);

    // BYPASS MENU: Go directly to game to avoid memory recursion
    // start_menu();  // COMMENTED OUT - was causing memory issues
    
    // Minimal initialization for direct game launch
    load_data();
    
    // Essential: Initialize deck system 
    init_deck(&data);
    
    // Verify data is properly initialized
    if (data.available_cards == NULL) {
        gfx_End();
        return -1; // Failed to initialize cards
    }
    
    game_t *game = init_game(&data);
    if (game == NULL) {
        free_data(&data);
        gfx_End();
        return -1; // Failed to initialize game
    }
    
    run_game(game);
    
    // Cleanup
    free_game(game);
    free_data(&data);
    // Simple memory system - no cleanup needed
    
    gfx_End();
    return 0;
}