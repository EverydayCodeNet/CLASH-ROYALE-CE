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

void init_button(button_t *button, bool tab, int x, int y, int width, int height, void *function, void *args) {
    button->x = x;
    button->y = y;
    button->width = width;
    button->height = height;
    button->color = 100;
    button->tab = tab;

    if (tab == true) {
        button->active = true;
    } else {
        button->active = false;
    }
   
    if (function != NULL) {
        button->event = ACTION;
        button->function = function;
        button->args = args;
    } else {
        button->event = NONE;
        button->function = NULL;
        button->args = NULL;
    }

    // Initialize border
    button->border.weight = 0;
    button->border.color = 0;
    button->active_color = 0;
}

// Function for changing button color attributes
void customize_button(button_t *button, int color, int active_color, gfx_sprite_t *sprite, gfx_sprite_t *icon) {
    button->color = color;
    button->active_color = active_color;
    if (sprite != NULL) button->sprite = sprite;
    if (icon != NULL) button->icon.sprite = icon;
    if (icon != NULL) button->has_icon = true;
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

void init_screen() {

}

void continue_chest_opening(void) {

}

screen_t *get_screen_by_name(screen_t *screen, char *screen_name) {
    screen_t *temp = screen;
    while (temp != NULL && strcmp(temp->name, screen_name) != 0) {
        temp = temp->next;
    }

    return temp;
}

void open_chest(screen_t *screen, chest_t *chest) {
    int num_primary_selections = 1;
    screen->active = false;

    screen_t *chest_opening = get_screen_by_name(screen, "chest_opening");
    chest_opening->active = true;

    // screen->next = init_screen(num_primary_selections, 150, data);
    // screen->next = 

    chest->status = UNLOCKING;
    chest->unlock_step = 0;
}

// This function is called via key press (enter), validate that the chest slot is empty or available to open
void handle_chests(screen_t *screen) {
    chest_t *chests = screen->data->chests;
    if (chests == NULL) return;
    // Get the 

    int primary_selection_index = screen->selection_index;
    int index = screen->primary_selections[primary_selection_index].secondary_selection_index;
    chest_t *chest = &chests[index];
    chest_status_t status = chest->status;
    if (status == LOCKED) {
        if (can_unlock(chests) == true) {
            // Replace this function with the one expression?
            unlock_chest(chest);
        }
    } else if (status == OPEN) {
        open_chest(screen, chest);
    }
        
}

// Return the left and right coords necessary to draw rectangles centered on each other
point_t find_center(int x, int y, int icon_x, int icon_y, int width, int height, int icon_width, int icon_height) {
    point_t point;
    point.x = x + (width - icon_width) / 2;
    point.y = y + (height - icon_height) / 2;

    return point;
}

void customize_icon(button_t *button, bool transparent, bool center_x, bool center_y, int x_offset, int y_offset) {
    button->icon.transparent = transparent;
    point_t center = find_center(button->x, button->y, button->x, button->y, button->width, button->height, button->icon.sprite->width, button->icon.sprite->height);

    if (center_x == true) {
        button->icon.x = center.x;
    } else {
        button->icon.x = button->x + x_offset;
    }

    if (center_y == true) {
        button->icon.y = center.y;
    } else {
        button->icon.y = button->y + y_offset;
    }

    if (transparent == true) button->icon.transparent = true;

}

// Set button x, y using for loops
void init_chest_slots(screen_t *screen, button_t *slots) {
    // maybe size down these slots to accomodate for chests
    const int SLOT_HEIGHT = 45;
    const int SLOT_WIDTH = 60;
    const int NUM_CHEST_SLOTS = 4;
    int y;

    gfx_SetColor(255);
    int NUM_CHESTS = 0;
    int rarity = 0;
    /*int remainingChestSlots = getAvailableChestSlots(chests);
    if (remainingChestSlots == -1) NUM_CHESTS = 4;
    else NUM_CHESTS = 4 - remainingChestSlots;*/

    gfx_sprite_t *chest_sprites[3] = {silver_chest, gold_chest, magical_chest};
    // for (int i = 0; i < NUM_CHEST_SLOTS; i++) {
    //     if (slots[i] != NULL) {
    //         NUM_CHESTS++;
    //         } else {
    //             // what about in the case where the chests are opened?
    //         break; // Exit the loop when a NULL pointer is encountered
    //         }
    //     }
    // chest slots
    for (int i = 0; i < NUM_CHEST_SLOTS; i++) {
        // make it green if the chest is unlocked with an increasing aura effect
        // make it yellow on hover
        y = 15 + (i * (10 + SLOT_HEIGHT));
        init_button(&slots[i], false, 75 , y, SLOT_WIDTH, SLOT_HEIGHT, handle_chests, screen); 
        // button_t *button, int weight, int color, int active_color
        customize_border(&slots[i], 1, 255, 171);
        gfx_sprite_t *icon = NULL;
        // icon, center_x, center_y,  
        // if (slots[i]->status != EMPTY) {
        //     icon = slots[i].sprite;
        // }
        chest_t *chest;
        if (screen->data->chests != NULL) {
            chest = &screen->data->chests[i];
            if (chest->status != EMPTY) icon = chest->sprite;
        } 

        // button_t *button, int color, int active_color, gfx_sprite_t *sprite, gfx_sprite_t *icon
        // customize_button(&slots[i], 100, 100, NULL, icon);
        // customize_icon(&slots[i], true, true, false, 0, 10);

        // customize border
    }

    // loop over array of chests
}

screen_t *init_deck_screen(data_t *data) {
    screen_t *deck = CR_MALLOC(sizeof(screen_t));
    
    // Set screen name
    char *deck_name = "deck";
    deck->name = CR_MALLOC(sizeof(char) * strlen(deck_name) + 1);
    strcpy(deck->name, deck_name);

    const int NUM_PRIMARY_SELECTIONS = 4;  // One for deck tab, one for inactive battle tab, one for grid, one for elixir bar
    const int NUM_CARDS = 8;
    const int TOTAL_BUTTONS = NUM_CARDS + 5;  // Cards + 2 tabs + grid area + elixir bar + inactive battle tab

    // Initialize all buttons
    button_t *buttons = CR_MALLOC(sizeof(button_t) * TOTAL_BUTTONS);
    
    // Initialize deck tab (top tab)
    init_button(&buttons[0], true, 0, 0, 60, 120, NULL, NULL);
    customize_button(&buttons[0], 83, 116, NULL, deck_icon); 
    customize_border(&buttons[0], 2, 255, 255);
    customize_icon(&buttons[0], true, true, true, 0, 0);

    // Initialize battle tab (bottom tab) as an inactive, non-selectable button
    init_button(&buttons[1], false, 0, 120, 60, 120, NULL, NULL);
    customize_button(&buttons[1], 83, 83, NULL, battle_icon); 
    customize_border(&buttons[1], 2, 255, 255);
    customize_icon(&buttons[1], true, true, true, 0, 0);

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
        int x = 225;
        int y = 15 + i * 40;
        
        if (i > 3) {
            x = 175;
            y = 15 + (i - 4) * 40;
        }
        
        init_button(&card_buttons[i], false, x, y, 40, 30, NULL, NULL);
        
        // Set button colors based on card rarity
        int color = 100;
        int active_color = 100;
        // MEMORY OPTIMIZATION: data->deck now points to available_cards array
        if (data && data->deck) {
            switch(data->deck[i].rarity) {
                case COMMON:
                    active_color = 151;
                    break;
                case RARE:
                    active_color = 150;
                    break;
                case EPIC:
                    active_color = 202;
                    break;
                case LEGENDARY:
                    active_color = 171;
                    break;
            }
        }
        customize_button(&card_buttons[i], color, active_color, NULL, data->deck[i].sprite);
        customize_border(&card_buttons[i], 1, 255, active_color);
        customize_icon(&card_buttons[i], true, true, true, 0, 0);
    }

    // Set up primary selections
    deck->num_primary_selections = NUM_PRIMARY_SELECTIONS;
    deck->primary_selections = CR_MALLOC(sizeof(primary_selection_t) * NUM_PRIMARY_SELECTIONS);
    
    // Assign tabs button
    deck->primary_selections[0].button = &buttons[0];
    deck->primary_selections[0].num_secondary_selections = 0;
    deck->primary_selections[0].secondary_selection_index = -1;

    // Assign inactive battle tab
    deck->primary_selections[1].button = &buttons[1]; 
    deck->primary_selections[1].num_secondary_selections = 0;
    deck->primary_selections[1].secondary_selection_index = -1;

    // Assign card grid
    deck->primary_selections[2].button = &buttons[2];
    deck->primary_selections[2].num_secondary_selections = NUM_CARDS;
    deck->primary_selections[2].secondary_selection_index = -1;

    // Assign elixir bar
    deck->primary_selections[3].button = &buttons[3];
    deck->primary_selections[3].num_secondary_selections = 0;
    deck->primary_selections[3].secondary_selection_index = -1;

    // Set up secondary selections for card grid
    secondary_selection_t *card_selections = CR_MALLOC(sizeof(secondary_selection_t));
    card_selections->buttons = card_buttons;
    deck->primary_selections[2].secondary_selections = card_selections;
    
    // Initialize other deck screen properties
    deck->selection_index = 0;
    deck->active = false;
    deck->data = data;
    set_selection_delay(deck, 150);
    deck->next = NULL;

    return deck;
}

