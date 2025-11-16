#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <graphx.h>
#include <sys/rtc.h>
#include <fileioc.h>
#include <keypadc.h>
#include <sys/timers.h>
#include <string.h>

#include "../game/data.h"
#include "../gfx/gfx.h"
#include "../game/game.h"
#include "../game/structs.h"
#include "../game/memory_simple.h"
#include "menu.h"

#define TOTAL_CARDS 8

const char *appvarName = "slota";

// create_save() function is now defined in data.c

// save_data() and load_data() functions are now defined in data.c

// Forward declarations
point_t find_center(int x, int y, int icon_x, int icon_y, int width, int height, int icon_width, int icon_height);
screen_t *get_screen_by_name(screen_t *screen, char *screen_name);

// Combined button setup function
void setup_button(button_t *button, bool tab, int x, int y, int width, int height,
                  void *function, void *args, int color, int active_color,
                  gfx_sprite_t *sprite, gfx_sprite_t *icon,
                  int border_weight, int border_color, int border_active_color,
                  bool transparent, bool center_x, bool center_y, int x_offset, int y_offset) {
    // Basic properties
    button->x = x;
    button->y = y;
    button->width = width;
    button->height = height;
    button->color = color;
    button->active_color = active_color;
    button->tab = tab;
    button->active = tab;

    // Function and event setup
    if (function != NULL) {
        button->event = ACTION;
        button->function = function;
        button->args = args;
    } else {
        button->event = NONE;
        button->function = NULL;
        button->args = NULL;
    }

    // Border setup
    button->border.weight = border_weight;
    button->border.color = border_color;
    button->border.active_color = border_active_color;

    // Sprite setup
    if (sprite != NULL) button->sprite = sprite;

    // Icon setup
    if (icon != NULL) {
        button->icon.sprite = icon;
        button->has_icon = true;
        button->icon.transparent = transparent;

        point_t center = find_center(x, y, x, y, width, height, icon->width, icon->height);
        button->icon.x = center_x ? center.x : x + x_offset;
        button->icon.y = center_y ? center.y : y + y_offset;
    } else {
        button->has_icon = false;
    }
}

// Simplified button init for basic cases
void init_button(button_t *button, bool tab, int x, int y, int width, int height, void *function, void *args) {
    setup_button(button, tab, x, y, width, height, function, args, 100, 0, NULL, NULL, 0, 0, 0, false, false, false, 0, 0);
}

// Legacy functions for compatibility
void customize_button(button_t *button, int color, int active_color, gfx_sprite_t *sprite, gfx_sprite_t *icon) {
    button->color = color;
    button->active_color = active_color;
    if (sprite != NULL) button->sprite = sprite;
    if (icon != NULL) {
        button->icon.sprite = icon;
        button->has_icon = true;
    }
}

// Tab button setup helper - simplified wrapper
void setup_tab_button(button_t *button, int x, int y, int width, int height, gfx_sprite_t *icon, bool active) {
    int color = active ? 116 : 83;  // Selected vs inactive tab colors
    int active_color = active ? 116 : 83;
    setup_button(button, true, x, y, width, height, NULL, NULL, color, active_color, NULL, icon, 0, 0, 0, true, true, true, 0, 0);
    button->active = active;
}

void customize_border(button_t *button, int weight, int color, int active_color) {
    button->border.weight = weight;
    button->border.color = color;
    button->border.active_color = active_color;
}

void set_selection_delay(screen_t *screen, int delay) {
    screen->delay = delay;
}

bool can_unlock(chest_t *chests) {
    const int NUM_CHESTS = 4;
    for (int i = 0; i < NUM_CHESTS; i++) {
        chest_t *chest = &chests[i];
        if (chest->status == UNLOCKING) return false;
    }
    return true;
}

void unlock_chest(chest_t *chest) {
    chest->status = UNLOCKING;
}

void continue_chest_opening(void) {
    // Implementation for chest opening continuation
}

void use_card_from_collection(void *args) {
    (void)args; // Suppress unused parameter warning
    // Get selected card from collection and add to deck
    // Implementation to be added
}

void view_card_upgrades(void *args) {
    (void)args; // Suppress unused parameter warning
    // Show upgrade options for selected card
    // Implementation to be added
}

void switch_to_collection(void *args) {
    screen_t *current_screen = (screen_t *)args;
    screen_t *collection_screen = get_screen_by_name(current_screen, "collection");

    if (collection_screen != NULL) {
        current_screen->active = false;
        collection_screen->active = true;
        collection_screen->selection_index = 1; // Start on card grid
    }
}

void switch_to_deck(void *args) {
    screen_t *current_screen = (screen_t *)args;
    screen_t *deck_screen = get_screen_by_name(current_screen, "deck");

    if (deck_screen != NULL) {
        current_screen->active = false;
        deck_screen->active = true;
        deck_screen->selection_index = 2; // Start on card grid
    }
}

screen_t *get_screen_by_name(screen_t *screen, char *screen_name) {
    screen_t *temp = screen;
    while (temp != NULL && strcmp(temp->name, screen_name) != 0) {
        temp = temp->next;
    }
    return temp;
}

void open_chest(screen_t *screen, chest_t *chest) {
    screen->active = false;
    screen_t *chest_opening = get_screen_by_name(screen, "chest_opening");
    chest_opening->active = true;
    chest->status = UNLOCKING;
    chest->unlock_step = 0;
}

