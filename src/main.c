#include <tice.h>
#include <graphx.h>
#include <fileioc.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <sys/rtc.h>

#include "ui/menu.h"
#include "gfx/gfx.h"
#include "game/memory_simple.h"
#include "game/game.h"
#include "game/data.h"
#include "game/sprite_system.h"

#define YELLOW 171
#define WHITE 255
#define PURPLE 202

// Function declarations that are normally in menu.h
void init_deck(data_t *data);
void free_game(game_t *game);
void load_data(void);
void free_data(data_t *data);
void start_menu(void);

// Digit sprite constants (digits sprite: 100 pixels wide, 10 pixels per digit)
#define DIGIT_WIDTH 10

int countDigits(int num) {
    int count = 0;
    do {
        num /= 10;
        count++;
    } while (num != 0);
    return count;
}

// Draw a single digit (0-9) at position (x, y) using the digits sprite sheet
// Uses clipping to draw only the portion of the sprite we need - no buffer allocation
void drawDigit(int digit, int x, int y) {
    // Set clip region to only show where we want the digit drawn
    gfx_SetClipRegion(x, y, x + DIGIT_WIDTH, y + digits_sprite_height);

    // Draw the full sprite offset so the correct digit lands in the clip region
    gfx_TransparentSprite(digits_sprite, x - (digit * DIGIT_WIDTH), y);

    // Reset clip region to full screen
    gfx_SetClipRegion(0, 0, 320, 240);
}

void drawRotatedIntXY(int num, int x, int y) {
    int padding = 8;

    // Handle zero specially
    if (num == 0) {
        drawDigit(0, x, y);
        return;
    }

    // Count digits
    int temp = num;
    int numDigits = 0;
    while (temp > 0) {
        numDigits++;
        temp /= 10;
    }

    // Draw each digit from least significant to most significant
    for (int i = 0; i < numDigits; i++) {
        int currentNum = num % 10;
        int drawY = y + padding * (numDigits - 1 - i);
        drawDigit(currentNum, x, drawY);
        num /= 10;
    }
}

char *current_screen = "loading";

void draw_main_tabs(void) {
    // Draw battle tab (bottom, active)
    gfx_SetColor(116);
    gfx_FillRectangle(0, 120, 60, 120);
    gfx_TransparentSprite(battle_icon, 10, 160);

    // Draw deck tab (top, inactive)
    gfx_SetColor(83);
    gfx_FillRectangle(0, 0, 60, 120);
    gfx_TransparentSprite(deck_icon, 10, 40);
}

void draw_deck_tabs(void) {
    // Draw deck tab (top, active)
    gfx_SetColor(116);
    gfx_FillRectangle(0, 0, 60, 120);
    gfx_TransparentSprite(deck_icon, 10, 40);

    // Draw battle tab (bottom, inactive)
    gfx_SetColor(83);
    gfx_FillRectangle(0, 120, 60, 120);
    gfx_TransparentSprite(battle_icon, 10, 160);
}

void draw_loading_screen(void) {
    gfx_FillScreen(0);
    gfx_TransparentSprite(logo, (GFX_LCD_WIDTH / 2) - (150 / 2), (GFX_LCD_HEIGHT / 2) - (150 / 2));

    if (kb_Data[6] & kb_Enter) current_screen = "main";
}

