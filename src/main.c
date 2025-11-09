#include <tice.h>
#include <graphx.h>
#include <stdlib.h>
#include <string.h>

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

char *current_screen = "loading";

void draw_main_tabs(void) {
    // Draw battle tab (bottom, active)
    gfx_SetColor(116);
    gfx_FillRectangle(0, 120, 60, 120);
    // gfx_SetColor(255);
    // gfx_Rectangle(0, 120, 60, 120);
    // gfx_Rectangle(1, 121, 58, 118);
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
    // Always "selected" display for tabs - experimenting with removing this
    // gfx_SetColor(255);
    // gfx_Rectangle(0, 0, 60, 120);
    // gfx_Rectangle(1, 1, 58, 118);
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
    const unsigned int MAX_ROW = 4;
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
    // draw_rotated_text(trophies)

    // Draw coin count
    gfx_TransparentSprite(gold_coin, 285, 80);
    // draw_rotated_text(trophies)

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
        gfx_SetColor(255);
        if (i == selected_chest && selected_row == 1) gfx_SetColor(YELLOW);
        gfx_Rectangle(75, 15 + (55 * i), 60, 45);
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
        gfx_SetColor(255);
        if (i == selected_card) gfx_SetColor(171);
        // Card 0-3 are right column (top to bottom), 4-7 are left column (top to bottom)
        int x = (i < 4) ? 225 : 175;
        int y = 15 + ((i < 4) ? i : (i - 4)) * 40;
        gfx_Rectangle(x, y, 40, 30);
    }

    // Handle initial card selection
    if (selected_card == -1 && (kb_Data[7] & kb_Right)) {
        selected_card = 0;
    }

    // Moving between cards - delay between cards
    if (selected_card > -1) {

        // Elixir bar
        // Draw the elixir based on how much it costs - segment
        gfx_SetColor(202);
        gfx_FillRectangle(175, 175, 90, 20);
        gfx_SetColor(0);
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

    // Draw victory/defeat status
    // gfx_SetTextFGColor(255);
    // gfx_PrintStringXY("VICTORY!", 120, 10);

    // Draw crown count (0-3 crowns earned)
    // Left side - Player crowns
    gfx_RotatedScaledTransparentSprite(empty_crown, 180, 60, 0, 128);
    gfx_RotatedScaledTransparentSprite(empty_crown, 180, 100, 0, 128);
    gfx_RotatedScaledTransparentSprite(empty_crown, 180, 140, 0, 128);

    // Right side crowns - opponent
    gfx_RotatedScaledTransparentSprite(empty_crown, 250, 60, 0, 128);
    gfx_RotatedScaledTransparentSprite(empty_crown, 250, 100, 0, 128);
    gfx_RotatedScaledTransparentSprite(empty_crown, 250, 140, 0, 128);

    // gfx_TransparentSprite(blue_crown, 30, 50);
    // gfx_TransparentSprite(blue_crown, 30, 90);
    // gfx_TransparentSprite(blue_crown, 30, 130);

    // Right side - Opponent crowns - if they won, else
    // gfx_TransparentSprite(red_crown, 280, 50);
    // gfx_TransparentSprite(red_crown, 280, 90);
    // gfx_TransparentSprite(red_crown, 280, 130);

    // Draw trophy reward/loss
    gfx_TransparentSprite(trophy, 122, 125);

    // Trophy count
    // gfx_PrintStringXY("+", )
    // gfx_SetTextFGColor(255);
    // gfx_PrintStringXY("+30", 170, 80);

    // Draw chest reward (if won)
    gfx_TransparentSprite(silver_chest, 110, 45);

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
        if (selected_button == 1) current_screen = "main";
    }
}

void draw_game(void) {
    load_data();
    init_deck(&data);  // CRITICAL: Initialize deck before init_game!
    game_t *game = init_game(&data);

    if (game) {
        run_game(game);
        free_game(game);
    }

    save_data();
    free_data(&data);
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
    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_SetTransparentColor(170);

    // Do we need a variable here to pass the current screen that should be drawn?
    draw_screens();

    // Maybe everything should run downstream of main menu and loop back to that
    // Save data and cleanup here
    gfx_End();
    return 0;
}