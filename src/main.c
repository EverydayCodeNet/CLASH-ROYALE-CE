#include <tice.h>
#include <graphx.h>
#include <fileioc.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ui/menu.h"
#include "gfx/gfx.h"
#include "game/memory_simple.h"
#include "game/game.h"
#include "game/data.h"

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

            // Draw timer at all times, only update the clock when it is unlocking
            if (chest->status != OPEN) {
                unsigned int remaining = get_chest_unlock_remaining(chest);
                int timer_x = slot_x + 45;
                int timer_y = slot_y + 5;

                if (remaining >= 60) {
                    int minutes_val = remaining / 60;
                    drawRotatedIntXY(minutes_val, timer_x, timer_y);
                    int digit_offset = 8 * countDigits(minutes_val);
                    gfx_TransparentSprite(minute, timer_x + 1, timer_y + digit_offset + 2);
                } else {
                    drawRotatedIntXY(remaining, timer_x, timer_y);
                    int digit_offset = 8 * countDigits(remaining);
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
        } else if (selected_row == 1) {
            // Handle chests here
        } else if (selected_row == 2) {
            selected_row = 0;
            current_screen = "game";
        } else if (selected_row == 3) {
            current_screen = "exit";
        }
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
        if (data.deck != NULL) {
            gfx_sprite_t *card_sprite = data.deck[i].sprite;
           
            if (card_sprite != NULL) {
                gfx_Sprite(card_sprite, x, y);
            }
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

        // Draw elixir cost number
        char elixir_str[4];
        sprintf(elixir_str, "%d", elixir_cost);
        gfx_SetTextFGColor(255);
        gfx_PrintStringXY(elixir_str, 215, 178);

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

    // Draw victory/defeat text
    // gfx_SetTextFGColor(255);
    // if (data.has_pending_result) {
    //     if (data.last_victory) {
    //         gfx_PrintStringXY("VICTORY!", 130, 10);
    //     } else {
    //         gfx_PrintStringXY("DEFEAT", 135, 10);
    //     }
    // }

    // Draw crown count (0-3 crowns earned)
    // Left side - Player crowns
    // for (unsigned int i = 0; i < 3; i++) {
    //     gfx_sprite_t *crown_sprite = (i < data.last_player_crowns) ? blue_crown : empty_crown;
    //     gfx_RotatedScaledTransparentSprite(crown_sprite, 180, 60 + (i * 40), 0, 128);
    // }

    // // // Right side crowns - opponent
    // for (unsigned int i = 0; i < 3; i++) {
    //     gfx_sprite_t *crown_sprite = (i < data.last_opponent_crowns) ? red_crown : empty_crown;
    //     gfx_RotatedScaledTransparentSprite(crown_sprite, 250, 60 + (i * 40), 0, 128);
    // }

    // Draw trophy reward/loss
    gfx_TransparentSprite(trophy, 122, 125);
    gfx_SetTextFGColor(255);
    if (data.last_trophy_change >= 0) {
        gfx_PrintStringXY("+", 140, 125);
        drawRotatedIntXY(data.last_trophy_change, 140, 133);
    } else {
        // Use "|" for minus sign since screen is rotated
        gfx_PrintStringXY("|", 140, 125);
        drawRotatedIntXY(-data.last_trophy_change, 140, 133);
    }

    // Draw chest reward (if won)
    if (data.last_chest_given) {
        gfx_sprite_t *chest_sprite = get_chest_sprite(data.last_chest_awarded);
        gfx_TransparentSprite(chest_sprite, 110, 45);
    }
    // gfx_TransparentSprite(silver_chest, 110, 45);

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
    while (exit == false) {
        kb_Scan();
        gfx_SetDrawBuffer();

        if (kb_Data[6] & kb_Clear && strcmp(current_screen, "gameover") != 0) {
            exit = true;
        }
        // Case on screens
        if (strcmp(current_screen, "loading") == 0) {
            draw_loading_screen();
        } else if (strcmp(current_screen, "main") == 0) {
            draw_main_screen();
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