void draw_main_screen(void) {
    static int selected_chest = -1;
    static int selected_row = 0; 
    const unsigned int MAX_ROW = 3;
    const unsigned int MAX_CHESTS = 4;
    // Do we need a variable for the tab we're working with?
    // Variable for checking whether to exit main screen and redirect to game handling
    kb_Scan();
    gfx_FillScreen(80);

    draw_main_tabs();

    // Draw white border on battle tab only when selected_row is 0
    if (selected_row == 0) {
        gfx_SetColor(255);
        gfx_Rectangle(0, 120, 60, 120);
        gfx_Rectangle(1, 121, 58, 118);
    }

    // Draw battle button
    gfx_TransparentSprite(battle, 185, 70);

    // Draw exit button
    gfx_Sprite(exit_button, 280, 200);

    // Draw trophy count
    gfx_TransparentSprite(trophy, 285, 15);
    drawRotatedIntXY(data.trophies, 287, 32);

    // Draw coin count
    gfx_TransparentSprite(gold_coin, 285, 80);
    drawRotatedIntXY(data.gold, 287, 100);

    // Handle tab navigation
    if (selected_row == 0 && (kb_Data[7] & kb_Up)) {
        delay(150);
        current_screen = "deck";
        return;
    }

    // Handle chest slot selection if row is selected - reset selected chest by default or only on change?
    if (selected_row == 1) {
        if ((selected_chest < 3) && (kb_Data[7] & kb_Down)) {
            delay(150);
            selected_chest++;
        } else if ((selected_chest > 0) && (kb_Data[7] & kb_Up)) {
            delay(150);
            selected_chest--;
        }
        // if ((kb_Data[7] & kb_Right)) {
        //     delay(150);
        //     // Reset chest to unselected
        //     selected_chest = -1;
        // }
    } else if (selected_row == 2) {
        gfx_SetColor(255);
        gfx_Rectangle(185, 70, 50, 100);
    } else if (selected_row == 3) {
        gfx_SetColor(255);
        gfx_Rectangle(280, 200, 25, 25);
    }
    
    // Draw chest slots and chests
    for (int i = 0; i < MAX_CHESTS; i++) {
        int slot_x = 75;
        int slot_y = 15 + (55 * i);
        int slot_width = 60;
        int slot_height = 45;

        // Draw slot border
        gfx_SetColor(255);
        if (i == selected_chest && selected_row == 1) gfx_SetColor(YELLOW);
        gfx_Rectangle(slot_x, slot_y, slot_width, slot_height);

        // Draw chest sprite if slot is not empty
        if (data.chests != NULL && data.chests[i].status != EMPTY) {
            chest_t *chest = &data.chests[i];
            gfx_sprite_t *chest_sprite = chest->sprite;
            if (chest_sprite != NULL) {
                // Center chest in slot
                int chest_x = slot_x + 5; // + (slot_width - chest_sprite->width) / 2
                int chest_y = slot_y + (slot_height - chest_sprite->height) / 2;
                gfx_TransparentSprite(chest_sprite, chest_x, chest_y);
            }

            // Show timer for LOCKED (total duration) and UNLOCKING (remaining time)
            if (chest->status == LOCKED || chest->status == UNLOCKING) {
                unsigned int time_val;
                if (chest->status == LOCKED) {
                    time_val = chest->duration;  // Show total unlock time
                } else {
                    time_val = get_chest_unlock_remaining(chest);  // Show remaining
                }
                int timer_x = slot_x + 45;
                int timer_y = slot_y + 5;

                if (time_val >= 60) {
                    int minutes_val = time_val / 60;
                    drawRotatedIntXY(minutes_val, timer_x, timer_y);
                    int digit_offset = 8 * countDigits(minutes_val);
                    gfx_TransparentSprite(minute, timer_x + 1, timer_y + digit_offset + 2);
                } else {
                    drawRotatedIntXY(time_val, timer_x, timer_y);
                    int digit_offset = 8 * countDigits(time_val);
                    gfx_TransparentSprite(second, timer_x + 1, timer_y + digit_offset + 2);
                }
            }
        }
    }
    
    if ((kb_Data[7] & kb_Right) && selected_row < (int)MAX_ROW) {
        delay(150);
        selected_row++;
        // When entering row 1, select first chest
        if (selected_row == 1) {
            selected_chest = 0;
        } else {
            selected_chest = -1;
        }
    } else if ((kb_Data[7] & kb_Left) && selected_row > 0) {
        delay(150);
        selected_row--;
        // When entering row 1, select first chest
        if (selected_row == 1) {
            selected_chest = 0;
        } else {
            selected_chest = -1;
        }
    }

    if (kb_Data[6] & kb_Enter) {
        if (selected_row == 0) {
            // Handle profile
        } else if (selected_row == 1 && selected_chest >= 0 && selected_chest < 4) {
            // Handle chests
            if (data.chests != NULL) {
                chest_t *chest = &data.chests[selected_chest];
                if (chest->status == LOCKED) {
                    // Check if we can start unlocking (no other chest unlocking)
                    bool can_unlock = true;
                    for (int i = 0; i < 4; i++) {
                        if (data.chests[i].status == UNLOCKING) {
                            can_unlock = false;
                            break;
                        }
                    }
                    if (can_unlock) {
                        chest->status = UNLOCKING;
                        chest->time_elapsed = 0;
                    }
                } else if (chest->status == OPEN) {
                    // Open the chest - go to opening sequence
                    chest->status = OPENING;
                    current_screen = "chest_opening";
                }
            }
            delay(150);
        } else if (selected_row == 2) {
            selected_row = 0;
            current_screen = "game";
        } else if (selected_row == 3) {
            current_screen = "exit";
        }
    }
}