void handle_chests(screen_t *screen) {
    chest_t *chests = screen->data->chests;
    if (chests == NULL) return;

    int primary_selection_index = screen->selection_index;
    int index = screen->primary_selections[primary_selection_index].secondary_selection_index;
    chest_t *chest = &chests[index];
    chest_status_t status = chest->status;
    if (status == LOCKED) {
        if (can_unlock(chests) == true) {
            unlock_chest(chest);
        }
    } else if (status == OPEN) {
        open_chest(screen, chest);
    }
}

point_t find_center(int x, int y, int icon_x, int icon_y, int width, int height, int icon_width, int icon_height) {
    (void)icon_x; (void)icon_y; // Suppress unused parameter warnings
    point_t point;
    point.x = x + (width - icon_width) / 2;
    point.y = y + (height - icon_height) / 2;
    return point;
}
                                                                                                                                                                                             
void setup_card_button(button_t *button, int x, int y, int width, int height, card_t *card) {
    int active_color = 100;
    if (card != NULL) {
        switch(card->rarity) {
            case COMMON: active_color = 151; break;
            case RARE: active_color = 150; break;
            case EPIC: active_color = 202; break;
            case LEGENDARY: active_color = 171; break;
        }
    }
    gfx_sprite_t *sprite = card ? card->sprite : NULL;
    setup_button(button, false, x, y, width, height, NULL, NULL, 100, active_color,
                 NULL, sprite, 1, 255, active_color, true, true, true, 0, 0);
}

// Helper function for common screen initialization
screen_t *create_screen(const char *name, int num_primary_selections, data_t *data) {
    screen_t *screen = CR_MALLOC(sizeof(screen_t));
    screen->name = CR_MALLOC(sizeof(char) * (strlen(name) + 1));
    strcpy(screen->name, name);
    screen->num_primary_selections = num_primary_selections;
    screen->primary_selections = CR_MALLOC(sizeof(primary_selection_t) * num_primary_selections);
    screen->selection_index = 0;
    screen->active = false;
    screen->data = data;
    screen->next = NULL;
    set_selection_delay(screen, 150);
    return screen;
}

void init_chest_slots(screen_t *screen, button_t *slots) {
    const int SLOT_HEIGHT = 45;
    const int SLOT_WIDTH = 60;
    const int NUM_CHEST_SLOTS = 4;

    for (int i = 0; i < NUM_CHEST_SLOTS; i++) {
        int y = 15 + (i * (10 + SLOT_HEIGHT));
        gfx_sprite_t *icon = NULL;

        if (screen->data->chests != NULL) {
            chest_t *chest = &screen->data->chests[i];
            if (chest->status != EMPTY) icon = chest->sprite;
        }

        setup_chest_slot(&slots[i], 75, y, SLOT_WIDTH, SLOT_HEIGHT, handle_chests, screen, icon);
    }
}

screen_t *init_deck_screen(data_t *data) {
    const int NUM_PRIMARY_SELECTIONS = 4;
    const int NUM_CARDS = 8;
    const int TOTAL_BUTTONS = NUM_CARDS + 5;

    screen_t *deck = create_screen("deck", NUM_PRIMARY_SELECTIONS, data);
    button_t *buttons = CR_MALLOC(sizeof(button_t) * TOTAL_BUTTONS);
    
    // Initialize deck tab (top tab) - clickable to switch to collection
    setup_tab_button(&buttons[0], 0, 0, 60, 120, deck_icon, true);
    buttons[0].function = switch_to_collection;
    buttons[0].args = deck;

    // Initialize battle tab (bottom tab) as an inactive, non-selectable button
    setup_tab_button(&buttons[1], 0, 120, 60, 120, battle_icon, false);

    // Initialize card grid area
    init_button(&buttons[2], false, 175, 15, 150, 200, NULL, NULL);
    customize_border(&buttons[2], 2, 255, 255);

    // Initialize elixir bar
    init_button(&buttons[3], false, 175, 175, 100, 20, NULL, NULL);
    customize_button(&buttons[3], 55, 55, NULL, NULL);
    customize_border(&buttons[3], 1, 0, 0);

    // Initialize card buttons
    button_t *card_buttons = CR_MALLOC(sizeof(button_t) * NUM_CARDS);
    for (int i = 0; i < NUM_CARDS; i++) {
        int x = (i > 3) ? 175 : 225;
        int y = 15 + ((i > 3) ? (i - 4) : i) * 40;

        card_t *card = (data && data->deck) ? &data->deck[i] : NULL;
        setup_card_button(&card_buttons[i], x, y, 40, 30, card);
    }

    // Set up primary selections
    deck->primary_selections[0].button = &buttons[0];
    deck->primary_selections[0].num_secondary_selections = 0;
    deck->primary_selections[0].secondary_selection_index = -1;

    deck->primary_selections[1].button = &buttons[1];
    deck->primary_selections[1].num_secondary_selections = 0;
    deck->primary_selections[1].secondary_selection_index = -1;

    deck->primary_selections[2].button = &buttons[2];
    deck->primary_selections[2].num_secondary_selections = NUM_CARDS;
    deck->primary_selections[2].secondary_selection_index = -1;

    deck->primary_selections[3].button = &buttons[3];
    deck->primary_selections[3].num_secondary_selections = 0;
    deck->primary_selections[3].secondary_selection_index = -1;

    // Set up secondary selections for card grid
    secondary_selection_t *card_selections = CR_MALLOC(sizeof(secondary_selection_t));
    card_selections->buttons = card_buttons;
    deck->primary_selections[2].secondary_selections = card_selections;

    return deck;
}

