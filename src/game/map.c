#include "graphx.h"
#include "towers.h"
#include "game.h"

#include "../gfx/gfx.h"

void draw_tiles(void) {
    const int TILE_SIZE = 10;
    const int GRASS_TILE_WIDTH = 20;
    const int MAP_HEIGHT = 24;
    const int MAP_WIDTH = 16;
    for (int row = 0; row < MAP_HEIGHT; row++) {
        for (int col = 0; col < MAP_WIDTH; col++) {
            if (row % 2 == 0) {
                if (col == 0) {
                    gfx_Sprite(grass,-TILE_SIZE,row * TILE_SIZE);
                }
                gfx_Sprite(grass,col * GRASS_TILE_WIDTH + TILE_SIZE,row * TILE_SIZE);
            } else {
                gfx_Sprite(grass,col * GRASS_TILE_WIDTH,row * TILE_SIZE);
            }
        }
    }

    // Replace LCD_HEIGHT / TILE_SIZE with a const variable
    for (int i = 0; i < (GFX_LCD_HEIGHT / TILE_SIZE); i++) {
        if (i % 2 == 0) {
            gfx_Sprite(riverbank_alt, (TILE_SIZE * 18) - (TILE_SIZE / 2), i * TILE_SIZE);
        } else {
            gfx_Sprite(riverbank, (TILE_SIZE * 18) - (TILE_SIZE / 2), i * TILE_SIZE);
        }
    }

    // Draw walkways
    // Walkway sprite is 180x180, rotation is around center
    const int WALKWAY_SIZE = 180;
    const int WALKWAY_CENTER = WALKWAY_SIZE / 2;

    // Left walkway - no rotation
    gfx_RotatedScaledTransparentSprite(walkway_small, 50, 40, 0, 128);
    gfx_RotatedScaledTransparentSprite(walkway_opponent_small, 140, 40, 0, 128);

    gfx_TransparentSprite(bridge,(18 * TILE_SIZE) + (TILE_SIZE / 2) - (bridge->width / 2),195 - (bridge->height / 2));
    gfx_TransparentSprite(bridge,(18 * TILE_SIZE) + (TILE_SIZE / 2) - (bridge->width / 2),65 - (bridge->height / 2));

}

void draw_map(game_t *game) {
    player_t *player = game->player;
    player_t *opponent = game->opponent;
    draw_tiles();
    draw_towers(player);
    draw_towers(opponent);
    // Draw towers in this map code or not?
    // Should also handle collisions with the map here

}

void draw_bounds(void) {
    // When cursor is not NULL
}