// Chest reward configuration by rarity
static const struct {
    unsigned int min_gold;
    unsigned int max_gold;
    unsigned int min_cards;
    unsigned int max_cards;
    unsigned int max_card_types;
} CHEST_REWARDS[] = {
    {20, 50, 2, 4, 2},    // SILVER
    {50, 150, 4, 8, 3},   // GOLD
    {100, 300, 8, 15, 4}  // MAGICAL
};

// Card weights by rarity (higher = more likely)
static const int RARITY_WEIGHTS[] = {60, 25, 12, 3};  // COMMON, RARE, EPIC, LEGENDARY

// Find the chest currently being opened (OPENING status)
static int find_opening_chest(void) {
    if (data.chests == NULL) return -1;
    for (int i = 0; i < 4; i++) {
        if (data.chests[i].status == OPENING) {
            return i;
        }
    }
    return -1;
}

// Select a random card index based on weighted rarity
static int select_weighted_card(void) {
    if (data.available_cards == NULL) return 0;

    // Calculate total weight
    int total_weight = 0;
    for (int i = 0; i < 8; i++) {
        rarity_t rarity = data.available_cards[i].rarity;
        // Bounds check for RARITY_WEIGHTS array (4 elements: COMMON=0, RARE=1, EPIC=2, LEGENDARY=3)
        if (rarity > LEGENDARY) rarity = COMMON;
        total_weight += RARITY_WEIGHTS[rarity];
    }

    // Safety check
    if (total_weight <= 0) return 0;

    // Pick random value and find matching card
    int roll = rand() % total_weight;
    int cumulative = 0;
    for (int i = 0; i < 8; i++) {
        rarity_t rarity = data.available_cards[i].rarity;
        if (rarity > LEGENDARY) rarity = COMMON;
        cumulative += RARITY_WEIGHTS[rarity];
        if (roll < cumulative) {
            return i;
        }
    }
    return 0;  // Fallback
}