screen_t *init_collection_screen(data_t *data) {
    const int NUM_PRIMARY_SELECTIONS = 4;
    const int NUM_CARDS = 8;
    const int NUM_COLLECTION_CARDS = 3;

    screen_t *collection = create_screen("collection", NUM_PRIMARY_SELECTIONS, data);
    button_t *buttons = CR_MALLOC(sizeof(button_t) * (NUM_CARDS + NUM_COLLECTION_CARDS + 3));

    // Initialize deck header button (clickable to switch back to deck)
    setup_button(&buttons[0], false, 20, 10, 60, 25, switch_to_deck, collection, 83, 116, NULL, NULL, 2, 255, 255, false, false, false, 0, 0);

    // Initialize main card grid area (8 cards like deck screen)
    button_t *main_cards = CR_MALLOC(sizeof(button_t) * NUM_CARDS);
    for (int i = 0; i < NUM_CARDS; i++) {
        int x = (i > 3) ? 175 : 225;
        int y = 45 + ((i > 3) ? (i - 4) : i) * 40;

        card_t *card = (data && data->available_cards) ? &data->available_cards[i] : NULL;
        setup_card_button(&main_cards[i], x, y, 40, 30, card);
    }

    // Initialize selected card info area
    setup_button(&buttons[1], false, 20, 170, 60, 30, NULL, NULL, 100, 100, NULL, NULL, 1, 255, 255, false, false, false, 0, 0);

    // Initialize USE button
    setup_button(&buttons[2], false, 100, 170, 40, 20, use_card_from_collection, collection, 83, 116, NULL, NULL, 2, 255, 255, false, false, false, 0, 0);

    // Initialize collection area (3 cards at bottom)
    button_t *collection_cards = CR_MALLOC(sizeof(button_t) * NUM_COLLECTION_CARDS);
    for (int i = 0; i < NUM_COLLECTION_CARDS; i++) {
        int x = 20 + i * 50;
        int y = 210;

        setup_button(&collection_cards[i], false, x, y, 40, 30, NULL, NULL, 100, 120, NULL, NULL, 1, 255, 255, false, false, false, 0, 0);
    }

    // Set up primary selections
    collection->primary_selections[0].button = &buttons[0];  // Deck header
    collection->primary_selections[0].num_secondary_selections = 0;
    collection->primary_selections[0].secondary_selection_index = -1;

    collection->primary_selections[1].button = &buttons[1];  // Card grid area
    collection->primary_selections[1].num_secondary_selections = NUM_CARDS;
    collection->primary_selections[1].secondary_selection_index = -1;

    collection->primary_selections[2].button = &buttons[2];  // USE button
    collection->primary_selections[2].num_secondary_selections = 0;
    collection->primary_selections[2].secondary_selection_index = -1;

    collection->primary_selections[3].button = &buttons[1];  // Collection area
    collection->primary_selections[3].num_secondary_selections = NUM_COLLECTION_CARDS;
    collection->primary_selections[3].secondary_selection_index = -1;

    // Set up secondary selections for main card grid
    secondary_selection_t *main_card_selections = CR_MALLOC(sizeof(secondary_selection_t));
    main_card_selections->buttons = main_cards;
    collection->primary_selections[1].secondary_selections = main_card_selections;

    // Set up secondary selections for collection area
    secondary_selection_t *collection_selections = CR_MALLOC(sizeof(secondary_selection_t));
    collection_selections->buttons = collection_cards;
    collection->primary_selections[3].secondary_selections = collection_selections;

    return collection;
}

void continue_from_results(void *args) {
    screen_t *screen = (screen_t *)args;
    // Return to main menu from battle results
    screen->active = false;
    screen_t *main_screen = screen;
    while (main_screen != NULL && strcmp(main_screen->name, "main") != 0) {
        main_screen = main_screen->next;
    }
    if (main_screen != NULL) {
        main_screen->active = true;
        main_screen->selection_index = 0;
    }
}

screen_t *init_battle_results_screen(data_t *data) {
    const int NUM_PRIMARY_SELECTIONS = 2;
    const int NUM_REWARD_CARDS = 6;

    screen_t *results = create_screen("battle_results", NUM_PRIMARY_SELECTIONS, data);
    button_t *buttons = CR_MALLOC(sizeof(button_t) * (NUM_REWARD_CARDS + 2));

    // Initialize king/character icon area
    setup_button(&buttons[0], false, 20, 50, 80, 100, NULL, NULL, 100, 120, NULL, NULL, 2, 255, 255, false, false, false, 0, 0);

    // Initialize continue button
    setup_button(&buttons[1], false, 120, 200, 80, 30, continue_from_results, results, 83, 116, NULL, NULL, 2, 255, 255, false, false, false, 0, 0);

    // Initialize reward cards (2x3 grid)
    button_t *reward_cards = CR_MALLOC(sizeof(button_t) * NUM_REWARD_CARDS);
    for (int i = 0; i < NUM_REWARD_CARDS; i++) {
        int col = i % 2;  // 2 columns
        int row = i / 2;  // 3 rows
        int x = 130 + col * 70;
        int y = 30 + row * 60;

        setup_button(&reward_cards[i], false, x, y, 60, 50, NULL, NULL, 100, 171, NULL, NULL, 2, 255, 171, false, false, false, 0, 0);
    }

    // Set up primary selections
    results->primary_selections[0].button = &buttons[0];  // King icon area
    results->primary_selections[0].num_secondary_selections = NUM_REWARD_CARDS;
    results->primary_selections[0].secondary_selection_index = -1;

    results->primary_selections[1].button = &buttons[1];  // Continue button
    results->primary_selections[1].num_secondary_selections = 0;
    results->primary_selections[1].secondary_selection_index = -1;

    // Set up secondary selections for reward cards
    secondary_selection_t *reward_selections = CR_MALLOC(sizeof(secondary_selection_t));
    reward_selections->buttons = reward_cards;
    results->primary_selections[0].secondary_selections = reward_selections;

    return results;
}