screen_t *init_screens(void) {
    // Main menu
    unsigned int num_selections = 4; // Active tab + 3 other selections
    unsigned int num_buttons = 5;    
    unsigned int num_secondary_selections = 4;
    char *name;

    // Allocate memory
    screen_t *main = CR_MALLOC(sizeof(screen_t));
    primary_selection_t *primary_selections = CR_MALLOC(sizeof(primary_selection_t) * num_selections);
    button_t *buttons = CR_MALLOC(sizeof(button_t) * num_buttons);

    name = "main";
    main->name = CR_MALLOC(sizeof(char) * strlen(name) + 1);
    strcpy(main->name, name);
    main->num_primary_selections = num_selections;
    
    // Initialize battle tab (active, bottom)
    init_button(&buttons[0], true, 0, 120, 60, 120, NULL, NULL);
    customize_button(&buttons[0], 83, 116, NULL, battle_icon);
    customize_border(&buttons[0], 2, 255, 255);
    customize_icon(&buttons[0], true, true, true, 0, 0);

    // Initialize deck tab (inactive, top) - set tab=true for display but don't include in selections
    init_button(&buttons[4], true, 0, 0, 60, 120, NULL, NULL);
    customize_button(&buttons[4], 83, 83, NULL, deck_icon);
    customize_border(&buttons[4], 2, 255, 255);
    customize_icon(&buttons[4], true, true, true, 0, 0);
    buttons[4].active = false;  // Explicitly set inactive even though it's a tab

    // Chest slots area
    init_button(&buttons[1], false, 75, 0, 50, 240, NULL, NULL);
    customize_border(&buttons[1], 2, 255, 255);
    
    // Start game button
    init_button(&buttons[2], false, 160, 70, battle->width, battle->height, &start_game, main);
    customize_border(&buttons[2], 10, 255, 255);

    // Exit button
    init_button(&buttons[3], false, 280, 205, exit_button->width, exit_button->height, NULL, NULL);
    customize_border(&buttons[3], 2, 255, 255);

    // Assign buttons to primary selections (deck tab not included)
    primary_selections[0].button = &buttons[0];  // Battle tab
    primary_selections[1].button = &buttons[1];  // Chest slots
    primary_selections[2].button = &buttons[2];  // Start game
    primary_selections[3].button = &buttons[3];  // Exit

    // Initialize secondary selections
    for (int i = 0; i < num_selections; i++) {
        primary_selections[i].secondary_selection_index = -1;
        primary_selections[i].num_secondary_selections = 0;
    }

    // Set up chest slots secondary selections
    primary_selections[1].num_secondary_selections = num_secondary_selections;
    secondary_selection_t *secondary_selections = CR_MALLOC(sizeof(secondary_selection_t));
    button_t *chest_slots = CR_MALLOC(sizeof(button_t) * num_secondary_selections);
    secondary_selections->buttons = chest_slots;
    
    // Initialize chest slots
    init_chest_slots(main, chest_slots);
    primary_selections[1].secondary_selections = secondary_selections;

    // Finish main screen setup
    main->primary_selections = primary_selections;
    main->selection_index = 0;
    main->active = true;
    main->data = &data;
    set_selection_delay(main, 150);

    // Set up linked screens
    screen_t *deck = init_deck_screen(&data);
    main->next = deck;

    // Initialize chest opening screen
    screen_t *chest_opening = CR_MALLOC(sizeof(screen_t));
    char *chest_name = "chest_opening";
    chest_opening->name = CR_MALLOC(sizeof(char) * strlen(chest_name) + 1);
    strcpy(chest_opening->name, chest_name);
    
    chest_opening->num_primary_selections = 1;
    chest_opening->primary_selections = CR_MALLOC(sizeof(primary_selection_t));
    
    button_t *chest_button = CR_MALLOC(sizeof(button_t));
    init_button(chest_button, false, 100, 100, 100, 20, continue_chest_opening, NULL);
    chest_opening->primary_selections[0].button = chest_button;
    
    chest_opening->active = false;
    chest_opening->next = NULL;
    deck->next = chest_opening;

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
    // Safety check for null sprite pointer
    if (icon.sprite == NULL) {
        return;
    }
    
    // Draw sprite with transparency flag
    if (icon.transparent) {
        gfx_TransparentSprite(icon.sprite, icon.x, icon.y);
    } else {
        gfx_Sprite(icon.sprite, icon.x, icon.y);
    }
}

