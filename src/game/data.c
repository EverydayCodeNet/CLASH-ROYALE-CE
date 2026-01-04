#include "data.h"
#include "memory_simple.h"
#include <fileioc.h>
#include "../gfx/gfx.h"

// Global data variable definition
data_t data = {0};

gfx_sprite_t *get_chest_sprite(chest_rarity_t rarity) {
    switch (rarity) {
        case SILVER: return silver_chest;
        case GOLD: return gold_chest;
        case MAGICAL: return magical_chest;
        default: return silver_chest;
    }
}

void init_data_defaults(data_t *data) {
    data->games_played = 0;
    data->games_won = 0;
    data->trophies = 0;
    data->gold = 100;
    data->time_played = 0;

    // Initialize default deck order (0-7 in sequence)
    for (int i = 0; i < 8; i++) {
        data->deck_order[i] = i;
    }

    // Initialize chests array with test chests
    data->chests = CR_MALLOC(sizeof(chest_t) * 4);

    // Chest 0: Silver (locked)
    data->chests[0].status = LOCKED;
    data->chests[0].rarity = SILVER;
    data->chests[0].gold = 50;
    data->chests[0].total_cards = 3;
    data->chests[0].unlock_step = 0;
    data->chests[0].time_elapsed = 0;
    data->chests[0].duration = 60;  // 1 minute
    data->chests[0].sprite = silver_chest;

    // Chest 1: Gold (locked)
    data->chests[1].status = LOCKED;
    data->chests[1].rarity = GOLD;
    data->chests[1].gold = 100;
    data->chests[1].total_cards = 6;
    data->chests[1].unlock_step = 0;
    data->chests[1].time_elapsed = 0;
    data->chests[1].duration = 120;  // 2 minutes
    data->chests[1].sprite = gold_chest;

    // Chest 2: Magical (locked)
    data->chests[2].status = LOCKED;
    data->chests[2].rarity = MAGICAL;
    data->chests[2].gold = 200;
    data->chests[2].total_cards = 10;
    data->chests[2].unlock_step = 0;
    data->chests[2].time_elapsed = 0;
    data->chests[2].duration = 240;  // 4 minutes
    data->chests[2].sprite = magical_chest;

    // Chest 3: Empty
    data->chests[3].status = EMPTY;
    data->chests[3].rarity = SILVER;
    data->chests[3].gold = 0;
    data->chests[3].total_cards = 0;
    data->chests[3].unlock_step = 0;
    data->chests[3].time_elapsed = 0;
    data->chests[3].duration = 0;
    data->chests[3].sprite = silver_chest;
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

    // Create save_data_t from current data (only primitive values)
    save_data_t save = {0};
    save.games_played = data.games_played;
    save.games_won = data.games_won;
    save.trophies = data.trophies;
    save.gold = data.gold;
    save.time_played = data.time_played;
    for (int i = 0; i < 8; i++) {
        save.deck_order[i] = data.deck_order[i];
    }

    // Save chest data
    for (int i = 0; i < 4; i++) {
        save.chests[i].rarity = data.chests[i].rarity;
        save.chests[i].status = data.chests[i].status;
        save.chests[i].gold = data.chests[i].gold;
        save.chests[i].total_cards = data.chests[i].total_cards;
        save.chests[i].time_elapsed = data.chests[i].time_elapsed;
        save.chests[i].duration = data.chests[i].duration;
    }

    ti_var_t slot;
    if ((slot = ti_Open("CRDATA", "w+"))) {
        ti_Write(&save, sizeof(save_data_t), 1, slot);
        ti_SetArchiveStatus(true, slot);
        ti_Close(slot);
    }
}

void load_data(void) {
    ti_var_t slot;
    if ((slot = ti_Open("CRDATA", "r"))) {
        save_data_t save = {0};
        ti_Read(&save, sizeof(save_data_t), 1, slot);
        ti_Close(slot);

        // Copy primitive values to data
        data.games_played = save.games_played;
        data.games_won = save.games_won;
        data.trophies = save.trophies;
        data.gold = save.gold;
        data.time_played = save.time_played;
        for (int i = 0; i < 8; i++) {
            data.deck_order[i] = save.deck_order[i];
        }

        // Load chest data from save
        data.chests = CR_MALLOC(sizeof(chest_t) * 4);
        for (int i = 0; i < 4; i++) {
            data.chests[i].rarity = save.chests[i].rarity;
            data.chests[i].status = save.chests[i].status;
            data.chests[i].gold = save.chests[i].gold;
            data.chests[i].total_cards = save.chests[i].total_cards;
            data.chests[i].time_elapsed = save.chests[i].time_elapsed;
            data.chests[i].duration = save.chests[i].duration;
            data.chests[i].unlock_step = 0;
            data.chests[i].sprite = get_chest_sprite(save.chests[i].rarity);

            // Fix corrupt/old chests: if UNLOCKING with no duration, mark as OPEN
            if (data.chests[i].status == UNLOCKING && data.chests[i].duration == 0) {
                data.chests[i].status = OPEN;
            }
            // If UNLOCKING and timer already complete, mark as OPEN
            if (data.chests[i].status == UNLOCKING &&
                data.chests[i].time_elapsed >= data.chests[i].duration) {
                data.chests[i].status = OPEN;
            }
        }
    } else {
        // File doesn't exist, create defaults
        init_data_defaults(&data);
        create_save();
    }
}

void save_data(void) {
    // Create save_data_t from current data (only primitive values)
    save_data_t save = {0};
    save.games_played = data.games_played;
    save.games_won = data.games_won;
    save.trophies = data.trophies;
    save.gold = data.gold;
    save.time_played = data.time_played;
    for (int i = 0; i < 8; i++) {
        save.deck_order[i] = data.deck_order[i];
    }

    // Save chest data
    for (int i = 0; i < 4; i++) {
        save.chests[i].rarity = data.chests[i].rarity;
        save.chests[i].status = data.chests[i].status;
        save.chests[i].gold = data.chests[i].gold;
        save.chests[i].total_cards = data.chests[i].total_cards;
        save.chests[i].time_elapsed = data.chests[i].time_elapsed;
        save.chests[i].duration = data.chests[i].duration;
    }

    ti_var_t slot;
    if ((slot = ti_Open("CRDATA", "w+"))) {
        ti_Write(&save, sizeof(save_data_t), 1, slot);
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