screen_t *init_screens(void) {
    const unsigned int num_selections = 4;
    const unsigned int num_buttons = 5;
    const unsigned int num_secondary_selections = 4;

    screen_t *main = create_screen("main", num_selections, &data);
    main->active = true;  // Main screen starts active
    button_t *buttons = CR_MALLOC(sizeof(button_t) * num_buttons);
    
    // Initialize battle tab (active, bottom)
    setup_tab_button(&buttons[0], 0, 120, 60, 120, battle_icon, true);

    // Initialize deck tab (inactive, top)
    setup_tab_button(&buttons[4], 0, 0, 60, 120, deck_icon, false);

    // Chest slots area
    init_button(&buttons[1], false, 75, 0, 50, 240, NULL, NULL);
    customize_border(&buttons[1], 2, 255, 255);
    
    // Start game button
    init_button(&buttons[2], false, 160, 70, battle->width, battle->height, &start_game, main);
    customize_border(&buttons[2], 10, 255, 255);

    // Exit button
    init_button(&buttons[3], false, 280, 205, exit_button->width, exit_button->height, NULL, NULL);
    customize_border(&buttons[3], 2, 255, 255);

    // XP/Level indicator (new UI element from screenshot)
    init_button(&buttons[4], false, 280, 20, 30, 15, NULL, NULL);
    customize_button(&buttons[4], 83, 83, NULL, NULL);

    // Assign buttons to primary selections
    main->primary_selections[0].button = &buttons[0];  // Battle tab
    main->primary_selections[1].button = &buttons[1];  // Chest slots
    main->primary_selections[2].button = &buttons[2];  // Start game
    main->primary_selections[3].button = &buttons[3];  // Exit

    // Initialize secondary selections
    for (int i = 0; i < num_selections; i++) {
        main->primary_selections[i].secondary_selection_index = -1;
        main->primary_selections[i].num_secondary_selections = 0;
    }

    // Set up chest slots secondary selections
    main->primary_selections[1].num_secondary_selections = num_secondary_selections;
    secondary_selection_t *secondary_selections = CR_MALLOC(sizeof(secondary_selection_t));
    button_t *chest_slots = CR_MALLOC(sizeof(button_t) * num_secondary_selections);
    secondary_selections->buttons = chest_slots;

    init_chest_slots(main, chest_slots);
    main->primary_selections[1].secondary_selections = secondary_selections;

    // Set up linked screens
    screen_t *deck = init_deck_screen(&data);
    screen_t *collection = init_collection_screen(&data);
    screen_t *battle_results = init_battle_results_screen(&data);

    main->next = deck;
    deck->next = collection;
    collection->next = battle_results;

    // Initialize chest opening screen
    screen_t *chest_opening = create_screen("chest_opening", 1, &data);
    button_t *chest_button = CR_MALLOC(sizeof(button_t));
    init_button(chest_button, false, 100, 100, 100, 20, continue_chest_opening, NULL);
    chest_opening->primary_selections[0].button = chest_button;
    battle_results->next = chest_opening;

    return main;
}

void free_all_screens(screen_t *screens) {
    screen_t *current = screens;
    while (current != NULL) {
        screen_t *next = (screen_t*)current->next;
        
        // Free primary selections and their buttons
        if (current->primary_selections != NULL) {
            for (unsigned int i = 0; i < current->num_primary_selections; i++) {
                if (current->primary_selections[i].button != NULL) {
                    CR_FREE(current->primary_selections[i].button);
                }
                if (current->primary_selections[i].secondary_selections != NULL) {
                    for (int j = 0; j < current->primary_selections[i].num_secondary_selections; j++) {
                        if (current->primary_selections[i].secondary_selections[j].buttons != NULL) {
                            CR_FREE(current->primary_selections[i].secondary_selections[j].buttons);
                        }
                    }
                    CR_FREE(current->primary_selections[i].secondary_selections);
                }
            }
            CR_FREE(current->primary_selections);
        }
        
        if (current->name != NULL) {
            CR_FREE(current->name);
        }
        
        CR_FREE(current);
        current = next;
    }
}

void free_ui(void) {
    // Additional UI cleanup if needed
}

void draw_icon(icon_t icon) {
    if (icon.sprite == NULL) return;

    if (icon.transparent) {
        gfx_TransparentSprite(icon.sprite, icon.x, icon.y);
    } else {
        gfx_Sprite(icon.sprite, icon.x, icon.y);
    }
}

void draw_button_border(button_t *button, bool selected) {
    (void)selected; // Suppress unused parameter warning
    if (button->border.weight <= 0) return;

    gfx_SetColor(button->border.color);
    gfx_FillRectangle(button->x, button->y, button->width, button->border.weight);
    gfx_FillRectangle(button->x, (button->y + button->height) - button->border.weight, button->width, button->border.weight);
    gfx_FillRectangle(button->x, button->y, button->border.weight, button->height);
    gfx_FillRectangle(button->x + button->width - button->border.weight, button->y, button->border.weight, button->height);
}

void draw_button(button_t *button, bool selected) {
    gfx_SetColor(button->color);
    gfx_FillRectangle(button->x, button->y, button->width, button->height);

    if (button->active || selected) {
        draw_button_border(button, selected);
    }

    if (button->has_icon) {
        draw_icon(button->icon);
    }
}

void draw_inactive_tab(screen_t *current_screen, screen_t *other_screen, const char *current_name) {
    button_t *inactive_tab = NULL;

    if (strcmp(current_name, "main") == 0) {
        inactive_tab = other_screen->primary_selections[0].button;
    } else if (strcmp(current_name, "deck") == 0) {
        inactive_tab = current_screen->primary_selections[1].button;
    }

    if (inactive_tab != NULL) {
        draw_button(inactive_tab, false);
    }
}

