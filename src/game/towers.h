#ifndef TOWERS_H
#define TOWERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <graphx.h>
#include "troops.h"
#include "utils.h"
#include "player.h"
#include "game.h"
#include "structs.h"

// Tower *createPrincessTower(int x, int y, int index, _Bool enemy);
// Tower *createKingTower(int x, int y, _Bool enemy);

// void initTowers(Tower *towers[], Tower *enemyTowers[]);
// void updateTowers(Player *player,Tower *enemyTowers[], Troop *troops[]);
// void drawTowers(Tower *towers[]);
// bool drawCenterBound(Tower *enemyTowers[]);

void init_towers(game_t *game);
void draw_towers(player_t *player);

#ifdef __cplusplus
}
#endif

#endif