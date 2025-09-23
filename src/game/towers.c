
#include <stdio.h>
#include <stdlib.h>
#include <graphx.h>
#include "structs.h"
#include "towers.h"
#include "player.h"
#include "troops.h"
#include "game.h"
#include "memory_simple.h"
#include "gfx/gfx.h"

#define NUM_TOWERS 3
#define PRINCESS_TOWER_WIDTH 50
#define KING_TOWER_WIDTH 60
#define PRINCESS_TOWER_MAX_HEALTH 3000
#define KING_TOWER_MAX_HEALTH 5000

tower_t *create_tower(int x, int y, bool active, gfx_sprite_t *sprite) {
    tower_t *tower = CR_MALLOC(sizeof(tower_t));
    tower->position.x = x;
    tower->position.y = y;
    tower->sprite = sprite;
    tower->nearest_troop = NULL;

    if (active == true) {
        tower->active = true;
        tower->range = 7.5;
        tower->damage = 100;
        tower->attack_speed = 10;
        tower->attack_ticks = 0;
        tower->projectile.sprite = arrow;
        tower->projectile.damage = tower->damage;
        tower->projectile.speed = 1;
        tower->MAX_HEALTH = PRINCESS_TOWER_MAX_HEALTH;
    } else {
        tower->active = false;
        tower->range = 7;
        tower->damage = 100;
        tower->attack_speed = 25;
        tower->attack_ticks = 0;
        tower->projectile.sprite = bullet;
        // Can move this to generic init_projectile function
        tower->projectile.damage = tower->damage;
        tower->projectile.speed = 1;
        tower->MAX_HEALTH = KING_TOWER_MAX_HEALTH;
    }

    tower->health = tower->MAX_HEALTH;

    return tower;
}

void init_towers(game_t *game) {
    tower_t *towers = CR_MALLOC(sizeof(tower_t) * NUM_TOWERS);
    towers[0] = *create_tower(90, 40, true, princess_tower);
    towers[1] = *create_tower(90, 170, true, princess_tower);
    towers[2] = *create_tower(50, 100, false, king_tower);

    tower_t *opponent_towers = CR_MALLOC(sizeof(tower_t) * NUM_TOWERS);
    opponent_towers[0] = *create_tower(230, 40, true, princess_tower_opponent);
    opponent_towers[1] = *create_tower(230, 170, true, princess_tower_opponent);
    opponent_towers[2] = *create_tower(260, 100, false, king_tower_opponent);

    game->player->towers = towers;
    game->opponent->towers = opponent_towers;
}

void draw_towers(player_t *player) {
    // Draw player and opponent towers
    // Get variables for the towers?

    int towerWidth, towerHeight;
    int maxHealth; 
    int xOffset = 0;

    int x = 0;
    int y = 0;

    tower_t *towers = player->towers;

    bool opponent = player->opponent;
    
    for (int i = 0; i < NUM_TOWERS; i++) {
        tower_t *tower = &towers[i];
        x = tower->position.x;
        y = tower->position.y;
        if (tower->health > 0) gfx_TransparentSprite(tower->sprite, x, y);
        
        // tertiary if statement that sees if the king tower is activated
        if (tower->health > 0 && tower->active == true) {
            // create values for tower 
            
            towerWidth = PRINCESS_TOWER_WIDTH;
            towerHeight = PRINCESS_TOWER_WIDTH;
            maxHealth = PRINCESS_TOWER_MAX_HEALTH;
            
            if (i == 2) {
                if (opponent == true) xOffset = -5;
                towerWidth = KING_TOWER_WIDTH;
                towerHeight = KING_TOWER_WIDTH;
                maxHealth = KING_TOWER_MAX_HEALTH;
            }

            if (opponent == false) {
                xOffset = -30;
            } 
            // put these in a variable to reduce arithmetic
            // healthX, healthY, maxHealth, width, height
            gfx_SetColor(136);
            gfx_FillRectangle(x + towerWidth + xOffset, y + (towerHeight / 4), 5,((towerWidth / 2)));
            gfx_SetColor(151);

            if (opponent == true) gfx_SetColor(150);
            gfx_FillRectangle(x + towerWidth + xOffset, y + (towerHeight / 4), 5,((towerWidth / 2) * tower->health) / maxHealth);
            //gfx_FillRectangle(enemyTowers[i]->x + PRINCESS_TOWER_WIDTH, enemyTowers[i]->y + (PRINCESS_TOWER_WIDTH / 4), 5,((PRINCESS_TOWER_WIDTH / 2) *enemyTowers[i]->health) / TOWER_MAX_HEALTH);
            gfx_SetColor(0);
            gfx_Rectangle(x + towerWidth + xOffset, y + (towerHeight / 4), 5,(towerWidth / 2));
            //gfx_Rectangle(enemyTowers[i]->x + PRINCESS_TOWER_WIDTH, enemyTowers[i]->y + (PRINCESS_TOWER_WIDTH / 4), 5,(PRINCESS_TOWER_WIDTH / 2));
        }
    }
}

// void update_towers(Player *player,Tower *enemyTowers[], Troop *troops[]) {
    // bool troopInRange;
    // for (int i = 0; i < NUM_TOWERS; i++) {
    //     // this is a terrible 
    //     if (i < 2) {
    //         if (enemyTowers[i]->health <= 0) {
    //             enemyTowers[2]->active = true;
    //         }
    //         if (enemyTowers[i]->health <= 0) enemyTowers[i]->active = false;
    //     } else if (i == 2){
    //         if (enemyTowers[i]->health < KING_TOWER_MAX_HEALTH) enemyTowers[i]->active = true;
    //     }
    
    //     if (player->numTroops > 0) {
    //         if (enemyTowers[i]->nearestTroop >= player->numTroops || enemyTowers[i]->nearestTroop == -1) {
    //             findNearestTroop(player,enemyTowers[i],troops);
    //         } 
    //         if (enemyTowers[i]->nearestTroop != -1) {
    //             troopInRange = inRange(enemyTowers[i]->x, enemyTowers[i]->y,troops[enemyTowers[i]->nearestTroop]->x,troops[enemyTowers[i]->nearestTroop]->y, enemyTowers[i]->range);
    //             if (enemyTowers[i]->active == true && troopInRange == true) {
    //                 troops[enemyTowers[i]->nearestTroop]->health -= 20;
    //             } 
    //         }
    //     }
    // }
// }