// Generate chest contents based on rarity
static void generate_chest_contents(chest_opening_state_t *state, chest_rarity_t rarity) {
    if (state == NULL) return;

    // Ensure rarity is valid (0=SILVER, 1=GOLD, 2=MAGICAL)
    int rarity_idx = (rarity <= MAGICAL) ? (int)rarity : 0;
    if (rarity_idx < 0 || rarity_idx > 2) rarity_idx = 0;

    // Generate random gold amount
    unsigned int gold_range = CHEST_REWARDS[rarity_idx].max_gold - CHEST_REWARDS[rarity_idx].min_gold;
    if (gold_range == 0) gold_range = 1;
    state->gold = CHEST_REWARDS[rarity_idx].min_gold + (rand() % (gold_range + 1));

    // Determine number of different card types (1 to max_card_types)
    unsigned int max_types = CHEST_REWARDS[rarity_idx].max_card_types;
    if (max_types == 0) max_types = 1;
    if (max_types > MAX_CHEST_CARD_TYPES) max_types = MAX_CHEST_CARD_TYPES;
    state->num_card_types = 1 + (rand() % max_types);
    if (state->num_card_types > MAX_CHEST_CARD_TYPES) {
        state->num_card_types = MAX_CHEST_CARD_TYPES;
    }

    // Calculate total cards to distribute
    unsigned int card_range = CHEST_REWARDS[rarity_idx].max_cards - CHEST_REWARDS[rarity_idx].min_cards;
    if (card_range == 0) card_range = 1;
    unsigned int total_cards = CHEST_REWARDS[rarity_idx].min_cards + (rand() % (card_range + 1));
    if (total_cards == 0) total_cards = 1;

    // Track which cards have been selected to avoid duplicates
    bool card_used[8] = {false};

    // Generate each card type
    for (unsigned int i = 0; i < state->num_card_types && i < MAX_CHEST_CARD_TYPES; i++) {
        // Select a card that hasn't been used yet
        int card_idx = 0;
        int attempts = 0;
        do {
            card_idx = select_weighted_card();
            if (card_idx < 0 || card_idx >= 8) card_idx = 0;
            attempts++;
        } while (card_used[card_idx] && attempts < 20);

        card_used[card_idx] = true;
        state->cards[i].card_index = card_idx;

        // Distribute cards - last type gets remaining cards
        if (i == state->num_card_types - 1) {
            state->cards[i].count = (total_cards > 0) ? total_cards : 1;
        } else {
            // Give 1 to half of remaining cards to this type
            unsigned int max_for_type = (total_cards > 1) ? (total_cards / 2) : 1;
            if (max_for_type == 0) max_for_type = 1;
            state->cards[i].count = 1 + (rand() % max_for_type);
            if (state->cards[i].count > total_cards) {
                state->cards[i].count = total_cards;
            }
            total_cards -= state->cards[i].count;
        }
    }

    state->generated = true;
}