void draw_elixir_overlay(screen_t *screen) {
    
}

void draw_screen(screen_t *screen) {
    screen_t *current_screen = screen;
    if (current_screen != NULL) {
        screen_t *other_screen = current_screen->next;
        if (strcmp(current_screen->name, "main") == 0) {
            // Draw inactive deck tab
            button_t *inactive_tab = &other_screen->primary_selections[0].button[0];
            gfx_SetColor(inactive_tab->color);
            gfx_FillRectangle(inactive_tab->x, inactive_tab->y, inactive_tab->width, inactive_tab->height);
            if (inactive_tab->has_icon) {
                draw_icon(inactive_tab->icon);
            }
        } else if (strcmp(current_screen->name, "deck") == 0) {
            // Draw inactive battle tab
            button_t *inactive_tab = &current_screen->primary_selections[1].button;
            gfx_SetColor(inactive_tab->color);
            gfx_FillRectangle(inactive_tab->x, inactive_tab->y, inactive_tab->width, inactive_tab->height);
            if (inactive_tab->has_icon) {
                draw_icon(inactive_tab->icon);
            }
            
            // Deck title
            gfx_Sprite(deck_text,280,15);

            // Draw elixir bar based on current card selected
            draw_elixir_overlay(current_screen->primary_selections);

        }
    }
    if (current_screen != NULL && current_screen->active == true) {
        for (int i = 0; i < current_screen->num_primary_selections; i++) {
            if (current_screen->primary_selections[i].num_secondary_selections == 0) {
                // Handle primary selections without secondary selections
                button_t *button = current_screen->primary_selections[i].button;
                gfx_SetColor(button->color);
                gfx_FillRectangle(button->x, button->y, button->width, button->height);
                
                if (button->border.weight > 0 && button->active == true) {
                    gfx_SetColor(button->border.color);
                    // Four rectangle fills (interior)
                    gfx_FillRectangle(button->x, button->y, button->width, button->border.weight);
                    gfx_FillRectangle(button->x, (button->y + button->height) - button->border.weight, button->width, button->border.weight);
                    // Verticals
                    gfx_FillRectangle(button->x, button->y, button->border.weight, button->height);
                    gfx_FillRectangle(button->x + button->width - button->border.weight, button->y, button->border.weight, button->height);
                }

                if (button->has_icon == true ) {
                    icon_t icon = button->icon;
                    draw_icon(icon);
                }
            } else {
                // Handle buttons with secondary selections
                for (int j = 0; j < current_screen->primary_selections[i].num_secondary_selections; j++) {
                    button_t *button = &current_screen->primary_selections[i].secondary_selections->buttons[j];
                    gfx_SetColor(button->color);
                    gfx_FillRectangle(button->x, button->y, button->width, button->height);
                    
                    // Draw border if selected
                    if (button->border.weight > 0 && j == current_screen->primary_selections[i].secondary_selection_index) {
                        gfx_SetColor(button->border.color);
                        // Four rectangle fills (interior)
                        gfx_FillRectangle(button->x, button->y, button->width, button->border.weight);
                        gfx_FillRectangle(button->x, (button->y + button->height) - button->border.weight, button->width, button->border.weight);
                        // Verticals
                        gfx_FillRectangle(button->x, button->y, button->border.weight, button->height);
                        gfx_FillRectangle(button->x + button->width - button->border.weight, button->y, button->border.weight, button->height);
                    }
                    
                    // Draw icon regardless of selection state
                    if (button->has_icon == true) {
                        icon_t icon = button->icon;
                        draw_icon(icon);
                    }
                }
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
            // Get the current secondary selection index
            int current_secondary_index = primary->secondary_selection_index;
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
                // Get the current secondary selection index
                int current_secondary_index = primary->secondary_selection_index;
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

// OLD: Shuffle deck in-place (COMMENTED OUT - keeping for reference)
/*
void shuffle_cards(card_t *cards, int size) {
    for (int i = 0; i < size; i++) {
        int j = rand() % size;

        card_t *temp = &cards[i];
        cards[i] = cards[j];
        cards[j] = *temp;
    }
}
*/

// COMMENTED OUT: assign_cards function no longer needed with index-based system
/*
// Randomly assign cards to the target deck
void assign_cards(card_t *template_deck, card_t *target_deck, int template_size, int target_size) {
    // Choose randomly from range of template deck size
    // Then assign it to each array value in target deck
    // Then shuffle deck

    // Array to mark which template_deck indices have been selected
    bool *selected = CR_CALLOC(template_size, sizeof(bool));
    
    int num_selected = 0;
    int temp_index;
    
    // Seed the random number generator
    // srand(time());
    
    // Assign random cards to the target deck
    while (num_selected < target_size) {
        // Generate a random index for the template_deck
        temp_index = rand() % template_size;

        // If the card at temp_index hasn't been selected yet, assign it to the target deck
        if (!selected[temp_index]) {
            target_deck[num_selected] = template_deck[temp_index];  // Copy the card
            selected[temp_index] = true; // Mark this index as selected
            num_selected++;
        }
    }

    CR_FREE(selected);

    shuffle_cards(target_deck, target_size);
}
*/

void init_cursor(card_t *card, gfx_sprite_t *cursor) {
    card->cursor.sprite = cursor;
}

void init_projectile(card_t *card, gfx_sprite_t *sprite, unsigned int damage, unsigned int speed) {
    card->projectile.sprite = sprite;
    card->projectile.damage = damage;
    card->projectile.speed = speed;
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

    init_card(&available_cards[0], miner_card, TROOP, LEGENDARY, GROUND_MOVEMENT, GROUND, true, 4);
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

    // MEMORY OPTIMIZATION: Reduced Giant movement from 4 to 2 frames (like Miner pattern)
    gfx_sprite_t **giant_movement = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    // giant_movement[0] = g_stepN;  // COMMENTED: Removed stepN to save memory
    giant_movement[0] = g_stepL;     // Now index 0: Left step
    // giant_movement[2] = g_stepN;  // COMMENTED: Was duplicate stepN frame
    giant_movement[1] = g_stepR;     // Now index 1: Right step

    gfx_sprite_t **giant_movement_rev = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    // giant_movement_rev[0] = g_stepN_opp;  // COMMENTED: Removed stepN to save memory
    giant_movement_rev[0] = g_stepL_opp;     // Now index 0: Left step (opponent)
    // giant_movement_rev[2] = g_stepN_opp;  // COMMENTED: Was duplicate stepN frame
    giant_movement_rev[1] = g_stepR_opp;     // Now index 1: Right step (opponent)

    gfx_sprite_t **giant_attack_cycle = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    giant_attack_cycle[0] = g_attack;
    giant_attack_cycle[1] = g_stepL;  // CHANGED: Use stepL instead of removed stepN

    gfx_sprite_t **giant_attack_cycle_rev = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    giant_attack_cycle_rev[0] = g_attack_opp;
    giant_attack_cycle_rev[1] = g_stepL_opp;  // CHANGED: Use stepL instead of removed stepN

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

    // MEMORY OPTIMIZATION: Reduced Knight movement from 4 to 2 frames (like Miner pattern)
    gfx_sprite_t **knight_movement = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    knight_movement[0] = knight_stepL;     // Now index 0: Left step
    // knight_movement[1] = knight_stepN;  // COMMENTED: Removed stepN to save memory  
    knight_movement[1] = knight_stepR;     // Now index 1: Right step
    // knight_movement[3] = knight_stepN;  // COMMENTED: Was duplicate stepN frame

    gfx_sprite_t **knight_movement_rev = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    knight_movement_rev[0] = knight_stepL_opp;     // Now index 0: Left step (opponent)
    // knight_movement_rev[1] = knight_stepN_opp;  // COMMENTED: Removed stepN to save memory
    knight_movement_rev[1] = knight_stepR_opp;     // Now index 1: Right step (opponent)
    // knight_movement_rev[3] = knight_stepN_opp;  // COMMENTED: Was duplicate stepN frame

    gfx_sprite_t **knight_attack_cycle = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    knight_attack_cycle[0] = knight_attack;
    knight_attack_cycle[1] = knight_stepL;  // CHANGED: Use stepL instead of removed stepN

    gfx_sprite_t **knight_attack_cycle_rev = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    knight_attack_cycle_rev[0] = knight_attack_opp;
    knight_attack_cycle_rev[1] = knight_stepL_opp;  // CHANGED: Use stepL instead of removed stepN

    // Musketeer movement and attack cycle
    gfx_sprite_t **musketeer_movement = CR_MALLOC(4 * sizeof(gfx_sprite_t *));
    musketeer_movement[0] = musketeer_stepL;
    musketeer_movement[1] = musketeer_stepN;
    musketeer_movement[2] = musketeer_stepR;
    musketeer_movement[3] = musketeer_stepN;

    gfx_sprite_t **musketeer_movement_rev = CR_MALLOC(2 * sizeof(gfx_sprite_t *));
    musketeer_movement_rev[0] = musketeer_stepL_opp;
    musketeer_movement_rev[1] = musketeer_stepR_opp;

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
    init_troop(&available_cards[3], giant_movement, giant_movement_rev, giant_attack_cycle, giant_attack_cycle_rev, 2, 2);  // CHANGED: 4->2 movement steps
    init_troop(&available_cards[7], knight_movement, knight_movement_rev, knight_attack_cycle, knight_attack_cycle_rev, 2, 2);  // CHANGED: 4->2 movement steps

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
        
        // Can still use active screen for drawing
        draw_screen(active);
        
        // Debug info using current active screen
        if (active != NULL) {
            gfx_PrintStringXY("Primary Selection Index: ", 10, 10);
            gfx_PrintInt(active->selection_index, 1);
            gfx_PrintStringXY("Secondary Selection Index: ", 10, 20);
            gfx_PrintInt(active->primary_selections[active->selection_index].secondary_selection_index, 1);
            gfx_PrintStringXY("# Secondary Selections: ", 10, 30);
            gfx_PrintInt(active->primary_selections[1].num_secondary_selections, 1);
            
            gfx_PrintStringXY("Current Screen: ", 10, 40);
            gfx_PrintString(active->name);
        }
        
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