void draw_elixir_overlay(screen_t *screen) {
    if (screen == NULL || screen->data == NULL || screen->data->deck == NULL) return;

    // Get the selected card index from the card grid (primary_selections[2])
    int selected_card_index = screen->primary_selections[2].secondary_selection_index;

    // Only show elixir if a card is selected
    if (selected_card_index >= 0 && selected_card_index < 8) {
        card_t *selected_card = &screen->data->deck[selected_card_index];
        int elixir_cost = selected_card->elixir;

        // Draw elixir bar background (purple/pink)
        gfx_SetColor(55);  // Dark purple background
        gfx_FillRectangle(175, 175, 100, 20);

        // Draw filled portion based on elixir cost (max 10 elixir)
        int filled_width = (elixir_cost * 100) / 10;
        gfx_SetColor(202);  // Bright purple/pink for elixir
        gfx_FillRectangle(175, 175, filled_width, 20);

        // Draw border
        gfx_SetColor(0);
        gfx_Rectangle(175, 175, 100, 20);

        // Draw elixir cost number
        char elixir_str[4];
        sprintf(elixir_str, "%d", elixir_cost);
        gfx_SetTextFGColor(255);  // White text
        gfx_PrintStringXY(elixir_str, 220, 178);
    } else {
        // No card selected - draw empty bar
        gfx_SetColor(55);
        gfx_FillRectangle(175, 175, 100, 20);
        gfx_SetColor(0);
        gfx_Rectangle(175, 175, 100, 20);
    }
}

void draw_screen(screen_t *screen) {
    if (screen == NULL) return;

    // Draw inactive tabs and screen-specific elements
    if (screen->next != NULL) {
        draw_inactive_tab(screen, screen->next, screen->name);
    }

    if (strcmp(screen->name, "deck") == 0) {
        gfx_Sprite(deck_text, 280, 15);
        draw_elixir_overlay(screen);
    } else if (strcmp(screen->name, "collection") == 0) {
        // Draw "DECK" header text
        gfx_SetColor(255);
        gfx_PrintStringXY("DECK", 20, 10);

        // Draw "COLLECTION" text at bottom
        gfx_PrintStringXY("COLLECTION", 20, 195);

        // Draw "USE" button text
        gfx_PrintStringXY("USE", 105, 175);

        // Draw selected card name placeholder
        gfx_PrintStringXY("<card name>", 85, 175);
    } else if (strcmp(screen->name, "battle_results") == 0) {
        // Draw results screen elements
        gfx_SetColor(255);
        gfx_PrintStringXY("VICTORY!", 120, 10);
        gfx_PrintStringXY("Continue", 130, 205);
    }

    // Draw screen-specific elements
    if (strcmp(screen->name, "main") == 0) {
        // Draw XP/Level indicator
        gfx_SetColor(255);
        gfx_PrintStringXY("15/40", 280, 25);
    }

    // Draw active screen buttons
    if (!screen->active) return;

    for (int i = 0; i < screen->num_primary_selections; i++) {
        primary_selection_t *primary = &screen->primary_selections[i];

        if (primary->num_secondary_selections == 0) {
            // Primary button without secondary selections
            draw_button(primary->button, false);
        } else {
            // Secondary buttons
            for (int j = 0; j < primary->num_secondary_selections; j++) {
                button_t *button = &primary->secondary_selections->buttons[j];
                bool selected = (j == primary->secondary_selection_index);
                draw_button(button, selected);
            }
        }
    }
}

screen_t *get_active_screen(screen_t *screens) {
    screen_t *screen = screens;

    // Find the first active screen
    while (screen != NULL) {
        if (screen->active == true) {
            return screen;  // Return the active screen
        }
        screen = screen->next;
    }

    return screen;  // No active screen found
}

bool handle_tab_navigation(screen_t *screens) {
    screen_t *screen = get_active_screen(screens);
    if (screen == NULL) return false;

    // Only handle tab navigation when on the tab button (selection_index == 0)
    if (screen->selection_index != 0) return false;

    bool input_handled = false;

    if (kb_Data[7] & kb_Up) {
        if (strcmp(screen->name, "main") == 0) {
            screen_t *deck_screen = (screen_t *)screen->next;
            if (deck_screen != NULL) {
                // Switch to deck screen when on battle tab
                screen->active = false;
                deck_screen->active = true;
                deck_screen->selection_index = 0;
                deck_screen->primary_selections[0].button->active = true;
                delay(screen->delay);
                input_handled = true;
            }
        }
    }
    
    if (kb_Data[7] & kb_Down) {
        if (strcmp(screen->name, "deck") == 0) {
            // Make sure we deactivate current button
            screen->primary_selections[0].button->active = false;
            screen->active = false;
            
            // Activate main screen (which is the head of the list)
            screens->active = true;
            screens->selection_index = 0;
            screens->primary_selections[0].button->active = true;
            delay(screen->delay);
            input_handled = true;
        }
    }

    return input_handled;
}