void draw_chest_opening(void) {
    // Static state persists across frames
    static chest_opening_state_t state = {0};
    static unsigned int sequence_index = 0;

    kb_Scan();
    gfx_FillScreen(80);

    // Find the chest being opened
    int chest_idx = find_opening_chest();
    if (chest_idx < 0) {
        state.generated = false;
        sequence_index = 0;
        current_screen = "main";
        return;
    }

    chest_t *chest = &data.chests[chest_idx];

    // Generate contents on first frame - TEST THIS
    if (!state.generated || state.opening_chest_index != chest_idx) {
        state.opening_chest_index = chest_idx;
        sequence_index = 0;
        state.num_card_types = 0;
        generate_chest_contents(&state, chest->rarity);
    }

    // Safety checks
    if (state.num_card_types == 0 || state.num_card_types > MAX_CHEST_CARD_TYPES) {
        state.num_card_types = 1;
    }

    unsigned int max_sequence = 2 + state.num_card_types;
    if (max_sequence > 10) max_sequence = 10;

    // Draw header
    gfx_TransparentSprite(trophy, 285, 15);
    drawRotatedIntXY(data.trophies, 287, 32);
    gfx_TransparentSprite(gold_coin, 285, 80);
    drawRotatedIntXY(data.gold, 287, 100);

    // Draw dots vertically on left side, centered vertically
    int total_dots = max_sequence + 1;
    int dot_spacing = 15;
    int dots_height = (total_dots - 1) * dot_spacing;
    int dot_x = 20;
    int dot_start_y = 120 - (dots_height / 2);
    for (unsigned int i = 0; i <= max_sequence; i++) {
        int dot_y = dot_start_y + (i * dot_spacing);
        if (i == sequence_index) {
            gfx_SetColor(YELLOW);
            gfx_FillCircle(dot_x, dot_y, 5);
        } else if (i < sequence_index) {
            gfx_SetColor(255);
            gfx_FillCircle(dot_x, dot_y, 4);
        } else {
            gfx_SetColor(255);
            gfx_Circle(dot_x, dot_y, 4);
        }
    }

    if (sequence_index == 0) {
        gfx_sprite_t *chest_sprite = chest->sprite;
        if (chest_sprite != NULL && chest_sprite->width > 0 && chest_sprite->height > 0) {
            int scaled_w = chest_sprite->width * 2;
            int scaled_h = chest_sprite->height * 2;
            int cx = 160 - (scaled_w / 2);
            int cy = 120 - (scaled_h / 2);
            if (cx >= 0 && cy >= 0 && cx + scaled_w <= 320 && cy + scaled_h <= 240) {
                gfx_RotatedScaledTransparentSprite(chest_sprite, cx, cy, 0, 128);
            }
        }
    } else if (sequence_index == 1) {
        if (gold_card != NULL && gold_card->width > 0 && gold_card->height > 0) {
            int scaled_w = gold_card->width * 2;
            int scaled_h = gold_card->height * 2;
            int gx = 160 - (scaled_w / 2);
            int gy = 100 - (scaled_h / 2);
            if (gx >= 0 && gy >= 0 && gx + scaled_w <= 320 && gy + scaled_h <= 240) {
                gfx_Sprite(gold_card, gx, gy);
            }
        }
        
        gfx_TransparentSprite(plus, 125, 110);
        drawRotatedIntXY(state.gold, 125, 120);
    } else if (sequence_index >= 2 && sequence_index < 2 + state.num_card_types) {
        unsigned int card_step = sequence_index - 2;
        if (card_step >= MAX_CHEST_CARD_TYPES) card_step = 0;
        int card_idx = state.cards[card_step].card_index;
        if (card_idx < 0 || card_idx >= 8) card_idx = 0;
        unsigned int card_count = state.cards[card_step].count;
        if (card_count == 0) card_count = 1;

        if (card_idx >= 0 && card_idx < 8) {
            gfx_sprite_t *card_sprite = data.available_cards[card_idx].sprite;
            if (card_sprite != NULL && card_sprite->width > 0 && card_sprite->height > 0) {
                int scaled_w = card_sprite->width * 2;
                int scaled_h = card_sprite->height * 2;
                int cx = 160 - (scaled_w / 2);
                int cy = 100 - (scaled_h / 2);
                if (cx >= 0 && cy >= 0 && cx + scaled_w <= 320 && cy + scaled_h <= 240) {
                    gfx_Sprite(card_sprite, cx, cy);
                }
            }
            gfx_TransparentSprite(plus, 125, 110);
            drawRotatedIntXY(card_count, 125, 120);

            rarity_t rarity = data.available_cards[card_idx].rarity;
            int bar_color = 151;
            if (rarity == RARE) bar_color = 150;
            else if (rarity == EPIC) bar_color = 202;
            else if (rarity == LEGENDARY) bar_color = 171;
            // gfx_SetColor(bar_color);
            // gfx_FillRectangle(110, 70, 10, 30);
            // gfx_SetColor(0);
            // gfx_Rectangle(110, 70, 10, 30);
        }
    } else {
        // Summary screen: 2-column grid like deck screen (right col first), chest at bottom
        int total_items = 1 + state.num_card_types;  // gold + cards
        int col_spacing = 50;
        int row_spacing = 40;

        // Center horizontally: right column first (like deck screen)
        int right_col_x = 160;
        int left_col_x = 160 - col_spacing;

        // Calculate rows needed per column (items 0-3 right, 4-7 left)
        int right_col_items = (total_items > 4) ? 4 : total_items;
        int grid_height = right_col_items * row_spacing;
        int start_y = 80 - (grid_height / 2) + 20;

        // Draw gold card first (item 0) - top of right column
        int gold_x = right_col_x;
        int gold_y = start_y;
        gfx_TransparentSprite(gold_card, gold_x, gold_y);
        // gfx_TransparentSprite(plus, gold_x + 20, gold_y + 5);
        // drawRotatedIntXY(state.gold, gold_x + 35, gold_y + 5);

        // Draw card types (items 1+)
        for (unsigned int i = 0; i < state.num_card_types; i++) {
            int item_idx = i + 1;  // offset by 1 for gold card
            // Items 0-3 right column, 4-7 left column (like deck screen)
            int card_x = (item_idx < 4) ? right_col_x : left_col_x;
            int card_y = start_y + ((item_idx < 4) ? item_idx : (item_idx - 4)) * row_spacing;

            int card_idx = state.cards[i].card_index;
            if (card_idx >= 0 && card_idx < 8) {
                gfx_sprite_t *card_sprite = data.available_cards[card_idx].sprite;
                if (card_sprite != NULL && card_sprite->width > 0 && card_sprite->height > 0) {
                    gfx_Sprite(card_sprite, card_x, card_y);
                    // drawRotatedIntXY(state.cards[i].count, card_x + 30, card_y + 35);
                }
            }
        }

        // Draw chest centered at bottom
        gfx_sprite_t *chest_sprite = chest->sprite;
        if (chest_sprite != NULL && chest_sprite->width > 0 && chest_sprite->height > 0) {
            int chest_x = 160 - (chest_sprite->width / 2);
            int chest_y = 180;
            gfx_TransparentSprite(chest_sprite, chest_x, chest_y);
        }
    }

    if (kb_Data[6] & kb_Enter) {
        // Wait for key release to prevent multiple triggers
        while (kb_Data[6] & kb_Enter) {
            kb_Scan();
        }
        delay(50);
        if (sequence_index < max_sequence) {
            sequence_index++;
        } else {
            data.gold += state.gold;
            chest->status = EMPTY;
            chest->gold = 0;
            chest->total_cards = 0;
            state.generated = false;
            sequence_index = 0;
            current_screen = "main";
        }
    }

    if (kb_Data[6] & kb_Clear) {
        delay(150);
        data.gold += state.gold;
        chest->status = EMPTY;
        chest->gold = 0;
        chest->total_cards = 0;
        state.generated = false;
        sequence_index = 0;
        current_screen = "main";
    }
}

