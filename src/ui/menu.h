#ifndef MENU_H
#define MENU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <graphx.h>
#include <keypadc.h>
#include "../game/data.h"

typedef struct {
    // Draw border on the inside or outside of the button?
    int weight;
    int color;
    int active_color;
} border_t;

typedef struct {
    gfx_sprite_t *sprite;
    int x;
    int y;
    bool transparent;
} icon_t;

typedef struct {
    // Optional sprite
    gfx_sprite_t *sprite;
    // Centered on rectangle if provided
    icon_t icon;

    int x;
    int y;
    int width;
    int height;
    int color;
    int active_color;

    bool has_icon;

    // Should there be a border struct pointer in here?
    // Border is NULL by default, and if not NULL, then pass it into init/customize borders
    border_t border;
    
    // Include highlight?
    enum {ACTION, NONE} event;
    // If button is in a tab, then keep it active, highlighted as a result
    bool tab;
    kb_key_t action_key;
    // Generic function pointer
    void (*function)(void *args);
    
    // Arguments for the function
    void *args;  // Void pointer to any arguments

    bool active;
    // Declare what happens when the button is selected
} button_t;

typedef struct {
    int count;
    button_t *buttons;
} secondary_selection_t;

typedef struct {
    // Number of buttons
    button_t *button;
    int num_secondary_selections;
    secondary_selection_t *secondary_selections;
    int secondary_selection_index;
} primary_selection_t;

typedef struct screen_t screen;
typedef struct {
    char *name;
    // Put the name of the screen as a typedef enum? Main, deck, chest opening, end screen
    // enum types are not dynamic
    // char type?
    // Determines how many times the user can press up/down, left/right arrows
    primary_selection_t *primary_selections;
    unsigned int num_primary_selections;
    // Can be 
    int selection_index;
    // selections_t *secondary_selections;
    // unsigned int secondary_selections;ai
    bool active;
    int background_color;
    int delay;
    data_t *data;
    // Pointer to background pattern function

    // Need linked list or array of the buttons here

    // Pointer to next screen
    void *next;
} screen_t;

void start_menu(void);

// Chest functions
bool award_chest(data_t *data, chest_rarity_t rarity);
void update_chest_timers(data_t *data, unsigned int delta_seconds);
unsigned int get_chest_unlock_remaining(chest_t *chest);
void format_chest_time(unsigned int seconds, char *buffer, int buffer_size);
unsigned int get_chest_unlock_duration(chest_rarity_t rarity);

// Rotated number drawing functions (defined in main.c)
int countDigits(int num);
void drawRotatedIntXY(int num, int x, int y);
void drawChestTimer(chest_t *chest, int x, int y);

#ifdef __cplusplus
}
#endif

#endif