void handle_screens(screen_t *screens) {
    // Get the current active screen
    screen_t *screen = get_active_screen(screens);
    
    // Ensure there is an active screen
    if (screen == NULL) {
        return;  // Exit if no active screen is found
    }
    
    // Skip rest of input handling if tab navigation occurred
    if (handle_tab_navigation(screens) == true) {   
        return;  
    }

    // Handle right button press to navigate the selections
    if (kb_Data[7] & kb_Right) {
        // Ensure we are not out of bounds
        if (screen->selection_index < screen->num_primary_selections - 1) {

            // Deactivate current button
            int current_index = screen->selection_index;
            if (screen->primary_selections[current_index].button->tab == false) {
                screen->primary_selections[current_index].button->active = false;
            }

            if (screen->primary_selections[current_index].num_secondary_selections > 0) {
                screen->primary_selections[current_index].secondary_selection_index = -1;
            }

            // Move to the next selection
            screen->selection_index++;

            // Activate the next button
            current_index = screen->selection_index;
            // if (screen->primary_selections[current_index].button != NULL) 
            screen->primary_selections[current_index].button->active = true;

             if (screen->primary_selections[current_index].num_secondary_selections > 0) {
                screen->primary_selections[current_index].secondary_selection_index = 0;
            }
            delay(screen->delay);
        }
    }

    // Handle left button press to navigate the selections
    if (kb_Data[7] & kb_Left) { 
        // Ensure we are not going below the first selection
        if (screen->selection_index > 0) {

            // Deactivate current button
            int current_index = screen->selection_index;
            if (screen->primary_selections[current_index].button->tab == false) {
                screen->primary_selections[current_index].button->active = false;
            }

              // Set the previous secondary selection index to -1 if it has secondary selections
            if (screen->primary_selections[current_index].num_secondary_selections > 0) {
                screen->primary_selections[current_index].secondary_selection_index = -1;
            }

            // Move to the previous selection
            screen->selection_index--;

            // Activate the previous button
            current_index = screen->selection_index;
            screen->primary_selections[current_index].button->active = true;

            
            if (screen->primary_selections[current_index].num_secondary_selections > 0) {
                screen->primary_selections[current_index].secondary_selection_index = 0;
            }


            delay(screen->delay);
        }
    }

    // Handle up button press to navigate secondary selections
    if (kb_Data[7] & kb_Up) { 
        // Get the current primary selection
        int primary_index = screen->selection_index;
        primary_selection_t *primary = &screen->primary_selections[primary_index];

        // Check if there are secondary selections
        if (primary->num_secondary_selections > 0 && screen->primary_selections[screen->selection_index].secondary_selection_index > 0) {
            // Move to previous secondary selection
            screen->primary_selections[screen->selection_index].secondary_selection_index--;
            delay(screen->delay);

            // Move to the previous secondary selection if possible
            // if (current_secondary_index > 0) {
            //     // Deactivate the current secondary button
            //     primary->secondary_selections[current_secondary_index].buttons->active = false;

            //     // Move to the previous secondary selection
            //     current_secondary_index--;

            //     primary->secondary_selection_index = current_secondary_index;
            //     // Activate the previous secondary button
            //     primary->secondary_selections[current_secondary_index].buttons->active = true;
            //     delay(screen->delay);
            // }
        }
    }

    // Handle down button press to navigate secondary selections
    if (kb_Data[7] & kb_Down) { 
        // Get the current primary selection
        int primary_index = screen->selection_index;
        primary_selection_t *primary = &screen->primary_selections[primary_index];

        // Check if there are secondary selections
        if (primary->num_secondary_selections > 0) {
            // Get the current secondary selection index

            if (primary->num_secondary_selections > 0 && screen->primary_selections[screen->selection_index].secondary_selection_index < primary->num_secondary_selections - 1) {
                // Move to next secondary selection
                screen->primary_selections[screen->selection_index].secondary_selection_index++;
                delay(screen->delay);
            }
        }
    }

    if (kb_Data[6] & kb_Enter) {
        int current_index = screen->selection_index;
        button_t *button = screen->primary_selections[current_index].button;
        if (button->function != NULL) {
            button->function(button->args);
        }
    }
    // if action key
}

// sprite, card type, rarity, can attack troops, can target air/ground, elixir
void init_card(card_t *card, gfx_sprite_t *sprite, card_type_t type, rarity_t rarity, movement_type_t movement_type, target_type_t target_type, bool target_troops, int elixir) {
    card->sprite = sprite;
    card->type = type;
    card->rarity = rarity;
    if (type == TROOP) {
        card->movement = movement_type;
        card->TARGET_TROOPS = target_troops;
    }
        
    if (type == TROOP || type == BUILDING) card->target = target_type;

    card->elixir = elixir;
}

void init_troop(card_t *troop, 
                gfx_sprite_t **forward_movement, 
                gfx_sprite_t **backward_movement, 
                gfx_sprite_t **attack_cycle, 
                gfx_sprite_t **attack_cycle_rev, 
                unsigned int movement_steps,
                unsigned int attack_steps) 
{
    troop->forward_movement = forward_movement;
    troop->backward_movement = backward_movement;
    troop->attack_cycle = attack_cycle;
    troop->attack_cycle_rev = attack_cycle_rev;

    troop->movement_steps = movement_steps;

    // Initialize new fields
    troop->movement_ticks = 0;
    troop->movement_update_rate = 10; // Adjust this value to control movement speed
    troop->movement_frame = 0;

    troop->attack_ticks = 0;
    troop->attack_frame = 0;
    troop->attack_steps = attack_steps;
}

// // card, sprite, radius, duration, elixir generated
void init_building(card_t *card, gfx_sprite_t *sprite, double radius, int duration, int elixir_generated) {
    card->duration = duration;
    card->elixir_generated = elixir_generated;
}

void init_spell(card_t *card, double radius, int duration, int color, int damage, int attack_ticks) {
    card->radius = radius;
    card->duration = duration;
    card->color = color;
    card->damage = damage;
    card->attack_ticks = attack_ticks;
}