void draw_deck_screen(void) {
    static int selected_card = -1;
    static int selected_row = 0;

    // Flag for showing additional info about the card
    bool card_info = false;

    gfx_FillScreen(80);

    draw_deck_tabs();

    // Draw white border on deck tab only when no card is selected
    if (selected_card == -1) {
        gfx_SetColor(255);
        gfx_Rectangle(0, 0, 60, 120);
        gfx_Rectangle(1, 1, 58, 118);
    }

    // Handle tab navigation
    // This needs to check if the selected card is not set
    if (selected_row == 0 && (kb_Data[7] & kb_Down) && selected_card == -1) {
        delay(150);
        current_screen = "main";
        return;
    }

    gfx_Sprite(deck_text, 280, 15);


    // gfx_SetColor(255);
    // gfx_Rectangle(175, 15, 150, 200);

    for (int i = 0; i < 8; i++) {
        gfx_SetColor(0);
        if (i == selected_card) gfx_SetColor(171);
        // Card 0-3 are right column (top to bottom), 4-7 are left column (top to bottom)
        int x = (i < 4) ? 225 : 175;
        int y = 15 + ((i < 4) ? i : (i - 4)) * 40;

        // Draw card sprite inside the rectangle
        if (data.deck != NULL && data.deck[i].sprite != NULL) {
            gfx_Sprite(data.deck[i].sprite, x, y);
        }
        // Debug sprite
        // gfx_Sprite(musketeer_card, x, y);
        gfx_Rectangle(x, y, 40, 30);
    }

    // Handle initial card selection
    if (selected_card == -1 && (kb_Data[7] & kb_Right)) {
        selected_card = 0;
    }

    // Moving between cards - delay between cards
    if (selected_card > -1) {

        // Elixir bar - show selected card's elixir cost
        int elixir_cost = 0;
        if (data.deck != NULL) {
            elixir_cost = data.deck[selected_card].elixir;
        }

        // Draw empty portion first (dark purple background)
        gfx_SetColor(55);
        gfx_FillRectangle(175, 175, 90, 20);

        // Draw filled portion based on elixir cost (max 10 elixir, 9px per elixir)
        gfx_SetColor(202);
        gfx_FillRectangle(175, 175, elixir_cost * 9, 20);

        // Draw segment lines (black vertical lines every 9px)
        gfx_SetColor(0);
        for (int i = 0; i <= 9; i++) {
            gfx_VertLine(175 + i * 9, 175, 20);
        }

        // Draw outer border
        gfx_Rectangle(175, 175, 90, 20);


        // kb_Up = move right in UI (left column to right column)
        if ((kb_Data[7] & kb_Up) && selected_card > 0) {
            delay(250);
            selected_card--;
        // kb_Down = move left in UI (right column to left column)
        } else if ((kb_Data[7] & kb_Down) && selected_card < 7) {
            delay(250);
            selected_card++;
        // kb_Left = move up in UI (within column)
        } else if ((kb_Data[7] & kb_Left) ) {
            delay(250);
            selected_card += 4;
            if (selected_card > 7) selected_card = -1;
        // kb_Right = move down in UI (within column)
        } else if ((kb_Data[7] & kb_Right && selected_card > 3)) {
            delay(250);
            selected_card -= 4;
        }
    }

    // If enter is pressed, lock the horizontal selection in place and allow them to upgrade the card
    if ((kb_Data[6] & kb_Enter) && selected_card > -1) {
        card_info = true;
    }

    if (card_info == true) {
        // Show upgrade information - if necessary cards are met, then show the gold cost to upgrade, if not show the remaining cards needed
    }

    // Collection

    // Render deck from data
}

