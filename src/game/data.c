#include "data.h"
#include "memory_simple.h"
#include <fileioc.h>
#include "../gfx/gfx.h"

// Global data variable definition
data_t data = {0};

void init_data_defaults(data_t *data) {
    data->games_played = 0;
    data->games_won = 0;
    data->trophies = 0;
    data->gold = 100;
    data->time_played = 0;
    
    // Initialize chests array
    data->chests = CR_MALLOC(sizeof(chest_t) * 4);
    for (int i = 0; i < 4; i++) {
        data->chests[i].status = EMPTY;
        data->chests[i].rarity = SILVER;
        data->chests[i].gold = 0;
        data->chests[i].total_cards = 0;
        data->chests[i].unlock_step = 0;
        data->chests[i].sprite = silver_chest;
    }
}

bool data_file_exists(void) {
    ti_var_t file = ti_Open("CRDATA", "r");
    if (file) {
        ti_Close(file);
        return true;
    }
    return false;
}

void create_save(void) {
    if (!data_file_exists()) {
        init_data_defaults(&data);
    }
    
    ti_var_t slot;
    if ((slot = ti_Open("CRDATA", "w+"))) {
        ti_Write(&data, sizeof(data), 1, slot);
        ti_SetArchiveStatus(true, slot);
        ti_Close(slot);
    }
}

void load_data(void) {
    ti_var_t slot;
    if ((slot = ti_Open("CRDATA", "r"))) {
        ti_Read(&data, sizeof(data), 1, slot);
        ti_Close(slot);
    } else {
        // File doesn't exist, create defaults
        init_data_defaults(&data);
        create_save();
    }
}

void save_data(void) {
    ti_var_t slot;
    if ((slot = ti_Open("CRDATA", "w+"))) {
        ti_Write(&data, sizeof(data), 1, slot);
        ti_SetArchiveStatus(true, slot);
        ti_Close(slot);
    }
}

void cleanup_data(data_t *data) {
    if (data == NULL) return;
    
    // Free chests array
    if (data->chests != NULL) {
        CR_FREE(data->chests);
        data->chests = NULL;
    }
    
    // Free card arrays
    if (data->available_cards != NULL) {
        CR_FREE(data->available_cards);
        data->available_cards = NULL;
    }
    
    if (data->unlocked_cards != NULL) {
        CR_FREE(data->unlocked_cards);
        data->unlocked_cards = NULL;
    }
    
    // Note: deck points to elements in available_cards, so don't free individual cards
    if (data->deck != NULL) {
        CR_FREE(data->deck);
        data->deck = NULL;
    }
}