// MEMORY OPTIMIZATION: Shuffle indices instead of copying entire card structures
void shuffle_card_indices(int *indices, int size) {
    for (int i = 0; i < size; i++) {
        int j = rand() % size;
        
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
}


void init_cursor(card_t *card, gfx_sprite_t *cursor) {
    card->cursor.sprite = cursor;
}

void init_projectile(card_t *card, gfx_sprite_t *sprite, unsigned int damage, unsigned int speed) {
    card->projectile_sprite = sprite;
    card->projectile_speed = speed;
    // damage is now stored directly in card->damage
    (void)damage;
}

// Attack type, speed, damage, range (tiles)
void set_attack_vars(card_t *card, attack_type_t attack_type, unsigned int attack_speed, unsigned int damage, double range) {
    card->attack_type = attack_type;
    card->attack_speed = attack_speed;
    card->damage = damage;
    card->radius = range;
}

void set_health(card_t *card, unsigned int health) {
    card->health = health;
}

void init_deck(data_t *data) {
    const int SIZE_OF_DECK = 8;
    unsigned int movement_steps = 0;

    // MEMORY OPTIMIZATION: Only allocate master card array - no more duplicates!
    card_t *available_cards = CR_MALLOC(sizeof(card_t) * TOTAL_CARDS);

    // REMOVED: Duplicate card arrays that wasted ~3.2KB of memory
    // card_t *starter_cards = CR_MALLOC(sizeof(card_t) * SIZE_OF_DECK);
    // card_t *deck = CR_MALLOC(sizeof(card_t) * SIZE_OF_DECK);

    init_card(&available_cards[0], miner_card, TROOP, LEGENDARY, GROUND_MOVEMENT, GROUND, true, 3);
    init_card(&available_cards[1], musketeer_card, TROOP, RARE, GROUND_MOVEMENT, GROUND, true, 4);
    init_card(&available_cards[2], balloon_card, TROOP, EPIC, AIR_MOVEMENT, GROUND, false, 5);
    init_card(&available_cards[3], giant_card, TROOP, LEGENDARY, GROUND_MOVEMENT, GROUND, true, 5);
    init_card(&available_cards[4], zap_card, SPELL, LEGENDARY, GROUND_MOVEMENT, GROUND, true, 2);
    init_card(&available_cards[5], poison_card, SPELL, LEGENDARY, STATIONARY, ALL, true, 4);
    init_card(&available_cards[6], elixir_collector_card, BUILDING, RARE, STATIONARY, ALL, false, 6);
    init_card(&available_cards[7], knight_card, TROOP, COMMON, GROUND_MOVEMENT, GROUND, true, 3);

    init_cursor(&available_cards[0], miner_stepL);
    init_cursor(&available_cards[1], musketeer_stepL);
    init_cursor(&available_cards[2], balloon);
    init_cursor(&available_cards[3], g_stepN);
    init_cursor(&available_cards[4], NULL);
    init_cursor(&available_cards[5], NULL);
    init_cursor(&available_cards[6], elixir_collector);
    init_cursor(&available_cards[7], knight_stepN);

    // init_projectile(card, sprite, damage, speed)
    init_projectile(&available_cards[1], bullet, 50, 2);

    // memcpy(attack_cycle, miner_attack_cycle_rev, sizeof(miner_attack_cycle_rev));

    gfx_sprite_t **miner_movement = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    miner_movement[0] = miner_stepL;
    miner_movement[1] = miner_stepR;

    gfx_sprite_t **miner_movement_rev = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    miner_movement_rev[0] = miner_stepL_opp;
    miner_movement_rev[1] = miner_stepR_opp;

    gfx_sprite_t **miner_attack_cycle = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    miner_attack_cycle[0] = miner_stepL;
    miner_attack_cycle[1] = miner_attack;

    gfx_sprite_t **miner_attack_cycle_rev = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    miner_attack_cycle_rev[0] = miner_stepL_opp;
    miner_attack_cycle_rev[1] = miner_attack_opp;

    // Giant movement with full 4 frames
    gfx_sprite_t **giant_movement = CR_MALLOC(4 * sizeof(gfx_sprite_t *));
    giant_movement[0] = g_stepN;
    giant_movement[1] = g_stepL;
    giant_movement[2] = g_stepN;
    giant_movement[3] = g_stepR;

    gfx_sprite_t **giant_movement_rev = CR_MALLOC(4 * sizeof(gfx_sprite_t *));
    giant_movement_rev[0] = g_stepN_opp;
    giant_movement_rev[1] = g_stepL_opp;
    giant_movement_rev[2] = g_stepN_opp;
    giant_movement_rev[3] = g_stepR_opp;

    gfx_sprite_t **giant_attack_cycle = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    giant_attack_cycle[0] = g_attack;
    giant_attack_cycle[1] = g_stepN;

    gfx_sprite_t **giant_attack_cycle_rev = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    giant_attack_cycle_rev[0] = g_attack_opp;
    giant_attack_cycle_rev[1] = g_stepN_opp;

    gfx_sprite_t **balloon_movement = CR_MALLOC(sizeof(gfx_sprite_t *));
    balloon_movement[0] = balloon;

    gfx_sprite_t **balloon_attack = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    balloon_attack[0] = balloon;
    balloon_attack[1] = balloon;

    gfx_sprite_t **balloon_movement_rev = CR_MALLOC(sizeof(gfx_sprite_t *));
    balloon_movement_rev[0] = balloon;

    gfx_sprite_t **balloon_attack_rev = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    balloon_attack_rev[0] = balloon;
    balloon_attack_rev[1] = balloon;

    // Knight movement with full 4 frames
    gfx_sprite_t **knight_movement = CR_MALLOC(4 * sizeof(gfx_sprite_t *));
    knight_movement[0] = knight_stepN;
    knight_movement[1] = knight_stepL;
    knight_movement[2] = knight_stepN;
    knight_movement[3] = knight_stepR;

    gfx_sprite_t **knight_movement_rev = CR_MALLOC(4 * sizeof(gfx_sprite_t *));
    knight_movement_rev[0] = knight_stepN_opp;
    knight_movement_rev[1] = knight_stepL_opp;
    knight_movement_rev[2] = knight_stepN_opp;
    knight_movement_rev[3] = knight_stepR_opp;

    gfx_sprite_t **knight_attack_cycle = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    knight_attack_cycle[0] = knight_attack;
    knight_attack_cycle[1] = knight_stepN;

    gfx_sprite_t **knight_attack_cycle_rev = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    knight_attack_cycle_rev[0] = knight_attack_opp;
    knight_attack_cycle_rev[1] = knight_stepN_opp;

    // Musketeer movement with full 4 frames
    gfx_sprite_t **musketeer_movement = CR_MALLOC(4 * sizeof(gfx_sprite_t *));
    musketeer_movement[0] = musketeer_stepN;
    musketeer_movement[1] = musketeer_stepL;
    musketeer_movement[2] = musketeer_stepN;
    musketeer_movement[3] = musketeer_stepR;

    gfx_sprite_t **musketeer_movement_rev = CR_MALLOC(4 * sizeof(gfx_sprite_t *));
    musketeer_movement_rev[0] = musketeer_stepL_opp;
    musketeer_movement_rev[1] = musketeer_stepN;
    musketeer_movement_rev[2] = musketeer_stepR_opp;
    musketeer_movement_rev[3] = musketeer_stepN;

    gfx_sprite_t **musketeer_attack_cycle = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    musketeer_attack_cycle[0] = musketeer_attack;
    musketeer_attack_cycle[1] = musketeer_stepN;

    gfx_sprite_t **musketeer_attack_cycle_rev = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    musketeer_attack_cycle_rev[0] = musketeer_attack_opp;
    musketeer_attack_cycle_rev[1] = musketeer_stepL_opp;

    // Initialize troops
    init_troop(&available_cards[0], miner_movement, miner_movement_rev, miner_attack_cycle, miner_attack_cycle_rev, 2, 2);
    init_troop(&available_cards[1], musketeer_movement, musketeer_movement_rev, musketeer_attack_cycle, musketeer_attack_cycle_rev, 4, 2);
    init_troop(&available_cards[2], balloon_movement, balloon_movement_rev, balloon_attack, balloon_attack_rev, 1, 1);
    init_troop(&available_cards[3], giant_movement, giant_movement_rev, giant_attack_cycle, giant_attack_cycle_rev, 4, 2);
    init_troop(&available_cards[7], knight_movement, knight_movement_rev, knight_attack_cycle, knight_attack_cycle_rev, 4, 2);

    // Attack type, speed, damage, range (tiles)
    set_attack_vars(&available_cards[0], MELEE, 1, 50, 1);
    set_attack_vars(&available_cards[1], RANGED, 1, 50, 5);
    set_attack_vars(&available_cards[2], MELEE, 1, 100, 0.5);
    set_attack_vars(&available_cards[2], MELEE, 1, 100, 1);
    set_attack_vars(&available_cards[7], MELEE, 1, 25, 0.5);

    // set_attack_speed;
    // set_damage()

    double radius = 2;
    unsigned int duration = 10;
    unsigned int color = 150;

    // damage at the end
    // need tick speed to distribute ticks
    init_spell(&available_cards[4], 0.5, duration, color, 50, 1);
    init_spell(&available_cards[5], 10, duration, color, 100, 4);


    unsigned int speed = 5;
    unsigned int elixir_generated = 2;
    unsigned int health = 1000;

    // should set cursor sprite be its own function
    // card, sprite, radius, duration, elixir generated
    // init_building(card_t *card, gfx_sprite_t *sprite, double radius, int duration, int elixir_generated) {

    init_building(&available_cards[6], elixir_collector, 0, 10, 1);
    // init_spell()
    // init_cursor(&available_cards[0])

    // if null assume radius
    // assign_cards(available_cards, starter_cards, TOTAL_CARDS, SIZE_OF_DECK);

    set_health(&available_cards[0], 500);
    set_health(&available_cards[1], 400);
    set_health(&available_cards[2], 750);
    set_health(&available_cards[3], 1000);
    set_health(&available_cards[6], 1000);
    set_health(&available_cards[7], 400);

    // MEMORY OPTIMIZATION: Only store master card array
    data->available_cards = available_cards;
    data->unlocked_cards = NULL;  // REMOVED: No longer needed - save memory
    data->deck = available_cards;  // Deck now points to same array
}

void cleanup_and_exit(screen_t *screens) {
    save_data();
    free_all_screens(screens);
    free_ui();
}

// Persistent wrapper around menus and game start functions
void start_menu(void) {
    bool exit = false;
    load_data();
    
    init_deck(&data);
    screen_t *screens = init_screens();
    
    gfx_SetDrawBuffer();
    
    while (exit == false) {
        kb_Scan();
        
        // Get current active screen for this frame
        screen_t *active = get_active_screen(screens);
        
        // Pass screens (head of list) instead of active screen
        handle_screens(screens);
        gfx_FillScreen(80);
        
        draw_screen(active);
        
        gfx_BlitBuffer();

        if (kb_Data[6] & kb_Clear) {
            save_data();
            free_all_screens(screens);
            free_data(&data);  // Add this
            free_ui();
            exit = true;
        }

    }

    // cleanup_and_exit(screen_t *screens);
}