void draw_gameover(void) {
    kb_Scan();

    gfx_FillScreen(80);

    // Draw crown count (0-3 crowns earned)
    // Left side - Player crowns
    for (unsigned int i = 0; i < 3; i++) {
        gfx_sprite_t *crown_sprite = (i < data.last_player_crowns) ? blue_crown : empty_crown;
        gfx_TransparentSprite(crown_sprite, 190, 40 + (i * 60));
    }

    // // // Right side crowns - opponent
    for (unsigned int i = 0; i < 3; i++) {
        gfx_sprite_t *crown_sprite = (i < data.last_opponent_crowns) ? red_crown : empty_crown;
        gfx_TransparentSprite(crown_sprite, 250, 40 + (i * 60));
    }

    // Draw trophy reward/loss
    gfx_TransparentSprite(trophy, 125, 120);
    if (data.last_trophy_change >= 0) {
        gfx_TransparentSprite(plus, 127 , 135);
        drawRotatedIntXY(data.last_trophy_change, 127, 147);
    } else {
        gfx_TransparentSprite(minus, 127, 135);
        drawRotatedIntXY(-data.last_trophy_change, 127, 147);  // Pass absolute value
    }

    // Draw chest reward (if won)
    if (data.last_chest_given) {
        gfx_sprite_t *chest_sprite = get_chest_sprite(data.last_chest_awarded);
        gfx_TransparentSprite(chest_sprite, 115, 60);
    }

    // Empty rectangle regardlss
    gfx_SetColor(255);
    gfx_Rectangle(115, 60, 40, 40);

    // Draw continue button
    gfx_TransparentSprite(play_again, 30, 15);

    // Draw OK button
    gfx_TransparentSprite(ok_button, 30, 125);

    // Selected button - static to persist across frames
    static int selected_button = 0;

    if ((kb_Data[7] & kb_Down)) {
        if (selected_button != 1) delay(150);
        selected_button = 1;
    } else if (kb_Data[7] & kb_Up) {
        if (selected_button != 0) delay(150);
        selected_button = 0;
    }

    // Draw selection indicator
    if (selected_button == 0) {
        gfx_SetColor(255);
        gfx_Rectangle(30, 15, play_again->width, play_again->height);
    } else {
        gfx_SetColor(255);
        gfx_Rectangle(30, 125, ok_button->width, ok_button->height);
    }

    // Return to main menu on ENTER
    if (kb_Data[6] & kb_Enter) {
        delay(150);
        if (selected_button == 0) current_screen = "game";
        if (selected_button == 1) {
            data.has_pending_result = false;
            current_screen = "main";
        }
    }
}

void draw_game(void) {
    // Data and deck already initialized at startup
    game_t *game = init_game(&data);

    if (game) {
        run_game(game);

        // Calculate and apply game results
        game_result_t result = calculate_game_result(game);
        apply_game_result(&data, &result);

        free_game(game);
    }

    current_screen = "gameover";
}

void draw_screens(void) {
    // Quit game can only be called from here
    bool exit = false;

    // Track time for chest unlock timers
    uint8_t last_seconds, last_minutes, last_hours;
    boot_GetTime(&last_seconds, &last_minutes, &last_hours);

    while (exit == false) {
        kb_Scan();
        gfx_SetDrawBuffer();

        // Update chest timers every second
        uint8_t curr_sec, curr_min, curr_hour;
        boot_GetTime(&curr_sec, &curr_min, &curr_hour);
        if (curr_sec != last_seconds || curr_min != last_minutes || curr_hour != last_hours) {
            unsigned int delta = (curr_hour - last_hours) * 3600 +
                                 (curr_min - last_minutes) * 60 +
                                 (curr_sec - last_seconds);
            update_chest_timers(&data, delta);
            last_seconds = curr_sec;
            last_minutes = curr_min;
            last_hours = curr_hour;
        }

        if (kb_Data[6] & kb_Clear && strcmp(current_screen, "gameover") != 0) {
            exit = true;
        }
        // Case on screens
        if (strcmp(current_screen, "loading") == 0) {
            draw_loading_screen();
        } else if (strcmp(current_screen, "main") == 0) {
            draw_main_screen();
        } else if (strcmp(current_screen, "chest_opening") == 0) {
            draw_chest_opening();
        } else if (strcmp(current_screen, "deck") == 0) {
            draw_deck_screen();
        } else if (strcmp(current_screen, "game") == 0) {
            draw_game();
        } else if (strcmp(current_screen, "gameover") == 0) {
            draw_gameover();
        } else if (strcmp(current_screen, "exit") == 0) {
            exit = true;
        }
        gfx_BlitBuffer();
    }
}

int main(void) {
    srand(rtc_Time());

    // Load sprites from appvars (must be called before gfx_Begin)
    if (CRUI_init() == 0 ||
        CRCARD_init() == 0 ||
        CRMAP_init() == 0 ||
        CRTRP1_init() == 0 ||
        CRTRP2_init() == 0 ||
        CRTRP3_init() == 0) {
        // Failed to load an appvar - exit gracefully
        return 1;
    }

    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_SetTransparentColor(170);

    // Initialize data and deck at startup
    load_data();
    init_deck(&data);

    // Do we need a variable here to pass the current screen that should be drawn?
    draw_screens();

    // Save data and cleanup here
    save_data();
    free_data(&data);

    // Maybe everything should run downstream of main menu and loop back to that
    gfx_End();
    return 0;
}